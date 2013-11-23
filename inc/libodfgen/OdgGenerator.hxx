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

#include "OdfDocumentHandler.hxx"

class OdgGeneratorPrivate;

/** A generator for vector drawings.
  *
  * See @c librevenge library for documentation of the
  * librevenge::WPGPaintInterface interface.
  */
class OdgGenerator : public librevenge::RVNGDrawingInterface
{
public:
	OdgGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdgGenerator();

	void startPage(const librevenge::RVNGPropertyList &);
	void endPage();
	void startLayer(const ::librevenge::RVNGPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const ::librevenge::RVNGPropertyList &propList);
	void endEmbeddedGraphics();

	void setStyle(const ::librevenge::RVNGPropertyList &propList);

	void drawRectangle(const ::librevenge::RVNGPropertyList &propList);
	void drawEllipse(const ::librevenge::RVNGPropertyList &propList);
	void drawPolyline(const ::librevenge::RVNGPropertyList &propList);
	void drawPolygon(const ::librevenge::RVNGPropertyList &propList);
	void drawPath(const ::librevenge::RVNGPropertyList &propList);
	void drawGraphicObject(const ::librevenge::RVNGPropertyList &propList);

	void startTextObject(const ::librevenge::RVNGPropertyList &propList);
	void endTextObject();

	void setDocumentMetaData(const librevenge::RVNGPropertyList &);
	void insertText(const librevenge::RVNGString &text);
	void insertTab();
	void insertSpace();
	void insertLineBreak();
	void insertField(const librevenge::RVNGPropertyList &);
	void openOrderedListLevel(const librevenge::RVNGPropertyList &);
	void openUnorderedListLevel(const librevenge::RVNGPropertyList &);
	void closeOrderedListLevel();
	void closeUnorderedListLevel();
	void openListElement(const librevenge::RVNGPropertyList &);
	void closeListElement();
	void openParagraph(const librevenge::RVNGPropertyList &);
	void closeParagraph();
	void openSpan(const librevenge::RVNGPropertyList &);
	void closeSpan();
	void startDocument(const librevenge::RVNGPropertyList &);
	void endDocument();
private:
	OdgGenerator(OdgGenerator const &);
	OdgGenerator &operator=(OdgGenerator const &);

	OdgGeneratorPrivate *mpImpl;
};

#endif // __ODGGENERATOR_HXX__

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
