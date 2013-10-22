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

#include <libetonyek/libetonyek.h>

#include "OdfDocumentHandler.hxx"

class OdpGeneratorPrivate;

/** A generator for presentations.
  *
  * See @c libetonyek library for documentation of the
  * libetonyek::KEYPresentationInterface interface.
  */
class OdpGenerator : public libetonyek::KEYPresentationInterface
{
public:
	OdpGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdpGenerator();

	void startDocument(const ::WPXPropertyList &propList);
	void endDocument();
	void setDocumentMetaData(const ::WPXPropertyList &propList);
	void startSlide(const ::WPXPropertyList &propList);
	void endSlide();
	void startLayer(const ::WPXPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const ::WPXPropertyList &propList);
	void endEmbeddedGraphics();
	void startGroup(const ::WPXPropertyList &propList);
	void endGroup();

	void setStyle(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &gradient);

	void drawRectangle(const ::WPXPropertyList &propList);
	void drawEllipse(const ::WPXPropertyList &propList);
	void drawPolyline(const ::WPXPropertyListVector &vertices);
	void drawPolygon(const ::WPXPropertyListVector &vertices);
	void drawPath(const ::WPXPropertyListVector &path);
	void drawGraphicObject(const ::WPXPropertyList &propList, const ::WPXBinaryData &binaryData);
	void drawConnector(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &path);

	void startTextObject(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &path);
	void endTextObject();
	void openParagraph(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &tabStops);
	void closeParagraph();
	void openSpan(const ::WPXPropertyList &propList);
	void closeSpan();
	void insertText(const ::WPXString &str);
	void insertTab();
	void insertSpace();
	void insertLineBreak();
	void insertField(const WPXString &type, const ::WPXPropertyList &propList);

	void openOrderedListLevel(const ::WPXPropertyList &propList);
	void openUnorderedListLevel(const ::WPXPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &tabStops);
	void closeListElement();

	void openTable(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &columns);
	void openTableRow(const ::WPXPropertyList &propList);
	void closeTableRow();
	void openTableCell(const ::WPXPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const ::WPXPropertyList &propList);
	void closeTable();

	void startComment(const ::WPXPropertyList &propList);
	void endComment();

	void startNotes(const ::WPXPropertyList &propList);
	void endNotes();

private:
	// disable copying
	OdpGenerator(OdpGenerator const &);
	OdpGenerator &operator=(OdpGenerator const &);

	OdpGeneratorPrivate *mpImpl;
};

#endif // LIBODFGEN_ODPGENERATOR_HXX_INCLUDED

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
