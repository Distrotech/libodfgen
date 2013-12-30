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
	librevenge::RVNGPropertyList paragraph;
	generator.openParagraph(paragraph);

	librevenge::RVNGPropertyList span;
	span.insert("style:font-name","Courier");
	span.insert("fo:font-size", 13, librevenge::RVNG_POINT);
	span.insert("librevenge:span-id",1);
	generator.defineCharacterStyle(span);
	span.clear();
	span.insert("librevenge:span-id",1);
	generator.openSpan(span);
	generator.insertText("basic ");
	generator.closeSpan();

	span.clear();
	span.insert("style:font-name","Geneva");
	span.insert("fo:font-size", 11, librevenge::RVNG_POINT);
	span.insert("fo:font-style", "italic");
	generator.openSpan(span);
	generator.insertText("italic ");
	generator.closeSpan();

	span.clear();
	span.insert("librevenge:span-id",1);
	generator.openSpan(span);
	generator.insertText("basic again");
	generator.closeSpan();

	generator.closeParagraph();

	// now test if the list works or not
	librevenge::RVNGPropertyList list;
	list.insert("text:min-label-width", 0.2, librevenge::RVNG_INCH);
	list.insert("text:space-before", 0.1, librevenge::RVNG_INCH);
	list.insert("style:num-format", "i");
	generator.openOrderedListLevel(list);

	list.clear();
	list.insert("fo:margin-left",0.2,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 1(direct)"); // i level 1(direct)
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
	generator.insertText("level 2(direct)");  // 1 level 2(direct)
	generator.closeSpan();
	generator.closeListElement();
	generator.closeOrderedListLevel();

	list.clear();
	list.insert("fo:margin-left",0.2,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 1(direct)"); // ii level 1(direct)
	generator.closeSpan();
	generator.closeListElement();

	generator.closeOrderedListLevel();

	list.clear();
	list.insert("text:min-label-width", 0.2, librevenge::RVNG_INCH);
	list.insert("text:space-before", 0.1, librevenge::RVNG_INCH);
	list.insert("style:num-format", "A");
	list.insert("style:display-name", "MyList2");
	generator.openOrderedListLevel(list);

	list.clear();
	list.insert("fo:margin-left",0.2,librevenge::RVNG_INCH);
	generator.openListElement(list);
	generator.openSpan(span);
	generator.insertText("level 1(direct + automatic reset)"); // A level 1(...)
	generator.closeSpan();
	generator.closeListElement();

	generator.closeOrderedListLevel();
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
	textbox.insert("svg:height",100, librevenge::RVNG_POINT);
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
	sendText(generator);
	generator.closeTextBox();
	generator.closeFrame();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testTextbox1.odt");
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

	librevenge::RVNGPropertyList frame;
	frame.insert("svg:x",0.2, librevenge::RVNG_INCH);
	frame.insert("svg:y",0.2, librevenge::RVNG_INCH);
	frame.insert("svg:width",200, librevenge::RVNG_POINT);
	frame.insert("svg:height",100, librevenge::RVNG_POINT);
	frame.insert("text:anchor-type", "page");
	frame.insert("text:anchor-page-number", 1);
	frame.insert("style:vertical-rel", "page");
	frame.insert("style:horizontal-rel", "page");
	frame.insert("style:horizontal-pos", "from-left");
	frame.insert("style:vertical-pos", "from-top");

	frame.insert("draw:stroke", "none");
	frame.insert("draw:fill", "none");
	generator.openFrame(frame);
	generator.openTextBox(librevenge::RVNGPropertyList());
	sendText(generator);
	generator.closeTextBox();
	generator.closeFrame();

	list.clear();
	list.insert("style:row-height", 40, librevenge::RVNG_POINT);
	generator.openSheetRow(list);
	generator.closeSheetRow();
	generator.closeSheet();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testTextbox1.ods");
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

	std::ofstream file("testTextbox1.odg");
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

	std::ofstream file("testTextbox1.odp");
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

