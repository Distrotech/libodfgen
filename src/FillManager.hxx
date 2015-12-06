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

#ifndef _FILLMANAGER_HXX_
#define _FILLMANAGER_HXX_

#include <map>
#include <vector>

#include <librevenge/librevenge.h>

#include "FilterInternal.hxx"

class OdfDocumentHandler;

class FillManager
{
public:
	FillManager() : mBitmapStyles(), mGradientStyles(), mHatchStyles(), mOpacityStyles(),
		mBitmapNameMap(), mGradientNameMap(), mHatchNameMap(), mOpacityNameMap()
	{}

	void clean();

	// write fill definition to style
	void write(OdfDocumentHandler *pHandler) const;

	/** append the fill into the element */
	void addProperties(librevenge::RVNGPropertyList const &style, librevenge::RVNGPropertyList &element);

private:
	// return a bitmap
	librevenge::RVNGString getStyleNameForBitmap(librevenge::RVNGString const &bitmap);
	librevenge::RVNGString getStyleNameForGradient(librevenge::RVNGPropertyList const &style,
	                                               bool &needCreateOpacityStyle);
	librevenge::RVNGString getStyleNameForHatch(librevenge::RVNGPropertyList const &style);
	librevenge::RVNGString getStyleNameForOpacity(librevenge::RVNGPropertyList const &style);

private:
	// graphics styles
	libodfgen::DocumentElementVector mBitmapStyles;
	libodfgen::DocumentElementVector mGradientStyles;
	libodfgen::DocumentElementVector mHatchStyles;
	libodfgen::DocumentElementVector mOpacityStyles;

	// bitmap content -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mBitmapNameMap;
	// gradient hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mGradientNameMap;
	// hatch hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mHatchNameMap;
	// opacity hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mOpacityNameMap;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
