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

#include <locale.h>
#include <math.h>

#include <string>
#include <map>
#include <stack>

#include "DocumentElement.hxx"
#include "FilterInternal.hxx"

#include "FontStyle.hxx"
#include "GraphicStyle.hxx"
#include "ListStyle.hxx"
#include "PageSpan.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"

#include "OdfGenerator.hxx"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Workaround for the incapacity of impress to have multiple page
// sizes in the same document. Once that limitation is lifted,
// remove this
#define MULTIPAGE_WORKAROUND 1

using namespace libodfgen;

namespace
{

struct GeneratorState
{
	bool mbIsTextBox;
	bool mbIsParagraph;
	bool mbIsTextOnPath;
	bool mInComment;
	bool mTableCellOpened;
	bool mInNotes;

	GeneratorState();
};

GeneratorState::GeneratorState()
	: mbIsTextBox(false)
	, mbIsParagraph(false)
	, mbIsTextOnPath(false)
	, mInComment(false)
	, mTableCellOpened(false)
	, mInNotes(false)
{
}

}

class OdpGeneratorPrivate : public OdfGenerator
{
public:
	OdpGeneratorPrivate();
	~OdpGeneratorPrivate();

	void openTextBoxFrame(const ::librevenge::RVNGPropertyList &propList);
	void closeTextBoxFrame();

	GraphicStyleManager &getGraphicManager()
	{
		return mGraphicManager;
	}

	void updatePageSpanPropertiesToCreatePage(librevenge::RVNGPropertyList &propList);

	void writeNotesStyles(OdfDocumentHandler *pHandler);

	//! returns the document type
	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeSettings(OdfDocumentHandler *pHandler);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType);

	// union of page size
	double mfMaxWidth;
	double mfMaxHeight;

	//! the current page
	PageSpan *mpCurrentPageSpan;
	//! the actual page index
	int miPageIndex;

	// generator state
	GeneratorState mState;

	libodfgen::DocumentElementVector mDummyMasterSlideStorage;

private:
	OdpGeneratorPrivate(const OdpGeneratorPrivate &);
	OdpGeneratorPrivate &operator=(const OdpGeneratorPrivate &);

};

OdpGeneratorPrivate::OdpGeneratorPrivate() :
	mfMaxWidth(0.0), mfMaxHeight(0.0),
	mpCurrentPageSpan(0), miPageIndex(0),
	mState(),
	mDummyMasterSlideStorage()
{
}

OdpGeneratorPrivate::~OdpGeneratorPrivate()
{
}

void OdpGeneratorPrivate::updatePageSpanPropertiesToCreatePage(librevenge::RVNGPropertyList &pList)
{
	double width=0;
	if (pList["svg:width"] && !pList["fo:page-width"])
		pList.insert("fo:page-width", pList["svg:width"]->clone());
	if (pList["fo:page-width"] && getInchValue(*pList["fo:page-width"], width) && width>mfMaxWidth)
		mfMaxWidth=width;
	double height=0;
	if (pList["svg:height"] && !pList["fo:page-height"])
		pList.insert("fo:page-height", pList["svg:height"]->clone());
	if (pList["fo:page-height"] && getInchValue(*pList["fo:page-height"], height) && height>mfMaxHeight)
		mfMaxHeight=height;

	// generate drawing-page style
	librevenge::RVNGPropertyList drawingPageStyle;
	librevenge::RVNGPropertyListVector drawingPageVector;
	drawingPageStyle.insert("draw:fill", "none");
	drawingPageVector.append(drawingPageStyle);
	pList.insert("librevenge:drawing-page", drawingPageVector);

	// do not generate footnote separator data
	pList.insert("librevenge:footnote", librevenge::RVNGPropertyListVector());
}

