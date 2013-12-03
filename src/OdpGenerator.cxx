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

#include "FilterInternal.hxx"
#include "DocumentElement.hxx"
#include "GraphicFunctions.hxx"
#include "ListStyle.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"
#include "FontStyle.hxx"
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

using namespace libodfgen;

namespace
{

static WPXString doubleToString(const double value)
{
	WPXProperty *prop = WPXPropertyFactory::newDoubleProp(value);
	WPXString retVal = prop->getStr();
	delete prop;
	return retVal;
}

} // anonymous namespace

namespace
{

struct GeneratorState
{
	bool mbIsTextBox;
	bool mbIsTextLine;
	bool mbIsTextOnPath;
	bool mInComment;
	bool mHeaderRow;
	bool mTableCellOpened;
	bool mInNotes;

	GeneratorState();
};

GeneratorState::GeneratorState()
	: mbIsTextBox(false)
	, mbIsTextLine(false)
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

// list state
struct ListState
{
	ListState();
	ListState(const ListState &state);

	ListStyle *mpCurrentListStyle;
	bool mbListElementParagraphOpened;
	std::stack<bool> mbListElementOpened;
private:
	ListState &operator=(const ListState &state);
};

ListState::ListState() :
	mpCurrentListStyle(0),
	mbListElementParagraphOpened(false),
	mbListElementOpened()
{
}

ListState::ListState(const ListState &state) :
	mpCurrentListStyle(state.mpCurrentListStyle),
	mbListElementParagraphOpened(state.mbListElementParagraphOpened),
	mbListElementOpened(state.mbListElementOpened)
{
}

}

namespace
{

class GraphicTableCellStyle : public TableCellStyle
{
public:
	GraphicTableCellStyle(const WPXPropertyList &xPropList, const char *psName);
	virtual ~GraphicTableCellStyle();

private:
	virtual void writeCompat(OdfDocumentHandler *pHandler, const WPXPropertyList &propList) const;
};

GraphicTableCellStyle::GraphicTableCellStyle(const WPXPropertyList &xPropList, const char *const psName)
	: TableCellStyle(xPropList, psName)
{
}

GraphicTableCellStyle::~GraphicTableCellStyle()
{
}

void GraphicTableCellStyle::writeCompat(OdfDocumentHandler *const pHandler, const WPXPropertyList &propList) const
{
	WPXPropertyList stylePropList;
	WPXPropertyList::Iter i(propList);

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
	WPXPropertyList paraPropList;
	paraPropList.insert("fo:border", "0.03pt solid #000000");

	pHandler->startElement("style:paragraph-properties", paraPropList);
	pHandler->endElement("style:paragraph-properties");
}

}

