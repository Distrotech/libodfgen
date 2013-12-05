/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include <libodfgen/libodfgen.hxx>

#include "DocumentElement.hxx"
#include "FilterInternal.hxx"
#include "FontStyle.hxx"
#include "GraphicFunctions.hxx"
#include "ListStyle.hxx"
#include "OdfGenerator.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"
#include <locale.h>
#include <math.h>
#include <string>
#include <map>
#include <stack>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Workaround for the incapacity of draw to have multiple page
// sizes in the same document. Once that limitation is lifted,
// remove this
#define MULTIPAGE_WORKAROUND 1

namespace
{

static librevenge::RVNGString doubleToString(const double value)
{
	librevenge::RVNGProperty *prop = librevenge::RVNGPropertyFactory::newDoubleProp(value);
	librevenge::RVNGString retVal = prop->getStr();
	delete prop;
	return retVal;
}

} // anonymous namespace

namespace
{

struct GeneratorState
{
	bool mbIsTextBox;
	bool mbIsParagraph;
	bool mbIsTextOnPath;
	bool mInComment;
	bool mHeaderRow;
	bool mTableCellOpened;
	bool mInNotes;

	GeneratorState();
};

GeneratorState::GeneratorState()
	: mbIsTextBox(false)
	, mbIsParagraph(false)
	, mbIsTextOnPath(false)
	, mInComment(false)
	, mHeaderRow(false)
	, mTableCellOpened(false)
	, mInNotes(false)
{
}

}

namespace
{

class GraphicTableCellStyle : public TableCellStyle
{
public:
	GraphicTableCellStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName);
	virtual ~GraphicTableCellStyle();

private:
	virtual void writeCompat(OdfDocumentHandler *pHandler, const librevenge::RVNGPropertyList &propList) const;
};

GraphicTableCellStyle::GraphicTableCellStyle(const librevenge::RVNGPropertyList &xPropList, const char *const psName)
	: TableCellStyle(xPropList, psName)
{
}

GraphicTableCellStyle::~GraphicTableCellStyle()
{
}

void GraphicTableCellStyle::writeCompat(OdfDocumentHandler *const pHandler, const librevenge::RVNGPropertyList &propList) const
{
	using librevenge::RVNGPropertyList;

	RVNGPropertyList stylePropList;
	RVNGPropertyList::Iter i(propList);

	/* first set padding, so that mPropList can redefine, if
	   mPropList["fo:padding"] is defined */
	stylePropList.insert("fo:padding", "0.0382in");
	stylePropList.insert("draw:fill", "none");
	stylePropList.insert("draw:textarea-horizontal-align", "center");

	for (i.rewind(); i.next();)
	{
		if (strcmp(i.key(), "fo:background-color") == 0)
		{
			stylePropList.insert("draw:fill", "solid");
			stylePropList.insert("draw:fill-color", i()->clone());
		}
		else if (strcmp(i.key(), "style:vertical-align")==0)
			stylePropList.insert("draw:textarea-vertical-align", i()->clone());
	}

	pHandler->startElement("style:graphic-properties", stylePropList);
	pHandler->endElement("style:graphic-properties");

	// HACK to get visible borders
	RVNGPropertyList paraPropList;
	paraPropList.insert("fo:border", "0.03pt solid #000000");

	pHandler->startElement("style:paragraph-properties", paraPropList);
	pHandler->endElement("style:paragraph-properties");
}

}

class OdpGeneratorPrivate : public OdfGenerator
{
public:
	OdpGeneratorPrivate();
	~OdpGeneratorPrivate();

	/** update a graphic style element */
	void _updateGraphicPropertiesElement(TagOpenElement &element, ::librevenge::RVNGPropertyList const &style);
	void _writeGraphicsStyle();
	void writeNotesStyles(OdfDocumentHandler *pHandler);
	void _drawPolySomething(const ::librevenge::RVNGPropertyListVector &vertices, bool isClosed);
	void _drawPath(const librevenge::RVNGPropertyListVector &path);

	//! returns the document type
	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeSettings(OdfDocumentHandler *pHandler);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler);
	void _writeMasterPages(OdfDocumentHandler *pHandler);
	void _writePageLayouts(OdfDocumentHandler *pHandler);

	// graphics styles
	std::vector<DocumentElement *> mGraphicsStrokeDashStyles;
	std::vector<DocumentElement *> mGraphicsGradientStyles;
	std::vector<DocumentElement *> mGraphicsBitmapStyles;
	std::vector<DocumentElement *> mGraphicsMarkerStyles;
	std::vector<DocumentElement *> mGraphicsAutomaticStyles;

	// page styles
	std::vector<DocumentElement *> mPageAutomaticStyles;
	std::vector<DocumentElement *> mPageMasterStyles;

	::librevenge::RVNGPropertyList mxStyle;
	::librevenge::RVNGPropertyListVector mxGradient;
	int miGradientIndex;
	int miBitmapIndex;
	int miStartMarkerIndex;
	int miEndMarkerIndex;
	int miDashIndex;
	int miGraphicsStyleIndex;
	int miPageIndex;
	double mfWidth, mfMaxWidth;
	double mfHeight, mfMaxHeight;

	// generator state
	GeneratorState mState;

private:
	OdpGeneratorPrivate(const OdpGeneratorPrivate &);
	OdpGeneratorPrivate &operator=(const OdpGeneratorPrivate &);

};

OdpGeneratorPrivate::OdpGeneratorPrivate() :
	mGraphicsStrokeDashStyles(),
	mGraphicsGradientStyles(),
	mGraphicsBitmapStyles(),
	mGraphicsMarkerStyles(),
	mGraphicsAutomaticStyles(),
	mPageAutomaticStyles(),
	mPageMasterStyles(),
	mxStyle(), mxGradient(),
	miGradientIndex(1),
	miBitmapIndex(1),
	miStartMarkerIndex(1),
	miEndMarkerIndex(1),
	miDashIndex(1),
	miGraphicsStyleIndex(1),
	miPageIndex(1),
	mfWidth(0.0),
	mfMaxWidth(0.0),
	mfHeight(0.0),
	mfMaxHeight(0.0),
	mState()
{
}

OdpGeneratorPrivate::~OdpGeneratorPrivate()
{
	emptyStorage(&mGraphicsAutomaticStyles);
	emptyStorage(&mGraphicsStrokeDashStyles);
	emptyStorage(&mGraphicsGradientStyles);
	emptyStorage(&mGraphicsBitmapStyles);
	emptyStorage(&mGraphicsMarkerStyles);
	emptyStorage(&mPageAutomaticStyles);
	emptyStorage(&mPageMasterStyles);
}

void OdpGeneratorPrivate::writeNotesStyles(OdfDocumentHandler *pHandler)
{
	{
		librevenge::RVNGPropertyList styleProps;
		styleProps.insert("style:name", "PresentationNotesPage");
		styleProps.insert("style:family", "drawing-page");

		pHandler->startElement("style:style", styleProps);

		librevenge::RVNGPropertyList pageProps;
		pageProps.insert("presentation:display-header", "true");
		pageProps.insert("presentation:display-footer", "true");
		pageProps.insert("presentation:display-date-time", "true");
		pageProps.insert("presentation:display-page-number", "false");

		pHandler->startElement("style:drawing-page-properties", pageProps);
		pHandler->endElement("style:drawing-page-properties");

		pHandler->endElement("style:style");
	}

	{
		librevenge::RVNGPropertyList styleProps;
		styleProps.insert("style:name", "PresentationNotesFrame");
		styleProps.insert("style:family", "presentation");

		pHandler->startElement("style:style", styleProps);

		librevenge::RVNGPropertyList graphicProps;
		graphicProps.insert("draw:fill", "none");
		graphicProps.insert("fo:min-height", "5in");

		pHandler->startElement("style:graphic-properties", graphicProps);
		pHandler->endElement("style:graphic-properties");

		librevenge::RVNGPropertyList paraProps;
		paraProps.insert("fo:margin-left", "0.24in");
		paraProps.insert("fo:margin-right", "0in");
		paraProps.insert("fo:text-indent", "0in");

		pHandler->startElement("style:para-properties", paraProps);
		pHandler->endElement("style:para-properties");

		pHandler->endElement("style:style");
	}

	{
		librevenge::RVNGPropertyList styleProps;
		styleProps.insert("style:name", "PresentationNotesTextBox");
		styleProps.insert("style:family", "graphic");

		pHandler->startElement("style:style", styleProps);

		librevenge::RVNGPropertyList graphicProps;
		graphicProps.insert("draw:fill", "none");

		pHandler->startElement("style:graphic-properties", graphicProps);
		pHandler->endElement("style:graphic-properties");

		pHandler->endElement("style:style");
	}
}

