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

static librevenge::RVNGString doubleToString(const double value)
{
	librevenge::RVNGProperty *prop = librevenge::RVNGPropertyFactory::newDoubleProp(value);
	librevenge::RVNGString retVal = prop->getStr();
	delete prop;
	return retVal;
}

} // anonymous namespace

class OdgGeneratorPrivate : public OdfGenerator
{
public:
	OdgGeneratorPrivate();
	~OdgGeneratorPrivate();

	/** update a graphic style element */
	void _updateGraphicPropertiesElement(TagOpenElement &element, ::librevenge::RVNGPropertyList const &style);
	void _storeGraphicsStyle();
	void _drawPolySomething(const ::librevenge::RVNGPropertyListVector &vertices, bool isClosed);
	void _drawPath(const librevenge::RVNGPropertyListVector &path);

	//! returns the document type
	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeSettings(OdfDocumentHandler *pHandler);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType);
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
	int miGradientIndex;
	int miBitmapIndex;
	int miStartMarkerIndex;
	int miEndMarkerIndex;
	int miDashIndex;
	int miGraphicsStyleIndex;
	int miPageIndex;
	double mfWidth, mfMaxWidth;
	double mfHeight, mfMaxHeight;

	bool mbIsTextBox;
	bool mbIsParagraph;
	bool mbIsTextOnPath;

	bool mbInTableCell;
private:
	OdgGeneratorPrivate(const OdgGeneratorPrivate &);
	OdgGeneratorPrivate &operator=(const OdgGeneratorPrivate &);

};

OdgGeneratorPrivate::OdgGeneratorPrivate() : OdfGenerator(),
	mGraphicsStrokeDashStyles(),
	mGraphicsGradientStyles(),
	mGraphicsBitmapStyles(),
	mGraphicsMarkerStyles(),
	mGraphicsAutomaticStyles(),
	mPageAutomaticStyles(),
	mPageMasterStyles(),
	mxStyle(),
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
	mbIsTextBox(false),
	mbIsParagraph(false),
	mbIsTextOnPath(false),
	mbInTableCell(false)
{
}

OdgGeneratorPrivate::~OdgGeneratorPrivate()
{
	emptyStorage(&mGraphicsAutomaticStyles);
	emptyStorage(&mGraphicsStrokeDashStyles);
	emptyStorage(&mGraphicsGradientStyles);
	emptyStorage(&mGraphicsBitmapStyles);
	emptyStorage(&mGraphicsMarkerStyles);
	emptyStorage(&mPageAutomaticStyles);
	emptyStorage(&mPageMasterStyles);
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

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML))
	{
		sendStorage(&mGraphicsAutomaticStyles, pHandler);
		mParagraphManager.write(pHandler);
		mSpanManager.write(pHandler);
		// writing out the lists styles
		for (std::vector<ListStyle *>::const_iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); ++iterListStyles)
			(*iterListStyles)->write(pHandler);
		mTableManager.write(pHandler, true);
	}

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
		_writePageLayouts(pHandler);

	pHandler->endElement("office:automatic-styles");
}

void OdgGeneratorPrivate::_writeMasterPages(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:master-styles").write(pHandler);
	sendStorage(&mPageMasterStyles, pHandler);
	pHandler->endElement("office:master-styles");
}

void OdgGeneratorPrivate::_writePageLayouts(OdfDocumentHandler *pHandler)
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
	sendStorage(&mPageAutomaticStyles, pHandler);
#endif

}

void OdgGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);
	sendStorage(&mGraphicsStrokeDashStyles, pHandler);
	sendStorage(&mGraphicsGradientStyles, pHandler);
	sendStorage(&mGraphicsBitmapStyles, pHandler);
	sendStorage(&mGraphicsMarkerStyles, pHandler);
	pHandler->endElement("office:styles");
}

bool OdgGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	pHandler->startDocument();

	std::string const documentType=getDocumentType(streamType);
	TagOpenElement docContentPropList(documentType.c_str());
	docContentPropList.addAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	docContentPropList.addAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	docContentPropList.addAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	docContentPropList.addAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	docContentPropList.addAttribute("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	docContentPropList.addAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
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
		mFontManager.writeFontsDeclaration(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
		_writeStyles(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML) || (streamType == ODF_STYLES_XML))
		_writeAutomaticStyles(pHandler, streamType);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
		_writeMasterPages(pHandler);

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

void OdgGenerator::startDocument(const librevenge::RVNGPropertyList &)
{
}

void OdgGenerator::endDocument()
{
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}

void OdgGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdgGenerator::startPage(const ::librevenge::RVNGPropertyList &propList)
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

void OdgGenerator::endPage()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:page"));
	mpImpl->miPageIndex++;
}

