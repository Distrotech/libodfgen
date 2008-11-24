/* libwpg
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02111-1301 USA
 *
 * For further information visit http://libwpg.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include "OdgExporter.hxx"
#include "DocumentElement.hxx"
#include "DocumentHandler.hxx"
#include <locale.h>
#include <math.h>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OdgExporter::OdgExporter(DocumentHandler *pHandler, const bool isFlatXML):
	mpHandler(pHandler),
	mxFillRule(AlternatingFill),
	miGradientIndex(1),
	miDashIndex(1), 
	miGraphicsStyleIndex(1),
	mfWidth(0.0f),
	mfHeight(0.0f),
	mbIsFlatXML(isFlatXML)
{
}

OdgExporter::~OdgExporter()
{

	for (std::vector<DocumentElement *>::iterator iterBody = mBodyElements.begin(); iterBody != mBodyElements.end(); iterBody++)
	{
		delete (*iterBody);
		(*iterBody) = NULL;
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsAutomaticStyles = mGraphicsAutomaticStyles.begin();
		iterGraphicsAutomaticStyles != mGraphicsAutomaticStyles.end(); iterGraphicsAutomaticStyles++)
	{
		delete((*iterGraphicsAutomaticStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsStrokeDashStyles = mGraphicsStrokeDashStyles.begin();
		iterGraphicsStrokeDashStyles != mGraphicsStrokeDashStyles.end(); iterGraphicsStrokeDashStyles++)
	{
		delete((*iterGraphicsStrokeDashStyles));
	}

	for (std::vector<DocumentElement *>::iterator iterGraphicsGradientStyles = mGraphicsGradientStyles.begin();
		iterGraphicsGradientStyles != mGraphicsGradientStyles.end(); iterGraphicsGradientStyles++)
	{
		delete((*iterGraphicsGradientStyles));
	}
}

void OdgExporter::startGraphics(double width, double height)
{
	miGradientIndex = 1;
	miDashIndex = 1;
	miGraphicsStyleIndex = 1;
	mfWidth = width;
	mfHeight = height;

	mpHandler->startDocument();
	TagOpenElement tmpOfficeDocumentContent("office:document");
	tmpOfficeDocumentContent.addAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	tmpOfficeDocumentContent.addAttribute("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	tmpOfficeDocumentContent.addAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	tmpOfficeDocumentContent.addAttribute("office:version", "1.0");
	if (mbIsFlatXML)
		tmpOfficeDocumentContent.addAttribute("office:mimetype", "application/vnd.oasis.opendocument.graphics");	
	tmpOfficeDocumentContent.write(mpHandler);
	
	TagOpenElement("office:settings").write(mpHandler);
	
	TagOpenElement configItemSetOpenElement("config:config-item-set");
	configItemSetOpenElement.addAttribute("config:name", "ooo:view-settings");
	configItemSetOpenElement.write(mpHandler);
	
	TagOpenElement configItemOpenElement("config:config-item");

	configItemOpenElement.addAttribute("config:name", "VisibleAreaTop");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(mpHandler);
	mpHandler->characters("0");
	mpHandler->endElement("config:config-item");
	
	configItemOpenElement.addAttribute("config:name", "VisibleAreaLeft");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(mpHandler);
	mpHandler->characters("0");
	mpHandler->endElement("config:config-item");
	
	configItemOpenElement.addAttribute("config:name", "VisibleAreaWidth");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(mpHandler);
	WPXString sWidth; sWidth.sprintf("%li", (unsigned long)(2540 * width));
	mpHandler->characters(sWidth);
	mpHandler->endElement("config:config-item");
	
	configItemOpenElement.addAttribute("config:name", "VisibleAreaHeight");
	configItemOpenElement.addAttribute("config:type", "int");
	configItemOpenElement.write(mpHandler);
	WPXString sHeight; sHeight.sprintf("%li", (unsigned long)(2540 * height));
	mpHandler->characters(sHeight);
	mpHandler->endElement("config:config-item");
	
	mpHandler->endElement("config:config-item-set");
	
	mpHandler->endElement("office:settings");
}

void OdgExporter::endGraphics()
{
	TagOpenElement("office:styles").write(mpHandler);

	for (std::vector<DocumentElement *>::const_iterator iterGraphicsStrokeDashStyles = mGraphicsStrokeDashStyles.begin();
		iterGraphicsStrokeDashStyles != mGraphicsStrokeDashStyles.end(); iterGraphicsStrokeDashStyles++)
	{
		(*iterGraphicsStrokeDashStyles)->write(mpHandler);
	}

	for (std::vector<DocumentElement *>::const_iterator iterGraphicsGradientStyles = mGraphicsGradientStyles.begin();
		iterGraphicsGradientStyles != mGraphicsGradientStyles.end(); iterGraphicsGradientStyles++)
	{
		(*iterGraphicsGradientStyles)->write(mpHandler);
	}
	
	mpHandler->endElement("office:styles");
	
	TagOpenElement("office:automatic-styles").write(mpHandler);

	// writing out the graphics automatic styles
	for (std::vector<DocumentElement *>::iterator iterGraphicsAutomaticStyles = mGraphicsAutomaticStyles.begin();
		iterGraphicsAutomaticStyles != mGraphicsAutomaticStyles.end(); iterGraphicsAutomaticStyles++)
	{
		(*iterGraphicsAutomaticStyles)->write(mpHandler);
	}

	TagOpenElement tmpStylePageLayoutOpenElement("style:page-layout");
	tmpStylePageLayoutOpenElement.addAttribute("style:name", "PM0");
	tmpStylePageLayoutOpenElement.write(mpHandler);

	TagOpenElement tmpStylePageLayoutPropertiesOpenElement("style:page-layout-properties");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-top", "0in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-bottom", "0in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-left", "0in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:margin-right", "0in");
	WPXString sValue;
	sValue = doubleToString(mfWidth); sValue.append("in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:page-width", sValue);
	sValue = doubleToString(mfHeight); sValue.append("in");
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("fo:page-height", sValue);
	tmpStylePageLayoutPropertiesOpenElement.addAttribute("style:print-orientation", "portrait");
	tmpStylePageLayoutPropertiesOpenElement.write(mpHandler);

	mpHandler->endElement("style:page-layout-properties");

	mpHandler->endElement("style:page-layout");

	TagOpenElement tmpStyleStyleOpenElement("style:style");
	tmpStyleStyleOpenElement.addAttribute("style:name", "dp1");
	tmpStyleStyleOpenElement.addAttribute("style:family", "drawing-page");
	tmpStyleStyleOpenElement.write(mpHandler);

	TagOpenElement tmpStyleDrawingPagePropertiesOpenElement("style:drawing-page-properties");
	// tmpStyleDrawingPagePropertiesOpenElement.addAttribute("draw:background-size", "border");
	tmpStyleDrawingPagePropertiesOpenElement.addAttribute("draw:fill", "none");
	tmpStyleDrawingPagePropertiesOpenElement.write(mpHandler);

	mpHandler->endElement("style:drawing-page-properties");

	mpHandler->endElement("style:style");
	
	mpHandler->endElement("office:automatic-styles");


	TagOpenElement("office:master-styles").write(mpHandler);

	TagOpenElement tmpStyleMasterPageOpenElement("style:master-page");
	tmpStyleMasterPageOpenElement.addAttribute("style:name", "Default");
	tmpStyleMasterPageOpenElement.addAttribute("style:page-layout-name", "PM0");
	tmpStyleMasterPageOpenElement.addAttribute("draw:style-name", "dp1");
	tmpStyleMasterPageOpenElement.write(mpHandler);

	mpHandler->endElement("style:master-page");

	mpHandler->endElement("office:master-styles");

	TagOpenElement("office:body").write(mpHandler);

	TagOpenElement("office:drawing").write(mpHandler);

	TagOpenElement tmpDrawPageOpenElement("draw:page");
	tmpDrawPageOpenElement.addAttribute("draw:name", "page1");
	tmpDrawPageOpenElement.addAttribute("draw:style-name", "dp1");
	tmpDrawPageOpenElement.addAttribute("draw:master-page-name", "Default");
	tmpDrawPageOpenElement.write(mpHandler);

	for (std::vector<DocumentElement *>::const_iterator bodyIter = mBodyElements.begin();
		bodyIter != mBodyElements.end(); bodyIter++)
	{
		(*bodyIter)->write(mpHandler);
	}	

	mpHandler->endElement("draw:page");
	mpHandler->endElement("office:drawing");
	mpHandler->endElement("office:body");
	mpHandler->endElement("office:document");

	mpHandler->endDocument();
}

void OdgExporter::setPen(const libwpg::WPGPen& pen)
{
	mxPen = pen;
}

void OdgExporter::setBrush(const libwpg::WPGBrush& brush)
{
	mxBrush = brush;
}

void OdgExporter::setFillRule(FillRule rule)
{
	mxFillRule = rule;
}

void OdgExporter::startLayer(unsigned int /* id */)
{
}