class OdpGeneratorPrivate
{
public:
	OdpGeneratorPrivate(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdpGeneratorPrivate();
	/** update a graphic style element */
	void _updateGraphicPropertiesElement(TagOpenElement &element, ::WPXPropertyList const &style, ::WPXPropertyListVector const &gradient);
	void _writeGraphicsStyle();
	void writeNotesStyles();
	void _drawPolySomething(const ::WPXPropertyListVector &vertices, bool isClosed);
	void _drawPath(const WPXPropertyListVector &path);

	void openListLevel(TagOpenElement *pListLevelOpenElement);
	void closeListLevel();

	//! returns the document type
	std::string getDocumentType() const;

	// body elements
	std::vector <DocumentElement *> mBodyElements;

	// graphics styles
	std::vector<DocumentElement *> mGraphicsStrokeDashStyles;
	std::vector<DocumentElement *> mGraphicsGradientStyles;
	std::vector<DocumentElement *> mGraphicsBitmapStyles;
	std::vector<DocumentElement *> mGraphicsMarkerStyles;
	std::vector<DocumentElement *> mGraphicsAutomaticStyles;

	// page styles
	std::vector<DocumentElement *> mPageAutomaticStyles;
	std::vector<DocumentElement *> mPageMasterStyles;

	// paragraph styles
	ParagraphStyleManager mParagraphManager;

	// span styles
	SpanStyleManager mSpanManager;

	// font styles
	FontStyleManager mFontManager;

	// table styles
	std::vector<TableStyle *> mTableStyles;
	TableStyle *mpCurrentTableStyle;

	OdfDocumentHandler *mpHandler;

	::WPXPropertyList mxStyle;
	::WPXPropertyListVector mxGradient;
	int miGradientIndex;
	int miBitmapIndex;
	int miStartMarkerIndex;
	int miEndMarkerIndex;
	int miDashIndex;
	int miGraphicsStyleIndex;
	int miPageIndex;
	double mfWidth, mfMaxWidth;
	double mfHeight, mfMaxHeight;

	const OdfStreamType mxStreamType;

	// generator state
	GeneratorState mState;
	std::stack<ListState> mListStates;

private:
	OdpGeneratorPrivate(const OdpGeneratorPrivate &);
	OdpGeneratorPrivate &operator=(const OdpGeneratorPrivate &);

};

OdpGeneratorPrivate::OdpGeneratorPrivate(OdfDocumentHandler *pHandler, const OdfStreamType streamType):
	mBodyElements(),
	mGraphicsStrokeDashStyles(),
	mGraphicsGradientStyles(),
	mGraphicsBitmapStyles(),
	mGraphicsMarkerStyles(),
	mGraphicsAutomaticStyles(),
	mPageAutomaticStyles(),
	mPageMasterStyles(),
	mParagraphManager(),
	mSpanManager(),
	mFontManager(),
	mTableStyles(),
	mpCurrentTableStyle(0),
	mpHandler(pHandler),
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
	mxStreamType(streamType),
	mState(),
	mListStates()
{
}

OdpGeneratorPrivate::~OdpGeneratorPrivate()
{

	for (std::vector<DocumentElement *>::iterator iterBody = mBodyElements.begin(); iterBody != mBodyElements.end(); ++iterBody)
	{
		delete (*iterBody);
		(*iterBody) = 0;
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsAutomaticStyles = mGraphicsAutomaticStyles.begin();
	        iterGraphicsAutomaticStyles != mGraphicsAutomaticStyles.end(); ++iterGraphicsAutomaticStyles)
	{
		delete((*iterGraphicsAutomaticStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsStrokeDashStyles = mGraphicsStrokeDashStyles.begin();
	        iterGraphicsStrokeDashStyles != mGraphicsStrokeDashStyles.end(); ++iterGraphicsStrokeDashStyles)
	{
		delete((*iterGraphicsStrokeDashStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsGradientStyles = mGraphicsGradientStyles.begin();
	        iterGraphicsGradientStyles != mGraphicsGradientStyles.end(); ++iterGraphicsGradientStyles)
	{
		delete((*iterGraphicsGradientStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsBitmapStyles = mGraphicsBitmapStyles.begin();
	        iterGraphicsBitmapStyles != mGraphicsBitmapStyles.end(); ++iterGraphicsBitmapStyles)
	{
		delete((*iterGraphicsBitmapStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsMarkerStyles = mGraphicsMarkerStyles.begin();
	        iterGraphicsMarkerStyles != mGraphicsMarkerStyles.end(); ++iterGraphicsMarkerStyles)
	{
		delete((*iterGraphicsMarkerStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterPageAutomaticStyles = mPageAutomaticStyles.begin();
	        iterPageAutomaticStyles != mPageAutomaticStyles.end(); ++iterPageAutomaticStyles)
	{
		delete((*iterPageAutomaticStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterPageMasterStyles = mPageMasterStyles.begin();
	        iterPageMasterStyles != mPageMasterStyles.end(); ++iterPageMasterStyles)
	{
		delete((*iterPageMasterStyles));
	}

	mParagraphManager.clean();
	mSpanManager.clean();
	mFontManager.clean();
}

void OdpGeneratorPrivate::writeNotesStyles()
{
	{
		WPXPropertyList styleProps;
		styleProps.insert("style:name", "PresentationNotesPage");
		styleProps.insert("style:family", "drawing-page");

		mpHandler->startElement("style:style", styleProps);

		WPXPropertyList pageProps;
		pageProps.insert("presentation:display-header", "true");
		pageProps.insert("presentation:display-footer", "true");
		pageProps.insert("presentation:display-date-time", "true");
		pageProps.insert("presentation:display-page-number", "false");

		mpHandler->startElement("style:drawing-page-properties", pageProps);
		mpHandler->endElement("style:drawing-page-properties");

		mpHandler->endElement("style:style");
	}

	{
		WPXPropertyList styleProps;
		styleProps.insert("style:name", "PresentationNotesFrame");
		styleProps.insert("style:family", "presentation");

		mpHandler->startElement("style:style", styleProps);

		WPXPropertyList graphicProps;
		graphicProps.insert("draw:fill", "none");
		graphicProps.insert("fo:min-height", "5in");

		mpHandler->startElement("style:graphic-properties", graphicProps);
		mpHandler->endElement("style:graphic-properties");

		WPXPropertyList paraProps;
		paraProps.insert("fo:margin-left", "0.24in");
		paraProps.insert("fo:margin-right", "0in");
		paraProps.insert("fo:text-indent", "0in");

		mpHandler->startElement("style:para-properties", paraProps);
		mpHandler->endElement("style:para-properties");

		mpHandler->endElement("style:style");
	}

	{
		WPXPropertyList styleProps;
		styleProps.insert("style:name", "PresentationNotesTextBox");
		styleProps.insert("style:family", "graphic");

		mpHandler->startElement("style:style", styleProps);

		WPXPropertyList graphicProps;
		graphicProps.insert("draw:fill", "none");

		mpHandler->startElement("style:graphic-properties", graphicProps);
		mpHandler->endElement("style:graphic-properties");

		mpHandler->endElement("style:style");
	}
}

void OdpGeneratorPrivate::openListLevel(TagOpenElement *pListLevelOpenElement)
{
	if (!mListStates.top().mbListElementOpened.empty() &&
	        !mListStates.top().mbListElementOpened.top())
	{
		mBodyElements.push_back(new TagOpenElement("text:list-item"));
		mListStates.top().mbListElementOpened.top() = true;
	}

	mListStates.top().mbListElementOpened.push(false);
	if (mListStates.top().mbListElementOpened.size() == 1)
	{
		// add a sanity check ( to avoid a crash if mpCurrentListStyle is NULL)
		if (mListStates.top().mpCurrentListStyle)
		{
			pListLevelOpenElement->addAttribute("text:style-name", mListStates.top().mpCurrentListStyle->getName());
		}
	}
}

void OdpGeneratorPrivate::closeListLevel()
{
	if (mListStates.top().mbListElementOpened.empty())
	{
		// this implies that openListLevel was not called, so it is better to stop here
		ODFGEN_DEBUG_MSG(("OdtGenerator: Attempting to close an unexisting level\n"));
		return;
	}
	if (mListStates.top().mbListElementOpened.top())
	{
		mBodyElements.push_back(new TagCloseElement("text:list-item"));
		mListStates.top().mbListElementOpened.top() = false;
	}

	mBodyElements.push_back(new TagCloseElement("text:list"));
	mListStates.top().mbListElementOpened.pop();
}

std::string OdpGeneratorPrivate::getDocumentType() const
{
	switch(mxStreamType)
	{
	case ODF_FLAT_XML:
		return "office:document";
	case ODF_CONTENT_XML:
		return "office:document-content";
	case ODF_STYLES_XML:
		return "office:document-styles";
	case ODF_SETTINGS_XML:
		return "office:document-settings";
	case ODF_META_XML:
		return "office:document-meta";
	default:
		return "office:document";
	}
}

OdpGenerator::OdpGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType):
	mpImpl(new OdpGeneratorPrivate(pHandler, streamType))
{
	mpImpl->mpHandler->startDocument();
	TagOpenElement tmpOfficeDocumentContent(mpImpl->getDocumentType().c_str());
	tmpOfficeDocumentContent.addAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:presentation", "urn:oasis:names:tc:opendocument:xmlns:presentation:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	tmpOfficeDocumentContent.addAttribute("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	// WARNING: this is not ODF!
	tmpOfficeDocumentContent.addAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	tmpOfficeDocumentContent.addAttribute("xmlns:officeooo", "http://openoffice.org/2009/office");
	tmpOfficeDocumentContent.addAttribute("office:version", "1.0");
	if (mpImpl->mxStreamType == ODF_FLAT_XML)
		tmpOfficeDocumentContent.addAttribute("office:mimetype", "application/vnd.oasis.opendocument.presentation");
	tmpOfficeDocumentContent.write(mpImpl->mpHandler);
}

OdpGenerator::~OdpGenerator()
{
	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_SETTINGS_XML))
	{
		TagOpenElement("office:settings").write(mpImpl->mpHandler);

		TagOpenElement configItemSetOpenElement("config:config-item-set");
		configItemSetOpenElement.addAttribute("config:name", "ooo:view-settings");
		configItemSetOpenElement.write(mpImpl->mpHandler);

		TagOpenElement configItemOpenElement("config:config-item");

		configItemOpenElement.addAttribute("config:name", "VisibleAreaTop");
		configItemOpenElement.addAttribute("config:type", "int");
		configItemOpenElement.write(mpImpl->mpHandler);
		mpImpl->mpHandler->characters("0");
		mpImpl->mpHandler->endElement("config:config-item");

		configItemOpenElement.addAttribute("config:name", "VisibleAreaLeft");
		configItemOpenElement.addAttribute("config:type", "int");
		configItemOpenElement.write(mpImpl->mpHandler);
		mpImpl->mpHandler->characters("0");
		mpImpl->mpHandler->endElement("config:config-item");

		configItemOpenElement.addAttribute("config:name", "VisibleAreaWidth");
		configItemOpenElement.addAttribute("config:type", "int");
		configItemOpenElement.write(mpImpl->mpHandler);
		WPXString sWidth;
		sWidth.sprintf("%li", (unsigned long)(2540 * mpImpl->mfMaxWidth));
		mpImpl->mpHandler->characters(sWidth);
		mpImpl->mpHandler->endElement("config:config-item");

		configItemOpenElement.addAttribute("config:name", "VisibleAreaHeight");
		configItemOpenElement.addAttribute("config:type", "int");
		configItemOpenElement.write(mpImpl->mpHandler);
		WPXString sHeight;
		sHeight.sprintf("%li", (unsigned long)(2540 * mpImpl->mfMaxHeight));
		mpImpl->mpHandler->characters(sHeight);
		mpImpl->mpHandler->endElement("config:config-item");

		mpImpl->mpHandler->endElement("config:config-item-set");

		mpImpl->mpHandler->endElement("office:settings");
	}


	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_STYLES_XML))
	{
		TagOpenElement("office:styles").write(mpImpl->mpHandler);

		for (std::vector<DocumentElement *>::const_iterator iterGraphicsStrokeDashStyles = mpImpl->mGraphicsStrokeDashStyles.begin();
		        iterGraphicsStrokeDashStyles != mpImpl->mGraphicsStrokeDashStyles.end(); ++iterGraphicsStrokeDashStyles)
		{
			(*iterGraphicsStrokeDashStyles)->write(mpImpl->mpHandler);
		}

		for (std::vector<DocumentElement *>::const_iterator iterGraphicsGradientStyles = mpImpl->mGraphicsGradientStyles.begin();
		        iterGraphicsGradientStyles != mpImpl->mGraphicsGradientStyles.end(); ++iterGraphicsGradientStyles)
		{
			(*iterGraphicsGradientStyles)->write(mpImpl->mpHandler);
		}

		for (std::vector<DocumentElement *>::const_iterator iterGraphicsBitmapStyles = mpImpl->mGraphicsBitmapStyles.begin();
		        iterGraphicsBitmapStyles != mpImpl->mGraphicsBitmapStyles.end(); ++iterGraphicsBitmapStyles)
		{
			(*iterGraphicsBitmapStyles)->write(mpImpl->mpHandler);
		}

		for (std::vector<DocumentElement *>::const_iterator iterGraphicsMarkerStyles = mpImpl->mGraphicsMarkerStyles.begin();
		        iterGraphicsMarkerStyles != mpImpl->mGraphicsMarkerStyles.end(); ++iterGraphicsMarkerStyles)
		{
			(*iterGraphicsMarkerStyles)->write(mpImpl->mpHandler);
		}
		mpImpl->mpHandler->endElement("office:styles");
	}


	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_CONTENT_XML) || (mpImpl->mxStreamType == ODF_STYLES_XML))
	{
		mpImpl->mFontManager.writeFontsDeclaration(mpImpl->mpHandler);

		TagOpenElement("office:automatic-styles").write(mpImpl->mpHandler);
	}

	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_CONTENT_XML))
	{
		// writing out the graphics automatic styles
		for (std::vector<DocumentElement *>::iterator iterGraphicsAutomaticStyles = mpImpl->mGraphicsAutomaticStyles.begin();
		        iterGraphicsAutomaticStyles != mpImpl->mGraphicsAutomaticStyles.end(); ++iterGraphicsAutomaticStyles)
		{
			(*iterGraphicsAutomaticStyles)->write(mpImpl->mpHandler);
		}
		mpImpl->mParagraphManager.write(mpImpl->mpHandler);
		mpImpl->mSpanManager.write(mpImpl->mpHandler);

		// writing out the table styles
		for (std::vector<TableStyle *>::const_iterator iterTableStyles = mpImpl->mTableStyles.begin(); iterTableStyles != mpImpl->mTableStyles.end(); ++iterTableStyles)
		{
			(*iterTableStyles)->write(mpImpl->mpHandler);
		}

		mpImpl->writeNotesStyles();
	}
#ifdef MULTIPAGE_WORKAROUND
	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_STYLES_XML))
	{
		TagOpenElement tmpStylePageLayoutOpenElement("style:page-layout");
		tmpStylePageLayoutOpenElement.addAttribute("style:name", "PM0");
		tmpStylePageLayoutOpenElement.write(mpImpl->mpHandler);

		TagOpenElement tmpStylePageLayoutPropertiesOpenElement("style:page-layout-properties");
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-top", "0in");
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-bottom", "0in");
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-left", "0in");
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-right", "0in");
		WPXString sValue;
		sValue = doubleToString(mpImpl->mfMaxWidth);
		sValue.append("in");
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:page-width", sValue);
		sValue = doubleToString(mpImpl->mfMaxHeight);
		sValue.append("in");
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:page-height", sValue);
		tmpStylePageLayoutPropertiesOpenElement.addAttribute("style:print-orientation", "portrait");
		tmpStylePageLayoutPropertiesOpenElement.write(mpImpl->mpHandler);

		mpImpl->mpHandler->endElement("style:page-layout-properties");

		mpImpl->mpHandler->endElement("style:page-layout");

		TagOpenElement tmpStyleStyleOpenElement("style:style");
		tmpStyleStyleOpenElement.addAttribute("style:name", "dp1");
		tmpStyleStyleOpenElement.addAttribute("style:family", "drawing-page");
		tmpStyleStyleOpenElement.write(mpImpl->mpHandler);

		TagOpenElement tmpStyleDrawingPagePropertiesOpenElement("style:drawing-page-properties");
		// tmpStyleDrawingPagePropertiesOpenElement.addAttribute("draw:background-size", "border");
		tmpStyleDrawingPagePropertiesOpenElement.addAttribute("draw:fill", "none");
		tmpStyleDrawingPagePropertiesOpenElement.write(mpImpl->mpHandler);

		mpImpl->mpHandler->endElement("style:drawing-page-properties");

		mpImpl->mpHandler->endElement("style:style");
	}
#else
	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_STYLES_XML))
	{
		// writing out the page automatic styles
		for (std::vector<DocumentElement *>::iterator iterPageAutomaticStyles = mpImpl->mPageAutomaticStyles.begin();
		        iterPageAutomaticStyles != mpImpl->mPageAutomaticStyles.end(); ++iterPageAutomaticStyles)
		{
			(*iterPageAutomaticStyles)->write(mpImpl->mpHandler);
		}
	}
#endif
	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_CONTENT_XML) || (mpImpl->mxStreamType == ODF_STYLES_XML))
	{
		mpImpl->mpHandler->endElement("office:automatic-styles");
	}

	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_STYLES_XML))
	{
		TagOpenElement("office:master-styles").write(mpImpl->mpHandler);

		for (std::vector<DocumentElement *>::const_iterator pageMasterIter = mpImpl->mPageMasterStyles.begin();
		        pageMasterIter != mpImpl->mPageMasterStyles.end(); ++pageMasterIter)
		{
			(*pageMasterIter)->write(mpImpl->mpHandler);
		}
		mpImpl->mpHandler->endElement("office:master-styles");
	}

	if ((mpImpl->mxStreamType == ODF_FLAT_XML) || (mpImpl->mxStreamType == ODF_CONTENT_XML))
	{
		TagOpenElement("office:body").write(mpImpl->mpHandler);

		TagOpenElement("office:presentation").write(mpImpl->mpHandler);

		for (std::vector<DocumentElement *>::const_iterator bodyIter = mpImpl->mBodyElements.begin();
		        bodyIter != mpImpl->mBodyElements.end(); ++bodyIter)
		{
			(*bodyIter)->write(mpImpl->mpHandler);
		}

		mpImpl->mpHandler->endElement("office:presentation");
		mpImpl->mpHandler->endElement("office:body");
	}

	mpImpl->mpHandler->endElement(mpImpl->getDocumentType().c_str());

	mpImpl->mpHandler->endDocument();

	delete mpImpl;
}

