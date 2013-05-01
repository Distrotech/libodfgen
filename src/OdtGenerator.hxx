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

#ifndef _ODTGENERATOR_HXX_
#define _ODTGENERATOR_HXX_

#include <libwpd/libwpd.h>

#include "OdfDocumentHandler.hxx"


typedef bool (*OdfEmbeddedObject)(const WPXBinaryData &data, OdfDocumentHandler *pHandler, const OdfStreamType streamType);
typedef bool (*OdfEmbeddedImage)(const WPXBinaryData &input, WPXBinaryData &output);

class OdtGeneratorPrivate;

class OdtGenerator : public WPXDocumentInterface
{
public:
	OdtGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdtGenerator();

	// WPXDocumentInterface's implementation
	void setDocumentMetaData(const WPXPropertyList &propList);
	void startDocument();
	void endDocument();

	void definePageStyle(const WPXPropertyList &);
	void openPageSpan(const WPXPropertyList &propList);
	void closePageSpan();

	void defineSectionStyle(const WPXPropertyList &, const WPXPropertyListVector &);
	void openSection(const WPXPropertyList &propList, const WPXPropertyListVector &columns);
	void closeSection();

	void openHeader(const WPXPropertyList &propList);
	void closeHeader();
	void openFooter(const WPXPropertyList &propList);
	void closeFooter();

	void defineParagraphStyle(const WPXPropertyList &, const WPXPropertyListVector &);
	void openParagraph(const WPXPropertyList &propList, const WPXPropertyListVector &tabStops);
	void closeParagraph();

	void defineCharacterStyle(const WPXPropertyList &);
	void openSpan(const WPXPropertyList &propList);
	void closeSpan();

	void insertTab();
	void insertSpace();
	void insertText(const WPXString &text);
	void insertLineBreak();
	void insertField(const WPXString &type, const WPXPropertyList &propList);

	void defineOrderedListLevel(const WPXPropertyList &propList);
	void defineUnorderedListLevel(const WPXPropertyList &propList);
	void openOrderedListLevel(const WPXPropertyList &propList);
	void openUnorderedListLevel(const WPXPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const WPXPropertyList &propList, const WPXPropertyListVector &tabStops);
	void closeListElement();

	void openFootnote(const WPXPropertyList &propList);
	void closeFootnote();
	void openEndnote(const WPXPropertyList &propList);
	void closeEndnote();
	void openComment(const WPXPropertyList &propList);
	void closeComment();
	void openTextBox(const WPXPropertyList &propList);
	void closeTextBox();

	void openTable(const WPXPropertyList &propList, const WPXPropertyListVector &columns);
	void openTableRow(const WPXPropertyList &propList);
	void closeTableRow();
	void openTableCell(const WPXPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const WPXPropertyList &propList);
	void closeTable();

	void openFrame(const WPXPropertyList &propList);
	void closeFrame();

	void insertBinaryObject(const WPXPropertyList &propList, const WPXBinaryData &data);
	void insertEquation(const WPXPropertyList &propList, const WPXString &data);

	// Register special converter for certain embedded binary objects
	void registerEmbeddedObjectHandler(const WPXString &mimeType, OdfEmbeddedObject objectHandler);
	void registerEmbeddedImageHandler(const WPXString &mimeType, OdfEmbeddedImage imageHandler);

private:
	OdtGenerator(OdtGenerator const &);
	OdtGenerator &operator=(OdtGenerator const &);

	OdtGeneratorPrivate *mpImpl;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
