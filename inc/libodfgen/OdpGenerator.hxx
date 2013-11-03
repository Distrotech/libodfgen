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
#ifndef LIBODFGEN_ODPGENERATOR_HXX_INCLUDED
#define LIBODFGEN_ODPGENERATOR_HXX_INCLUDED

#include <librevenge/librevenge.h>

#include "OdfDocumentHandler.hxx"

class OdpGeneratorPrivate;

/** A generator for presentations.
  *
  * See @c librevenge library for documentation of the
  * librevenge::KEYPresentationInterface interface.
  */
class OdpGenerator : public RVNGPresentationInterface
{
public:
	OdpGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdpGenerator();

	void startDocument(const ::RVNGPropertyList &propList);
	void endDocument();
	void setDocumentMetaData(const ::RVNGPropertyList &propList);
	void startSlide(const ::RVNGPropertyList &propList);
	void endSlide();
	void startLayer(const ::RVNGPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const ::RVNGPropertyList &propList);
	void endEmbeddedGraphics();
	void startGroup(const ::RVNGPropertyList &propList);
	void endGroup();

	void setStyle(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &gradient);

	void drawRectangle(const ::RVNGPropertyList &propList);
	void drawEllipse(const ::RVNGPropertyList &propList);
	void drawPolyline(const ::RVNGPropertyListVector &vertices);
	void drawPolygon(const ::RVNGPropertyListVector &vertices);
	void drawPath(const ::RVNGPropertyListVector &path);
	void drawGraphicObject(const ::RVNGPropertyList &propList, const ::RVNGBinaryData &binaryData);
	void drawConnector(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &path);

	void startTextObject(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &path);
	void endTextObject();
	void openParagraph(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &tabStops);
	void closeParagraph();
	void openSpan(const ::RVNGPropertyList &propList);
	void closeSpan();
	void insertText(const ::RVNGString &str);
	void insertTab();
	void insertSpace();
	void insertLineBreak();
	void insertField(const RVNGString &type, const ::RVNGPropertyList &propList);

	void openOrderedListLevel(const ::RVNGPropertyList &propList);
	void openUnorderedListLevel(const ::RVNGPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &tabStops);
	void closeListElement();

	void openTable(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &columns);
	void openTableRow(const ::RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const ::RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const ::RVNGPropertyList &propList);
	void closeTable();

	void startComment(const ::RVNGPropertyList &propList);
	void endComment();

	void startNotes(const ::RVNGPropertyList &propList);
	void endNotes();

private:
	// disable copying
	OdpGenerator(OdpGenerator const &);
	OdpGenerator &operator=(OdpGenerator const &);

	OdpGeneratorPrivate *mpImpl;
};

#endif // LIBODFGEN_ODPGENERATOR_HXX_INCLUDED

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
