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

#include <libwpd/libwpd.h>
#include <libwpg/libwpg.h>

#include "OdfDocumentHandler.hxx"

class OdgGeneratorPrivate;

/** A generator for vector drawings.
  *
  * See @c libwpg library for documentation of the
  * libwpg::WPGPaintInterface interface.
  */
class OdgGenerator : public libwpg::WPGPaintInterface
{
public:
	OdgGenerator(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	~OdgGenerator();

	void startGraphics(const ::WPXPropertyList &propList);
	void endGraphics();
	void startLayer(const ::WPXPropertyList &propList);
	void endLayer();
	void startEmbeddedGraphics(const ::WPXPropertyList &propList);
	void endEmbeddedGraphics();

	void setStyle(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &gradient);

	void drawRectangle(const ::WPXPropertyList &propList);
	void drawEllipse(const ::WPXPropertyList &propList);
	void drawPolyline(const ::WPXPropertyListVector &vertices);
	void drawPolygon(const ::WPXPropertyListVector &vertices);
	void drawPath(const ::WPXPropertyListVector &path);
	void drawGraphicObject(const ::WPXPropertyList &propList, const ::WPXBinaryData &binaryData);

	void startTextObject(const ::WPXPropertyList &propList, const ::WPXPropertyListVector &path);
	void endTextObject();
	void startTextLine(const ::WPXPropertyList &propList);
	void endTextLine();
	void startTextSpan(const ::WPXPropertyList &propList);
	void endTextSpan();
	void insertText(const ::WPXString &str);

private:
	OdgGenerator(OdgGenerator const &);
	OdgGenerator &operator=(OdgGenerator const &);

	OdgGeneratorPrivate *mpImpl;
};

#endif // __ODGGENERATOR_HXX__

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
