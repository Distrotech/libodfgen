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

#include <locale.h>
#include <math.h>
#include <string>
#include <map>

#include <libodfgen/libodfgen.hxx>

#include "FilterInternal.hxx"
#include "DocumentElement.hxx"

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

// Workaround for the incapacity of draw to have multiple page
// sizes in the same document. Once that limitation is lifted,
// remove this
#define MULTIPAGE_WORKAROUND 1

using namespace libodfgen;

class OdgGeneratorPrivate : public OdfGenerator
{
public:
	OdgGeneratorPrivate();
	~OdgGeneratorPrivate();

	GraphicStyleManager &getGraphicManager()
	{
		return mGraphicManager;
	}

	void updatePageSpanPropertiesToCreatePage(librevenge::RVNGPropertyList &propList);

	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeSettings(OdfDocumentHandler *pHandler);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType);

	//
	// state gestion
	//

	//! the state we use for writing the final document
	struct State
	{
		//! constructor
		State() : mbIsTextBox(false), miIntricatedTextBox(0), mbInTableCell(false),
			mbInFalseLayerGroup(false)
		{
		}
		/** flag to know if a text box is opened */
		bool mbIsTextBox;
		/** number of intricated text box, in case a textbox is called inside a text box */
		int miIntricatedTextBox;
		/** flag to know if a table cell is opened */
		bool mbInTableCell;
		/** flag to know if a group was used instead of a layer */
		bool mbInFalseLayerGroup;
	};

	// returns the actual state
	State &getState()
	{
		if (mStateStack.empty())
		{
			ODFGEN_DEBUG_MSG(("OdgGeneratorPrivate::getState: no state\n"));
			mStateStack.push(State());
		}
		return mStateStack.top();
	}
	// push a state
	void pushState()
	{
		mStateStack.push(State());
	}
	// pop a state
	void popState()
	{
		if (!mStateStack.empty())
			mStateStack.pop();
		else
		{
			ODFGEN_DEBUG_MSG(("OdgGeneratorPrivate::popState: no state\n"));
		}
	}
	std::stack<State> mStateStack;

	// union of page size
	double mfMaxWidth;
	double mfMaxHeight;

	//! the current page
	PageSpan *mpCurrentPageSpan;
	//! the actual page index
	int miPageIndex;
	libodfgen::DocumentElementVector mDummyMasterSlideStorage;

private:
	OdgGeneratorPrivate(const OdgGeneratorPrivate &);
	OdgGeneratorPrivate &operator=(const OdgGeneratorPrivate &);

};

OdgGeneratorPrivate::OdgGeneratorPrivate() : OdfGenerator(),
	mStateStack(),
	mfMaxWidth(0.0), mfMaxHeight(0.0),
	mpCurrentPageSpan(0), miPageIndex(0),
	mDummyMasterSlideStorage()
{
	pushState();
}

OdgGeneratorPrivate::~OdgGeneratorPrivate()
{
}

void OdgGeneratorPrivate::updatePageSpanPropertiesToCreatePage(librevenge::RVNGPropertyList &pList)
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

void OdgGeneratorPrivate::_writeSettings(OdfDocumentHandler *pHandler)
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

void OdgGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType)
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

	pHandler->endElement("office:automatic-styles");
}

void OdgGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);
	mPageSpanManager.writePageStyles(pHandler, Style::Z_Style);

	mGraphicManager.write(pHandler, Style::Z_Style);
	mParagraphManager.write(pHandler, Style::Z_Style);
	mSpanManager.write(pHandler, Style::Z_Style);
	mListManager.write(pHandler, Style::Z_Style);
	pHandler->endElement("office:styles");
}