void OdpGeneratorPrivate::_writeSettings(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:settings").write(pHandler);

	TagOpenElement configItemSetOpenElement("config:config-item-set");
	configItemSetOpenElement.addAttribute("config:name", "ooo:view-settings");
	configItemSetOpenElement.write(pHandler);

	TagOpenElement configItemOpenElement("config:config-item");

	configItemOpenElement.addAttribute("config:name", "VisibleAreaTop");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(pHandler);
	pHandler->characters("0");
	pHandler->endElement("config:config-item");

	configItemOpenElement.addAttribute("config:name", "VisibleAreaLeft");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(pHandler);
	pHandler->characters("0");
	pHandler->endElement("config:config-item");

	configItemOpenElement.addAttribute("config:name", "VisibleAreaWidth");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(pHandler);
	librevenge::RVNGString sWidth;
	sWidth.sprintf("%li", (unsigned long)(2540 * mfMaxWidth));
	pHandler->characters(sWidth);
	pHandler->endElement("config:config-item");

	configItemOpenElement.addAttribute("config:name", "VisibleAreaHeight");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(pHandler);
	librevenge::RVNGString sHeight;
	sHeight.sprintf("%li", (unsigned long)(2540 * mfMaxHeight));
	pHandler->characters(sHeight);
	pHandler->endElement("config:config-item");

	pHandler->endElement("config:config-item-set");

	pHandler->endElement("office:settings");
}

void OdpGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:automatic-styles").write(pHandler);

	// CHECKME: previously, this part was not done in STYLES

	// writing out the graphics automatic styles
	sendStorage(&mGraphicsAutomaticStyles, pHandler);

	mParagraphManager.write(pHandler);
	mSpanManager.write(pHandler);

	// writing out the lists styles
	for (std::vector<ListStyle *>::const_iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); ++iterListStyles)
		(*iterListStyles)->write(pHandler);
	// writing out the table styles
	for (std::vector<TableStyle *>::const_iterator iterTableStyles = mTableStyles.begin(); iterTableStyles != mTableStyles.end(); ++iterTableStyles)
	{
		(*iterTableStyles)->write(pHandler);
	}

	writeNotesStyles(pHandler);

	// CHECKME: previously, this part was not done in CONTENT
	_writePageLayouts(pHandler);

	pHandler->endElement("office:automatic-styles");
}

void OdpGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);
	sendStorage(&mGraphicsStrokeDashStyles, pHandler);
	sendStorage(&mGraphicsGradientStyles, pHandler);
	sendStorage(&mGraphicsBitmapStyles, pHandler);
	sendStorage(&mGraphicsMarkerStyles, pHandler);
	pHandler->endElement("office:styles");
}

void OdpGeneratorPrivate::_writePageLayouts(OdfDocumentHandler *pHandler)
{
#ifdef MULTIPAGE_WORKAROUND
	TagOpenElement tmpStylePageLayoutOpenElement("style:page-layout");
	tmpStylePageLayoutOpenElement.addAttribute("style:name", "PM0");
	tmpStylePageLayoutOpenElement.write(pHandler);

	TagOpenElement tmpStylePageLayoutPropertiesOpenElement("style:page-layout-properties");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-top", "0in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-bottom", "0in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-left", "0in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-right", "0in");
	librevenge::RVNGString sValue;
	sValue = doubleToString(mfMaxWidth);
	sValue.append("in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:page-width", sValue);
	sValue = doubleToString(mfMaxHeight);
	sValue.append("in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:page-height", sValue);
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("style:print-orientation", "portrait");
	tmpStylePageLayoutPropertiesOpenElement.write(pHandler);

	pHandler->endElement("style:page-layout-properties");
	pHandler->endElement("style:page-layout");

	TagOpenElement tmpStyleStyleOpenElement("style:style");
	tmpStyleStyleOpenElement.addAttribute("style:name", "dp1");
	tmpStyleStyleOpenElement.addAttribute("style:family", "drawing-page");
	tmpStyleStyleOpenElement.write(pHandler);

	TagOpenElement tmpStyleDrawingPagePropertiesOpenElement("style:drawing-page-properties");
	// tmpStyleDrawingPagePropertiesOpenElement.addAttribute("draw:background-size", "border");
	tmpStyleDrawingPagePropertiesOpenElement.addAttribute("draw:fill", "none");
	tmpStyleDrawingPagePropertiesOpenElement.write(pHandler);

	pHandler->endElement("style:drawing-page-properties");
	pHandler->endElement("style:style");
#else
	// writing out the page automatic styles
	sendStorage(&mPageAutomaticStyles, pHandler);
#endif
}

void OdpGeneratorPrivate::_writeMasterPages(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:master-styles").write(pHandler);
	sendStorage(&mPageMasterStyles, pHandler);
	pHandler->endElement("office:master-styles");
}

bool OdpGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	pHandler->startDocument();

	std::string const documentType=getDocumentType(streamType);
	TagOpenElement docContentPropList(documentType.c_str());
	docContentPropList.addAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	docContentPropList.addAttribute("xmlns:presentation", "urn:oasis:names:tc:opendocument:xmlns:presentation:1.0");
	docContentPropList.addAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	docContentPropList.addAttribute("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	docContentPropList.addAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	docContentPropList.addAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	docContentPropList.addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	docContentPropList.addAttribute("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	docContentPropList.addAttribute("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	docContentPropList.addAttribute("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	// WARNING: this is not ODF!
	docContentPropList.addAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	docContentPropList.addAttribute("xmlns:officeooo", "http://openoffice.org/2009/office");
	docContentPropList.addAttribute("office:version", "1.0", true);
	if (streamType == ODF_FLAT_XML)
		docContentPropList.addAttribute("office:mimetype", "application/vnd.oasis.opendocument.presentation");
	docContentPropList.write(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_META_XML))
		writeDocumentMetaData(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_SETTINGS_XML))
		_writeSettings(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML) || (streamType == ODF_STYLES_XML))
		mFontManager.writeFontsDeclaration(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
		_writeStyles(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML) || (streamType == ODF_STYLES_XML))
		_writeAutomaticStyles(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
		_writeMasterPages(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML))
	{
		TagOpenElement("office:body").write(pHandler);
		TagOpenElement("office:presentation").write(pHandler);
		sendStorage(&mBodyStorage, pHandler);
		pHandler->endElement("office:presentation");
		pHandler->endElement("office:body");
	}

	pHandler->endElement(documentType.c_str());

	pHandler->endDocument();
	return true;
}

OdpGenerator::OdpGenerator(): mpImpl(new OdpGeneratorPrivate)
{
}

OdpGenerator::~OdpGenerator()
{
	delete mpImpl;
}

void OdpGenerator::addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
{
	if (mpImpl)
		mpImpl->addDocumentHandler(pHandler, streamType);
}

void OdpGenerator::startDocument(const ::librevenge::RVNGPropertyList &/*propList*/)
{
}

void OdpGenerator::endDocument()
{
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}