void OdgExporter::endLayer(unsigned int)
{
}

void OdgExporter::drawRectangle(const libwpg::WPGRect& rect, double rx, double /* ry */)
{
	writeGraphicsStyle();
	TagOpenElement *pDrawRectElement = new TagOpenElement("draw:rect");
	WPXString sValue;
	sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
	pDrawRectElement->addAttribute("draw:style-name", sValue);
	sValue = doubleToString(rect.x1); sValue.append("in");
	pDrawRectElement->addAttribute("svg:x", sValue);
	sValue = doubleToString(rect.y1); sValue.append("in");
	pDrawRectElement->addAttribute("svg:y", sValue);
	sValue = doubleToString(rect.width()); sValue.append("in");
	pDrawRectElement->addAttribute("svg:width", sValue);
	sValue = doubleToString(rect.height()); sValue.append("in");
	pDrawRectElement->addAttribute("svg:height", sValue);
	sValue = doubleToString(rx); sValue.append("in");
	// FIXME: what to do when rx != ry ?
	pDrawRectElement->addAttribute("draw:corner-radius", sValue);
	mBodyElements.push_back(pDrawRectElement);
	mBodyElements.push_back(new TagCloseElement("draw:rect"));	
}

void OdgExporter::drawEllipse(const libwpg::WPGPoint& center, double rx, double ry, double rotation, const libwpg::WPGPoint& from, const libwpg::WPGPoint& to)
{
	writeGraphicsStyle();
	TagOpenElement *pDrawEllipseElement = new TagOpenElement("draw:ellipse");
	WPXString sValue;
	sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
	pDrawEllipseElement->addAttribute("draw:style-name", sValue);
	sValue = doubleToString(2 * rx); sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:width", sValue);
	sValue = doubleToString(2 * ry); sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:height", sValue);
	if (rotation != 0.0)
	{
		while(rotation < -180)
			rotation += 360;
		while(rotation > 180)
			rotation -= 360;
		double radrotation = rotation*M_PI/180.0;
		double deltax = sqrt(pow(rx, 2.0) + pow(ry, 2.0))*cos(atan(ry/rx) - radrotation ) - rx;
		double deltay = sqrt(pow(rx, 2.0) + pow(ry, 2.0))*sin(atan(ry/rx) - radrotation ) - ry;
		sValue = "rotate("; sValue.append(doubleToString(radrotation)); sValue.append(") ");
		sValue.append("translate("); sValue.append(doubleToString(center.x - rx - deltax)); sValue.append("in, "); sValue.append(doubleToString(center.y - ry - deltay)); sValue.append("in)");
		pDrawEllipseElement->addAttribute("svg:transform", sValue);
	}
	else
	{
		sValue = doubleToString(center.x-rx); sValue.append("in");
		pDrawEllipseElement->addAttribute("svg:x", sValue);
		sValue = doubleToString(center.y-ry); sValue.append("in");
		pDrawEllipseElement->addAttribute("svg:y", sValue);
	}
	mBodyElements.push_back(pDrawEllipseElement);
	mBodyElements.push_back(new TagCloseElement("draw:ellipse"));
}