void OdpGeneratorPrivate::openTextBoxFrame(const ::librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGPropertyList tmpList(propList), graphicStyle;
	if (!propList["draw:stroke"])
		tmpList.insert("draw:stroke", "none");
	if (!propList["draw:fill"])
		tmpList.insert("draw:fill", "none");
	mGraphicManager.addGraphicProperties(tmpList, graphicStyle);
	mGraphicManager.addFrameProperties(propList, graphicStyle);
	librevenge::RVNGString sValue=mGraphicManager.findOrAdd
	                              (graphicStyle, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic);

	TagOpenElement *pDrawFrameOpenElement = new TagOpenElement("draw:frame");
	pDrawFrameOpenElement->addAttribute("draw:style-name", sValue);
	pDrawFrameOpenElement->addAttribute("draw:layer", "layout");

	if (!propList["svg:width"] && !propList["svg:height"])
	{
		pDrawFrameOpenElement->addAttribute("fo:min-width", "1in");
		pDrawFrameOpenElement->addAttribute("svg:width", "10in");
	}
	else
	{
		if (propList["svg:width"])
			pDrawFrameOpenElement->addAttribute("svg:width", propList["svg:width"]->getStr());
		if (propList["svg:height"])
			pDrawFrameOpenElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	}
	static char const *attrib[]=
	{
		"fo:min-width", "fo:min-height", "fo:max-width", "fo:max-height", "fo:padding-top", "fo:padding-bottom",
		"fo:padding-left", "fo:padding-right", "draw:textarea-vertical-align", "draw:fill", "draw:fill-color"
	};
	for (unsigned i=0; i<ODFGEN_N_ELEMENTS(attrib); ++i)
	{
		if (propList[attrib[i]])
			pDrawFrameOpenElement->addAttribute(attrib[i], propList[attrib[i]]->getStr());
	}

	double x = 0.0;
	double y = 0.0;
	if (propList["svg:x"])
		libodfgen::getInchValue(*propList["svg:x"], x);
	if (propList["svg:y"])
		libodfgen::getInchValue(*propList["svg:y"], y);
	double angle(propList["librevenge:rotate"] ? - M_PI * propList["librevenge:rotate"]->getDouble() / 180.0 : 0.0);
	if (angle < 0 || angle > 0)
	{
		// compute position: make sure that the center position remains invariant
		double width = 0.0;
		double height = 0.0;
		if (propList["librevenge:rotate-cx"])
		{
			getInchValue(*propList["librevenge:rotate-cx"],width);
			width = 2.0*(width-x);
		}
		else if (propList["svg:width"])
			getInchValue(*propList["svg:width"],width);
		if (propList["librevenge:rotate-cy"])
		{
			getInchValue(*propList["librevenge:rotate-cy"],height);
			height = 2.0*(height-y);
		}
		else if (propList["svg:height"])
			getInchValue(*propList["svg:height"],height);
		double deltax((width*cos(angle)+height*sin(angle)-width)/2.0);
		double deltay((-width*sin(angle)+height*cos(angle)-height)/2.0);
		x -= deltax;
		y -= deltay;
	}
	shared_ptr<librevenge::RVNGProperty> svg_x(librevenge::RVNGPropertyFactory::newInchProp(x));
	shared_ptr<librevenge::RVNGProperty> svg_y(librevenge::RVNGPropertyFactory::newInchProp(y));
	if (angle < 0 || angle > 0)
	{
		shared_ptr<librevenge::RVNGProperty> librevenge_rotate(librevenge::RVNGPropertyFactory::newDoubleProp(angle));
		sValue.sprintf("rotate (%s) translate(%s, %s)",
		               librevenge_rotate->getStr().cstr(),
		               svg_x->getStr().cstr(),
		               svg_y->getStr().cstr());
		pDrawFrameOpenElement->addAttribute("draw:transform", sValue);
	}
	else
	{
		if (propList["svg:x"])
			pDrawFrameOpenElement->addAttribute("svg:x", svg_x->getStr());
		if (propList["svg:y"])
			pDrawFrameOpenElement->addAttribute("svg:y", svg_y->getStr());
	}
	getCurrentStorage()->push_back(pDrawFrameOpenElement);
}

void OdpGeneratorPrivate::closeTextBoxFrame()
{
	getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
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

		pHandler->startElement("style:paragraph-properties", paraProps);
		pHandler->endElement("style:paragraph-properties");

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

void OdpGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	TagOpenElement("office:automatic-styles").write(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
	{
		mPageSpanManager.writePageStyles(pHandler, Style::Z_StyleAutomatic);

		mSpanManager.write(pHandler, Style::Z_StyleAutomatic);
		mParagraphManager.write(pHandler, Style::Z_StyleAutomatic);
		mListManager.write(pHandler, Style::Z_StyleAutomatic);
		mGraphicManager.write(pHandler, Style::Z_StyleAutomatic);
		mTableManager.write(pHandler, Style::Z_StyleAutomatic, true);
	}
	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML))
	{
		mPageSpanManager.writePageStyles(pHandler, Style::Z_ContentAutomatic);

		mSpanManager.write(pHandler, Style::Z_ContentAutomatic);
		mParagraphManager.write(pHandler, Style::Z_ContentAutomatic);
		mListManager.write(pHandler, Style::Z_ContentAutomatic);
		mGraphicManager.write(pHandler, Style::Z_ContentAutomatic);
		mTableManager.write(pHandler, Style::Z_ContentAutomatic, true);
	}

	// checkme: do we want to write in the Z_ContentAutomatic or in Z_StyleAutomatic
	writeNotesStyles(pHandler);

	pHandler->endElement("office:automatic-styles");
}

void OdpGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);
	mPageSpanManager.writePageStyles(pHandler, Style::Z_Style);

	mParagraphManager.write(pHandler, Style::Z_Style);
	mSpanManager.write(pHandler, Style::Z_Style);
	mListManager.write(pHandler, Style::Z_Style);
	mGraphicManager.write(pHandler, Style::Z_Style);
	pHandler->endElement("office:styles");
}

bool OdpGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	if (streamType == ODF_MANIFEST_XML)
	{
		pHandler->startDocument();
		TagOpenElement manifestElement("manifest:manifest");
		manifestElement.addAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
		manifestElement.write(pHandler);

		TagOpenElement mainFile("manifest:file-entry");
		mainFile.addAttribute("manifest:media-type", "application/vnd.oasis.opendocument.presentation");
		mainFile.addAttribute("manifest:full-path", "/");
		mainFile.write(pHandler);
		TagCloseElement("manifest:file-entry").write(pHandler);
		appendFilesInManifest(pHandler);

		TagCloseElement("manifest:manifest").write(pHandler);
		pHandler->endDocument();
		return true;
	}

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
	docContentPropList.addAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	docContentPropList.addAttribute("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
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
	{
		TagOpenElement("office:font-face-decls").write(pHandler);
		mFontManager.write(pHandler, Style::Z_Font);
		TagCloseElement("office:font-face-decls").write(pHandler);
	}

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
		_writeStyles(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML) || (streamType == ODF_STYLES_XML))
		_writeAutomaticStyles(pHandler, streamType);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
	{
		TagOpenElement("office:master-styles").write(pHandler);
		mPageSpanManager.writeMasterPages(pHandler);
		appendLayersMasterStyles(pHandler);
		pHandler->endElement("office:master-styles");
	}

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

librevenge::RVNGStringVector OdpGenerator::getObjectNames() const
{
	if (mpImpl)
		return mpImpl->getObjectNames();
	return librevenge::RVNGStringVector();
}

bool OdpGenerator::getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler)
{
	if (!mpImpl)
		return false;
	return mpImpl->getObjectContent(objectName, pHandler);
}

void OdpGenerator::startDocument(const ::librevenge::RVNGPropertyList &/*propList*/)
{
}

void OdpGenerator::endDocument()
{
#ifdef MULTIPAGE_WORKAROUND
	if (mpImpl->miPageIndex>1)
		mpImpl->getPageSpanManager().resetPageSizeAndMargins(mpImpl->mfMaxWidth, mpImpl->mfMaxHeight);
#endif
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}

void OdpGenerator::setDocumentMetaData(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdpGenerator::defineEmbeddedFont(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineEmbeddedFont(propList);
}

void OdpGenerator::startSlide(const ::librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGPropertyList pList(propList);

	mpImpl->mpCurrentPageSpan=0;
	if (pList["librevenge:master-page-name"])
	{
		mpImpl->mpCurrentPageSpan=mpImpl->getPageSpanManager().get(pList["librevenge:master-page-name"]->getStr());
		if (!mpImpl->mpCurrentPageSpan)
			pList.remove("librevenge:master-page-name");
	}

	if (!mpImpl->mpCurrentPageSpan)
	{
		mpImpl->updatePageSpanPropertiesToCreatePage(pList);
		mpImpl->mpCurrentPageSpan=mpImpl->getPageSpanManager().add(pList);
	}
	++mpImpl->miPageIndex;
	librevenge::RVNGString pageName;
	if (propList["draw:name"])
		pageName.appendEscapedXML(propList["draw:name"]->getStr());
	else
		pageName.sprintf("page%i", mpImpl->miPageIndex);
	TagOpenElement *pDrawPageOpenElement = new TagOpenElement("draw:page");
	pDrawPageOpenElement->addAttribute("draw:name", pageName);
	pDrawPageOpenElement->addAttribute("draw:style-name", mpImpl->mpCurrentPageSpan->getDrawingName());
	pDrawPageOpenElement->addAttribute("draw:master-page-name", mpImpl->mpCurrentPageSpan->getMasterName());
	mpImpl->getCurrentStorage()->push_back(pDrawPageOpenElement);
}

void OdpGenerator::endSlide()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:page"));
}