void OdpGenerator::setDocumentMetaData(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdpGenerator::startSlide(const ::librevenge::RVNGPropertyList &propList)
{
	if (propList["svg:width"])
	{
		mpImpl->mfWidth = propList["svg:width"]->getDouble();
		mpImpl->mfMaxWidth = mpImpl->mfMaxWidth < mpImpl->mfWidth ? mpImpl->mfWidth : mpImpl->mfMaxWidth;
	}

	if (propList["svg:height"])
	{
		mpImpl->mfHeight = propList["svg:height"]->getDouble();
		mpImpl->mfMaxHeight = mpImpl->mfMaxHeight < mpImpl->mfHeight ? mpImpl->mfHeight : mpImpl->mfMaxHeight;
	}

	TagOpenElement *pStyleMasterPageOpenElement = new TagOpenElement("style:master-page");

	TagOpenElement *pDrawPageOpenElement = new TagOpenElement("draw:page");

	TagOpenElement *pStylePageLayoutOpenElement = new TagOpenElement("style:page-layout");

	librevenge::RVNGString sValue;
	if (propList["draw:name"])
		sValue = librevenge::RVNGString(propList["draw:name"]->getStr(), true); // escape special xml characters
	else
		sValue.sprintf("page%i", mpImpl->miPageIndex);
	pDrawPageOpenElement->addAttribute("draw:name", sValue);
#ifdef MULTIPAGE_WORKAROUND
	pStyleMasterPageOpenElement->addAttribute("style:page-layout-name", "PM0");
	pStylePageLayoutOpenElement->addAttribute("style:page-layout-name", "PM0");
#else
	sValue.sprintf("PM%i", mpImpl->miPageIndex);
	pStyleMasterPageOpenElement->addAttribute("style:page-layout-name", sValue);
	pStylePageLayoutOpenElement->addAttribute("style:name", sValue);
#endif

	mpImpl->mPageAutomaticStyles.push_back(pStylePageLayoutOpenElement);

	TagOpenElement *pStylePageLayoutPropertiesOpenElement = new TagOpenElement("style:page-layout-properties");
	pStylePageLayoutPropertiesOpenElement->addAttribute("fo:margin-top", "0in");
	pStylePageLayoutPropertiesOpenElement->addAttribute("fo:margin-bottom", "0in");
	pStylePageLayoutPropertiesOpenElement->addAttribute("fo:margin-left", "0in");
	pStylePageLayoutPropertiesOpenElement->addAttribute("fo:margin-right", "0in");
	sValue.sprintf("%s%s", doubleToString(mpImpl->mfWidth).cstr(), "in");
	pStylePageLayoutPropertiesOpenElement->addAttribute("fo:page-width", sValue);
	sValue.sprintf("%s%s", doubleToString(mpImpl->mfHeight).cstr(), "in");
	pStylePageLayoutPropertiesOpenElement->addAttribute("fo:page-height", sValue);
	pStylePageLayoutPropertiesOpenElement->addAttribute("style:print-orientation", "portrait");
	mpImpl->mPageAutomaticStyles.push_back(pStylePageLayoutPropertiesOpenElement);

	mpImpl->mPageAutomaticStyles.push_back(new TagCloseElement("style:page-layout-properties"));

	mpImpl->mPageAutomaticStyles.push_back(new TagCloseElement("style:page-layout"));

#ifdef MULTIPAGE_WORKAROUND
	pDrawPageOpenElement->addAttribute("draw:style-name", "dp1");
	pStyleMasterPageOpenElement->addAttribute("draw:style-name", "dp1");
#else
	sValue.sprintf("dp%i", mpImpl->miPageIndex);
	pDrawPageOpenElement->addAttribute("draw:style-name", sValue);
	pStyleMasterPageOpenElement->addAttribute("draw:style-name", sValue);
#endif

	TagOpenElement *pStyleStyleOpenElement = new TagOpenElement("style:style");
	pStyleStyleOpenElement->addAttribute("style:name", sValue);
	pStyleStyleOpenElement->addAttribute("style:family", "drawing-page");
	mpImpl->mPageAutomaticStyles.push_back(pStyleStyleOpenElement);

#ifdef MULTIPAGE_WORKAROUND
	pDrawPageOpenElement->addAttribute("draw:master-page-name", "Default");
	pStyleMasterPageOpenElement->addAttribute("style:name", "Default");
#else
	sValue.sprintf("Page%i", mpImpl->miPageIndex);
	pDrawPageOpenElement->addAttribute("draw:master-page-name", sValue);
	pStyleMasterPageOpenElement->addAttribute("style:name", sValue);
#endif

	mpImpl->getCurrentStorage()->push_back(pDrawPageOpenElement);

	mpImpl->mPageMasterStyles.push_back(pStyleMasterPageOpenElement);
	mpImpl->mPageMasterStyles.push_back(new TagCloseElement("style:master-page"));


	TagOpenElement *pStyleDrawingPagePropertiesOpenElement = new TagOpenElement("style:drawing-page-properties");
	pStyleDrawingPagePropertiesOpenElement->addAttribute("draw:fill", "none");
	mpImpl->mPageAutomaticStyles.push_back(pStyleDrawingPagePropertiesOpenElement);

	mpImpl->mPageAutomaticStyles.push_back(new TagCloseElement("style:drawing-page-properties"));

	mpImpl->mPageAutomaticStyles.push_back(new TagCloseElement("style:style"));
}

void OdpGenerator::endSlide()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:page"));
	mpImpl->miPageIndex++;
}

void OdpGenerator::setStyle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->mxStyle.clear();
	mpImpl->mxStyle = propList;
	const librevenge::RVNGPropertyListVector *gradient = propList.child("svg:linearGradient");
	if (!gradient)
		gradient = propList.child("svg:radialGradient");
	mpImpl->mxGradient = gradient ? *gradient : librevenge::RVNGPropertyListVector();
}

void OdpGenerator::startLayer(const ::librevenge::RVNGPropertyList & /* propList */)
{
}

void OdpGenerator::endLayer()
{
}

void OdpGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	if (!propList["svg:x"] || !propList["svg:y"] ||
	        !propList["svg:width"] || !propList["svg:height"])
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::drawRectangle: position undefined\n"));
		return;
	}
	mpImpl->_writeGraphicsStyle();
	TagOpenElement *pDrawRectElement = new TagOpenElement("draw:rect");
	librevenge::RVNGString sValue;
	sValue.sprintf("gr%i", mpImpl->miGraphicsStyleIndex-1);
	pDrawRectElement->addAttribute("draw:style-name", sValue);
	pDrawRectElement->addAttribute("svg:x", propList["svg:x"]->getStr());
	pDrawRectElement->addAttribute("svg:y", propList["svg:y"]->getStr());
	pDrawRectElement->addAttribute("svg:width", propList["svg:width"]->getStr());
	pDrawRectElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	// FIXME: what to do when rx != ry ?
	if (propList["svg:rx"])
		pDrawRectElement->addAttribute("draw:corner-radius", propList["svg:rx"]->getStr());
	else
		pDrawRectElement->addAttribute("draw:corner-radius", "0.0000in");
	mpImpl->getCurrentStorage()->push_back(pDrawRectElement);
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:rect"));
}

void OdpGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	if (!propList["svg:rx"] || !propList["svg:ry"] || !propList["svg:cx"] || !propList["svg:cy"])
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::drawEllipse: position undefined\n"));
		return;
	}
	mpImpl->_writeGraphicsStyle();
	TagOpenElement *pDrawEllipseElement = new TagOpenElement("draw:ellipse");
	librevenge::RVNGString sValue;
	sValue.sprintf("gr%i", mpImpl->miGraphicsStyleIndex-1);
	pDrawEllipseElement->addAttribute("draw:style-name", sValue);
	sValue = doubleToString(2 * propList["svg:rx"]->getDouble());
	sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:width", sValue);
	sValue = doubleToString(2 * propList["svg:ry"]->getDouble());
	sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:height", sValue);
	if (propList["librevenge:rotate"] && propList["librevenge:rotate"]->getDouble() != 0.0)
	{
		double rotation = propList["librevenge:rotate"]->getDouble();
		while (rotation < -180)
			rotation += 360;
		while (rotation > 180)
			rotation -= 360;
		double radrotation = rotation*M_PI/180.0;
		double deltax = sqrt(pow(propList["svg:rx"]->getDouble(), 2.0)
		                     + pow(propList["svg:ry"]->getDouble(), 2.0))*cos(atan(propList["svg:ry"]->getDouble()/propList["svg:rx"]->getDouble())
		                             - radrotation) - propList["svg:rx"]->getDouble();
		double deltay = sqrt(pow(propList["svg:rx"]->getDouble(), 2.0)
		                     + pow(propList["svg:ry"]->getDouble(), 2.0))*sin(atan(propList["svg:ry"]->getDouble()/propList["svg:rx"]->getDouble())
		                             - radrotation) - propList["svg:ry"]->getDouble();
		sValue = "rotate(";
		sValue.append(doubleToString(radrotation));
		sValue.append(") ");
		sValue.append("translate(");
		sValue.append(doubleToString(propList["svg:cx"]->getDouble() - propList["svg:rx"]->getDouble() - deltax));
		sValue.append("in, ");
		sValue.append(doubleToString(propList["svg:cy"]->getDouble() - propList["svg:ry"]->getDouble() - deltay));
		sValue.append("in)");
		pDrawEllipseElement->addAttribute("draw:transform", sValue);
	}
	else
	{
		sValue = doubleToString(propList["svg:cx"]->getDouble()-propList["svg:rx"]->getDouble());
		sValue.append("in");
		pDrawEllipseElement->addAttribute("svg:x", sValue);
		sValue = doubleToString(propList["svg:cy"]->getDouble()-propList["svg:ry"]->getDouble());
		sValue.append("in");
		pDrawEllipseElement->addAttribute("svg:y", sValue);
	}
	mpImpl->getCurrentStorage()->push_back(pDrawEllipseElement);
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:ellipse"));
}

void OdpGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	const ::librevenge::RVNGPropertyListVector *vertices = propList.child("svg:points");
	if (vertices && vertices->count())
		mpImpl->_drawPolySomething(*vertices, false);
}

void OdpGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	const ::librevenge::RVNGPropertyListVector *vertices = propList.child("svg:points");
	if (vertices && vertices->count())
		mpImpl->_drawPolySomething(*vertices, true);
}