void OdgExporter::drawPolyline(const libwpg::WPGPointArray& vertices)
{
	drawPolySomething(vertices, false);
}

void OdgExporter::drawPolygon(const libwpg::WPGPointArray& vertices)
{
	drawPolySomething(vertices, true);
}

void OdgExporter::drawPolySomething(const libwpg::WPGPointArray& vertices, bool isClosed)
{
	if(vertices.count() < 2)
		return;

	if(vertices.count() == 2)
	{
		const libwpg::WPGPoint& p1 = vertices[0];
		const libwpg::WPGPoint& p2 = vertices[1];

		writeGraphicsStyle();
		TagOpenElement *pDrawLineElement = new TagOpenElement("draw:line");
		WPXString sValue;
		sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
		pDrawLineElement->addAttribute("draw:style-name", sValue);
		pDrawLineElement->addAttribute("draw:text-style-name", "P1");
		pDrawLineElement->addAttribute("draw:layer", "layout");
		sValue = doubleToString(p1.x); sValue.append("in");
		pDrawLineElement->addAttribute("svg:x1", sValue);
		sValue = doubleToString(p1.y); sValue.append("in");
		pDrawLineElement->addAttribute("svg:y1", sValue);
		sValue = doubleToString(p2.x); sValue.append("in");
		pDrawLineElement->addAttribute("svg:x2", sValue);
		sValue = doubleToString(p2.y); sValue.append("in");
		pDrawLineElement->addAttribute("svg:y2", sValue);
		mBodyElements.push_back(pDrawLineElement);
		mBodyElements.push_back(new TagCloseElement("draw:line"));
	}
	else
	{
		// draw as path
		libwpg::WPGPath path;
		path.moveTo(vertices[0]);
		for(unsigned long ii = 1; ii < vertices.count(); ii++)
			path.lineTo(vertices[ii]);
		path.closed = isClosed;
		drawPath(path);
	}
}

