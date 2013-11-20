/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#ifndef _ODSGENERATOR_HXX_
#define _ODSGENERATOR_HXX_

#include <librevenge/librevenge.h>

#include "OdfDocumentHandler.hxx"

class OdsGeneratorPrivate;

/** A generator for text documents.
  *
  * See @c libdocumentinterface library for documentation of the librevenge::RVNGSpreadsheetInterface
  * interface.
  */
class OdsGenerator : public librevenge::RVNGSpreadsheetInterface
{
public:
	OdsGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdsGenerator();

	void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);
	void startDocument();
	void endDocument();

	void definePageStyle(const librevenge::RVNGPropertyList &) {}
	void openPageSpan(const librevenge::RVNGPropertyList &propList);
	void closePageSpan();

	void defineSectionStyle(const librevenge::RVNGPropertyList &, const librevenge::RVNGPropertyListVector &) {}
	void openSection(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &columns);
	void closeSection();

	void openHeader(const librevenge::RVNGPropertyList &propList);
	void closeHeader();
	void openFooter(const librevenge::RVNGPropertyList &propList);
	void closeFooter();

	void defineSheetFormula(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &formula);
	void insertSheetConditionInNumberingStyle(const librevenge::RVNGPropertyList &propList);
	void defineSheetNumberingStyle(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &formats) ;
	void openSheet(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &columns);
	void closeSheet();
	void openSheetRow(const librevenge::RVNGPropertyList &propList);
	void closeSheetRow();
	void openSheetCell(const librevenge::RVNGPropertyList &propList);
	void closeSheetCell();

	void openChart(const librevenge::RVNGPropertyList &propList);
	void closeChart();
	void insertChartSerie(const librevenge::RVNGPropertyList &propList);

	void defineParagraphStyle(const librevenge::RVNGPropertyList &, const librevenge::RVNGPropertyListVector &) {}
	void openParagraph(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops);
	void closeParagraph();

	void defineCharacterStyle(const librevenge::RVNGPropertyList &) {}
	void openSpan(const librevenge::RVNGPropertyList &propList);
	void closeSpan();

	void insertTab();
	void insertSpace();
	void insertText(const librevenge::RVNGString &text);
	void insertLineBreak();
	void insertField(const librevenge::RVNGString &type, const librevenge::RVNGPropertyList &propList);

	void defineOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	void defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	void openOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops);
	void closeListElement();

	void openFootnote(const librevenge::RVNGPropertyList &propList);
	void closeFootnote();

	void openComment(const librevenge::RVNGPropertyList &propList);
	void closeComment();
	void openTextBox(const librevenge::RVNGPropertyList &propList);
	void closeTextBox();

	void openTable(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &columns);
	void openTableRow(const librevenge::RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const librevenge::RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList);
	void closeTable();

	void openFrame(const librevenge::RVNGPropertyList &propList);
	void closeFrame();

	void insertBinaryObject(const librevenge::RVNGPropertyList &propList);

	//
	// simple Graphic
	//

	void startGraphic(const librevenge::RVNGPropertyList &propList);
	void endGraphic();

	void startGraphicPage(const librevenge::RVNGPropertyList &propList);
	void endGraphicPage();

	void setGraphicStyle(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &gradient);
	void startGraphicLayer(const librevenge::RVNGPropertyList &propList);
	void endGraphicLayer();

	void drawRectangle(const librevenge::RVNGPropertyList &propList);
	void drawEllipse(const librevenge::RVNGPropertyList &propList);
	void drawPolygon(const librevenge::RVNGPropertyListVector &vertices);
	void drawPolyline(const librevenge::RVNGPropertyListVector &vertices);
	void drawPath(const librevenge::RVNGPropertyList &propList);

	void insertEquation(const librevenge::RVNGPropertyList &, const librevenge::RVNGString &) {}

	/** Registers a handler for embedded objects.
	  *
	  * @param[in] mimeType MIME type of the object
	  * @param[in] objectHandler a function that handles processing of
	  *		the object's data and generating output
	  */
	void registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler);

private:
	OdsGenerator(OdsGenerator const &);
	OdsGenerator &operator=(OdsGenerator const &);

	OdsGeneratorPrivate *mpImpl;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