bool OdgGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	if (streamType == ODF_MANIFEST_XML)
	{
		pHandler->startDocument();
		TagOpenElement manifestElement("manifest:manifest");
		manifestElement.addAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
		manifestElement.write(pHandler);
		TagOpenElement mainFile("manifest:file-entry");
		mainFile.addAttribute("manifest:media-type", "application/vnd.oasis.opendocument.graphics");
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
	docContentPropList.addAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	docContentPropList.addAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	docContentPropList.addAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	docContentPropList.addAttribute("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	docContentPropList.addAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	docContentPropList.addAttribute("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	docContentPropList.addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	docContentPropList.addAttribute("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	docContentPropList.addAttribute("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	docContentPropList.addAttribute("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	docContentPropList.addAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	docContentPropList.addAttribute("office:version", "1.0", true);
	if (streamType == ODF_FLAT_XML)
		docContentPropList.addAttribute("office:mimetype", "application/vnd.oasis.opendocument.graphics");
	docContentPropList.write(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_META_XML)
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
		TagOpenElement("office:drawing").write(pHandler);
		sendStorage(&mBodyStorage, pHandler);
		pHandler->endElement("office:drawing");
		pHandler->endElement("office:body");
	}

	pHandler->endElement(documentType.c_str());

	pHandler->endDocument();
	return true;
}

OdgGenerator::OdgGenerator() : mpImpl(new OdgGeneratorPrivate)
{
}

OdgGenerator::~OdgGenerator()
{
	delete mpImpl;
}

void OdgGenerator::addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
{
	if (mpImpl)
		mpImpl->addDocumentHandler(pHandler, streamType);
}

librevenge::RVNGStringVector OdgGenerator::getObjectNames() const
{
	if (mpImpl)
		return mpImpl->getObjectNames();
	return librevenge::RVNGStringVector();
}

bool OdgGenerator::getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler)
{
	if (!mpImpl)
		return false;
	return mpImpl->getObjectContent(objectName, pHandler);
}

void OdgGenerator::startDocument(const librevenge::RVNGPropertyList &)
{
}

void OdgGenerator::endDocument()
{
#ifdef MULTIPAGE_WORKAROUND
	if (mpImpl->miPageIndex>1)
		mpImpl->getPageSpanManager().resetPageSizeAndMargins(mpImpl->mfMaxWidth, mpImpl->mfMaxHeight);
#endif
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}

void OdgGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdgGenerator::defineEmbeddedFont(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineEmbeddedFont(propList);
}

void OdgGenerator::startPage(const ::librevenge::RVNGPropertyList &propList)
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

void OdgGenerator::endPage()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:page"));
}

void OdgGenerator::startMasterPage(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->inMasterPage())
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::startMasterPage: oops a master page is already started\n"));
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
		ODFGEN_DEBUG_MSG(("OdgGenerator::startMasterPage: creation of the master page has failed\n"));
		mpImpl->pushStorage(&mpImpl->mDummyMasterSlideStorage);
	}
	mpImpl->pushState();
}

void OdgGenerator::endMasterPage()
{
	if (!mpImpl->inMasterPage())
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::endMasterPage: find no opend master page\n"));
		return;
	}
	mpImpl->popState();
	mpImpl->popStorage();
	mpImpl->endMasterPage();
	mpImpl->mDummyMasterSlideStorage.clear();
}

void OdgGenerator::setStyle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineGraphicStyle(propList);
}

void OdgGenerator::startLayer(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->inMasterPage())
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::startLayer: can not be called in master page\n"));
		return;
	}

	mpImpl->pushState();
	if (propList["draw:layer"]&&!propList["draw:layer"]->getStr().empty())
		mpImpl->openLayer(propList);
	else
	{
		mpImpl->getState().mbInFalseLayerGroup=true;
		mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:g"));
	}
}

void OdgGenerator::endLayer()
{
	if (mpImpl->inMasterPage())
		return;

	if (mpImpl->getState().mbInFalseLayerGroup)
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:g"));
	else
		mpImpl->closeLayer();
	mpImpl->popState();
}

void OdgGenerator::openGroup(const ::librevenge::RVNGPropertyList & /* propList */)
{
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:g"));
}

void OdgGenerator::closeGroup()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:g"));
}

void OdgGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawRectangle(propList);
}

void OdgGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawEllipse(propList);
}

void OdgGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPolySomething(propList, false);
}

void OdgGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPolySomething(propList, true);
}

void OdgGenerator::drawPath(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPath(propList);
}

void OdgGenerator::drawGraphicObject(const ::librevenge::RVNGPropertyList &propList)
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
	pDrawFrameElement->addAttribute("draw:style-name",
	                                mpImpl->getGraphicManager().findOrAdd
	                                (finalStyle, mpImpl->useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic));
	pDrawFrameElement->addAttribute("draw:layer", mpImpl->getLayerName(propList));

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

void OdgGenerator::drawConnector(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawConnector(propList);
}

void OdgGenerator::startEmbeddedGraphics(const librevenge::RVNGPropertyList &)
{
}

void OdgGenerator::endEmbeddedGraphics()
{
}

void OdgGenerator::startTextObject(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->getState().mbIsTextBox)
	{
		// this seems to make LibreOffice crash, so ...
		ODFGEN_DEBUG_MSG(("OdgGenerator::startTextObject: sending intricated text box is not implemented\n"));
		++mpImpl->getState().miIntricatedTextBox;
		return;
	}
	librevenge::RVNGPropertyList tmpList(propList), graphicStyle;
	if (!propList["draw:stroke"])
		tmpList.insert("draw:stroke", "none");
	if (!propList["draw:fill"])
		tmpList.insert("draw:fill", "none");
	mpImpl->getGraphicManager().addGraphicProperties(tmpList, graphicStyle);
	mpImpl->getGraphicManager().addFrameProperties(propList, graphicStyle);
	librevenge::RVNGString sValue=mpImpl->getGraphicManager().findOrAdd
	                              (graphicStyle, mpImpl->useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic);

	TagOpenElement *pDrawFrameOpenElement = new TagOpenElement("draw:frame");
	pDrawFrameOpenElement->addAttribute("draw:style-name", sValue);
	pDrawFrameOpenElement->addAttribute("draw:layer", mpImpl->getLayerName(propList));

	if (!propList["svg:width"] && !propList["svg:height"])
	{
		pDrawFrameOpenElement->addAttribute("svg:width", "10in");
		pDrawFrameOpenElement->addAttribute("fo:min-width", "1in");
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
		getInchValue(*propList["svg:x"],x);
	if (propList["svg:y"])
		getInchValue(*propList["svg:y"],y);
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
	mpImpl->getCurrentStorage()->push_back(pDrawFrameOpenElement);
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:text-box"));

	// push the different states
	mpImpl->pushState();
	mpImpl->pushListState();
	mpImpl->getState().mbIsTextBox = true;
}

