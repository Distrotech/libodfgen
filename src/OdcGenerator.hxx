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
#ifndef __ODCGENERATOR_HXX__
#define __ODCGENERATOR_HXX__

#include <librevenge/librevenge.h>

#include <libodfgen/OdfDocumentHandler.hxx>

class OdfGenerator;
class OdcGeneratorPrivate;

/** A generator for chart generation.
  *
  * See @c librevenge library for documentation of the
  * librevenge::RVNGSpreadsheetInterface interface.
  */
class OdcGenerator
{
public:
	OdcGenerator();
	~OdcGenerator();
	void addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	librevenge::RVNGStringVector getObjectNames() const;
	bool getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler);

	void setDocumentMetaData(const librevenge::RVNGPropertyList &);
	void startDocument(const librevenge::RVNGPropertyList &);
	void endDocument();

	void defineChartStyle(const librevenge::RVNGPropertyList &propList);
	void openChart(const librevenge::RVNGPropertyList &propList);
	void closeChart();
	void openChartTextObject(const librevenge::RVNGPropertyList &propList);
	void closeChartTextObject();
	void openChartPlotArea(const librevenge::RVNGPropertyList &propList);
	void closeChartPlotArea();
	void insertChartAxis(const librevenge::RVNGPropertyList &axis);
	void openChartSerie(const librevenge::RVNGPropertyList &series);
	void closeChartSerie();

	void openTable(const ::librevenge::RVNGPropertyList &propList);
	void closeTable();
	void openTableRow(const ::librevenge::RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const ::librevenge::RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList);

	void defineParagraphStyle(const librevenge::RVNGPropertyList &propList);
	void openParagraph(const librevenge::RVNGPropertyList &propList);
	void closeParagraph();

	void defineCharacterStyle(const librevenge::RVNGPropertyList &propList);
	void openSpan(const librevenge::RVNGPropertyList &propList);
	void closeSpan();

	void openLink(const librevenge::RVNGPropertyList &propList);
	void closeLink();

	void insertText(const librevenge::RVNGString &text);
	void insertTab();
	void insertSpace();
	void insertLineBreak();
	void insertField(const librevenge::RVNGPropertyList &propList);

	void openOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const librevenge::RVNGPropertyList &propList);
	void closeListElement();

	//! retrieve data from another odfgenerator ( the list and the embedded handler)
	void initStateWith(OdfGenerator const &orig);
private:
	OdcGenerator(OdcGenerator const &);
	OdcGenerator &operator=(OdcGenerator const &);

	OdcGeneratorPrivate *mpImpl;
};

#endif // __ODCGENERATOR_HXX__

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
