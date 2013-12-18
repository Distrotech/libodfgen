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

template <class Generator>
static void sendGraphic(Generator &generator, void (Generator::*SetStyle)(const librevenge::RVNGPropertyList &), double xDiff=0, double yDiff=0)
{
	librevenge::RVNGPropertyList style1;
	style1.insert("draw:stroke", "solid");
	style1.insert("svg:stroke-color", "#FF0000");
	style1.insert("svg:stroke-width", 3, librevenge::RVNG_POINT);
	style1.insert("draw:fill", "none");
	(generator.*SetStyle)(style1);

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
	style2.insert("draw:fill-color", "#0000FF");
	(generator.*SetStyle)(style2);

	shape.clear();
	shape.insert("svg:x", 70+xDiff, librevenge::RVNG_POINT);
	shape.insert("svg:y", 80+yDiff, librevenge::RVNG_POINT);
	shape.insert("svg:width", 100, librevenge::RVNG_POINT);
	shape.insert("svg:height", 30, librevenge::RVNG_POINT);
	generator.drawRectangle(shape);

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

	librevenge::RVNGPropertyList style(style1);
	style.insert("draw:marker-start-path", "m10 0-10 30h20z");
	style.insert("draw:marker-start-viewbox", "0 0 20 30");
	style.insert("draw:marker-start-center", "false");
	style.insert("draw:marker-start-width", "5pt");
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
	style.insert("draw:start-color", "#FF00FF");
	style.insert("draw:start-opacity", 1., librevenge::RVNG_PERCENT);
	style.insert("draw:end-color", "#700070");
	style.insert("draw:end-opacity", 1., librevenge::RVNG_PERCENT);
	(generator.*SetStyle)(style);

	// point does not works for ellipse so inch
	shape.clear();
	shape.insert("svg:cx", 3+xDiff/72., librevenge::RVNG_INCH);
	shape.insert("svg:cy", 1.2+yDiff/72., librevenge::RVNG_INCH);
	shape.insert("svg:rx", 0.8, librevenge::RVNG_INCH);
	shape.insert("svg:ry", 0.4, librevenge::RVNG_INCH);
	generator.drawEllipse(shape);

	style.insert("draw:stroke", "solid");
	style.insert("draw:shadow", "visible");
	style.insert("draw:shadow-color", "#000020");
	style.insert("draw:shadow-opacity", 1, librevenge::RVNG_PERCENT);
	style.insert("draw:shadow-offset-x", "30");
	style.insert("draw:shadow-offset-y", "30");
	style.insert("draw:angle", 30);
	(generator.*SetStyle)(style);
	shape.insert("svg:cy", 1.8+yDiff/72., librevenge::RVNG_INCH);
	shape.insert("librevenge:rotate", 30);
	generator.drawEllipse(shape);
}

static void createOdg()
{
	StringDocumentHandler content;
	OdgGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument(librevenge::RVNGPropertyList());
	librevenge::RVNGPropertyList page;
	page.insert("svg:x",0, librevenge::RVNG_POINT);
	page.insert("svg:y",0, librevenge::RVNG_POINT);
	page.insert("svg:width", 9, librevenge::RVNG_INCH);
	page.insert("svg:height", 11, librevenge::RVNG_INCH);
	page.insert("librevenge:enforce-frame",true);
	generator.startPage(page);
	sendGraphic(generator, &OdgGenerator::setStyle);
	generator.endPage();
	generator.endDocument();

	std::ofstream file("testGraphic1.odg");
	file << content.cstr();
}

static void createOdp()
{
	StringDocumentHandler content;
	OdpGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument(librevenge::RVNGPropertyList());
	librevenge::RVNGPropertyList page;
	page.insert("svg:x",0, librevenge::RVNG_POINT);
	page.insert("svg:y",0, librevenge::RVNG_POINT);
	page.insert("svg:width", 9, librevenge::RVNG_INCH);
	page.insert("svg:height", 11, librevenge::RVNG_INCH);
	generator.startSlide(page);
	sendGraphic(generator,  &OdpGenerator::setStyle);
	generator.endSlide();
	generator.endDocument();

	std::ofstream file("testGraphic1.odp");
	file << content.cstr();
}

static void createOds()
{
	StringDocumentHandler content;
	OdsGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument(librevenge::RVNGPropertyList());
	librevenge::RVNGPropertyList page;
	page.insert("librevenge:num-pages", 1);
	page.insert("fo:page-height", 11.5, librevenge::RVNG_INCH);
	page.insert("fo:page-width", 9, librevenge::RVNG_INCH);
	page.insert("style:print-orientation", "portrait");
	page.insert("fo:margin-left", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-right", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-top", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-bottom", 0.1, librevenge::RVNG_INCH);
	generator.openPageSpan(page);

	librevenge::RVNGPropertyList list;
	librevenge::RVNGPropertyListVector columns;
	for (size_t c = 0; c < 2; c++)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", 3, librevenge::RVNG_INCH);
		columns.append(column);
	}
	list.insert("librevenge:columns", columns);
	generator.openSheet(list);

	generator.openGroup(librevenge::RVNGPropertyList());
	sendGraphic(generator, &OdsGenerator::defineGraphicStyle);
	generator.closeGroup();

	list.clear();
	list.insert("style:row-height", 40, librevenge::RVNG_POINT);
	generator.openSheetRow(list);
	generator.closeSheetRow();
	generator.closeSheet();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testGraphic1.ods");
	file << content.cstr();
}

static void createOdt()
{
	StringDocumentHandler content;
	OdtGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument(librevenge::RVNGPropertyList());
	librevenge::RVNGPropertyList page;
	page.insert("librevenge:num-pages", 1);
	page.insert("fo:page-height", 11.5, librevenge::RVNG_INCH);
	page.insert("fo:page-width", 9, librevenge::RVNG_INCH);
	page.insert("style:print-orientation", "portrait");
	page.insert("fo:margin-left", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-right", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-top", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-bottom", 0.1, librevenge::RVNG_INCH);
	generator.openPageSpan(page);

	{
		// anchor on page
		librevenge::RVNGPropertyList group;
		group.insert("text:anchor-type", "page");
		group.insert("text:anchor-page-number", "1");
		group.insert("style:vertical-rel", "page");
		group.insert("style:horizontal-rel", "page");
		group.insert("style:horizontal-pos", "from-left");
		group.insert("style:vertical-pos", "from-top");
		group.insert("style:wrap", "run-through");
		group.insert("style:run-through", "background");
		generator.openGroup(group);
		sendGraphic(generator, &OdtGenerator::defineGraphicStyle, 100, 200);
		generator.closeGroup();
	}
	librevenge::RVNGPropertyList para, span;
	generator.openParagraph(para);
	generator.openSpan(span);
	{
		// a basic char
		librevenge::RVNGPropertyList group;
		group.insert("svg:width",400, librevenge::RVNG_POINT);
		group.insert("fo:min-height",300, librevenge::RVNG_POINT);
		group.insert("text:anchor-type", "char");
		group.insert("style:vertical-rel", "char");
		group.insert("style:horizontal-rel", "char");
		generator.openGroup(group);
		sendGraphic(generator, &OdtGenerator::defineGraphicStyle);
		generator.closeGroup();
	}
	generator.closeSpan();
	generator.closeParagraph();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testGraphic1.odt");
	file << content.cstr();
}

int main()
{
	createOdg();
	createOdp();
	createOds();
	createOdt();
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

