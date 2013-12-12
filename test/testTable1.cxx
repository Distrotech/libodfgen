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
static void sendTableContent(Generator &generator)
{
	librevenge::RVNGPropertyList span;
	span.insert("style:font-name","Courier");
	span.insert("fo:font-size", 13, librevenge::RVNG_POINT);
	span.insert("librevenge:span-id",1);
	generator.defineCharacterStyle(span);
	span.clear();
	span.insert("librevenge:span-id",1);

	librevenge::RVNGPropertyList para;
	para.insert("librevenge:paragraph-id",1);
	para.insert("fo:margin-left",0.1,librevenge::RVNG_INCH);
	para.insert("fo:text-align","left");
	generator.defineParagraphStyle(para);
	para.clear();
	para.insert("librevenge:paragraph-id",1);

	librevenge::RVNGPropertyList row, cell;
	row.insert("librevenge:is-header-row", false);
	cell.insert("table:number-rows-spanned", 1);
	cell.insert("table:number-columns-spanned", 1);

	row.insert("style:row-height", 1, librevenge::RVNG_INCH);
	generator.openTableRow(row);
	cell.insert("librevenge:row", 1);
	cell.insert("librevenge:column", 1);
	cell.insert("fo:border", "1pt dashed #00ff00");
	generator.openTableCell(cell);
	generator.closeTableCell();
	cell.remove("fo:border");

	cell.insert("librevenge:column", 2);
	generator.openTableCell(cell);
	generator.openParagraph(para);
	generator.openSpan(span);
	generator.insertText("B1");
	generator.closeSpan();
	generator.closeParagraph();
	generator.closeTableCell();

	cell.insert("librevenge:column", 3);
	cell.insert("fo:background-color", "#ff0000");
	generator.openTableCell(cell);
	generator.closeTableCell();
	cell.remove("fo:background-color");
	generator.closeTableRow();

	row.insert("style:row-height", 0.3, librevenge::RVNG_INCH);
	generator.openTableRow(row);
	cell.insert("librevenge:row", 2);

	cell.insert("librevenge:column", 1);
	cell.insert("table:number-rows-spanned", 2);
	cell.insert("table:number-columns-spanned", 2);
	generator.openTableCell(cell);
	generator.openParagraph(para);
	generator.openSpan(span);
	generator.insertText("2 rows 2 col");
	generator.closeSpan();
	generator.closeParagraph();
	generator.closeTableCell();
	generator.insertCoveredTableCell(librevenge::RVNGPropertyList());
	cell.insert("librevenge:column", 3);
	cell.insert("table:number-rows-spanned", 2);
	cell.insert("table:number-columns-spanned", 1);
	generator.openTableCell(cell);
	generator.openParagraph(para);
	generator.openSpan(span);
	generator.insertText("2 rows");
	generator.closeSpan();
	generator.closeParagraph();
	generator.closeTableCell();
	generator.closeTableRow();

	row.insert("style:row-height", 1, librevenge::RVNG_INCH);
	generator.openTableRow(row);
	generator.insertCoveredTableCell(librevenge::RVNGPropertyList());
	generator.insertCoveredTableCell(librevenge::RVNGPropertyList());
	generator.insertCoveredTableCell(librevenge::RVNGPropertyList());
	generator.closeTableRow();

	// copy of table 1
	row.insert("style:row-height", 1, librevenge::RVNG_INCH);
	generator.openTableRow(row);
	cell.insert("librevenge:row", 4);
	cell.insert("table:number-rows-spanned", 1);
	cell.insert("librevenge:column", 1);
	cell.insert("fo:border", "1pt dashed #00ff00");
	generator.openTableCell(cell);
	generator.closeTableCell();
	cell.remove("fo:border");

	cell.insert("librevenge:column", 2);
	generator.openTableCell(cell);
	generator.openParagraph(para);
	generator.openSpan(span);
	generator.insertText("B1");
	generator.closeSpan();
	generator.closeParagraph();
	generator.closeTableCell();

	cell.insert("librevenge:column", 3);
	cell.insert("fo:background-color", "#ff0000");
	generator.openTableCell(cell);
	generator.closeTableCell();
	cell.remove("fo:background-color");
	generator.closeTableRow();
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

	librevenge::RVNGPropertyList textbox;
	textbox.insert("svg:x",0.2, librevenge::RVNG_INCH);
	textbox.insert("svg:y",0.2, librevenge::RVNG_INCH);
	textbox.insert("svg:width",200, librevenge::RVNG_POINT);
	textbox.insert("fo:min-height",100, librevenge::RVNG_POINT);
	textbox.insert("text:anchor-type", "page");
	textbox.insert("text:anchor-page-number", 1);
	textbox.insert("style:vertical-rel", "page");
	textbox.insert("style:horizontal-rel", "page");
	textbox.insert("style:horizontal-pos", "from-left");
	textbox.insert("style:vertical-pos", "from-top");

	textbox.insert("draw:stroke", "none");
	textbox.insert("draw:fill", "none");
	generator.openFrame(textbox);
	generator.openTextBox(librevenge::RVNGPropertyList());

	librevenge::RVNGPropertyList table;
	librevenge::RVNGPropertyListVector columns;
	for (size_t c = 0; c < 3; ++c)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", 2, librevenge::RVNG_INCH);
		columns.append(column);
	}
	table.insert("style:width", 6, librevenge::RVNG_INCH);
	table.insert("librevenge:table-columns", columns);
	generator.openTable(table);
	sendTableContent(generator);
	generator.closeTable();

	generator.closeTextBox();
	generator.closeFrame();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testTable1.odt");
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

	librevenge::RVNGPropertyList textbox;
	textbox.insert("svg:x",0.2, librevenge::RVNG_INCH);
	textbox.insert("svg:y",0.2, librevenge::RVNG_INCH);
	textbox.insert("svg:width",6, librevenge::RVNG_INCH);
	textbox.insert("svg:height",180, librevenge::RVNG_POINT);
	textbox.insert("text:anchor-type", "page");
	textbox.insert("text:anchor-page-number", 1);
	textbox.insert("style:vertical-rel", "page");
	textbox.insert("style:horizontal-rel", "page");
	textbox.insert("style:horizontal-pos", "from-left");
	textbox.insert("style:vertical-pos", "from-top");

	textbox.insert("draw:stroke", "none");
	textbox.insert("draw:fill", "none");
	generator.openFrame(textbox);
	librevenge::RVNGPropertyList table;
	columns.clear();
	for (size_t c = 0; c < 3; ++c)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", 2, librevenge::RVNG_INCH);
		columns.append(column);
	}
	table.insert("style:width", 6, librevenge::RVNG_INCH);
	table.insert("librevenge:table-columns", columns);
	generator.openTable(table);
	sendTableContent(generator);
	generator.closeTable();
	generator.closeFrame();

	list.clear();
	list.insert("style:row-height", 40, librevenge::RVNG_POINT);
	generator.openSheetRow(list);
	generator.closeSheetRow();
	generator.closeSheet();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testTable1.ods");
	file << content.cstr();
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

	librevenge::RVNGPropertyList table;
	table.insert("svg:x",2, librevenge::RVNG_INCH);
	table.insert("svg:y",2, librevenge::RVNG_INCH);
	table.insert("svg:width",200, librevenge::RVNG_POINT);
	table.insert("fo:min-height",100, librevenge::RVNG_POINT);
	table.insert("draw:stroke", "none");
	table.insert("draw:fill", "none");

	librevenge::RVNGPropertyListVector columns;
	for (size_t c = 0; c < 3; ++c)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", 2, librevenge::RVNG_INCH);
		columns.append(column);
	}
	table.insert("librevenge:table-columns", columns);

	generator.startTableObject(table);
	sendTableContent(generator);
	generator.endTableObject();

	generator.endPage();
	generator.endDocument();

	std::ofstream file("testTable1.odg");
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

	librevenge::RVNGPropertyList table;
	table.insert("svg:x",0.2, librevenge::RVNG_INCH);
	table.insert("svg:y",0.02, librevenge::RVNG_INCH);
	table.insert("svg:width",200, librevenge::RVNG_POINT);
	table.insert("fo:min-height",100, librevenge::RVNG_POINT);
	table.insert("draw:stroke", "none");
	table.insert("draw:fill", "none");

	librevenge::RVNGPropertyListVector columns;
	for (size_t c = 0; c < 3; ++c)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", 2, librevenge::RVNG_INCH);
		columns.append(column);
	}
	table.insert("librevenge:table-columns", columns);

	generator.startTableObject(table);
	sendTableContent(generator);
	generator.endTableObject();

	generator.endSlide();
	generator.endDocument();

	std::ofstream file("testTable1.odp");
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

