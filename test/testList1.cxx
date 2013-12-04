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
static void sendText(Generator &generator)
{
	librevenge::RVNGPropertyList span;
	span.insert("style:font-name","Geneva");
	span.insert("fo:font-size", 12, librevenge::RVNG_POINT);
	span.insert("librevenge:span-id",1);
	generator.defineCharacterStyle(span);
	span.clear();
	span.insert("librevenge:span-id",1);

	// begin by a basic paragraph
	librevenge::RVNGPropertyList para;
	para.insert("librevenge:paragraph-id",1);
	generator.defineParagraphStyle(span);

	para.clear();
	para.insert("librevenge:paragraph-id",1);
	generator.openParagraph(para);
	generator.openSpan(span);
	generator.insertText("basic ");
	generator.closeSpan();
	generator.closeParagraph();

	librevenge::RVNGPropertyList list;
	// first test list using librevenge:id
	list.clear();
	list.insert("librevenge:id",1);
	list.insert("librevenge:level",1);
	list.insert("text:min-label-width", 0.2, librevenge::RVNG_INCH);
	list.insert("text:space-before", 0.1, librevenge::RVNG_INCH);
	list.insert("style:num-format", "1");
	generator.defineOrderedListLevel(list);
	list.insert("librevenge:level",2);
	list.insert("style:num-format", "I");
	generator.defineOrderedListLevel(list);

	list.clear();
	list.insert("librevenge:id",1);
	generator.openOrderedListLevel(list);

	list.insert("fo:margin-left",0.2,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 1");
	generator.closeSpan();
	generator.closeListElement();

	list.clear();
	list.insert("librevenge:id",2);
	generator.openOrderedListLevel(list);
	list.insert("fo:margin-left",0.5,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 2");
	generator.closeSpan();
	generator.closeListElement();
	generator.closeOrderedListLevel();
	generator.closeOrderedListLevel();

	// now redefine level 2 and try again
	list.clear();
	list.insert("librevenge:id",2);
	list.insert("librevenge:level",1);
	list.insert("text:min-label-width", 0.2, librevenge::RVNG_INCH);
	list.insert("text:space-before", 0.1, librevenge::RVNG_INCH);
	list.insert("style:num-format", "1");
	generator.defineOrderedListLevel(list);
	list.insert("librevenge:level",2);
	list.insert("style:num-format", "A");
	generator.defineOrderedListLevel(list);

	list.clear();
	list.insert("librevenge:id",2);
	generator.openOrderedListLevel(list);
	generator.openOrderedListLevel(list);
	list.insert("fo:margin-left",0.5,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 2 (redef)");
	generator.closeSpan();
	generator.closeListElement();
	generator.closeOrderedListLevel();
	generator.closeOrderedListLevel();

	list.clear();
	list.insert("librevenge:id",1);
	generator.openOrderedListLevel(list);
	list.insert("fo:margin-left",0.2,librevenge::RVNG_INCH);
	list.insert("text:start-value", 2);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 1 again");
	generator.closeSpan();
	generator.closeListElement();

	generator.closeOrderedListLevel();

	// paragraph
	para.insert("librevenge:paragraph-id",1);
	generator.openParagraph(para);
	generator.openSpan(span);
	generator.insertText("basic again");
	generator.closeSpan();
	generator.closeParagraph();

	// now test list without id
	list.clear();
	list.insert("text:min-label-width", 0.2, librevenge::RVNG_INCH);
	list.insert("text:space-before", 0.1, librevenge::RVNG_INCH);
	list.insert("style:num-format", "i");
	generator.openOrderedListLevel(list);

	list.clear();
	list.insert("fo:margin-left",0.2,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 1(def by hand)");
	generator.closeSpan();
	generator.closeListElement();

	list.clear();
	list.insert("text:min-label-width", 0.2, librevenge::RVNG_INCH);
	list.insert("text:space-before", 0.1, librevenge::RVNG_INCH);
	list.insert("style:num-format", "1");
	generator.openOrderedListLevel(list);
	list.insert("fo:margin-left",0.5,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 2(def by hand)");
	generator.closeSpan();
	generator.closeListElement();
	generator.closeOrderedListLevel();
	generator.closeOrderedListLevel();
}

static void createOdt()
{
	StringDocumentHandler content;
	OdtGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument();
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
	sendText(generator);
	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testList1.odt");
	file << content.cstr();
}

static void createOds()
{
	StringDocumentHandler content;
	OdsGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument();
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
	list.clear();
	list.insert("style:row-height", 40, librevenge::RVNG_POINT);
	generator.openSheetRow(list);
	list.clear();
	list.insert("librevenge:column", 1);
	list.insert("librevenge:row", 1);
	list.insert("table:number-columns-spanned", 1);
	list.insert("table:number-rows-spanned", 1);
	generator.openSheetCell(list);
	sendText(generator);
	generator.closeSheetCell();
	generator.closeSheetRow();
	generator.closeSheet();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testList1.ods");
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

	librevenge::RVNGPropertyList textbox;
	textbox.insert("svg:x",0.2, librevenge::RVNG_INCH);
	textbox.insert("svg:y",0.02, librevenge::RVNG_INCH);
	textbox.insert("svg:width",200, librevenge::RVNG_POINT);
	textbox.insert("svg:height",100, librevenge::RVNG_POINT);
	textbox.insert("draw:stroke", "none");
	textbox.insert("draw:fill", "none");
	generator.startTextObject(textbox);
	sendText(generator);
	generator.endTextObject();
	generator.endPage();
	generator.endDocument();

	std::ofstream file("testList1.odg");
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

	librevenge::RVNGPropertyList textbox;
	textbox.insert("svg:x",0.2, librevenge::RVNG_INCH);
	textbox.insert("svg:y",0.02, librevenge::RVNG_INCH);
	textbox.insert("svg:width",200, librevenge::RVNG_POINT);
	textbox.insert("svg:height",100, librevenge::RVNG_POINT);
	textbox.insert("draw:stroke", "none");
	textbox.insert("draw:fill", "none");
	generator.startTextObject(textbox);
	sendText(generator);
	generator.endTextObject();

	generator.endSlide();
	generator.endDocument();

	std::ofstream file("testList1.odp");
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

