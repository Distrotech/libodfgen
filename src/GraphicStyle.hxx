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

#ifndef _GRAPHICSTYLE_HXX_
#define _GRAPHICSTYLE_HXX_

#include <map>
#include <vector>

#include <librevenge/librevenge.h>

#include "FilterInternal.hxx"

#include "Style.hxx"

class OdfDocumentHandler;

class GraphicStyleManager : public StyleManager
{
public:
	GraphicStyleManager() : mBitmapStyles(), mGradientStyles(), mMarkerStyles(), mStrokeDashStyles(),
		mBitmapNameMap(), mGradientNameMap(), mMarkerNameMap(), mOpacityNameMap(), mStrokeDashNameMap()
	{
	}
	virtual ~GraphicStyleManager()
	{
		clean();
	}
	void clean();
	void write(OdfDocumentHandler *) const {}
	// write basic style
	void writeStyles(OdfDocumentHandler *pHandler) const;
	/** update a graphic style element */
	void updateElement(TagOpenElement &element, librevenge::RVNGPropertyList const &style);

protected:
	// return a bitmap
	librevenge::RVNGString getStyleNameForBitmap(librevenge::RVNGString const &bitmap);
	librevenge::RVNGString getStyleNameForGradient(librevenge::RVNGPropertyList const &style,
	                                               bool &needCreateOpacityStyle);
	librevenge::RVNGString getStyleNameForMarker(librevenge::RVNGPropertyList const &style, bool startMarker);
	librevenge::RVNGString getStyleNameForOpacity(librevenge::RVNGPropertyList const &style);
	librevenge::RVNGString getStyleNameForStrokeDash(librevenge::RVNGPropertyList const &style);
	// graphics styles
	std::vector<DocumentElement *> mBitmapStyles;
	std::vector<DocumentElement *> mGradientStyles;
	std::vector<DocumentElement *> mMarkerStyles;
	std::vector<DocumentElement *> mOpacityStyles;
	std::vector<DocumentElement *> mStrokeDashStyles;

	// bitmap content -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mBitmapNameMap;
	// gradient hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mGradientNameMap;
	// marker hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mMarkerNameMap;
	// opacity hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mOpacityNameMap;
	// stroke dash hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mStrokeDashNameMap;
};



#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
