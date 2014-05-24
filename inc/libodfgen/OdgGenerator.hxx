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
#ifndef __ODGGENERATOR_HXX__
#define __ODGGENERATOR_HXX__

#include <librevenge/librevenge.h>

#include "libodfgen-api.hxx"
#include "OdfDocumentHandler.hxx"

class OdfGenerator;
class OdgGeneratorPrivate;

/** A generator for vector drawings.
  *
  * See @c librevenge library for documentation of the
  * librevenge::WPGPaintInterface interface.
  */
class ODFGENAPI OdgGenerator : public librevenge::RVNGDrawingInterface
{
public:
	OdgGenerator();
	~OdgGenerator();
	void addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	librevenge::RVNGStringVector getObjectNames() const;
	bool getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler);
	void setDocumentMetaData(const librevenge::RVNGPropertyList &);

	void defineEmbeddedFont(const librevenge::RVNGPropertyList &propList);

	void startPage(const librevenge::RVNGPropertyList &);
	void endPage();
	void startMasterPage(const librevenge::RVNGPropertyList &);
	void endMasterPage();
	void startLayer(const ::librevenge::RVNGPropertyList &propList);
	void endLayer();
	void openGroup(const ::librevenge::RVNGPropertyList &propList);
	void closeGroup();
	void startEmbeddedGraphics(const ::librevenge::RVNGPropertyList &propList);
	void endEmbeddedGraphics();

	void setStyle(const ::librevenge::RVNGPropertyList &propList);

	void drawRectangle(const ::librevenge::RVNGPropertyList &propList);
	void drawEllipse(const ::librevenge::RVNGPropertyList &propList);
	void drawPolyline(const ::librevenge::RVNGPropertyList &propList);
	void drawPolygon(const ::librevenge::RVNGPropertyList &propList);
	void drawPath(const ::librevenge::RVNGPropertyList &propList);
	void drawGraphicObject(const ::librevenge::RVNGPropertyList &propList);
	void drawConnector(const ::librevenge::RVNGPropertyList &propList);

	void startTableObject(const ::librevenge::RVNGPropertyList &propList);
	void openTableRow(const ::librevenge::RVNGPropertyList &propList);
	void closeTableRow();
	void openTableCell(const ::librevenge::RVNGPropertyList &propList);
	void closeTableCell();
	void insertCoveredTableCell(const ::librevenge::RVNGPropertyList &propList);
	void endTableObject();

	void startTextObject(const ::librevenge::RVNGPropertyList &propList);
	void endTextObject();

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

	void startDocument(const librevenge::RVNGPropertyList &);
	void endDocument();

	/** Registers a handler for embedded images.
	  *
	  * @param[in] mimeType MIME type of the image
	  * @param[in] imageHandler a function that handles processing of
	  *		the image's data and generating output
	  */
	void registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler);
	/** Registers a handler for embedded objects.
	  *
	  * @param[in] mimeType MIME type of the object
	  * @param[in] objectHandler a function that handles processing of
	  *		the object's data and generating output
	  */
	void registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler);

	//! retrieve data from another odfgenerator ( the list and the embedded handler)
	void initStateWith(OdfGenerator const &orig);
private:
	OdgGenerator(OdgGenerator const &);
	OdgGenerator &operator=(OdgGenerator const &);

	OdgGeneratorPrivate *mpImpl;
};

#endif // __ODGGENERATOR_HXX__

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
