/* FontStyle: Stores (and writes) font-based information that is needed at
 * the head of an OO document.
 *
 * Copyright (C) 2002-2003 William Lachance (wrlach@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * For further information visit http://libwpd.sourceforge.net
 *
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */
#include "FontStyle.hxx"
#include "WriterProperties.hxx"
#include "DocumentElement.hxx"

FontStyle::FontStyle(const char *psName, const char *psFontFamily) : Style(psName),
	msFontFamily(psFontFamily, true),
	msFontPitch(IMP_DEFAULT_FONT_PITCH)
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
	for (std::map<WPXString, FontStyle *, ltstr>::iterator iter = mHash.begin();
	        iter != mHash.end(); iter++)
	{
		delete(iter->second);
	}
	mHash.clear();
}

void FontStyleManager::writeFontsDeclaration(OdfDocumentHandler *pHandler) const
{
	TagOpenElement("office:font-face-decls").write(pHandler);
	std::map<WPXString, FontStyle *, ltstr>::const_iterator iter;
	for (std::map<WPXString, FontStyle *, ltstr>::const_iterator iter = mHash.begin();
	        iter != mHash.end(); iter++)
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
	std::map<WPXString, FontStyle *, ltstr>::const_iterator iter =
	    mHash.find(psFontFamily);
	if (iter!=mHash.end()) return iter->second->getName();

	// ok create a new font
	mHash[psFontFamily] = new FontStyle(psFontFamily, psFontFamily);
	return psFontFamily;
}

