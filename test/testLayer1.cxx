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

#include <iostream>
#include <fstream>

#include <librevenge/librevenge.h>
#include <libodfgen/libodfgen.hxx>

#include "StringDocumentHandler.hxx"

// must draw 3 red shapes in layer Red, 2 shapes in layer Blue and 1 black shape with not layer
template <class Generator>
static void sendGraphic(Generator &generator, void (Generator::*SetStyle)(const librevenge::RVNGPropertyList &), double xDiff=0, double yDiff=0)
{
	// the Red layer
	librevenge::RVNGPropertyList style1;
	style1.insert("draw:stroke", "solid");
	style1.insert("svg:stroke-color", "#FF0000");
	style1.insert("svg:stroke-width", 3, librevenge::RVNG_POINT);
	style1.insert("draw:fill", "none");
	(generator.*SetStyle)(style1);

	librevenge::RVNGPropertyList layer;
	layer.insert("draw:layer", "Red");
	generator.startLayer(layer);

	librevenge::RVNGPropertyList shape, point;
	librevenge::RVNGPropertyListVector vertices;
	point.insert("svg:x", 20+xDiff, librevenge::RVNG_POINT);
	point.insert("svg:y", 40+yDiff, librevenge::RVNG_POINT);
	vertices.append(point);
	point.insert("svg:x", 200+xDiff, librevenge::RVNG_POINT);
	point.insert("svg:y", 40+yDiff, librevenge::RVNG_POINT);
	vertices.append(point);
	shape.insert("svg:points", vertices);
	generator.drawPolyline(shape);

	librevenge::RVNGPropertyList style2;
	style2.clear();
	style2.insert("draw:stroke", "dash");
	style2.insert("draw:dots1", 1);
	style2.insert("draw:dots1-length", librevenge::RVNG_POINT);
	style2.insert("svg:stroke-color", "#00FF00");
	style2.insert("svg:stroke-width", 3, librevenge::RVNG_POINT);
	style2.insert("draw:fill", "solid");
	style2.insert("draw:fill-color", "#FF0000");
	(generator.*SetStyle)(style2);

	shape.clear();
	shape.insert("svg:x", 70+xDiff, librevenge::RVNG_POINT);
	shape.insert("svg:y", 80+yDiff, librevenge::RVNG_POINT);
	shape.insert("svg:width", 100, librevenge::RVNG_POINT);
	shape.insert("svg:height", 30, librevenge::RVNG_POINT);
	generator.drawRectangle(shape);
	generator.endLayer();

	//
	// end of the Red layer
	//

	// shape outside any layer

	style1.insert("svg:stroke-color", "#000000");
	(generator.*SetStyle)(style1);
	shape.clear();
	vertices.clear();
	point.insert("svg:x", 20+xDiff, librevenge::RVNG_POINT);
	point.insert("svg:y", 100+yDiff, librevenge::RVNG_POINT);
	vertices.append(point);
	point.insert("svg:x", 200+xDiff, librevenge::RVNG_POINT);
	point.insert("svg:y", 100+yDiff, librevenge::RVNG_POINT);
	vertices.append(point);
	shape.insert("svg:points", vertices);
	generator.drawPolyline(shape);

	//
	// the Blue layer
	//
	layer.insert("draw:layer", "Blue");
	generator.startLayer(layer);

	librevenge::RVNGPropertyList style(style1);
	style.insert("draw:marker-start-path", "m10 0-10 30h20z");
	style.insert("draw:marker-start-viewbox", "0 0 20 30");
	style.insert("draw:marker-start-center", "false");
	style.insert("draw:marker-start-width", "5pt");
	style.insert("svg:stroke-color", "#0000FF");
	(generator.*SetStyle)(style);

	shape.clear();
	vertices.clear();
	point.insert("svg:x", 20+xDiff, librevenge::RVNG_POINT);
	point.insert("svg:y", 200+yDiff, librevenge::RVNG_POINT);
	vertices.append(point);
	point.insert("svg:x", 200+xDiff, librevenge::RVNG_POINT);
	point.insert("svg:y", 200+yDiff, librevenge::RVNG_POINT);
	vertices.append(point);
	shape.insert("svg:points", vertices);
	generator.drawPolyline(shape);

	style=style2;
	style.insert("draw:fill", "gradient");
	style.insert("draw:style", "axial");
	style.insert("draw:start-color", "#FF0000");
	style.insert("draw:start-opacity", 1., librevenge::RVNG_PERCENT);
	style.insert("draw:end-color", "#700070");
	style.insert("draw:end-opacity", 1., librevenge::RVNG_PERCENT);
	(generator.*SetStyle)(style);

	// we want this one to be in red layer, so we insert "draw:layer"="Red"
	shape.clear();
	shape.insert("svg:cx", 3+xDiff/72., librevenge::RVNG_INCH);
	shape.insert("svg:cy", 1.2+yDiff/72., librevenge::RVNG_INCH);
	shape.insert("svg:rx", 0.8, librevenge::RVNG_INCH);
	shape.insert("svg:ry", 0.4, librevenge::RVNG_INCH);
	shape.insert("draw:layer", "Red");
	generator.drawEllipse(shape);

	style=style2;
	style.insert("draw:fill", "gradient");
	style.insert("draw:style", "axial");
	style.insert("draw:start-color", "#0000FF");
	style.insert("draw:start-opacity", 1., librevenge::RVNG_PERCENT);
	style.insert("draw:end-color", "#700070");
	style.insert("draw:end-opacity", 1., librevenge::RVNG_PERCENT);
	(generator.*SetStyle)(style);

	// this shape is inserted on a not-existing layer: "Green", so it will appear in the current layer "Blue"
	shape.remove("draw:layer");
	style.insert("draw:stroke", "solid");
	style.insert("draw:shadow", "visible");
	style.insert("draw:shadow-color", "#000020");
	style.insert("draw:shadow-opacity", 1, librevenge::RVNG_PERCENT);
	style.insert("draw:shadow-offset-x", "30");
	style.insert("draw:shadow-offset-y", "30");
	style.insert("draw:angle", 30);
	shape.insert("draw:layer", "Green");
	(generator.*SetStyle)(style);
	shape.insert("svg:cy", 1.8+yDiff/72., librevenge::RVNG_INCH);
	shape.insert("librevenge:rotate", 30);
	generator.drawEllipse(shape);

	generator.endLayer();

	//
	// end of the Blue layer
	//
}

static void createOdg()
{
	StringDocumentHandler content;
	OdgGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument(librevenge::RVNGPropertyList());
	librevenge::RVNGPropertyList page;
	page.insert("draw:name", "Basic");
	page.insert("svg:width", 9, librevenge::RVNG_INCH);
	page.insert("svg:height", 11, librevenge::RVNG_INCH);
	page.insert("librevenge:enforce-frame",true);
	generator.startPage(page);

	sendGraphic(generator, &OdgGenerator::setStyle);

	generator.endPage();

	/*
	  now let's try what happens if we reuse the same layer names:

	  Red will become Red#0, Blue will become Blue#0
	*/
	page.insert("draw:name", "Reusing same layer names is bad");
	generator.startPage(page);
	sendGraphic(generator, &OdgGenerator::setStyle);
	generator.endPage();

	generator.endDocument();

	std::ofstream file("testLayer1.odg");
	file << content.cstr();
}

int main()
{
	createOdg();
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

