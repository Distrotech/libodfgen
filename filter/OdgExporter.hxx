/* libwpg
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
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

#ifndef __ODGEXPORTER_HXX__
#define __ODGEXPORTER_HXX__

#include <iostream>
#include <sstream>
#include <string>

#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>
#include "DocumentElement.hxx"
#include "DocumentHandler.hxx"

class OdgExporter : public libwpg::WPGPaintInterface {
public:
	OdgExporter(DocumentHandler *pHandler, const bool isFlatXML = false);
	~OdgExporter();

	void startGraphics(double width, double height);
	void endGraphics();
	void startLayer(unsigned int id);
	void endLayer(unsigned int id);
	void startEmbeddedGraphics(double /* width */, double /* height */) {}
	void endEmbeddedGraphics() {}

	void setPen(const libwpg::WPGPen& pen);
	void setBrush(const libwpg::WPGBrush& brush);
	void setFillRule(FillRule rule);

	void drawRectangle(const libwpg::WPGRect& rect, double rx, double ry);
	void drawEllipse(const libwpg::WPGPoint& center, double rx, double ry, double rotation, const libwpg::WPGPoint& from, const libwpg::WPGPoint& to);
	void drawPolyline(const libwpg::WPGPointArray& vertices);
	void drawPolygon(const libwpg::WPGPointArray& vertices);
	void drawPath(const libwpg::WPGPath& path);
	void drawBitmap(const libwpg::WPGBitmap& bitmap);
	void drawImageObject(const ::WPXPropertyList &propList, const ::WPXBinaryData& binaryData);

private:
	void writeGraphicsStyle();
	WPXString doubleToString(const double value);
	void drawPolySomething(const libwpg::WPGPointArray& vertices, bool isClosed);
	
	// body elements
	std::vector <DocumentElement *> mBodyElements;

	// graphics styles
	std::vector<DocumentElement *> mGraphicsStrokeDashStyles;
	std::vector<DocumentElement *> mGraphicsGradientStyles;
	std::vector<DocumentElement *> mGraphicsAutomaticStyles;

	DocumentHandler *mpHandler;

	libwpg::WPGPen mxPen;
	libwpg::WPGBrush mxBrush;
	FillRule mxFillRule;
	int miGradientIndex;
	int miDashIndex;
	int miGraphicsStyleIndex;
	double mfWidth;
	double mfHeight;

	const bool mbIsFlatXML;
};

#endif // __ODGEXPORTER_HXX__