void OdgGenerator::endTextObject()
{
	OdgGeneratorPrivate::State &state=mpImpl->getState();
	if (!state.mbIsTextBox) return;
	if (state.miIntricatedTextBox)
	{
		// we did not open textbox when seeing intricated text box
		--state.miIntricatedTextBox;
		return;
	}
	// pop the different state
	mpImpl->popListState();
	mpImpl->popState();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
}

void OdgGenerator::startTableObject(const ::librevenge::RVNGPropertyList &propList)
{
	// table must be inside a frame
	TagOpenElement *pFrameOpenElement = new TagOpenElement("draw:frame");

	pFrameOpenElement->addAttribute("draw:style-name", "standard");
	pFrameOpenElement->addAttribute("draw:layer", mpImpl->getLayerName(propList));

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

	mpImpl->pushListState();
	mpImpl->pushState();
}

void OdgGenerator::endTableObject()
{
	mpImpl->popState();
	mpImpl->popListState();

	mpImpl->closeTable();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
}

void OdgGenerator::openTableRow(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->openTableRow(propList);
}

void OdgGenerator::closeTableRow()
{
	mpImpl->closeTableRow();
}

void OdgGenerator::openTableCell(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->getState().mbInTableCell)
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::openTableCell: a table cell in a table cell?!\n"));
		return;
	}
	librevenge::RVNGPropertyList pList(propList);
	if (pList["fo:background-color"])
	{
		pList.insert("draw:fill", "solid");
		pList.insert("draw:fill-color", pList["fo:background-color"]->getStr());
	}
	else if (!pList["draw:fill"])
		pList.insert("draw:fill", "none");
	mpImpl->getState().mbInTableCell = mpImpl->openTableCell(pList);
}

void OdgGenerator::closeTableCell()
{
	if (!mpImpl->getState().mbInTableCell)
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::closeTableCell: no table cell is opened\n"));
		return;
	}

	mpImpl->closeTableCell();
	mpImpl->getState().mbInTableCell = false;
}

void OdgGenerator::insertCoveredTableCell(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->insertCoveredTableCell(propList);
}

void OdgGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListLevel(propList, true);
}

void OdgGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListLevel(propList, false);
}

void OdgGenerator::closeOrderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdgGenerator::closeUnorderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdgGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListElement(propList);
}

void OdgGenerator::closeListElement()
{
	mpImpl->closeListElement();
}

void OdgGenerator::defineParagraphStyle(librevenge::RVNGPropertyList const &propList)
{
	mpImpl->defineParagraphStyle(propList);
}

void OdgGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGPropertyList finalPropList(propList);
	finalPropList.insert("style:parent-style-name", "Standard");
	mpImpl->openParagraph(finalPropList);
}

void OdgGenerator::closeParagraph()
{
	mpImpl->closeParagraph();
}

void OdgGenerator::defineCharacterStyle(librevenge::RVNGPropertyList const &propList)
{
	mpImpl->defineCharacterStyle(propList);
}


void OdgGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openSpan(propList);
}

void OdgGenerator::closeSpan()
{
	mpImpl->closeSpan();
}

void OdgGenerator::openLink(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openLink(propList);
}

void OdgGenerator::closeLink()
{
	mpImpl->closeLink();
}

void OdgGenerator::insertTab()
{
	mpImpl->insertTab();
}

void OdgGenerator::insertSpace()
{
	mpImpl->insertSpace();
}

void OdgGenerator::insertLineBreak()
{
	mpImpl->insertLineBreak();
}

void OdgGenerator::insertField(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->insertField(propList);
}

void OdgGenerator::insertText(const librevenge::RVNGString &text)
{
	mpImpl->insertText(text);
}

void OdgGenerator::initStateWith(OdfGenerator const &orig)
{
	mpImpl->initStateWith(orig);
}

void OdgGenerator::registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler)
{
	mpImpl->registerEmbeddedObjectHandler(mimeType, objectHandler);
}

void OdgGenerator::registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler)
{
	mpImpl->registerEmbeddedImageHandler(mimeType, imageHandler);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
