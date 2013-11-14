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

#include <librevenge/librevenge.h>

#include "OdfDocumentHandler.hxx"

/** Handler for embedded objects.
  *
  * @param[in] data the object's data
  * @param[in] pHandler the current OdfDocumentHandler
  * @param[in] streamType type of the object
  */
typedef bool (*OdfEmbeddedObject)(const librevenge::RVNGBinaryData &data, OdfDocumentHandler *pHandler, const OdfStreamType streamType);

/** Handler for embedded images.
  *
  * @param[in] input the image's data
  * @param[in] output the same image in format suitable for the used
  * OdfDocumentHandler.
  */
typedef bool (*OdfEmbeddedImage)(const librevenge::RVNGBinaryData &input, librevenge::RVNGBinaryData &output);

class OdtGeneratorPrivate;

/** A generator for text documents.
  *
  * See @c librevenge library for documentation of the ::librevenge::RVNGDocumentInterface
  * interface.
  */
class OdtGenerator : public librevenge::RVNGTextInterface
{
public:
	OdtGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdtGenerator();

	void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);
	void startDocument();
	void endDocument();

	void definePageStyle(const librevenge::RVNGPropertyList &);
	void openPageSpan(const librevenge::RVNGPropertyList &propList);
	void closePageSpan();

	void defineSectionStyle(const librevenge::RVNGPropertyList &, const librevenge::RVNGPropertyListVector &);
	void openSection(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &columns);
	void closeSection();

	void openHeader(const librevenge::RVNGPropertyList &propList);
	void closeHeader();
	void openFooter(const librevenge::RVNGPropertyList &propList);
	void closeFooter();

	void defineParagraphStyle(const librevenge::RVNGPropertyList &, const librevenge::RVNGPropertyListVector &);
	void openParagraph(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops);
	void closeParagraph();

	void defineCharacterStyle(const librevenge::RVNGPropertyList &);
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
	void openEndnote(const librevenge::RVNGPropertyList &propList);
	void closeEndnote();
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
	void insertEquation(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGString &data);

	/** Registers a handler for embedded objects.
	  *
	  * @param[in] mimeType MIME type of the object
	  * @param[in] objectHandler a function that handles processing of
	  *		the object's data and generating output
	  */
	void registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler);

	/** Registers a handler for embedded images.
	  *
	  * The handler converts the image to a format suitable for the used
	  * OdfDocumentHandler.
	  *
	  * @param[in] mimeType MIME type of the image
	  * @param[in] imageHandler a function that handles processing of
	  *		the images's data and generating output
	  */
	void registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler);

private:
	OdtGenerator(OdtGenerator const &);
	OdtGenerator &operator=(OdtGenerator const &);

	OdtGeneratorPrivate *mpImpl;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