void OdpGeneratorPrivate::_drawPolySomething(const ::librevenge::RVNGPropertyListVector &vertices, bool isClosed)
{
	if (vertices.count() < 2)
		return;

	if (vertices.count() == 2)
	{
		if (!vertices[0]["svg:x"]||!vertices[0]["svg:y"]||!vertices[1]["svg:x"]||!vertices[1]["svg:y"])
		{
			ODFGEN_DEBUG_MSG(("OdpGeneratorPrivate::_drawPolySomething: some vertices are not defined\n"));
			return;
		}
		_writeGraphicsStyle();
		TagOpenElement *pDrawLineElement = new TagOpenElement("draw:line");
		librevenge::RVNGString sValue;
		sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
		pDrawLineElement->addAttribute("draw:style-name", sValue);
		pDrawLineElement->addAttribute("draw:layer", "layout");
		pDrawLineElement->addAttribute("svg:x1", vertices[0]["svg:x"]->getStr());
		pDrawLineElement->addAttribute("svg:y1", vertices[0]["svg:y"]->getStr());
		pDrawLineElement->addAttribute("svg:x2", vertices[1]["svg:x"]->getStr());
		pDrawLineElement->addAttribute("svg:y2", vertices[1]["svg:y"]->getStr());
		getCurrentStorage()->push_back(pDrawLineElement);
		getCurrentStorage()->push_back(new TagCloseElement("draw:line"));
	}
	else
	{
		::librevenge::RVNGPropertyListVector path;
		::librevenge::RVNGPropertyList element;

		for (unsigned long ii = 0; ii < vertices.count(); ++ii)
		{
			element = vertices[ii];
			if (ii == 0)
				element.insert("librevenge:path-action", "M");
			else
				element.insert("librevenge:path-action", "L");
			path.append(element);
			element.clear();
		}
		if (isClosed)
		{
			element.insert("librevenge:path-action", "Z");
			path.append(element);
		}
		_drawPath(path);
	}
}

void OdpGeneratorPrivate::_drawPath(const librevenge::RVNGPropertyListVector &path)
{
	if (path.count() == 0)
		return;

	double px = 0.0, py = 0.0, qx = 0.0, qy = 0.0;
	if (!libodfgen::getPathBBox(path, px, py, qx, qy))
		return;

	librevenge::RVNGString sValue;
	_writeGraphicsStyle();
	TagOpenElement *pDrawPathElement = new TagOpenElement("draw:path");
	sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
	pDrawPathElement->addAttribute("draw:style-name", sValue);
	pDrawPathElement->addAttribute("draw:layer", "layout");
	sValue = doubleToString(px);
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:x", sValue);
	sValue = doubleToString(py);
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:y", sValue);
	sValue = doubleToString((qx - px));
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:width", sValue);
	sValue = doubleToString((qy - py));
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:height", sValue);
	sValue.sprintf("%i %i %i %i", 0, 0, (unsigned)(2540*(qx - px)), (unsigned)(2540*(qy - py)));
	pDrawPathElement->addAttribute("svg:viewBox", sValue);

	pDrawPathElement->addAttribute("svg:d", libodfgen::convertPath(path, px, py));
	getCurrentStorage()->push_back(pDrawPathElement);
	getCurrentStorage()->push_back(new TagCloseElement("draw:path"));
}

void OdpGenerator::drawPath(const librevenge::RVNGPropertyList &propList)
{
	const librevenge::RVNGPropertyListVector *path = propList.child("svg:d");
	if (path && path->count())
		mpImpl->_drawPath(*path);
}

void OdpGenerator::drawGraphicObject(const ::librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:mime-type"] || propList["librevenge:mime-type"]->getStr().len() <= 0)
		return;
	if (!propList["office:binary-data"])
		return;
	if (!propList["svg:x"] || !propList["svg:y"] || !propList["svg:width"] || !propList["svg:height"])
		return;

	bool flipX(propList["draw:mirror-horizontal"] && propList["draw:mirror-horizontal"]->getInt());
	bool flipY(propList["draw:mirror-vertical"] && propList["draw:mirror-vertical"]->getInt());
	if ((flipX && !flipY) || (!flipX && flipY))
		mpImpl->mxStyle.insert("style:mirror", "horizontal");
	else
		mpImpl->mxStyle.insert("style:mirror", "none");
	if (propList["draw:color-mode"])
		mpImpl->mxStyle.insert("draw:color-mode", propList["draw:color-mode"]->getStr());
	if (propList["draw:luminance"])
		mpImpl->mxStyle.insert("draw:luminance", propList["draw:luminance"]->getStr());
	if (propList["draw:contrast"])
		mpImpl->mxStyle.insert("draw:contrast", propList["draw:contrast"]->getStr());
	if (propList["draw:gamma"])
		mpImpl->mxStyle.insert("draw:gamma", propList["draw:gamma"]->getStr());
	if (propList["draw:red"])
		mpImpl->mxStyle.insert("draw:red", propList["draw:red"]->getStr());
	if (propList["draw:green"])
		mpImpl->mxStyle.insert("draw:green", propList["draw:green"]->getStr());
	if (propList["draw:blue"])
		mpImpl->mxStyle.insert("draw:blue", propList["draw:blue"]->getStr());


	mpImpl->_writeGraphicsStyle();

	double x = propList["svg:x"]->getDouble();
	double y = propList["svg:y"]->getDouble();
	double height = propList["svg:height"]->getDouble();
	double width = propList["svg:width"]->getDouble();

	if (flipY)
	{
		x += width;
		y += height;
		width *= -1.0;
		height *= -1.0;
	}

	double angle(propList["librevenge:rotate"] ? - M_PI * propList["librevenge:rotate"]->getDouble() / 180.0 : 0.0);
	if (angle != 0.0)
	{
		double deltax((width*cos(angle)+height*sin(angle)-width)/2.0);
		double deltay((-width*sin(angle)+height*cos(angle)-height)/2.0);
		x -= deltax;
		y -= deltay;
	}

	librevenge::RVNGPropertyList framePropList;

	framePropList.insert("svg:x", x);
	framePropList.insert("svg:y", y);
	framePropList.insert("svg:height", height);
	framePropList.insert("svg:width", width);

	TagOpenElement *pDrawFrameElement = new TagOpenElement("draw:frame");

	librevenge::RVNGString sValue;
	sValue.sprintf("gr%i", mpImpl->miGraphicsStyleIndex-1);
	pDrawFrameElement->addAttribute("draw:style-name", sValue);

	pDrawFrameElement->addAttribute("svg:height", framePropList["svg:height"]->getStr());
	pDrawFrameElement->addAttribute("svg:width", framePropList["svg:width"]->getStr());

	if (angle != 0.0)
	{
		framePropList.insert("librevenge:rotate", angle, librevenge::RVNG_GENERIC);
		sValue.sprintf("rotate (%s) translate(%s, %s)",
		               framePropList["librevenge:rotate"]->getStr().cstr(),
		               framePropList["svg:x"]->getStr().cstr(),
		               framePropList["svg:y"]->getStr().cstr());
		pDrawFrameElement->addAttribute("draw:transform", sValue);
	}
	else
	{
		pDrawFrameElement->addAttribute("svg:x", framePropList["svg:x"]->getStr());
		pDrawFrameElement->addAttribute("svg:y", framePropList["svg:y"]->getStr());
	}
	mpImpl->getCurrentStorage()->push_back(pDrawFrameElement);

	mpImpl->insertBinaryObject(propList);

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
}

void OdpGenerator::drawConnector(const ::librevenge::RVNGPropertyList &/*propList*/)
{
}

void OdpGeneratorPrivate::_writeGraphicsStyle()
{
	TagOpenElement *pStyleStyleElement = new TagOpenElement("style:style");
	librevenge::RVNGString sValue;
	sValue.sprintf("gr%i",  miGraphicsStyleIndex);
	pStyleStyleElement->addAttribute("style:name", sValue);
	pStyleStyleElement->addAttribute("style:family", "graphic");
	pStyleStyleElement->addAttribute("style:parent-style-name", "standard");
	mGraphicsAutomaticStyles.push_back(pStyleStyleElement);

	TagOpenElement *pStyleGraphicsPropertiesElement = new TagOpenElement("style:graphic-properties");
	_updateGraphicPropertiesElement(*pStyleGraphicsPropertiesElement, mxStyle);
	mGraphicsAutomaticStyles.push_back(pStyleGraphicsPropertiesElement);
	mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:style"));
	miGraphicsStyleIndex++;
}