void OdpGenerator::startDocument(const ::WPXPropertyList &/*propList*/)
{
}

void OdpGenerator::endDocument()
{
}

void OdpGenerator::setDocumentMetaData(const ::WPXPropertyList &/*propList*/)
{
}

void OdpGenerator::startSlide(const ::WPXPropertyList &propList)
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

	WPXString sValue;
	if (propList["draw:name"])
		sValue = WPXString(propList["draw:name"]->getStr(), true); // escape special xml characters
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

	mpImpl->mBodyElements.push_back(pDrawPageOpenElement);

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
	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:page"));
	mpImpl->miPageIndex++;
}

void OdpGenerator::setStyle(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &gradient)
{
	mpImpl->mxStyle.clear();
	mpImpl->mxStyle = propList;
	mpImpl->mxGradient = gradient;
}

void OdpGenerator::startLayer(const ::WPXPropertyList & /* propList */)
{
}

void OdpGenerator::endLayer()
{
}

void OdpGenerator::drawRectangle(const ::WPXPropertyList &propList)
{
	if (!propList["svg:x"] || !propList["svg:y"] ||
	        !propList["svg:width"] || !propList["svg:height"])
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::drawRectangle: position undefined\n"));
		return;
	}
	mpImpl->_writeGraphicsStyle();
	TagOpenElement *pDrawRectElement = new TagOpenElement("draw:rect");
	WPXString sValue;
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
	mpImpl->mBodyElements.push_back(pDrawRectElement);
	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:rect"));
}

