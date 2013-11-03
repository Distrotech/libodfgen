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
typedef bool (*OdfEmbeddedObject)(const RVNGBinaryData &data, OdfDocumentHandler *pHandler, const OdfStreamType streamType);

/** Handler for embedded images.
  *
  * @param[in] input the image's data
  * @param[in] output the same image in format suitable for the used
  * OdfDocumentHandler.
  */
typedef bool (*OdfEmbeddedImage)(const RVNGBinaryData &input, RVNGBinaryData &output);

class OdtGeneratorPrivate;

/** A generator for text documents.
  *
  * See @c librevenge library for documentation of the ::RVNGDocumentInterface
  * interface.
  */
class OdtGenerator : public RVNGTextInterface
{
public:
	OdtGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdtGenerator();

	void setDocumentMetaData(const RVNGPropertyList &propList);
	void startDocument();
	void endDocument();

	void definePageStyle(const RVNGPropertyList &);
	void openPageSpan(const RVNGPropertyList &propList);
	void closePageSpan();

	void defineSectionStyle(const RVNGPropertyList &, const RVNGPropertyListVector &);
	void openSection(const RVNGPropertyList &propList, const RVNGPropertyListVector &columns);
	void closeSection();

	void openHeader(const RVNGPropertyList &propList);
	void closeHeader();
	void openFooter(const RVNGPropertyList &propList);
	void closeFooter();

	void defineParagraphStyle(const RVNGPropertyList &, const RVNGPropertyListVector &);
	void openParagraph(const RVNGPropertyList &propList, const RVNGPropertyListVector &tabStops);
	void closeParagraph();

	void defineCharacterStyle(const RVNGPropertyList &);
	void openSpan(const RVNGPropertyList &propList);
	void closeSpan();

	void insertTab();
	void insertSpace();
	void insertText(const RVNGString &text);
	void insertLineBreak();
	void insertField(const RVNGString &type, const RVNGPropertyList &propList);

	void defineOrderedListLevel(const RVNGPropertyList &propList);
	void defineUnorderedListLevel(const RVNGPropertyList &propList);
	void openOrderedListLevel(const RVNGPropertyList &propList);
	void openUnorderedListLevel(const RVNGPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const RVNGPropertyList &propList, const RVNGPropertyListVector &tabStops);
	void closeListElement();

	void openFootnote(const RVNGPropertyList &propList);
	void closeFootnote();
	void openEndnote(const RVNGPropertyList &propList);
	void closeEndnote();
	void openComment(const RVNGPropertyList &propList);
	void closeComment();
	void openTextBox(const RVNGPropertyList &propList);
	void closeTextBox();

	void openTable(const RVNGPropertyList &propList, const RVNGPropertyListVector &columns);
	void openTableRow(const RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const RVNGPropertyList &propList);
	void closeTable();

	void openFrame(const RVNGPropertyList &propList);
	void closeFrame();

	void insertBinaryObject(const RVNGPropertyList &propList, const RVNGBinaryData &data);
	void insertEquation(const RVNGPropertyList &propList, const RVNGString &data);

	/** Registers a handler for embedded objects.
	  *
	  * @param[in] mimeType MIME type of the object
	  * @param[in] objectHandler a function that handles processing of
	  *		the object's data and generating output
	  */
	void registerEmbeddedObjectHandler(const RVNGString &mimeType, OdfEmbeddedObject objectHandler);

	/** Registers a handler for embedded images.
	  *
	  * The handler converts the image to a format suitable for the used
	  * OdfDocumentHandler.
	  *
	  * @param[in] mimeType MIME type of the image
	  * @param[in] imageHandler a function that handles processing of
	  *		the images's data and generating output
	  */
	void registerEmbeddedImageHandler(const RVNGString &mimeType, OdfEmbeddedImage imageHandler);

private:
	OdtGenerator(OdtGenerator const &);
	OdtGenerator &operator=(OdtGenerator const &);

	OdtGeneratorPrivate *mpImpl;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
