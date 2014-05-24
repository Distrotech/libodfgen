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

#include "libodfgen-api.hxx"
#include "OdfDocumentHandler.hxx"

class OdtGeneratorPrivate;
class OdfGenerator;

/** A generator for text documents.
  *
  * See @c librevenge library for documentation of the ::librevenge::RVNGTextInterface
  * interface.
  */
class ODFGENAPI OdtGenerator : public librevenge::RVNGTextInterface
{
public:
	OdtGenerator();
	~OdtGenerator();
	void addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	librevenge::RVNGStringVector getObjectNames() const;
	bool getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler);

	void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);
	void startDocument(const librevenge::RVNGPropertyList &);
	void endDocument();

	void defineEmbeddedFont(const librevenge::RVNGPropertyList &propList);

	void definePageStyle(const librevenge::RVNGPropertyList &);
	void openPageSpan(const librevenge::RVNGPropertyList &propList);
	void closePageSpan();

	void defineSectionStyle(const librevenge::RVNGPropertyList &);
	void openSection(const librevenge::RVNGPropertyList &propList);
	void closeSection();

	void openHeader(const librevenge::RVNGPropertyList &propList);
	void closeHeader();
	void openFooter(const librevenge::RVNGPropertyList &propList);
	void closeFooter();

	void defineParagraphStyle(const librevenge::RVNGPropertyList &propList);
	void openParagraph(const librevenge::RVNGPropertyList &propList);
	void closeParagraph();

	void defineCharacterStyle(const librevenge::RVNGPropertyList &propList);
	void openSpan(const librevenge::RVNGPropertyList &propList);
	void closeSpan();

	void openLink(const librevenge::RVNGPropertyList &propList);
	void closeLink();

	void insertTab();
	void insertSpace();
	void insertText(const librevenge::RVNGString &text);
	void insertLineBreak();
	void insertField(const librevenge::RVNGPropertyList &propList);

	void openOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const librevenge::RVNGPropertyList &propList);
	void closeListElement();

	void openFootnote(const librevenge::RVNGPropertyList &propList);
	void closeFootnote();
	void openEndnote(const librevenge::RVNGPropertyList &propList);
	void closeEndnote();
	void openComment(const librevenge::RVNGPropertyList &propList);
	void closeComment();
	void openTextBox(const librevenge::RVNGPropertyList &propList);
	void closeTextBox();

	void openTable(const librevenge::RVNGPropertyList &propList);
	void openTableRow(const librevenge::RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const librevenge::RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList);
	void closeTable();

	//
	// simple Graphic
	//

	void openGroup(const librevenge::RVNGPropertyList &propList);
	void closeGroup();

	void defineGraphicStyle(const librevenge::RVNGPropertyList &propList);
	void drawRectangle(const librevenge::RVNGPropertyList &propList);
	void drawEllipse(const librevenge::RVNGPropertyList &propList);
	void drawPolygon(const librevenge::RVNGPropertyList &propList);
	void drawPolyline(const librevenge::RVNGPropertyList &propList);
	void drawPath(const librevenge::RVNGPropertyList &propList);
	void drawConnector(const librevenge::RVNGPropertyList &propList);

	void openFrame(const librevenge::RVNGPropertyList &propList);
	void closeFrame();

	void insertBinaryObject(const librevenge::RVNGPropertyList &propList);
	void insertEquation(const librevenge::RVNGPropertyList &propList);

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
	//! retrieve data from another odfgenerator ( the list and the embedded handler)
	void initStateWith(OdfGenerator const &orig);

private:
	OdtGenerator(OdtGenerator const &);
	OdtGenerator &operator=(OdtGenerator const &);

	OdtGeneratorPrivate *mpImpl;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