void OdpGenerator::drawEllipse(const ::WPXPropertyList &propList)
{
	if (!propList["svg:rx"] || !propList["svg:ry"] || !propList["svg:cx"] || !propList["svg:cy"])
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::drawEllipse: position undefined\n"));
		return;
	}
	mpImpl->_writeGraphicsStyle();
	TagOpenElement *pDrawEllipseElement = new TagOpenElement("draw:ellipse");
	WPXString sValue;
	sValue.sprintf("gr%i", mpImpl->miGraphicsStyleIndex-1);
	pDrawEllipseElement->addAttribute("draw:style-name", sValue);
	sValue = doubleToString(2 * propList["svg:rx"]->getDouble());
	sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:width", sValue);
	sValue = doubleToString(2 * propList["svg:ry"]->getDouble());
	sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:height", sValue);
	if (propList["libwpg:rotate"] && propList["libwpg:rotate"]->getDouble() != 0.0)
	{
		double rotation = propList["libwpg:rotate"]->getDouble();
		while(rotation < -180)
			rotation += 360;
		while(rotation > 180)
			rotation -= 360;
		double radrotation = rotation*M_PI/180.0;
		double deltax = sqrt(pow(propList["svg:rx"]->getDouble(), 2.0)
		                     + pow(propList["svg:ry"]->getDouble(), 2.0))*cos(atan(propList["svg:ry"]->getDouble()/propList["svg:rx"]->getDouble())
		                             - radrotation ) - propList["svg:rx"]->getDouble();
		double deltay = sqrt(pow(propList["svg:rx"]->getDouble(), 2.0)
		                     + pow(propList["svg:ry"]->getDouble(), 2.0))*sin(atan(propList["svg:ry"]->getDouble()/propList["svg:rx"]->getDouble())
		                             - radrotation ) - propList["svg:ry"]->getDouble();
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
	mpImpl->mBodyElements.push_back(pDrawEllipseElement);
	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:ellipse"));
}

void OdpGenerator::drawPolyline(const ::WPXPropertyListVector &vertices)
{
	mpImpl->_drawPolySomething(vertices, false);
}

void OdpGenerator::drawPolygon(const ::WPXPropertyListVector &vertices)
{
	mpImpl->_drawPolySomething(vertices, true);
}

void OdpGeneratorPrivate::_drawPolySomething(const ::WPXPropertyListVector &vertices, bool isClosed)
{
	if(vertices.count() < 2)
		return;

	if(vertices.count() == 2)
	{
		if (!vertices[0]["svg:x"]||!vertices[0]["svg:y"]||!vertices[1]["svg:x"]||!vertices[1]["svg:y"])
		{
			ODFGEN_DEBUG_MSG(("OdpGeneratorPrivate::_drawPolySomething: some vertices are not defined\n"));
			return;
		}
		_writeGraphicsStyle();
		TagOpenElement *pDrawLineElement = new TagOpenElement("draw:line");
		WPXString sValue;
		sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
		pDrawLineElement->addAttribute("draw:style-name", sValue);
		pDrawLineElement->addAttribute("draw:layer", "layout");
		pDrawLineElement->addAttribute("svg:x1", vertices[0]["svg:x"]->getStr());
		pDrawLineElement->addAttribute("svg:y1", vertices[0]["svg:y"]->getStr());
		pDrawLineElement->addAttribute("svg:x2", vertices[1]["svg:x"]->getStr());
		pDrawLineElement->addAttribute("svg:y2", vertices[1]["svg:y"]->getStr());
		mBodyElements.push_back(pDrawLineElement);
		mBodyElements.push_back(new TagCloseElement("draw:line"));
	}
	else
	{
		::WPXPropertyListVector path;
		::WPXPropertyList element;

		for (unsigned long ii = 0; ii < vertices.count(); ++ii)
		{
			element = vertices[ii];
			if (ii == 0)
				element.insert("libwpg:path-action", "M");
			else
				element.insert("libwpg:path-action", "L");
			path.append(element);
			element.clear();
		}
		if (isClosed)
		{
			element.insert("libwpg:path-action", "Z");
			path.append(element);
		}
		_drawPath(path);
	}
}