void OdgExporter::drawPath(const libwpg::WPGPath& path)
{
	if(path.count() == 0)
		return;

	// try to find the bounding box
	// this is simple convex hull technique, the bounding box might not be
	// accurate but that should be enough for this purpose
	libwpg::WPGPoint p = path.element(0).point;
	libwpg::WPGPoint q = path.element(0).point;
	for(unsigned k = 0; k < path.count(); k++)
	{
		libwpg::WPGPathElement element = path.element(k);
		p.x = (p.x > element.point.x) ? element.point.x : p.x; 
		p.y = (p.y > element.point.y) ? element.point.y : p.y; 
		q.x = (q.x < element.point.x) ? element.point.x : q.x; 
		q.y = (q.y < element.point.y) ? element.point.y : q.y; 
		if(element.type == libwpg::WPGPathElement::CurveToElement)
		{
			p.x = (p.x > element.extra1.x) ? element.extra1.x : p.x; 
			p.y = (p.y > element.extra1.y) ? element.extra1.y : p.y; 
			q.x = (q.x < element.extra1.x) ? element.extra1.x : q.x; 
			q.y = (q.y < element.extra1.y) ? element.extra1.y : q.y; 
			p.x = (p.x > element.extra2.x) ? element.extra2.x : p.x; 
			p.y = (p.y > element.extra2.y) ? element.extra2.y : p.y; 
			q.x = (q.x < element.extra2.x) ? element.extra2.x : q.x; 
			q.y = (q.y < element.extra2.y) ? element.extra2.y : q.y; 
		}
	}
	double vw = q.x - p.x;
	double vh = q.y - p.y;
		
	writeGraphicsStyle();

	TagOpenElement *pDrawPathElement = new TagOpenElement("draw:path");
	WPXString sValue;
	sValue.sprintf("gr%i", miGraphicsStyleIndex-1);
	pDrawPathElement->addAttribute("draw:style-name", sValue);
	pDrawPathElement->addAttribute("draw:text-style-name", "P1");
	pDrawPathElement->addAttribute("draw:layer", "layout");
	sValue = doubleToString(p.x); sValue.append("in");
	pDrawPathElement->addAttribute("svg:x", sValue);
	sValue = doubleToString(p.y); sValue.append("in");
	pDrawPathElement->addAttribute("svg:y", sValue);
	sValue = doubleToString(vw); sValue.append("in");
	pDrawPathElement->addAttribute("svg:width", sValue);
	sValue = doubleToString(vh); sValue.append("in");
	pDrawPathElement->addAttribute("svg:height", sValue);
	sValue.sprintf("%i %i %i %i", 0, 0, (unsigned)(vw*2540), (unsigned)(vh*2540));
	pDrawPathElement->addAttribute("svg:viewBox", sValue);

    sValue.clear();
	for(unsigned i = 0; i < path.count(); i++)
	{
		libwpg::WPGPathElement element = path.element(i);
		libwpg::WPGPoint point = element.point;
        WPXString sElement;
		switch(element.type)
		{
			// 2540 is 2.54*1000, 2.54 in = 1 inch
			case libwpg::WPGPathElement::MoveToElement:
			    sElement.sprintf("M%i %i", (unsigned)((point.x-p.x)*2540), (unsigned)((point.y-p.y)*2540));
				break;
				
			case libwpg::WPGPathElement::LineToElement:
			    sElement.sprintf("L%i %i", (unsigned)((point.x-p.x)*2540), (unsigned)((point.y-p.y)*2540));
				break;
			
			case libwpg::WPGPathElement::CurveToElement:
                sElement.sprintf("C%i %i %i %i %i %i", (unsigned)((element.extra1.x-p.x)*2540),
                (int)((element.extra1.y-p.y)*2540), (unsigned)((element.extra2.x-p.x)*2540),
                (int)((element.extra2.y-p.y)*2540), (unsigned)((point.x-p.x)*2540), (unsigned)((point.y-p.y)*2540));
				break;
			
			default:
				break;
		}
		sValue.append(sElement);
	}
    if(path.closed)
		sValue.append(" Z");
	pDrawPathElement->addAttribute("svg:d", sValue);
	mBodyElements.push_back(pDrawPathElement);
	mBodyElements.push_back(new TagCloseElement("draw:path"));
}

