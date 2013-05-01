/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2003 William Lachance (wrlach@gmail.com)
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

#include "FilterInternal.hxx"

#include "FontStyle.hxx"

#include "DocumentElement.hxx"

FontStyle::FontStyle(const char *psName, const char *psFontFamily) : Style(psName),
	msFontFamily(psFontFamily, true)
{
}

FontStyle::~FontStyle()
{
}

void FontStyle::write(OdfDocumentHandler *pHandler) const
{
	TagOpenElement styleOpen("style:font-face");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("svg:font-family", msFontFamily);
	styleOpen.write(pHandler);
	TagCloseElement styleClose("style:font-face");
	styleClose.write(pHandler);
}

void FontStyleManager::clean()
{
	mStyleHash.clear();
}

void FontStyleManager::writeFontsDeclaration(OdfDocumentHandler *pHandler) const
{
	TagOpenElement("office:font-face-decls").write(pHandler);
	std::map<WPXString, shared_ptr<FontStyle>, ltstr>::const_iterator iter;
	for (iter = mStyleHash.begin(); iter != mStyleHash.end(); ++iter)
	{
		(iter->second)->write(pHandler);
	}

	TagOpenElement symbolFontOpen("style:font-face");
	symbolFontOpen.addAttribute("style:name", "StarSymbol");
	symbolFontOpen.addAttribute("svg:font-family", "StarSymbol");
	symbolFontOpen.addAttribute("style:font-charset", "x-symbol");
	symbolFontOpen.write(pHandler);
	pHandler->endElement("style:font-face");

	pHandler->endElement("office:font-face-decls");
}

WPXString FontStyleManager::findOrAdd(const char *psFontFamily)
{
	std::map<WPXString, shared_ptr<FontStyle>, ltstr>::const_iterator iter =
	    mStyleHash.find(psFontFamily);
	if (iter!=mStyleHash.end()) return psFontFamily;

	// ok create a new font
	shared_ptr<FontStyle> font(new FontStyle(psFontFamily, psFontFamily));
	mStyleHash[psFontFamily] = font;
	return psFontFamily;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