void OdpGeneratorPrivate::_drawPath(const WPXPropertyListVector &path)
{
	if(path.count() == 0)
		return;
	// This must be a mistake and we do not want to crash lower
	if(path[0]["libwpg:path-action"]->getStr() == "Z")
		return;

	// try to find the bounding box
	// this is simple convex hull technique, the bounding box might not be
	// accurate but that should be enough for this purpose
	bool isFirstPoint = true;

	double px = 0.0, py = 0.0, qx = 0.0, qy = 0.0;
	double lastX = 0.0;
	double lastY = 0.0;
	double lastPrevX = 0.0;
	double lastPrevY = 0.0;

	for(unsigned k = 0; k < path.count(); ++k)
	{
		if (!path[k]["libwpg:path-action"])
			continue;
		std::string action=path[k]["libwpg:path-action"]->getStr().cstr();
		if (action.length()!=1 || action[0]=='Z') continue;

		bool coordOk=path[k]["svg:x"]&&path[k]["svg:y"];
		bool coord1Ok=coordOk && path[k]["svg:x1"]&&path[k]["svg:y1"];
		bool coord2Ok=coord1Ok && path[k]["svg:x2"]&&path[k]["svg:y2"];
		double x=lastX, y=lastY;
		if (isFirstPoint)
		{
			if (!coordOk)
			{
				ODFGEN_DEBUG_MSG(("OdpGeneratorPrivate::_drawPath: the first point has no coordinate\n"));
				continue;
			}
			qx = px = x = path[k]["svg:x"]->getDouble();
			qy = py = y = path[k]["svg:y"]->getDouble();
			lastPrevX = lastX = px;
			lastPrevY = lastY = py;
			isFirstPoint = false;
		}
		else
		{
			if (path[k]["svg:x"]) x=path[k]["svg:x"]->getDouble();
			if (path[k]["svg:y"]) y=path[k]["svg:y"]->getDouble();
			px = (px > x) ? x : px;
			py = (py > y) ? y : py;
			qx = (qx < x) ? x : qx;
			qy = (qy < y) ? y : qy;
		}

		double xmin=px, xmax=qx, ymin=py, ymax=qy;
		bool lastPrevSet=false;

		if(action[0] == 'C' && coord2Ok)
		{
			getCubicBezierBBox(lastX, lastY, path[k]["svg:x1"]->getDouble(), path[k]["svg:y1"]->getDouble(),
			                   path[k]["svg:x2"]->getDouble(), path[k]["svg:y2"]->getDouble(),
			                   x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-path[k]["svg:x2"]->getDouble();
			lastPrevY=2*y-path[k]["svg:y2"]->getDouble();
		}
		else if(action[0] == 'S' && coord1Ok)
		{
			getCubicBezierBBox(lastX, lastY, lastPrevX, lastPrevY,
			                   path[k]["svg:x1"]->getDouble(), path[k]["svg:y1"]->getDouble(),
			                   x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-path[k]["svg:x1"]->getDouble();
			lastPrevY=2*y-path[k]["svg:y1"]->getDouble();
		}
		else if(action[0] == 'Q' && coord1Ok)
		{
			getQuadraticBezierBBox(lastX, lastY, path[k]["svg:x1"]->getDouble(), path[k]["svg:y1"]->getDouble(),
			                       x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-path[k]["svg:x1"]->getDouble();
			lastPrevY=2*y-path[k]["svg:y1"]->getDouble();
		}
		else if(action[0] == 'T' && coordOk)
		{
			getQuadraticBezierBBox(lastX, lastY, lastPrevX, lastPrevY,
			                       x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-lastPrevX;
			lastPrevY=2*y-lastPrevY;
		}
		else if(action[0] == 'A' && coordOk && path[k]["svg:rx"] && path[k]["svg:ry"])
		{
			getEllipticalArcBBox(lastX, lastY, path[k]["svg:rx"]->getDouble(), path[k]["svg:ry"]->getDouble(),
			                     path[k]["libwpg:rotate"] ? path[k]["libwpg:rotate"]->getDouble() : 0.0,
			                     path[k]["libwpg:large-arc"] ? path[k]["libwpg:large-arc"]->getInt() : 1,
			                     path[k]["libwpg:sweep"] ? path[k]["libwpg:sweep"]->getInt() : 1,
			                     x, y, xmin, ymin, xmax, ymax);
		}
		else if (action[0] != 'M' && action[0] != 'L' && action[0] != 'H' && action[0] != 'V')
		{
			ODFGEN_DEBUG_MSG(("OdpGeneratorPrivate::_drawPath: problem reading a path\n"));
		}
		px = (px > xmin ? xmin : px);
		py = (py > ymin ? ymin : py);
		qx = (qx < xmax ? xmax : qx);
		qy = (qy < ymax ? ymax : qy);
		lastX = x;
		lastY = y;
		if (!lastPrevSet)
		{
			lastPrevX=lastX;
			lastPrevY=lastY;
		}
	}


	WPXString sValue;
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

	sValue.clear();
	for(unsigned i = 0; i < path.count(); ++i)
	{
		if (!path[i]["libwpg:path-action"])
			continue;
		std::string action=path[i]["libwpg:path-action"]->getStr().cstr();
		if (action.length()!=1) continue;
		bool coordOk=path[i]["svg:x"]&&path[i]["svg:y"];
		bool coord1Ok=coordOk && path[i]["svg:x1"]&&path[i]["svg:y1"];
		bool coord2Ok=coord1Ok && path[i]["svg:x2"]&&path[i]["svg:y2"];
		WPXString sElement;
		// 2540 is 2.54*1000, 2.54 in = 1 inch
		if (path[i]["svg:x"] && action[0] == 'H')
		{
			sElement.sprintf("H%i", (unsigned)((path[i]["svg:x"]->getDouble()-px)*2540));
			sValue.append(sElement);
		}
		else if (path[i]["svg:y"] && action[0] == 'V')
		{
			sElement.sprintf("V%i", (unsigned)((path[i]["svg:y"]->getDouble()-py)*2540));
			sValue.append(sElement);
		}
		else if (coordOk && (action[0] == 'M' || action[0] == 'L' || action[0] == 'T'))
		{
			sElement.sprintf("%c%i %i", action[0], (unsigned)((path[i]["svg:x"]->getDouble()-px)*2540),
			                 (unsigned)((path[i]["svg:y"]->getDouble()-py)*2540));
			sValue.append(sElement);
		}
		else if (coord1Ok && (action[0] == 'Q' || action[0] == 'S'))
		{
			sElement.sprintf("%c%i %i %i %i", action[0], (unsigned)((path[i]["svg:x1"]->getDouble()-px)*2540),
			                 (unsigned)((path[i]["svg:y1"]->getDouble()-py)*2540), (unsigned)((path[i]["svg:x"]->getDouble()-px)*2540),
			                 (unsigned)((path[i]["svg:y"]->getDouble()-py)*2540));
			sValue.append(sElement);
		}
		else if (coord2Ok && action[0] == 'C')
		{
			sElement.sprintf("C%i %i %i %i %i %i", (unsigned)((path[i]["svg:x1"]->getDouble()-px)*2540),
			                 (unsigned)((path[i]["svg:y1"]->getDouble()-py)*2540), (unsigned)((path[i]["svg:x2"]->getDouble()-px)*2540),
			                 (unsigned)((path[i]["svg:y2"]->getDouble()-py)*2540), (unsigned)((path[i]["svg:x"]->getDouble()-px)*2540),
			                 (unsigned)((path[i]["svg:y"]->getDouble()-py)*2540));
			sValue.append(sElement);
		}
		else if (coordOk && path[i]["svg:rx"] && path[i]["svg:ry"] && action[0] == 'A')
		{
			sElement.sprintf("A%i %i %i %i %i %i %i", (unsigned)((path[i]["svg:rx"]->getDouble())*2540),
			                 (unsigned)((path[i]["svg:ry"]->getDouble())*2540), (path[i]["libwpg:rotate"] ? path[i]["libwpg:rotate"]->getInt() : 0),
			                 (path[i]["libwpg:large-arc"] ? path[i]["libwpg:large-arc"]->getInt() : 1),
			                 (path[i]["libwpg:sweep"] ? path[i]["libwpg:sweep"]->getInt() : 1),
			                 (unsigned)((path[i]["svg:x"]->getDouble()-px)*2540), (unsigned)((path[i]["svg:y"]->getDouble()-py)*2540));
			sValue.append(sElement);
		}
		else if (action[0] == 'Z')
			sValue.append(" Z");
	}
	pDrawPathElement->addAttribute("svg:d", sValue);
	mBodyElements.push_back(pDrawPathElement);
	mBodyElements.push_back(new TagCloseElement("draw:path"));
}

void OdpGenerator::drawPath(const WPXPropertyListVector &path)
{
	mpImpl->_drawPath(path);
}

void OdpGenerator::drawGraphicObject(const ::WPXPropertyList &propList, const ::WPXBinaryData &binaryData)
{
	if (!propList["libwpg:mime-type"] || propList["libwpg:mime-type"]->getStr().len() <= 0)
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

	double angle(propList["libwpg:rotate"] ? - M_PI * propList["libwpg:rotate"]->getDouble() / 180.0 : 0.0);
	if (angle != 0.0)
	{
		double deltax((width*cos(angle)+height*sin(angle)-width)/2.0);
		double deltay((-width*sin(angle)+height*cos(angle)-height)/2.0);
		x -= deltax;
		y -= deltay;
	}

	WPXPropertyList framePropList;

	framePropList.insert("svg:x", x);
	framePropList.insert("svg:y", y);
	framePropList.insert("svg:height", height);
	framePropList.insert("svg:width", width);

	TagOpenElement *pDrawFrameElement = new TagOpenElement("draw:frame");

	WPXString sValue;
	sValue.sprintf("gr%i", mpImpl->miGraphicsStyleIndex-1);
	pDrawFrameElement->addAttribute("draw:style-name", sValue);

	pDrawFrameElement->addAttribute("svg:height", framePropList["svg:height"]->getStr());
	pDrawFrameElement->addAttribute("svg:width", framePropList["svg:width"]->getStr());

	if (angle != 0.0)
	{
		framePropList.insert("libwpg:rotate", angle, WPX_GENERIC);
		sValue.sprintf("rotate (%s) translate(%s, %s)",
		               framePropList["libwpg:rotate"]->getStr().cstr(),
		               framePropList["svg:x"]->getStr().cstr(),
		               framePropList["svg:y"]->getStr().cstr());
		pDrawFrameElement->addAttribute("draw:transform", sValue);
	}
	else
	{
		pDrawFrameElement->addAttribute("svg:x", framePropList["svg:x"]->getStr());
		pDrawFrameElement->addAttribute("svg:y", framePropList["svg:y"]->getStr());
	}
	mpImpl->mBodyElements.push_back(pDrawFrameElement);

	if (propList["libwpg:mime-type"]->getStr() == "object/ole")
		mpImpl->mBodyElements.push_back(new TagOpenElement("draw:object-ole"));
	else
		mpImpl->mBodyElements.push_back(new TagOpenElement("draw:image"));

	mpImpl->mBodyElements.push_back(new TagOpenElement("office:binary-data"));

	::WPXString base64Binary = binaryData.getBase64Data();
	mpImpl->mBodyElements.push_back(new CharDataElement(base64Binary.cstr()));

	mpImpl->mBodyElements.push_back(new TagCloseElement("office:binary-data"));

	if (propList["libwpg:mime-type"]->getStr() == "object/ole")
		mpImpl->mBodyElements.push_back(new TagCloseElement("draw:object-ole"));
	else
		mpImpl->mBodyElements.push_back(new TagCloseElement("draw:image"));

	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:frame"));
}

void OdpGenerator::drawConnector(const ::WPXPropertyList &/*propList*/, const ::WPXPropertyListVector &/*path*/)
{
}

void OdpGeneratorPrivate::_writeGraphicsStyle()
{
	TagOpenElement *pStyleStyleElement = new TagOpenElement("style:style");
	WPXString sValue;
	sValue.sprintf("gr%i",  miGraphicsStyleIndex);
	pStyleStyleElement->addAttribute("style:name", sValue);
	pStyleStyleElement->addAttribute("style:family", "graphic");
	pStyleStyleElement->addAttribute("style:parent-style-name", "standard");
	mGraphicsAutomaticStyles.push_back(pStyleStyleElement);

	TagOpenElement *pStyleGraphicsPropertiesElement = new TagOpenElement("style:graphic-properties");
	_updateGraphicPropertiesElement(*pStyleGraphicsPropertiesElement, mxStyle, mxGradient);
	mGraphicsAutomaticStyles.push_back(pStyleGraphicsPropertiesElement);
	mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:style"));
	miGraphicsStyleIndex++;
}

void OdpGeneratorPrivate::_updateGraphicPropertiesElement(TagOpenElement &element, ::WPXPropertyList const &style, ::WPXPropertyListVector const &gradient)
{
	bool bUseOpacityGradient = false;

	if (style["draw:stroke"] && style["draw:stroke"]->getStr() == "dash")
	{
		TagOpenElement *pDrawStrokeDashElement = new TagOpenElement("draw:stroke-dash");
		WPXString sValue;
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
		WPXString sValue;
		TagOpenElement *pDrawMarkerElement = new TagOpenElement("draw:marker");
		sValue.sprintf("StartMarker_%i", miStartMarkerIndex);
		pDrawMarkerElement->addAttribute("draw:name", sValue);
		if (style["draw:marker-start-viewbox"])
			pDrawMarkerElement->addAttribute("svg:viewBox", style["draw:marker-start-viewbox"]->getStr());
		pDrawMarkerElement->addAttribute("svg:d", style["draw:marker-start-path"]->getStr());
		mGraphicsMarkerStyles.push_back(pDrawMarkerElement);
		mGraphicsMarkerStyles.push_back(new TagCloseElement("draw:marker"));
	}
	if(style["draw:marker-end-path"])
	{
		WPXString sValue;
		TagOpenElement *pDrawMarkerElement = new TagOpenElement("draw:marker");
		sValue.sprintf("EndMarker_%i", miEndMarkerIndex);
		pDrawMarkerElement->addAttribute("draw:name", sValue);
		if (style["draw:marker-end-viewbox"])
			pDrawMarkerElement->addAttribute("svg:viewBox", style["draw:marker-end-viewbox"]->getStr());
		pDrawMarkerElement->addAttribute("svg:d", style["draw:marker-end-path"]->getStr());
		mGraphicsMarkerStyles.push_back(pDrawMarkerElement);
		mGraphicsMarkerStyles.push_back(new TagCloseElement("draw:marker"));
	}

	if(style["draw:fill"] && style["draw:fill"]->getStr() == "gradient")
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
		WPXString sValue;
		sValue.sprintf("Gradient_%i", miGradientIndex);
		pDrawGradientElement->addAttribute("draw:name", sValue);
		sValue.sprintf("Transparency_%i", miGradientIndex++);
		pDrawOpacityElement->addAttribute("draw:name", sValue);

		// ODG angle unit is 0.1 degree
		double angle = style["draw:angle"] ? style["draw:angle"]->getDouble() : 0.0;
		while(angle < 0)
			angle += 360;
		while(angle > 360)
			angle -= 360;
		sValue.sprintf("%i", (unsigned)(angle*10));
		pDrawGradientElement->addAttribute("draw:angle", sValue);
		pDrawOpacityElement->addAttribute("draw:angle", sValue);

		if (!gradient.count())
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

			if (style["libwpg:start-opacity"])
				pDrawOpacityElement->addAttribute("draw:start", style["libwpg:start-opacity"]->getStr());
			else
				pDrawOpacityElement->addAttribute("draw:start", "100%");

			if (style["libwpg:end-opacity"])
				pDrawOpacityElement->addAttribute("draw:end", style["libwpg:end-opacity"]->getStr());
			else
				pDrawOpacityElement->addAttribute("draw:end", "100%");

			mGraphicsGradientStyles.push_back(pDrawGradientElement);
			mGraphicsGradientStyles.push_back(new TagCloseElement("draw:gradient"));

			// Work around a mess in LibreOffice where both opacities of 100% are interpreted as complete transparency
			// Nevertheless, when one is different, immediately, they are interpreted correctly
			if (style["libwpg:start-opacity"] && style["libwpg:end-opacity"]
			        && (style["libwpg:start-opacity"]->getDouble() != 1.0 || style["libwpg:end-opacity"]->getDouble() != 1.0))
			{
				bUseOpacityGradient = true;
				mGraphicsGradientStyles.push_back(pDrawOpacityElement);
				mGraphicsGradientStyles.push_back(new TagCloseElement("draw:opacity"));
			}
		}
		else if(gradient.count() >= 2)
		{
			sValue.sprintf("%i", (unsigned)(angle*10));
			pDrawGradientElement->addAttribute("draw:angle", sValue);

			pDrawGradientElement->addAttribute("draw:start-color", gradient[1]["svg:stop-color"]->getStr());
			pDrawGradientElement->addAttribute("draw:end-color", gradient[0]["svg:stop-color"]->getStr());
			if (style["svg:cx"])
				pDrawGradientElement->addAttribute("draw:cx", style["svg:cx"]->getStr());
			if (style["svg:cy"])
				pDrawGradientElement->addAttribute("draw:cy", style["svg:cy"]->getStr());
			if (gradient[1]["svg:stop-opacity"])
			{
				pDrawOpacityElement->addAttribute("draw:start", gradient[1]["svg:stop-opacity"]->getStr());
				bUseOpacityGradient = true;
			}
			else
				pDrawOpacityElement->addAttribute("draw:start", "100%");
			if (gradient[0]["svg:stop-opacity"])
			{
				pDrawOpacityElement->addAttribute("draw:end", gradient[0]["svg:stop-opacity"]->getStr());
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
		if(!bUseOpacityGradient)
			delete pDrawOpacityElement;
	}

	if(style["draw:fill"] && style["draw:fill"]->getStr() == "bitmap" &&
	        style["draw:fill-image"] && style["libwpg:mime-type"])
	{
		TagOpenElement *pDrawBitmapElement = new TagOpenElement("draw:fill-image");
		WPXString sValue;
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

	WPXString sValue;
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

	if(style["draw:fill"] && style["draw:fill"]->getStr() == "none")
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

	if(style["draw:fill"] && style["draw:fill"]->getStr() == "solid")
	{
		element.addAttribute("draw:fill", "solid");
		if (style["draw:fill-color"])
			element.addAttribute("draw:fill-color", style["draw:fill-color"]->getStr());
		if (style["draw:opacity"])
			element.addAttribute("draw:opacity", style["draw:opacity"]->getStr());
	}

	if(style["draw:fill"] && style["draw:fill"]->getStr() == "gradient")
	{
		if (!gradient.count() || gradient.count() >= 2)
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
			if (gradient[0]["svg:stop-color"])
			{
				element.addAttribute("draw:fill", "solid");
				element.addAttribute("draw:fill-color", gradient[0]["svg:stop-color"]->getStr());
			}
			else
				element.addAttribute("draw:fill", "solid");
		}
	}

	if(style["draw:fill"] && style["draw:fill"]->getStr() == "bitmap")
	{
		if (style["draw:fill-image"] && style["libwpg:mime-type"])
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


	if(style["draw:marker-start-path"])
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

void OdpGenerator::startEmbeddedGraphics(const WPXPropertyList &)
{
}

void OdpGenerator::endEmbeddedGraphics()
{
}

void OdpGenerator::startGroup(const ::WPXPropertyList &/*propList*/)
{
	mpImpl->mBodyElements.push_back(new TagOpenElement("draw:g"));
}

void OdpGenerator::endGroup()
{
	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:g"));
}

void OdpGenerator::startTextObject(const WPXPropertyList &propList, const WPXPropertyListVector &/*path*/)
{
	TagOpenElement *pDrawFrameOpenElement = new TagOpenElement("draw:frame");
	TagOpenElement *pStyleStyleOpenElement = new TagOpenElement("style:style");

	WPXString sValue;
	sValue.sprintf("gr%i",  mpImpl->miGraphicsStyleIndex++);
	pStyleStyleOpenElement->addAttribute("style:name", sValue);
	pStyleStyleOpenElement->addAttribute("style:family", "graphic");
	pStyleStyleOpenElement->addAttribute("style:parent-style-name", "standard");
	mpImpl->mGraphicsAutomaticStyles.push_back(pStyleStyleOpenElement);

	pDrawFrameOpenElement->addAttribute("draw:style-name", sValue);
	pDrawFrameOpenElement->addAttribute("draw:layer", "layout");

	TagOpenElement *pStyleGraphicPropertiesOpenElement = new TagOpenElement("style:graphic-properties");
	WPXPropertyList styleList(propList);
	if (!propList["draw:stroke"])
		styleList.insert("draw:stroke", "none");
	if (!propList["draw:fill"])
		styleList.insert("draw:fill", "none");
	// the transformation is managed latter, so even if this changes nothing...
	if (propList["libwpg:rotate"])
		styleList.insert("libwpg:rotate", 0);
	mpImpl->_updateGraphicPropertiesElement(*pStyleGraphicPropertiesOpenElement, styleList, WPXPropertyListVector());

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
		if(propList["svg:width"])
			pDrawFrameOpenElement->addAttribute("svg:width", propList["svg:width"]->getStr());
		if(propList["svg:height"])
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
	double angle(propList["libwpg:rotate"] ? - M_PI * propList["libwpg:rotate"]->getDouble() / 180.0 : 0.0);
	if (angle != 0.0)
	{
		// compute position: make sure that the center position remains invariant
		double width = 0.0;
		double height = 0.0;
		if (propList["libwpg:rotate-cx"])
			width = 2.0*(propList["libwpg:rotate-cx"]->getDouble()-x);
		else if (propList["svg:width"])
			width = propList["svg:width"]->getDouble();
		if (propList["libwpg:rotate-cy"])
			height = 2.0*(propList["libwpg:rotate-cy"]->getDouble()-y);
		else if (propList["svg:height"])
			height = propList["svg:height"]->getDouble();
		double deltax((width*cos(angle)+height*sin(angle)-width)/2.0);
		double deltay((-width*sin(angle)+height*cos(angle)-height)/2.0);
		x -= deltax;
		y -= deltay;
	}
	WPXProperty *svg_x = WPXPropertyFactory::newInchProp(x);
	WPXProperty *svg_y = WPXPropertyFactory::newInchProp(y);
	if (angle != 0.0)
	{
		WPXProperty *libwpg_rotate = WPXPropertyFactory::newDoubleProp(angle);
		sValue.sprintf("rotate (%s) translate(%s, %s)",
		               libwpg_rotate->getStr().cstr(),
		               svg_x->getStr().cstr(),
		               svg_y->getStr().cstr());
		delete libwpg_rotate;
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
	mpImpl->mBodyElements.push_back(pDrawFrameOpenElement);
	mpImpl->mBodyElements.push_back(new TagOpenElement("draw:text-box"));
	mpImpl->mGraphicsAutomaticStyles.push_back(pStyleGraphicPropertiesOpenElement);
	mpImpl->mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));
	mpImpl->mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:style"));
	mpImpl->mState.mbIsTextBox = true;
}

void OdpGenerator::endTextObject()
{
	if (mpImpl->mState.mbIsTextBox)
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("draw:text-box"));
		mpImpl->mBodyElements.push_back(new TagCloseElement("draw:frame"));
		mpImpl->mState.mbIsTextBox = false;
	}
}

void OdpGenerator::openParagraph(const WPXPropertyList &propList, const WPXPropertyListVector &/*tabStops*/)
{
	WPXPropertyList finalPropList(propList);
	finalPropList.insert("style:parent-style-name", "Standard");
	WPXString paragName = mpImpl->mParagraphManager.findOrAdd(finalPropList, WPXPropertyListVector());


	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pParagraphOpenElement = new TagOpenElement("text:p");
	pParagraphOpenElement->addAttribute("text:style-name", paragName);
	mpImpl->mBodyElements.push_back(pParagraphOpenElement);
}

void OdpGenerator::closeParagraph()
{
	mpImpl->mBodyElements.push_back(new TagCloseElement("text:p"));
}

void OdpGenerator::openSpan(const WPXPropertyList &propList)
{
	if (propList["style:font-name"])
		mpImpl->mFontManager.findOrAdd(propList["style:font-name"]->getStr().cstr());

	WPXString sName = mpImpl->mSpanManager.findOrAdd(propList);

	TagOpenElement *pSpanOpenElement = new TagOpenElement("text:span");
	pSpanOpenElement->addAttribute("text:style-name", sName.cstr());
	mpImpl->mBodyElements.push_back(pSpanOpenElement);
}

void OdpGenerator::closeSpan()
{
	mpImpl->mBodyElements.push_back(new TagCloseElement("text:span"));
}

void OdpGenerator::insertText(const WPXString &text)
{
	mpImpl->mBodyElements.push_back(new TextElement(text));
}

void OdpGenerator::insertTab()
{
	mpImpl->mBodyElements.push_back(new TagOpenElement("text:tab"));
	mpImpl->mBodyElements.push_back(new TagCloseElement("text:tab"));
}

void OdpGenerator::insertSpace()
{
	mpImpl->mBodyElements.push_back(new TagOpenElement("text:s"));
	mpImpl->mBodyElements.push_back(new TagCloseElement("text:s"));
}

void OdpGenerator::insertLineBreak()
{
	mpImpl->mBodyElements.push_back(new TagOpenElement("text:line-break"));
	mpImpl->mBodyElements.push_back(new TagCloseElement("text:line-break"));
}

void OdpGenerator::insertField(const WPXString &/*type*/, const ::WPXPropertyList &/*propList*/)
{
}

void OdpGenerator::openOrderedListLevel(const ::WPXPropertyList &/*propList*/)
{
	if (mpImpl->mListStates.top().mbListElementParagraphOpened)
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("text:p"));
		mpImpl->mListStates.top().mbListElementParagraphOpened = false;
	}

	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
	mpImpl->openListLevel(pListLevelOpenElement);

	mpImpl->mBodyElements.push_back(pListLevelOpenElement);
}

void OdpGenerator::openUnorderedListLevel(const ::WPXPropertyList &/*propList*/)
{
	if (mpImpl->mListStates.top().mbListElementParagraphOpened)
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("text:p"));
		mpImpl->mListStates.top().mbListElementParagraphOpened = false;
	}
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
	mpImpl->openListLevel(pListLevelOpenElement);

	mpImpl->mBodyElements.push_back(pListLevelOpenElement);
}

