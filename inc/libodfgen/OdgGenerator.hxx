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
class OdgGenerator : public RVNGDrawingInterface
{
public:
	OdgGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdgGenerator();

	void startPage(const RVNGPropertyList &);
	void endPage();
	void startLayer(const ::RVNGPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const ::RVNGPropertyList &propList);
	void endEmbeddedGraphics();

	void setStyle(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &gradient);

	void drawRectangle(const ::RVNGPropertyList &propList);
	void drawEllipse(const ::RVNGPropertyList &propList);
	void drawPolyline(const ::RVNGPropertyListVector &vertices);
	void drawPolygon(const ::RVNGPropertyListVector &vertices);
	void drawPath(const ::RVNGPropertyListVector &path);
	void drawGraphicObject(const ::RVNGPropertyList &propList, const ::RVNGBinaryData &binaryData);

	void startTextObject(const ::RVNGPropertyList &propList, const ::RVNGPropertyListVector &path);
	void endTextObject();

	void setDocumentMetaData(const RVNGPropertyList &) {}
	void insertText(const RVNGString &text);
	void insertTab() {}
	void insertSpace() {}
	void insertLineBreak() {}
	void insertField(const RVNGString &, const RVNGPropertyList &) {}
	void openOrderedListLevel(const RVNGPropertyList &) {}
	void openUnorderedListLevel(const RVNGPropertyList &) {}
	void closeOrderedListLevel() {}
	void closeUnorderedListLevel() {}
	void openListElement(const RVNGPropertyList &, const RVNGPropertyListVector &) {}
	void closeListElement() {}
	void openParagraph(const RVNGPropertyList &, const RVNGPropertyListVector &);
	void closeParagraph();
	void openSpan(const RVNGPropertyList &);
	void closeSpan();
	void startDocument(const RVNGPropertyList &) {}
	void endDocument() {}
private:
	OdgGenerator(OdgGenerator const &);
	OdgGenerator &operator=(OdgGenerator const &);

	OdgGeneratorPrivate *mpImpl;
};

#endif // __ODGGENERATOR_HXX__

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