void OdpGenerator::startMasterSlide(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->inMasterPage())
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::startMasterSlide oops a master page is already started\n"));
		return;
	}
	mpImpl->startMasterPage(propList);
	bool ok=mpImpl->inMasterPage() && propList["librevenge:master-page-name"];
	if (ok)
	{
		librevenge::RVNGPropertyList pList(propList);
		mpImpl->updatePageSpanPropertiesToCreatePage(pList);

		PageSpan *pageSpan=mpImpl->getPageSpanManager().add(pList, true);
		if (pageSpan)
		{
			libodfgen::DocumentElementVector *pMasterElements = new libodfgen::DocumentElementVector;
			pageSpan->setMasterContent(pMasterElements);
			mpImpl->pushStorage(pMasterElements);
		}
		else
			ok=false;
	}
	if (!ok)
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::startMasterSlide creation of the master page has failed\n"));
		mpImpl->pushStorage(&mpImpl->mDummyMasterSlideStorage);
	}
}

void OdpGenerator::endMasterSlide()
{
	if (!mpImpl->inMasterPage())
	{
		ODFGEN_DEBUG_MSG(("OdpGenerator::endMasterSlide: find no opend master page\n"));
		return;
	}
	mpImpl->popStorage();
	mpImpl->endMasterPage();
	mpImpl->mDummyMasterSlideStorage.clear();
}

void OdpGenerator::setStyle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineGraphicStyle(propList);
}

void OdpGenerator::setSlideTransition(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::startLayer(const ::librevenge::RVNGPropertyList & /* propList */)
{
}

void OdpGenerator::endLayer()
{
}

void OdpGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawRectangle(propList);
}

void OdpGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawEllipse(propList);
}

void OdpGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPolySomething(propList, false);
}

void OdpGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPolySomething(propList, true);
}