void OdpGenerator::closeOrderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdpGenerator::closeUnorderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdpGenerator::openListElement(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &tabStops)
{
	if (mpImpl->mListStates.top().mbListElementOpened.top())
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("text:list-item"));
		mpImpl->mListStates.top().mbListElementOpened.top() = false;
	}

	WPXPropertyList finalPropList(propList);
	finalPropList.insert("style:parent-style-name", "Standard");
	WPXString paragName = mpImpl->mParagraphManager.findOrAdd(finalPropList, tabStops);

	TagOpenElement *pOpenListItem = new TagOpenElement("text:list-item");
	if (propList["text:start-value"] && propList["text:start-value"]->getInt() > 0)
		pOpenListItem->addAttribute("text:start-value", propList["text:start-value"]->getStr());
	mpImpl->mBodyElements.push_back(pOpenListItem);

	TagOpenElement *pOpenListElementParagraph = new TagOpenElement("text:p");
	pOpenListElementParagraph->addAttribute("text:style-name", paragName);
	mpImpl->mBodyElements.push_back(pOpenListElementParagraph);

	mpImpl->mListStates.top().mbListElementOpened.top() = true;
	mpImpl->mListStates.top().mbListElementParagraphOpened = true;
}

void OdpGenerator::closeListElement()
{
	// this code is kind of tricky, because we don't actually close the list element (because this list element
	// could contain another list level in OOo's implementation of lists). that is done in the closeListLevel
	// code (or when we open another list element)

	if (mpImpl->mListStates.top().mbListElementParagraphOpened)
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("text:p"));
		mpImpl->mListStates.top().mbListElementParagraphOpened = false;
	}
}

