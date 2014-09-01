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

//! graphic style class
class GraphicStyle : public Style
{
public:
	//! constructor
	GraphicStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName, Style::Zone zone);
	//! destructor
	~GraphicStyle();
	//! write content to the document handler
	virtual void write(OdfDocumentHandler *pHandler) const;

private:
	librevenge::RVNGPropertyList mPropList;
};


class GraphicStyleManager : public StyleManager
{
public:
	GraphicStyleManager() : mBitmapStyles(), mGradientStyles(), mMarkerStyles(), mOpacityStyles(),
		mStrokeDashStyles(), mStyles(), mBitmapNameMap(), mGradientNameMap(), mMarkerNameMap(),
		mOpacityNameMap(), mStrokeDashNameMap(), mStyleNameMap() {}
	virtual ~GraphicStyleManager()
	{
		clean();
	}
	void clean();
	//! write all
	void write(OdfDocumentHandler *pHandler) const
	{
		write(pHandler, Style::Z_Style);
		write(pHandler, Style::Z_StyleAutomatic);
		write(pHandler, Style::Z_ContentAutomatic);
	}
	// write automatic/name/... style
	void write(OdfDocumentHandler *pHandler, Style::Zone zone) const;

	/** find a style ( or add it to the stored styles) and returns the style name */
	librevenge::RVNGString findOrAdd(librevenge::RVNGPropertyList const &propList, Style::Zone zone=Style::Z_Unknown);

	/** append the graphic in the element, ie. the stroke, pattern, bitmap, marker properties */
	void addGraphicProperties(librevenge::RVNGPropertyList const &style, librevenge::RVNGPropertyList &element);
	/** append the frame, ... properties in the element, ie. all properties excepted the graphic properties */
	void addFrameProperties(librevenge::RVNGPropertyList const &propList, librevenge::RVNGPropertyList &element);

protected:
	// return a bitmap
	librevenge::RVNGString getStyleNameForBitmap(librevenge::RVNGString const &bitmap);
	librevenge::RVNGString getStyleNameForGradient(librevenge::RVNGPropertyList const &style,
	                                               bool &needCreateOpacityStyle);
	librevenge::RVNGString getStyleNameForMarker(librevenge::RVNGPropertyList const &style, bool startMarker);
	librevenge::RVNGString getStyleNameForOpacity(librevenge::RVNGPropertyList const &style);
	librevenge::RVNGString getStyleNameForStrokeDash(librevenge::RVNGPropertyList const &style);
	// graphics styles
	libodfgen::DocumentElementVector mBitmapStyles;
	libodfgen::DocumentElementVector mGradientStyles;
	libodfgen::DocumentElementVector mMarkerStyles;
	libodfgen::DocumentElementVector mOpacityStyles;
	libodfgen::DocumentElementVector mStrokeDashStyles;
	std::vector<shared_ptr<GraphicStyle> > mStyles;

	// bitmap content -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mBitmapNameMap;
	// gradient hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mGradientNameMap;
	// marker hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mMarkerNameMap;
	// opacity hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mOpacityNameMap;
	// stroke dash hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mStrokeDashNameMap;
	// style hash -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mStyleNameMap;
};



#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