void OdgExporter::drawBitmap(const libwpg::WPGBitmap& bitmap)
{
	TagOpenElement *pDrawFrameElement = new TagOpenElement("draw:frame");
	WPXString sValue;
	sValue = doubleToString(bitmap.rect.x1); sValue.append("in");
	pDrawFrameElement->addAttribute("svg:x", sValue);
	sValue = doubleToString(bitmap.rect.y1); sValue.append("in");
	pDrawFrameElement->addAttribute("svg:y", sValue);
	sValue = doubleToString(bitmap.rect.height()); sValue.append("in");
	pDrawFrameElement->addAttribute("svg:height", sValue);
	sValue = doubleToString(bitmap.rect.width()); sValue.append("in");
	pDrawFrameElement->addAttribute("svg:width", sValue);
	mBodyElements.push_back(pDrawFrameElement);
	
	mBodyElements.push_back(new TagOpenElement("draw:image"));
	
	mBodyElements.push_back(new TagOpenElement("office:binary-data"));
	
	::WPXString base64Binary;
	bitmap.generateBase64DIB(base64Binary);
	mBodyElements.push_back(new CharDataElement(base64Binary.cstr()));
	
	mBodyElements.push_back(new TagCloseElement("office:binary-data"));
	
	mBodyElements.push_back(new TagCloseElement("draw:image"));
	
	mBodyElements.push_back(new TagCloseElement("draw:frame"));
}