void OdpGenerator::openTable(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &columns)
{
	if (mpImpl->mState.mInComment)
		return;

	WPXString sTableName;
	sTableName.sprintf("Table%i", mpImpl->mTableStyles.size());

	// FIXME: we base the table style off of the page's margin left, ignoring (potential) wordperfect margin
	// state which is transmitted inside the page. could this lead to unacceptable behaviour?
	// WLACH_REFACTORING: characterize this behaviour, probably should nip it at the bud within libwpd
	TableStyle *pTableStyle = new TableStyle(propList, columns, sTableName.cstr());

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

	mpImpl->mBodyElements.push_back(pFrameOpenElement);

	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");

	pTableOpenElement->addAttribute("table:name", sTableName.cstr());
	pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
	mpImpl->mBodyElements.push_back(pTableOpenElement);

	for (int i=0; i<pTableStyle->getNumColumns(); ++i)
	{
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		WPXString sColumnStyleName;
		sColumnStyleName.sprintf("%s.Column%i", sTableName.cstr(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
		mpImpl->mBodyElements.push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpImpl->mBodyElements.push_back(pTableColumnCloseElement);
	}
}

void OdpGenerator::openTableRow(const ::WPXPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;

	if (!mpImpl->mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::openTableRow called with no table\n"));
		return;
	}

	if (propList["libwpd:is-header-row"] && (propList["libwpd:is-header-row"]->getInt()))
	{
		mpImpl->mBodyElements.push_back(new TagOpenElement("table:table-header-rows"));
		mpImpl->mState.mHeaderRow = true;
	}

	WPXString sTableRowStyleName;
	sTableRowStyleName.sprintf("%s.Row%i", mpImpl->mpCurrentTableStyle->getName().cstr(), mpImpl->mpCurrentTableStyle->getNumTableRowStyles());
	TableRowStyle *pTableRowStyle = new TableRowStyle(propList, sTableRowStyleName.cstr());
	mpImpl->mpCurrentTableStyle->addTableRowStyle(pTableRowStyle);

	TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
	pTableRowOpenElement->addAttribute("table:style-name", sTableRowStyleName);
	mpImpl->mBodyElements.push_back(pTableRowOpenElement);
}

void OdpGenerator::closeTableRow()
{
	if (mpImpl->mState.mInComment || !mpImpl->mpCurrentTableStyle)
		return;

	mpImpl->mBodyElements.push_back(new TagCloseElement("table:table-row"));
	if (mpImpl->mState.mHeaderRow)
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("table:table-header-rows"));
		mpImpl->mState.mHeaderRow = false;
	}
}