void OdpGenerator::drawPath(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPath(propList);
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

	librevenge::RVNGPropertyList style=mpImpl->getGraphicStyle();
	if ((flipX && !flipY) || (!flipX && flipY))
		style.insert("style:mirror", "horizontal");
	else
		style.insert("style:mirror", "none");
	if (propList["draw:color-mode"])
		style.insert("draw:color-mode", propList["draw:color-mode"]->getStr());
	if (propList["draw:luminance"])
		style.insert("draw:luminance", propList["draw:luminance"]->getStr());
	if (propList["draw:contrast"])
		style.insert("draw:contrast", propList["draw:contrast"]->getStr());
	if (propList["draw:gamma"])
		style.insert("draw:gamma", propList["draw:gamma"]->getStr());
	if (propList["draw:red"])
		style.insert("draw:red", propList["draw:red"]->getStr());
	if (propList["draw:green"])
		style.insert("draw:green", propList["draw:green"]->getStr());
	if (propList["draw:blue"])
		style.insert("draw:blue", propList["draw:blue"]->getStr());

	double x, y;
	double height, width;
	getInchValue(*propList["svg:x"], x);
	getInchValue(*propList["svg:y"], y);
	getInchValue(*propList["svg:height"], height);
	getInchValue(*propList["svg:width"], width);

	if (flipY)
	{
		x += width;
		y += height;
		width *= -1.0;
		height *= -1.0;
	}

	double angle(propList["librevenge:rotate"] ? - M_PI * propList["librevenge:rotate"]->getDouble() / 180.0 : 0.0);
	if (angle < 0 || angle > 0)
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

	librevenge::RVNGPropertyList finalStyle;
	mpImpl->getGraphicManager().addGraphicProperties(style, finalStyle);
	pDrawFrameElement->addAttribute
	("draw:style-name", mpImpl->getGraphicManager().findOrAdd
	 (finalStyle, mpImpl->useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic));

	pDrawFrameElement->addAttribute("svg:height", framePropList["svg:height"]->getStr());
	pDrawFrameElement->addAttribute("svg:width", framePropList["svg:width"]->getStr());

	if (angle < 0 || angle > 0)
	{
		framePropList.insert("librevenge:rotate", angle, librevenge::RVNG_GENERIC);
		librevenge::RVNGString sValue;
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

void OdpGenerator::drawConnector(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawConnector(propList);
}

void OdpGenerator::startEmbeddedGraphics(const librevenge::RVNGPropertyList &)
{
}

void OdpGenerator::endEmbeddedGraphics()
{
}

void OdpGenerator::openGroup(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->openGroup(propList);
}

void OdpGenerator::closeGroup()
{
	mpImpl->closeGroup();
}

void OdpGenerator::startTextObject(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openTextBoxFrame(propList);
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:text-box"));
	mpImpl->mState.mbIsTextBox = true;
	mpImpl->pushListState();
}

void OdpGenerator::endTextObject()
{
	if (!mpImpl->mState.mbIsTextBox)
		return;

	mpImpl->popListState();
	mpImpl->mState.mbIsTextBox = false;
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
	mpImpl->closeTextBoxFrame();
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

void OdpGenerator::openLink(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openLink(propList);
}

void OdpGenerator::closeLink()
{
	mpImpl->closeLink();
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

void OdpGenerator::startTableObject(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;
	mpImpl->pushListState();

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
	mpImpl->openTable(propList);
}

void OdpGenerator::endTableObject()
{
	if (mpImpl->mState.mInComment)
		return;
	mpImpl->closeTable();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
	mpImpl->popListState();
}

void OdpGenerator::openTableRow(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;
	mpImpl->openTableRow(propList);
}

void OdpGenerator::closeTableRow()
{
	if (mpImpl->mState.mInComment)
		return;
	mpImpl->closeTableRow();
}

void OdpGenerator::openTableCell(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;

	if (mpImpl->mState.mTableCellOpened)
	{
		ODFGEN_DEBUG_MSG(("a table cell in a table cell?!\n"));
		return;
	}

	librevenge::RVNGPropertyList pList(propList);
	pList.insert("fo:padding", "0.0382in");
	pList.insert("draw:fill", "none");
	pList.insert("draw:textarea-horizontal-align", "center");

	if (pList["fo:background-color"])
	{
		pList.insert("draw:fill", "solid");
		pList.insert("draw:fill-color", pList["fo:background-color"]->getStr());
	}
	if (!propList["fo:border"])
		pList.insert("fo:border", "0.03pt solid #000000");
	mpImpl->mState.mTableCellOpened = mpImpl->openTableCell(pList);
}

void OdpGenerator::closeTableCell()
{
	if (mpImpl->mState.mInComment)
		return;

	if (!mpImpl->mState.mTableCellOpened)
	{
		ODFGEN_DEBUG_MSG(("no table cell is opened\n"));
		return;
	}

	mpImpl->closeTableCell();
	mpImpl->mState.mTableCellOpened = false;
}

void OdpGenerator::insertCoveredTableCell(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mState.mInComment)
		return;

	mpImpl->insertCoveredTableCell(propList);
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
	double val;
	if (propList["svg:x"] && getInchValue(*propList["svg:x"], val))
		commentElement->addAttribute("svg:x", doubleToString(72 * val));
	if (propList["svg:y"] && getInchValue(*propList["svg:y"], val))
		commentElement->addAttribute("svg:y", doubleToString(72 * val));
	if (propList["svg:width"] && getInchValue(*propList["svg:width"], val))
		commentElement->addAttribute("svg:width", doubleToString(72 * val));
	if (propList["svg:height"] && getInchValue(*propList["svg:height"], val))
		commentElement->addAttribute("svg:height", doubleToString(72 * val));

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

void OdpGenerator::defineChartStyle(const ::librevenge::RVNGPropertyList &)
{
	// TODO: implement me
}

void OdpGenerator::openChart(const ::librevenge::RVNGPropertyList &)
{
	// TODO: implement me
}

void OdpGenerator::closeChart()
{
	// TODO: implement me
}

void OdpGenerator::openChartTextObject(const ::librevenge::RVNGPropertyList &)
{
	// TODO: implement me
}

void OdpGenerator::closeChartTextObject()
{
	// TODO: implement me
}

void OdpGenerator::openChartPlotArea(const ::librevenge::RVNGPropertyList &)
{
	// TODO: implement me
}

void OdpGenerator::closeChartPlotArea()
{
	// TODO: implement me
}

void OdpGenerator::insertChartAxis(const ::librevenge::RVNGPropertyList &)
{
	// TODO: implement me
}

void OdpGenerator::openChartSeries(const ::librevenge::RVNGPropertyList &)
{
	// TODO: implement me
}

void OdpGenerator::closeChartSeries()
{
	// TODO: implement me
}

void OdpGenerator::openAnimationSequence(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::closeAnimationSequence()
{
	// TODO: implement me
}

void OdpGenerator::openAnimationGroup(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::closeAnimationGroup()
{
	// TODO: implement me
}

void OdpGenerator::openAnimationIteration(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::closeAnimationIteration()
{
	// TODO: implement me
}

void OdpGenerator::insertMotionAnimation(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::insertColorAnimation(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::insertAnimation(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
}

void OdpGenerator::insertEffect(const ::librevenge::RVNGPropertyList &/*propList*/)
{
	// TODO: implement me
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
