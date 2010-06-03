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

enum OdgStreamType { ODG_FLAT_XML, ODG_CONTENT_XML, ODG_STYLES_XML, ODG_SETTINGS_XML, ODG_META_XML };

class OdgExporter : public libwpg::WPGPaintInterface {
public:
	OdgExporter(DocumentHandler *pHandler, const OdgStreamType streamType);
	~OdgExporter();

	void startGraphics(const ::WPXPropertyList &propList);
	void endGraphics();
	void startLayer(const ::WPXPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const ::WPXPropertyList& /*propList*/) {}
	void endEmbeddedGraphics() {}

	void setStyle(const ::WPXPropertyListVector& gradient, const ::WPXPropertyList &propList);

	void drawRectangle(const ::WPXPropertyList &propList);
	void drawEllipse(const ::WPXPropertyList &propList);
	void drawPolyline(const ::WPXPropertyListVector& vertices);
	void drawPolygon(const ::WPXPropertyListVector& vertices);
	void drawPath(const ::WPXPropertyListVector& path);
	void drawImageObject(const ::WPXPropertyList &propList, const ::WPXBinaryData& binaryData);

private:
	void writeGraphicsStyle();
	WPXString doubleToString(const double value);
	void drawPolySomething(const ::WPXPropertyListVector& vertices, bool isClosed);
	
	// body elements
	std::vector <DocumentElement *> mBodyElements;

	// graphics styles
	std::vector<DocumentElement *> mGraphicsStrokeDashStyles;
	std::vector<DocumentElement *> mGraphicsGradientStyles;
	std::vector<DocumentElement *> mGraphicsAutomaticStyles;

	DocumentHandler *mpHandler;

	::WPXPropertyList mxStyle;
	libwpg::WPGDashArray mxDashArray;
	::WPXPropertyListVector mxGradient;
	int miGradientIndex;
	int miDashIndex;
	int miGraphicsStyleIndex;
	double mfWidth;
	double mfHeight;

	const OdgStreamType mxStreamType;
};

#endif // __ODGEXPORTER_HXX__