void OdgGenerator::setStyle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->mxStyle.clear();
	mpImpl->mxStyle = propList;
}

void OdgGenerator::startLayer(const ::librevenge::RVNGPropertyList & /* propList */)
{
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("draw:g"));
}

void OdgGenerator::endLayer()
{
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:g"));
}

void OdgGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	if (!propList["svg:x"] || !propList["svg:y"] ||
	        !propList["svg:width"] || !propList["svg:height"])
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::drawRectangle: position undefined\n"));
		return;
	}
	mpImpl->_storeGraphicsStyle();
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

void OdgGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	if (!propList["svg:rx"] || !propList["svg:ry"] || !propList["svg:cx"] || !propList["svg:cy"])
	{
		ODFGEN_DEBUG_MSG(("OdgGenerator::drawEllipse: position undefined\n"));
		return;
	}
	mpImpl->_storeGraphicsStyle();
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

void OdgGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	const ::librevenge::RVNGPropertyListVector *vertices = propList.child("svg:points");
	if (vertices && vertices->count())
		mpImpl->_drawPolySomething(*vertices, false);
}

void OdgGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	const ::librevenge::RVNGPropertyListVector *vertices = propList.child("svg:points");
	if (vertices && vertices->count())
		mpImpl->_drawPolySomething(*vertices, true);
}

void OdgGeneratorPrivate::_drawPolySomething(const ::librevenge::RVNGPropertyListVector &vertices, bool isClosed)
{
	if (vertices.count() < 2)
		return;

	if (vertices.count() == 2)
	{
		if (!vertices[0]["svg:x"]||!vertices[0]["svg:y"]||!vertices[1]["svg:x"]||!vertices[1]["svg:y"])
		{
			ODFGEN_DEBUG_MSG(("OdgGeneratorPrivate::_drawPolySomething: some vertices are not defined\n"));
			return;
		}
		_storeGraphicsStyle();
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

void OdgGeneratorPrivate::_drawPath(const librevenge::RVNGPropertyListVector &path)
{
	if (!path.count())
		return;

	double px = 0.0, py = 0.0, qx = 0.0, qy = 0.0;
	if (!libodfgen::getPathBBox(path, px, py, qx, qy))
		return;

	librevenge::RVNGString sValue;
	_storeGraphicsStyle();
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

void OdgGenerator::drawPath(const librevenge::RVNGPropertyList &propList)
{
	const librevenge::RVNGPropertyListVector *path = propList.child("svg:d");
	if (path && path->count())
		mpImpl->_drawPath(*path);
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


	mpImpl->_storeGraphicsStyle();

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

void OdgGeneratorPrivate::_storeGraphicsStyle()
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

void OdgGeneratorPrivate::_updateGraphicPropertiesElement(TagOpenElement &element, ::librevenge::RVNGPropertyList const &style)
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

void OdgGenerator::startEmbeddedGraphics(const librevenge::RVNGPropertyList &)
{
}

void OdgGenerator::endEmbeddedGraphics()
{
}

void OdgGenerator::startTextObject(const librevenge::RVNGPropertyList &propList)
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
	mpImpl->mbIsTextBox = true;

	mpImpl->pushListState();
}

void OdgGenerator::endTextObject()
{
	if (!mpImpl->mbIsTextBox) return;
	mpImpl->popListState();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
	mpImpl->mbIsTextBox = false;
}

void OdgGenerator::startTableObject(const ::librevenge::RVNGPropertyList &propList)
{
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

void OdgGenerator::endTableObject()
{
	mpImpl->closeTable();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));
	mpImpl->popListState();
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
	if (mpImpl->mbInTableCell)
	{
		ODFGEN_DEBUG_MSG(("a table cell in a table cell?!\n"));
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
	mpImpl->mbInTableCell = mpImpl->openTableCell(pList);
}

void OdgGenerator::closeTableCell()
{
	if (!mpImpl->mbInTableCell)
	{
		ODFGEN_DEBUG_MSG(("no table cell is opened\n"));
		return;
	}

	mpImpl->closeTableCell();
	mpImpl->mbInTableCell = false;
}

void OdgGenerator::insertCoveredTableCell(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->insertCoveredTableCell(propList);
}

void OdgGenerator::defineOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineListLevel(propList, true);
}

void OdgGenerator::defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineListLevel(propList, false);
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