void OdgExporter::drawImageObject(const ::WPXPropertyList &propList, const ::WPXBinaryData& binaryData)
{
	if (!propList["libwpg:mime-type"] && propList["libwpg:mime-type"]->getStr().len() <= 0)
		return;
	TagOpenElement *pDrawFrameElement = new TagOpenElement("draw:frame");
	
	
	WPXString sValue;
	if (propList["svg:x"])
		pDrawFrameElement->addAttribute("svg:x", propList["svg:x"]->getStr());
	if (propList["svg:y"])
		pDrawFrameElement->addAttribute("svg:y", propList["svg:y"]->getStr());
	if (propList["svg:height"])
		pDrawFrameElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	if (propList["svg:width"])
		pDrawFrameElement->addAttribute("svg:width", propList["svg:width"]->getStr());
	mBodyElements.push_back(pDrawFrameElement);
	
	mBodyElements.push_back(new TagOpenElement("draw:image"));
	
	mBodyElements.push_back(new TagOpenElement("office:binary-data"));
	
	::WPXString base64Binary = binaryData.getBase64Data();
	mBodyElements.push_back(new CharDataElement(base64Binary.cstr()));
	
	mBodyElements.push_back(new TagCloseElement("office:binary-data"));
	
	mBodyElements.push_back(new TagCloseElement("draw:image"));
	
	mBodyElements.push_back(new TagCloseElement("draw:frame"));
}