void OdpGenerator::openTableCell(const ::WPXPropertyList &propList)
{
	if (!mpImpl->mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::openTableCell called with no table\n"));
		return;
	}

	if (mpImpl->mState.mTableCellOpened)
	{
		ODFGEN_DEBUG_MSG(("a table cell in a table cell?!\n"));
		return;
	}

	WPXString sTableCellStyleName;
	sTableCellStyleName.sprintf( "%s.Cell%i", mpImpl->mpCurrentTableStyle->getName().cstr(), mpImpl->mpCurrentTableStyle->getNumTableCellStyles());
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
	mpImpl->mBodyElements.push_back(pTableCellOpenElement);

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

	mpImpl->mBodyElements.push_back(new TagCloseElement("table:table-cell"));
	mpImpl->mState.mTableCellOpened = false;
}

void OdpGenerator::insertCoveredTableCell(const ::WPXPropertyList &/*propList*/)
{
	if (mpImpl->mState.mInComment || !mpImpl->mpCurrentTableStyle)
		return;

	mpImpl->mBodyElements.push_back(new TagOpenElement("table:covered-table-cell"));
	mpImpl->mBodyElements.push_back(new TagCloseElement("table:covered-table-cell"));
}

void OdpGenerator::closeTable()
{
	if (!mpImpl->mState.mInComment)
	{
		mpImpl->mBodyElements.push_back(new TagCloseElement("table:table"));
		mpImpl->mBodyElements.push_back(new TagCloseElement("draw:frame"));
	}
}

void OdpGenerator::startComment(const ::WPXPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
	{
		ODFGEN_DEBUG_MSG(("a comment within a comment?!\n"));
		return;
	}

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

	mpImpl->mBodyElements.push_back(commentElement);
}

void OdpGenerator::endComment()
{
	if (!mpImpl->mState.mInComment)
	{
		ODFGEN_DEBUG_MSG(("there is no comment to close\n"));
		return;
	}

	mpImpl->mState.mInComment = false;
	mpImpl->mBodyElements.push_back(new TagCloseElement("officeooo:annotation"));
}

void OdpGenerator::startNotes(const ::WPXPropertyList &/*propList*/)
{
	if (mpImpl->mState.mInNotes)
	{
		ODFGEN_DEBUG_MSG(("notes in notes?!\n"));
		return;
	}

	TagOpenElement *const notesElement = new TagOpenElement("presentation:notes");
	notesElement->addAttribute("draw:style-name", "PresentationNotesPage");

	mpImpl->mBodyElements.push_back(notesElement);

	TagOpenElement *const thumbnailElement = new TagOpenElement("draw:page-thumbnail");
	thumbnailElement->addAttribute("draw:layer", "layout");
	thumbnailElement->addAttribute("presentation:class", "page");
	// TODO: should the dimensions be hardcoded? If not, where
	// should they come from?
	thumbnailElement->addAttribute("svg:width", "5.5in");
	thumbnailElement->addAttribute("svg:height", "4.12in");
	thumbnailElement->addAttribute("svg:x", "1.5in");
	thumbnailElement->addAttribute("svg:y", "0.84in");
	WPXString pageNumber;
	pageNumber.sprintf("%i", mpImpl->miPageIndex);
	thumbnailElement->addAttribute("draw:page-number", pageNumber);

	mpImpl->mBodyElements.push_back(thumbnailElement);
	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:page-thumbnail"));

	TagOpenElement *const frameElement = new TagOpenElement("draw:frame");
	frameElement->addAttribute("presentation:style-name", "PresentationNotesFrame");
	frameElement->addAttribute("draw:layer", "layout");
	frameElement->addAttribute("presentation:class", "notes");
	frameElement->addAttribute("svg:width", "6.8in");
	frameElement->addAttribute("svg:height", "4.95in");
	frameElement->addAttribute("svg:x", "0.85in");
	frameElement->addAttribute("svg:y", "5.22in");

	mpImpl->mBodyElements.push_back(frameElement);

	mpImpl->mBodyElements.push_back(new TagOpenElement("draw:text-box"));

	mpImpl->mState.mInNotes = true;
}

void OdpGenerator::endNotes()
{
	if (!mpImpl->mState.mInNotes)
	{
		ODFGEN_DEBUG_MSG(("no notes opened\n"));
		return;
	}

	mpImpl->mState.mInNotes = false;

	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:text-box"));
	mpImpl->mBodyElements.push_back(new TagCloseElement("draw:frame"));
	mpImpl->mBodyElements.push_back(new TagCloseElement("presentation:notes"));
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