void OdpGeneratorPrivate::_updateGraphicPropertiesElement(TagOpenElement &element, ::librevenge::RVNGPropertyList const &style)
{
	bool bUseOpacityGradient = false;
	const librevenge::RVNGPropertyListVector *gradient = style.child("svg:linearGradient");
	if (!gradient)
		gradient = style.child("svg:radialGradient");


	if (style["draw:stroke"] && style["draw:stroke"]->getStr() == "dash")
	{
		TagOpenElement *pDrawStrokeDashElement = new TagOpenElement("draw:stroke-dash");
		librevenge::RVNGString sValue;
		sValue.sprintf("Dash_%i", miDashIndex++);
		pDrawStrokeDashElement->addAttribute("draw:name", sValue);
		if (style["svg:stoke-linecap"])
			pDrawStrokeDashElement->addAttribute("draw:style", style["svg:stroke-linecap"]->getStr());
		else
			pDrawStrokeDashElement->addAttribute("draw:style", "rect");
		if (style["draw:distance"])
			pDrawStrokeDashElement->addAttribute("draw:distance", style["draw:distance"]->getStr());
		if (style["draw:dots1"])
			pDrawStrokeDashElement->addAttribute("draw:dots1", style["draw:dots1"]->getStr());
		if (style["draw:dots1-length"])
			pDrawStrokeDashElement->addAttribute("draw:dots1-length", style["draw:dots1-length"]->getStr());
		if (style["draw:dots2"])
			pDrawStrokeDashElement->addAttribute("draw:dots2", style["draw:dots2"]->getStr());
		if (style["draw:dots2-length"])
			pDrawStrokeDashElement->addAttribute("draw:dots2-length", style["draw:dots2-length"]->getStr());
		mGraphicsStrokeDashStyles.push_back(pDrawStrokeDashElement);
		mGraphicsStrokeDashStyles.push_back(new TagCloseElement("draw:stroke-dash"));
	}

	if (style["draw:marker-start-path"])
	{
		librevenge::RVNGString sValue;
		TagOpenElement *pDrawMarkerElement = new TagOpenElement("draw:marker");
		sValue.sprintf("StartMarker_%i", miStartMarkerIndex);
		pDrawMarkerElement->addAttribute("draw:name", sValue);
		if (style["draw:marker-start-viewbox"])
			pDrawMarkerElement->addAttribute("svg:viewBox", style["draw:marker-start-viewbox"]->getStr());
		pDrawMarkerElement->addAttribute("svg:d", style["draw:marker-start-path"]->getStr());
		mGraphicsMarkerStyles.push_back(pDrawMarkerElement);
		mGraphicsMarkerStyles.push_back(new TagCloseElement("draw:marker"));
	}
	if (style["draw:marker-end-path"])
	{
		librevenge::RVNGString sValue;
		TagOpenElement *pDrawMarkerElement = new TagOpenElement("draw:marker");
		sValue.sprintf("EndMarker_%i", miEndMarkerIndex);
		pDrawMarkerElement->addAttribute("draw:name", sValue);
		if (style["draw:marker-end-viewbox"])
			pDrawMarkerElement->addAttribute("svg:viewBox", style["draw:marker-end-viewbox"]->getStr());
		pDrawMarkerElement->addAttribute("svg:d", style["draw:marker-end-path"]->getStr());
		mGraphicsMarkerStyles.push_back(pDrawMarkerElement);
		mGraphicsMarkerStyles.push_back(new TagCloseElement("draw:marker"));
	}

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "gradient")
	{
		TagOpenElement *pDrawGradientElement = new TagOpenElement("draw:gradient");
		TagOpenElement *pDrawOpacityElement = new TagOpenElement("draw:opacity");
		if (style["draw:style"])
		{
			pDrawGradientElement->addAttribute("draw:style", style["draw:style"]->getStr());
			pDrawOpacityElement->addAttribute("draw:style", style["draw:style"]->getStr());
		}
		else
		{
			pDrawGradientElement->addAttribute("draw:style", "linear");
			pDrawOpacityElement->addAttribute("draw:style", "linear");
		}
		librevenge::RVNGString sValue;
		sValue.sprintf("Gradient_%i", miGradientIndex);
		pDrawGradientElement->addAttribute("draw:name", sValue);
		sValue.sprintf("Transparency_%i", miGradientIndex++);
		pDrawOpacityElement->addAttribute("draw:name", sValue);

		// ODG angle unit is 0.1 degree
		double angle = style["draw:angle"] ? style["draw:angle"]->getDouble() : 0.0;
		while (angle < 0)
			angle += 360;
		while (angle > 360)
			angle -= 360;
		sValue.sprintf("%i", (unsigned)(angle*10));
		pDrawGradientElement->addAttribute("draw:angle", sValue);
		pDrawOpacityElement->addAttribute("draw:angle", sValue);

		if (!gradient || !gradient->count())
		{
			if (style["draw:start-color"])
				pDrawGradientElement->addAttribute("draw:start-color", style["draw:start-color"]->getStr());
			if (style["draw:end-color"])
				pDrawGradientElement->addAttribute("draw:end-color", style["draw:end-color"]->getStr());

			if (style["draw:border"])
			{
				pDrawGradientElement->addAttribute("draw:border", style["draw:border"]->getStr());
				pDrawOpacityElement->addAttribute("draw:border", style["draw:border"]->getStr());
			}
			else
			{
				pDrawGradientElement->addAttribute("draw:border", "0%");
				pDrawOpacityElement->addAttribute("draw:border", "0%");
			}

			if (style["svg:cx"])
			{
				pDrawGradientElement->addAttribute("draw:cx", style["svg:cx"]->getStr());
				pDrawOpacityElement->addAttribute("draw:cx", style["svg:cx"]->getStr());
			}
			else if (style["draw:cx"])
			{
				pDrawGradientElement->addAttribute("draw:cx", style["draw:cx"]->getStr());
				pDrawOpacityElement->addAttribute("draw:cx", style["draw:cx"]->getStr());
			}

			if (style["svg:cy"])
			{
				pDrawGradientElement->addAttribute("draw:cy", style["svg:cy"]->getStr());
				pDrawOpacityElement->addAttribute("draw:cy", style["svg:cy"]->getStr());
			}
			else if (style["draw:cx"])
			{
				pDrawGradientElement->addAttribute("draw:cx", style["svg:cx"]->getStr());
				pDrawOpacityElement->addAttribute("draw:cx", style["svg:cx"]->getStr());
			}

			if (style["draw:start-intensity"])
				pDrawGradientElement->addAttribute("draw:start-intensity", style["draw:start-intensity"]->getStr());
			else
				pDrawGradientElement->addAttribute("draw:start-intensity", "100%");

			if (style["draw:end-intensity"])
				pDrawGradientElement->addAttribute("draw:end-intensity", style["draw:end-intensity"]->getStr());
			else
				pDrawGradientElement->addAttribute("draw:end-intensity", "100%");

			if (style["librevenge:start-opacity"])
				pDrawOpacityElement->addAttribute("draw:start", style["librevenge:start-opacity"]->getStr());
			else
				pDrawOpacityElement->addAttribute("draw:start", "100%");

			if (style["librevenge:end-opacity"])
				pDrawOpacityElement->addAttribute("draw:end", style["librevenge:end-opacity"]->getStr());
			else
				pDrawOpacityElement->addAttribute("draw:end", "100%");

			mGraphicsGradientStyles.push_back(pDrawGradientElement);
			mGraphicsGradientStyles.push_back(new TagCloseElement("draw:gradient"));

			// Work around a mess in LibreOffice where both opacities of 100% are interpreted as complete transparency
			// Nevertheless, when one is different, immediately, they are interpreted correctly
			if (style["librevenge:start-opacity"] && style["librevenge:end-opacity"]
			        && (style["librevenge:start-opacity"]->getDouble() != 1.0 || style["librevenge:end-opacity"]->getDouble() != 1.0))
			{
				bUseOpacityGradient = true;
				mGraphicsGradientStyles.push_back(pDrawOpacityElement);
				mGraphicsGradientStyles.push_back(new TagCloseElement("draw:opacity"));
			}
		}
		else if (gradient->count() >= 2)
		{
			sValue.sprintf("%i", (unsigned)(angle*10));
			pDrawGradientElement->addAttribute("draw:angle", sValue);

			pDrawGradientElement->addAttribute("draw:start-color", (*gradient)[1]["svg:stop-color"]->getStr());
			pDrawGradientElement->addAttribute("draw:end-color", (*gradient)[0]["svg:stop-color"]->getStr());
			if (style["svg:cx"])
				pDrawGradientElement->addAttribute("draw:cx", style["svg:cx"]->getStr());
			if (style["svg:cy"])
				pDrawGradientElement->addAttribute("draw:cy", style["svg:cy"]->getStr());
			if ((*gradient)[1]["svg:stop-opacity"])
			{
				pDrawOpacityElement->addAttribute("draw:start", (*gradient)[1]["svg:stop-opacity"]->getStr());
				bUseOpacityGradient = true;
			}
			else
				pDrawOpacityElement->addAttribute("draw:start", "100%");
			if ((*gradient)[0]["svg:stop-opacity"])
			{
				pDrawOpacityElement->addAttribute("draw:end", (*gradient)[0]["svg:stop-opacity"]->getStr());
				bUseOpacityGradient = true;
			}
			else
				pDrawOpacityElement->addAttribute("draw:end", "100%");
			pDrawGradientElement->addAttribute("draw:border", "0%");
			mGraphicsGradientStyles.push_back(pDrawGradientElement);
			mGraphicsGradientStyles.push_back(new TagCloseElement("draw:gradient"));
			if (bUseOpacityGradient)
			{
				mGraphicsGradientStyles.push_back(pDrawOpacityElement);
				mGraphicsGradientStyles.push_back(new TagCloseElement("draw:opacity"));
			}
		}
		else
		{
			/* if gradient.count() == 1 for some reason we would leak
			 * pDrawGradientElement
			 */
			delete pDrawGradientElement;
		}
		if (!bUseOpacityGradient)
			delete pDrawOpacityElement;
	}

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "bitmap" &&
	        style["draw:fill-image"] && style["librevenge:mime-type"])
	{
		TagOpenElement *pDrawBitmapElement = new TagOpenElement("draw:fill-image");
		librevenge::RVNGString sValue;
		sValue.sprintf("Bitmap_%i", miBitmapIndex++);
		pDrawBitmapElement->addAttribute("draw:name", sValue);
		mGraphicsBitmapStyles.push_back(pDrawBitmapElement);
		mGraphicsBitmapStyles.push_back(new TagOpenElement("office:binary-data"));
		mGraphicsBitmapStyles.push_back(new CharDataElement(style["draw:fill-image"]->getStr()));
		mGraphicsBitmapStyles.push_back(new TagCloseElement("office:binary-data"));
		mGraphicsBitmapStyles.push_back(new TagCloseElement("draw:fill-image"));
	}

	if (style["draw:color-mode"] && style["draw:color-mode"]->getStr().len() > 0)
		element.addAttribute("draw:color-mode", style["draw:color-mode"]->getStr());
	if (style["draw:luminance"] && style["draw:luminance"]->getStr().len() > 0)
		element.addAttribute("draw:luminance", style["draw:luminance"]->getStr());
	if (style["draw:contrast"] && style["draw:contrast"]->getStr().len() > 0)
		element.addAttribute("draw:contrast", style["draw:contrast"]->getStr());
	if (style["draw:gamma"] && style["draw:gamma"]->getStr().len() > 0)
		element.addAttribute("draw:gamma", style["draw:gamma"]->getStr());
	if (style["draw:red"] && style["draw:red"]->getStr().len() > 0)
		element.addAttribute("draw:red", style["draw:red"]->getStr());
	if (style["draw:green"] && style["draw:green"]->getStr().len() > 0)
		element.addAttribute("draw:green", style["draw:green"]->getStr());
	if (style["draw:blue"] && style["draw:blue"]->getStr().len() > 0)
		element.addAttribute("draw:blue", style["draw:blue"]->getStr());

	librevenge::RVNGString sValue;
	if (style["draw:stroke"] && style["draw:stroke"]->getStr() == "none")
		element.addAttribute("draw:stroke", "none");
	else
	{
		if (style["svg:stroke-width"])
			element.addAttribute("svg:stroke-width", style["svg:stroke-width"]->getStr());

		if (style["svg:stroke-color"])
			element.addAttribute("svg:stroke-color", style["svg:stroke-color"]->getStr());

		if (style["svg:stroke-opacity"])
			element.addAttribute("svg:stroke-opacity", style["svg:stroke-opacity"]->getStr());

		if (style["svg:stroke-linejoin"])
			element.addAttribute("draw:stroke-linejoin", style["svg:stroke-linejoin"]->getStr());

		if (style["svg:stroke-linecap"])
			element.addAttribute("svg:stoke-linecap", style["svg:stroke-linecap"]->getStr());

		if (style["draw:stroke"] && style["draw:stroke"]->getStr() == "dash")
		{
			element.addAttribute("draw:stroke", "dash");
			sValue.sprintf("Dash_%i", miDashIndex-1);
			element.addAttribute("draw:stroke-dash", sValue);
		}
		else
			element.addAttribute("draw:stroke", "solid");
	}

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "none")
		element.addAttribute("draw:fill", "none");
	else
	{
		if (style["draw:shadow"])
			element.addAttribute("draw:shadow", style["draw:shadow"]->getStr());
		else
			element.addAttribute("draw:shadow", "hidden");
		if (style["draw:shadow-offset-x"])
			element.addAttribute("draw:shadow-offset-x", style["draw:shadow-offset-x"]->getStr());
		if (style["draw:shadow-offset-y"])
			element.addAttribute("draw:shadow-offset-y", style["draw:shadow-offset-y"]->getStr());
		if (style["draw:shadow-color"])
			element.addAttribute("draw:shadow-color", style["draw:shadow-color"]->getStr());
		if (style["draw:shadow-opacity"])
			element.addAttribute("draw:shadow-opacity", style["draw:shadow-opacity"]->getStr());
		if (style["svg:fill-rule"])
			element.addAttribute("svg:fill-rule", style["svg:fill-rule"]->getStr());
	}

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "solid")
	{
		element.addAttribute("draw:fill", "solid");
		if (style["draw:fill-color"])
			element.addAttribute("draw:fill-color", style["draw:fill-color"]->getStr());
		if (style["draw:opacity"])
			element.addAttribute("draw:opacity", style["draw:opacity"]->getStr());
	}

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "gradient")
	{
		if (!gradient || !gradient->count() || gradient->count() >= 2)
		{
			element.addAttribute("draw:fill", "gradient");
			sValue.sprintf("Gradient_%i", miGradientIndex-1);
			element.addAttribute("draw:fill-gradient-name", sValue);
			if (bUseOpacityGradient)
			{
				sValue.sprintf("Transparency_%i", miGradientIndex-1);
				element.addAttribute("draw:opacity-name", sValue);
			}
		}
		else
		{
			if ((*gradient)[0]["svg:stop-color"])
			{
				element.addAttribute("draw:fill", "solid");
				element.addAttribute("draw:fill-color", (*gradient)[0]["svg:stop-color"]->getStr());
			}
			else
				element.addAttribute("draw:fill", "solid");
		}
	}

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "bitmap")
	{
		if (style["draw:fill-image"] && style["librevenge:mime-type"])
		{
			element.addAttribute("draw:fill", "bitmap");
			sValue.sprintf("Bitmap_%i", miBitmapIndex-1);
			element.addAttribute("draw:fill-image-name", sValue);
			if (style["draw:fill-image-width"])
				element.addAttribute("draw:fill-image-width", style["draw:fill-image-width"]->getStr());
			else if (style["svg:width"])
				element.addAttribute("draw:fill-image-width", style["svg:width"]->getStr());
			if (style["draw:fill-image-height"])
				element.addAttribute("draw:fill-image-height", style["draw:fill-image-height"]->getStr());
			else if (style["svg:height"])
				element.addAttribute("draw:fill-image-height", style["svg:height"]->getStr());
			if (style["style:repeat"])
				element.addAttribute("style:repeat", style["style:repeat"]->getStr());
			if (style["draw:fill-image-ref-point"])
				element.addAttribute("draw:fill-image-ref-point", style["draw:fill-image-ref-point"]->getStr());
			if (style["draw:fill-image-ref-point-x"])
				element.addAttribute("draw:fill-image-ref-point-x", style["draw:fill-image-ref-point-x"]->getStr());
			if (style["draw:fill-image-ref-point-y"])
				element.addAttribute("draw:fill-image-ref-point-y", style["draw:fill-image-ref-point-y"]->getStr());
		}
		else
			element.addAttribute("draw:fill", "none");
	}


	if (style["draw:marker-start-path"])
	{
		sValue.sprintf("StartMarker_%i", miStartMarkerIndex++);
		element.addAttribute("draw:marker-start", sValue);
		if (style["draw:marker-start-center"])
			element.addAttribute("draw:marker-start-center", style["draw:marker-start-center"]->getStr());
		if (style["draw:marker-start-width"])
			element.addAttribute("draw:marker-start-width", style["draw:marker-start-width"]->getStr());
		else
			element.addAttribute("draw:marker-start-width", "0.118in");
	}
	if (style["draw:marker-end-path"])
	{
		sValue.sprintf("EndMarker_%i", miEndMarkerIndex++);
		element.addAttribute("draw:marker-end", sValue);
		if (style["draw:marker-end-center"])
			element.addAttribute("draw:marker-end-center", style["draw:marker-end-center"]->getStr());
		if (style["draw:marker-end-width"])
			element.addAttribute("draw:marker-end-width", style["draw:marker-end-width"]->getStr());
		else
			element.addAttribute("draw:marker-end-width", "0.118in");
	}
	if (style["style:mirror"])
		element.addAttribute("style:mirror", style["style:mirror"]->getStr());
}