void OdgExporter::writeGraphicsStyle()
{
	if(!mxPen.solid && (mxPen.dashArray.count() >=2 ) )
	{
		// ODG only supports dashes with the same length of spaces inbetween
		// here we take the first space and assume everything else the same
		// note that dash length is written in percentage ?????????????????
		double distance = mxPen.dashArray.at(1);
		TagOpenElement *pDrawStrokeDashElement = new TagOpenElement("draw:stroke-dash");
		pDrawStrokeDashElement->addAttribute("draw:style", "rect");
		WPXString sValue;
		sValue.sprintf("Dash_%i", miDashIndex++);
		pDrawStrokeDashElement->addAttribute("draw:name", sValue);
		sValue = doubleToString(distance); sValue.append("in");
		pDrawStrokeDashElement->addAttribute("draw:distance", sValue);
		WPXString sName;
		// We have to find out how to do this intelligently, since the ODF is allowing only
		// two pairs draw:dots1 draw:dots1-length and draw:dots2 draw:dots2-length
		for(unsigned i = 0; i < mxPen.dashArray.count()/2 && i < 2; i++)
		{
			sName.sprintf("draw:dots%i", i+1);
			pDrawStrokeDashElement->addAttribute(sName.cstr(), "1");
			sName.sprintf("draw:dots%i-length", i+1);
			sValue = doubleToString(mxPen.dashArray.at(i*2)); sValue.append("in");
			pDrawStrokeDashElement->addAttribute(sName.cstr(), sValue);
		}
		mGraphicsStrokeDashStyles.push_back(pDrawStrokeDashElement);
		mGraphicsStrokeDashStyles.push_back(new TagCloseElement("draw:stroke-dash"));
	}

	if(mxBrush.style == libwpg::WPGBrush::Gradient)
	{
		TagOpenElement *pDrawGradientElement = new TagOpenElement("draw:gradient");
		pDrawGradientElement->addAttribute("draw:style", "linear");
		WPXString sValue;
		sValue.sprintf("Gradient_%i", miGradientIndex++);
		pDrawGradientElement->addAttribute("draw:name", sValue);

		// ODG angle unit is 0.1 degree
		double angle = -mxBrush.gradient.angle();
		while(angle < 0)
			angle += 360;
		while(angle > 360)
			angle -= 360;

		sValue.sprintf("%i", (unsigned)(angle*10));
		pDrawGradientElement->addAttribute("draw:angle", sValue);

		libwpg::WPGColor startColor = mxBrush.gradient.stopColor(0);
		libwpg::WPGColor stopColor = mxBrush.gradient.stopColor(1);
		sValue.sprintf("#%.2x%.2x%.2x", (startColor.red & 0xff), (startColor.green & 0xff), (startColor.blue & 0xff));
		pDrawGradientElement->addAttribute("draw:start-color", sValue);
		sValue.sprintf("#%.2x%.2x%.2x", (stopColor.red & 0xff), (stopColor.green & 0xff), (stopColor.blue & 0xff));
		pDrawGradientElement->addAttribute("draw:end-color", sValue);
		pDrawGradientElement->addAttribute("draw:start-intensity", "100%");
		pDrawGradientElement->addAttribute("draw:end-intensity", "100%");
		pDrawGradientElement->addAttribute("draw:border", "0%");
		mGraphicsGradientStyles.push_back(pDrawGradientElement);
		mGraphicsGradientStyles.push_back(new TagCloseElement("draw:gradient"));
	}

	TagOpenElement *pStyleStyleElement = new TagOpenElement("style:style");
	WPXString sValue;
	sValue.sprintf("gr%i",  miGraphicsStyleIndex);
	pStyleStyleElement->addAttribute("style:name", sValue);
	pStyleStyleElement->addAttribute("style:family", "graphic");
	pStyleStyleElement->addAttribute("style:parent-style-name", "standard");
	mGraphicsAutomaticStyles.push_back(pStyleStyleElement);

	TagOpenElement *pStyleGraphicsPropertiesElement = new TagOpenElement("style:graphic-properties");

	if(mxPen.width > 0.0)
	{
		sValue = doubleToString(mxPen.width); sValue.append("in");
		pStyleGraphicsPropertiesElement->addAttribute("svg:stroke-width", sValue);

		sValue.sprintf("#%.2x%.2x%.2x", (mxPen.foreColor.red & 0xff),
			(mxPen.foreColor.green & 0xff), (mxPen.foreColor.blue & 0xff));
		pStyleGraphicsPropertiesElement->addAttribute("svg:stroke-color", sValue);

		if(!mxPen.solid)
		{
			pStyleGraphicsPropertiesElement->addAttribute("draw:stroke", "dash");
			sValue.sprintf("Dash_%i", miDashIndex-1);
			pStyleGraphicsPropertiesElement->addAttribute("draw:stroke-dash", sValue);
		}
	}
	else
		pStyleGraphicsPropertiesElement->addAttribute("draw:stroke", "none");

	if(mxBrush.style == libwpg::WPGBrush::NoBrush)
		pStyleGraphicsPropertiesElement->addAttribute("draw:fill", "none");

	if(mxBrush.style == libwpg::WPGBrush::Solid)
	{
		pStyleGraphicsPropertiesElement->addAttribute("draw:fill", "solid");
		sValue.sprintf("#%.2x%.2x%.2x", (mxBrush.foreColor.red & 0xff),
			(mxBrush.foreColor.green & 0xff), (mxBrush.foreColor.blue & 0xff));
		pStyleGraphicsPropertiesElement->addAttribute("draw:fill-color", sValue);
	}

	if(mxBrush.style == libwpg::WPGBrush::Gradient)
	{
		pStyleGraphicsPropertiesElement->addAttribute("draw:fill", "gradient");
		sValue.sprintf("Gradient_%i", miGradientIndex-1);
		pStyleGraphicsPropertiesElement->addAttribute("draw:fill-gradient-name", sValue);
	}

	mGraphicsAutomaticStyles.push_back(pStyleGraphicsPropertiesElement);
	mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mGraphicsAutomaticStyles.push_back(new TagCloseElement("style:style"));
	miGraphicsStyleIndex++;
}

WPXString OdgExporter::doubleToString(const double value)
{
	WPXString tempString;
	tempString.sprintf("%.4f", value);
	std::string decimalPoint(localeconv()->decimal_point);
	if ((decimalPoint.size() == 0) || (decimalPoint == "."))
		return tempString;
	std::string stringValue(tempString.cstr());
	if (!stringValue.empty())
	{
		std::string::size_type pos;
		while ((pos = stringValue.find(decimalPoint)) != std::string::npos)
			stringValue.replace(pos,decimalPoint.size(),".");
	}
	return WPXString(stringValue.c_str());
}