void OdpGenerator::startEmbeddedGraphics(const librevenge::RVNGPropertyList &)
{
}

void OdpGenerator::endEmbeddedGraphics()
{
}

void OdpGenerator::startGroup(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:g"));
}

void OdpGenerator::endGroup()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:g"));
}

void OdpGenerator::startTextObject(const librevenge::RVNGPropertyList &propList)
{
	TagOpenElement *pDrawFrameOpenElement = new TagOpenElement("draw:frame");
	TagOpenElement *pStyleStyleOpenElement = new TagOpenElement("style:style");

	librevenge::RVNGString sValue;
	sValue.sprintf("gr%i",  mpImpl->miGraphicsStyleIndex++);
	pStyleStyleOpenElement->addAttribute("style:name", sValue);
	pStyleStyleOpenElement->addAttribute("style:family", "graphic");
	pStyleStyleOpenElement->addAttribute("style:parent-style-name", "standard");
	mpImpl->mGraphicsAutomaticStyles.push_back(pStyleStyleOpenElement);

	pDrawFrameOpenElement->addAttribute("draw:style-name", sValue);
	pDrawFrameOpenElement->addAttribute("draw:layer", "layout");

	TagOpenElement *pStyleGraphicPropertiesOpenElement = new TagOpenElement("style:graphic-properties");
	librevenge::RVNGPropertyList styleList(propList);
	if (!propList["draw:stroke"])
		styleList.insert("draw:stroke", "none");
	if (!propList["draw:fill"])
		styleList.insert("draw:fill", "none");
	// the transformation is managed latter, so even if this changes nothing...
	if (propList["librevenge:rotate"])
		styleList.insert("librevenge:rotate", 0);
	mpImpl->_updateGraphicPropertiesElement(*pStyleGraphicPropertiesOpenElement, styleList);

	if (!propList["svg:width"] && !propList["svg:height"])
	{
		if (!propList["fo:min-width"])
		{
			pDrawFrameOpenElement->addAttribute("fo:min-width", "1in");
			pStyleGraphicPropertiesOpenElement->addAttribute("fo:min-width", "1in");
		}
		pDrawFrameOpenElement->addAttribute("svg:width", "10in");
	}
	else
	{
		if (propList["svg:width"])
			pDrawFrameOpenElement->addAttribute("svg:width", propList["svg:width"]->getStr());
		if (propList["svg:height"])
			pDrawFrameOpenElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	}
	if (propList["fo:min-width"])
	{
		pDrawFrameOpenElement->addAttribute("fo:min-width", propList["fo:min-width"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:min-width", propList["fo:min-width"]->getStr());
	}
	if (propList["fo:min-height"])
	{
		pDrawFrameOpenElement->addAttribute("fo:min-height", propList["fo:min-height"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:min-height", propList["fo:min-height"]->getStr());
	}
	if (propList["fo:max-width"])
	{
		pDrawFrameOpenElement->addAttribute("fo:max-width", propList["fo:max-height"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:max-width", propList["fo:max-width"]->getStr());
	}
	if (propList["fo:max-height"])
	{
		pDrawFrameOpenElement->addAttribute("fo:max-height", propList["fo:max-height"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:max-height", propList["fo:max-height"]->getStr());
	}
	if (propList["fo:padding-top"])
	{
		pDrawFrameOpenElement->addAttribute("fo:padding-top", propList["fo:padding-top"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:padding-top", propList["fo:padding-top"]->getStr());
	}
	if (propList["fo:padding-bottom"])
	{
		pDrawFrameOpenElement->addAttribute("fo:padding-bottom", propList["fo:padding-bottom"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:padding-bottom", propList["fo:padding-bottom"]->getStr());
	}
	if (propList["fo:padding-left"])
	{
		pDrawFrameOpenElement->addAttribute("fo:padding-left", propList["fo:padding-left"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:padding-left", propList["fo:padding-left"]->getStr());
	}
	if (propList["fo:padding-right"])
	{
		pDrawFrameOpenElement->addAttribute("fo:padding-right", propList["fo:padding-right"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("fo:padding-right", propList["fo:padding-right"]->getStr());
	}
	if (propList["draw:textarea-vertical-align"])
	{
		pDrawFrameOpenElement->addAttribute("draw:textarea-vertical-align", propList["draw:textarea-vertical-align"]->getStr());
		pStyleGraphicPropertiesOpenElement->addAttribute("draw:textarea-vertical-align", propList["draw:textarea-vertical-align"]->getStr());
	}

	double x = 0.0;
	double y = 0.0;
	if (propList["svg:x"])
		x = propList["svg:x"]->getDouble();
	if (propList["svg:y"])
		y = propList["svg:y"]->getDouble();
	double angle(propList["librevenge:rotate"] ? - M_PI * propList["librevenge:rotate"]->getDouble() / 180.0 : 0.0);
	if (angle != 0.0)
	{
		// compute position: make sure that the center position remains invariant
		double width = 0.0;
		double height = 0.0;
		if (propList["librevenge:rotate-cx"])
			width = 2.0*(propList["librevenge:rotate-cx"]->getDouble()-x);
		else if (propList["svg:width"])
			width = propList["svg:width"]->getDouble();
		if (propList["librevenge:rotate-cy"])
			height = 2.0*(propList["librevenge:rotate-cy"]->getDouble()-y);
		else if (propList["svg:height"])
			height = propList["svg:height"]->getDouble();
		double deltax((width*cos(angle)+height*sin(angle)-width)/2.0);
		double deltay((-width*sin(angle)+height*cos(angle)-height)/2.0);
		x -= deltax;
		y -= deltay;
	}
	librevenge::RVNGProperty *svg_x = librevenge::RVNGPropertyFactory::newInchProp(x);
	librevenge::RVNGProperty *svg_y = librevenge::RVNGPropertyFactory::newInchProp(y);
	if (angle != 0.0)
	{
		librevenge::RVNGProperty *librevenge_rotate = librevenge::RVNGPropertyFactory::newDoubleProp(angle);
		sValue.sprintf("rotate (%s) translate(%s, %s)",
		               librevenge_rotate->getStr().cstr(),
		               svg_x->getStr().cstr(),
		               svg_y->getStr().cstr());
		delete librevenge_rotate;
		pDrawFrameOpenElement->addAttribute("draw:transform", sValue);
	}
	else
	{
		if (propList["svg:x"])
			pDrawFrameOpenElement->addAttribute("svg:x", svg_x->getStr());
		if (propList["svg:y"])
			pDrawFrameOpenElement->addAttribute("svg:y", svg_y->getStr());
	}
	delete svg_x;
	delete svg_y;
	mpImpl->getCurrentStorage()->push_back(pDrawFrameOpenElement);
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:text-box"));
	mpImpl->mGraphicsAutomaticStyles.push_back(pStyleGraphicPropertiesOpenElement);
	mpImpl->mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));
	mpImpl->mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:style"));
	mpImpl->mState.mbIsTextBox = true;
	mpImpl->pushListState();
}

void OdpGenerator::endTextObject()
{
	if (mpImpl->mState.mbIsTextBox)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
		mpImpl->mState.mbIsTextBox = false;
		mpImpl->popListState();
	}
}

void OdpGenerator::defineParagraphStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineParagraphStyle(propList);
}

void OdpGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openParagraph(propList);
}

void OdpGenerator::closeParagraph()
{
	mpImpl->closeParagraph();
}

void OdpGenerator::defineCharacterStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineCharacterStyle(propList);
}

void OdpGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openSpan(propList);
}

void OdpGenerator::closeSpan()
{
	mpImpl->closeSpan();
}

void OdpGenerator::insertText(const librevenge::RVNGString &text)
{
	mpImpl->insertText(text);
}

void OdpGenerator::insertTab()
{
	mpImpl->insertTab();
}

void OdpGenerator::insertSpace()
{
	mpImpl->insertSpace();
}

void OdpGenerator::insertLineBreak()
{
	mpImpl->insertLineBreak();
}

void OdpGenerator::insertField(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->insertField(propList);
}

void OdpGenerator::defineOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineListLevel(propList, true);
}

void OdpGenerator::defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineListLevel(propList, false);
}

void OdpGenerator::openOrderedListLevel(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListLevel(propList, true);
}

void OdpGenerator::openUnorderedListLevel(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListLevel(propList, false);
}

void OdpGenerator::closeOrderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdpGenerator::closeUnorderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdpGenerator::openListElement(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListElement(propList);
}

void OdpGenerator::closeListElement()
{
	mpImpl->closeListElement();
}

void OdpGenerator::openTable(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;
	mpImpl->pushListState();

	librevenge::RVNGString sTableName;
	sTableName.sprintf("Table%i", mpImpl->mTableStyles.size());

	// FIXME: we base the table style off of the page's margin left, ignoring (potential) wordperfect margin
	// state which is transmitted inside the page. could this lead to unacceptable behaviour?
	const librevenge::RVNGPropertyListVector *columns = propList.child("librevenge:table-columns");
	TableStyle *pTableStyle = new TableStyle(propList, (columns ? *columns : librevenge::RVNGPropertyListVector()), sTableName.cstr());

	mpImpl->mTableStyles.push_back(pTableStyle);

	mpImpl->mpCurrentTableStyle = pTableStyle;

	// table must be inside a frame
	TagOpenElement *pFrameOpenElement = new TagOpenElement("draw:frame");

	pFrameOpenElement->addAttribute("draw:style-name", "standard");
	if (propList["svg:x"])
		pFrameOpenElement->addAttribute("svg:x", propList["svg:x"]->getStr());
	if (propList["svg:y"])
		pFrameOpenElement->addAttribute("svg:y", propList["svg:y"]->getStr());
	if (propList["svg:width"])
		pFrameOpenElement->addAttribute("svg:width", propList["svg:width"]->getStr());
	if (propList["svg:height"])
		pFrameOpenElement->addAttribute("svg:height", propList["svg:height"]->getStr());

	mpImpl->getCurrentStorage()->push_back(pFrameOpenElement);

	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");

	pTableOpenElement->addAttribute("table:name", sTableName.cstr());
	pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
	mpImpl->getCurrentStorage()->push_back(pTableOpenElement);

	for (int i=0; i<pTableStyle->getNumColumns(); ++i)
	{
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		librevenge::RVNGString sColumnStyleName;
		sColumnStyleName.sprintf("%s.Column%i", sTableName.cstr(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
		mpImpl->getCurrentStorage()->push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpImpl->getCurrentStorage()->push_back(pTableColumnCloseElement);
	}
}

void OdpGenerator::openTableRow(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;

	if (!mpImpl->mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::openTableRow called with no table\n"));
		return;
	}

	if (propList["librevenge:is-header-row"] && (propList["librevenge:is-header-row"]->getInt()))
	{
		mpImpl->getCurrentStorage()->push_back(new TagOpenElement("table:table-header-rows"));
		mpImpl->mState.mHeaderRow = true;
	}

	librevenge::RVNGString sTableRowStyleName;
	sTableRowStyleName.sprintf("%s.Row%i", mpImpl->mpCurrentTableStyle->getName().cstr(), mpImpl->mpCurrentTableStyle->getNumTableRowStyles());
	TableRowStyle *pTableRowStyle = new TableRowStyle(propList, sTableRowStyleName.cstr());
	mpImpl->mpCurrentTableStyle->addTableRowStyle(pTableRowStyle);

	TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
	pTableRowOpenElement->addAttribute("table:style-name", sTableRowStyleName);
	mpImpl->getCurrentStorage()->push_back(pTableRowOpenElement);
}

void OdpGenerator::closeTableRow()
{
	if (mpImpl->mState.mInComment || !mpImpl->mpCurrentTableStyle)
		return;
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-row"));
	if (mpImpl->mState.mHeaderRow)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-header-rows"));
		mpImpl->mState.mHeaderRow = false;
	}
}

void OdpGenerator::openTableCell(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::openTableCell called with no table\n"));
		return;
	}

	if (mpImpl->mState.mTableCellOpened)
	{
		ODFGEN_DEBUG_MSG(("a table cell in a table cell?!\n"));
		return;
	}

	librevenge::RVNGString sTableCellStyleName;
	sTableCellStyleName.sprintf("%s.Cell%i", mpImpl->mpCurrentTableStyle->getName().cstr(), mpImpl->mpCurrentTableStyle->getNumTableCellStyles());
	TableCellStyle *pTableCellStyle = new GraphicTableCellStyle(propList, sTableCellStyleName.cstr());
	mpImpl->mpCurrentTableStyle->addTableCellStyle(pTableCellStyle);

	TagOpenElement *pTableCellOpenElement = new TagOpenElement("table:table-cell");
	pTableCellOpenElement->addAttribute("table:style-name", sTableCellStyleName);
	if (propList["table:number-columns-spanned"])
		pTableCellOpenElement->addAttribute("table:number-columns-spanned",
		                                    propList["table:number-columns-spanned"]->getStr().cstr());
	if (propList["table:number-rows-spanned"])
		pTableCellOpenElement->addAttribute("table:number-rows-spanned",
		                                    propList["table:number-rows-spanned"]->getStr().cstr());
	mpImpl->getCurrentStorage()->push_back(pTableCellOpenElement);

	mpImpl->mState.mTableCellOpened = true;
}

void OdpGenerator::closeTableCell()
{
	if (mpImpl->mState.mInComment || !mpImpl->mpCurrentTableStyle)
		return;

	if (!mpImpl->mState.mTableCellOpened)
	{
		ODFGEN_DEBUG_MSG(("no table cell is opened\n"));
		return;
	}

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-cell"));
	mpImpl->mState.mTableCellOpened = false;
}

void OdpGenerator::insertCoveredTableCell(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	if (mpImpl->mState.mInComment || !mpImpl->mpCurrentTableStyle)
		return;

	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("table:covered-table-cell"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:covered-table-cell"));
}

void OdpGenerator::closeTable()
{
	if (!mpImpl->mState.mInComment)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table"));
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
		mpImpl->popListState();
	}
}

void OdpGenerator::startComment(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
	{
		ODFGEN_DEBUG_MSG(("a comment within a comment?!\n"));
		return;
	}
	mpImpl->pushListState();
	mpImpl->mState.mInComment = true;

	TagOpenElement *const commentElement = new TagOpenElement("officeooo:annotation");

	// position & size
	if (propList["svg:x"])
		commentElement->addAttribute("svg:x", doubleToString(72 * propList["svg:x"]->getDouble()));
	if (propList["svg:y"])
		commentElement->addAttribute("svg:y", doubleToString(72 * propList["svg:y"]->getDouble()));
	if (propList["svg:width"])
		commentElement->addAttribute("svg:width", doubleToString(72 * propList["svg:width"]->getDouble()));
	if (propList["svg:height"])
		commentElement->addAttribute("svg:height", doubleToString(72 * propList["svg:height"]->getDouble()));

	mpImpl->getCurrentStorage()->push_back(commentElement);
}

void OdpGenerator::endComment()
{
	if (!mpImpl->mState.mInComment)
	{
		ODFGEN_DEBUG_MSG(("there is no comment to close\n"));
		return;
	}
	mpImpl->popListState();
	mpImpl->mState.mInComment = false;
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("officeooo:annotation"));
}

void OdpGenerator::startNotes(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	if (mpImpl->mState.mInNotes)
	{
		ODFGEN_DEBUG_MSG(("notes in notes?!\n"));
		return;
	}

	mpImpl->pushListState();
	TagOpenElement *const notesElement = new TagOpenElement("presentation:notes");
	notesElement->addAttribute("draw:style-name", "PresentationNotesPage");

	mpImpl->getCurrentStorage()->push_back(notesElement);

	TagOpenElement *const thumbnailElement = new TagOpenElement("draw:page-thumbnail");
	thumbnailElement->addAttribute("draw:layer", "layout");
	thumbnailElement->addAttribute("presentation:class", "page");
	// TODO: should the dimensions be hardcoded? If not, where
	// should they come from?
	thumbnailElement->addAttribute("svg:width", "5.5in");
	thumbnailElement->addAttribute("svg:height", "4.12in");
	thumbnailElement->addAttribute("svg:x", "1.5in");
	thumbnailElement->addAttribute("svg:y", "0.84in");
	librevenge::RVNGString pageNumber;
	pageNumber.sprintf("%i", mpImpl->miPageIndex);
	thumbnailElement->addAttribute("draw:page-number", pageNumber);

	mpImpl->getCurrentStorage()->push_back(thumbnailElement);
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:page-thumbnail"));

	TagOpenElement *const frameElement = new TagOpenElement("draw:frame");
	frameElement->addAttribute("presentation:style-name", "PresentationNotesFrame");
	frameElement->addAttribute("draw:layer", "layout");
	frameElement->addAttribute("presentation:class", "notes");
	frameElement->addAttribute("svg:width", "6.8in");
	frameElement->addAttribute("svg:height", "4.95in");
	frameElement->addAttribute("svg:x", "0.85in");
	frameElement->addAttribute("svg:y", "5.22in");

	mpImpl->getCurrentStorage()->push_back(frameElement);

	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:text-box"));

	mpImpl->mState.mInNotes = true;
}

void OdpGenerator::endNotes()
{
	if (!mpImpl->mState.mInNotes)
	{
		ODFGEN_DEBUG_MSG(("no notes opened\n"));
		return;
	}
	mpImpl->popListState();
	mpImpl->mState.mInNotes = false;

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("presentation:notes"));
}

void OdpGenerator::initStateWith(OdfGenerator const &orig)
{
	mpImpl->initStateWith(orig);
}

void OdpGenerator::registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler)
{
	mpImpl->registerEmbeddedObjectHandler(mimeType, objectHandler);
}

void OdpGenerator::registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler)
{
	mpImpl->registerEmbeddedImageHandler(mimeType, imageHandler);
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
