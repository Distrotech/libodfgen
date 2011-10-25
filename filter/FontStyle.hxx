/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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
#ifndef _FONTSTYLE_H
#define _FONTSTYLE_H
#include <map>

#include <libwpd/libwpd.h>

#include "Style.hxx"
#include "WriterProperties.hxx"

class FontStyle : public Style
{
public:
	FontStyle(const char *psName, const char *psFontFamily);
	~FontStyle();
	virtual void write(OdfDocumentHandler *pHandler) const;
	const WPXString &getFontFamily() const
	{
		return msFontFamily;
	}

private:
	WPXString msFontFamily;
	WPXString msFontPitch;
};

class FontStyleManager : public StyleManager
{
public:
	FontStyleManager() : mHash() {}
	virtual ~FontStyleManager()
	{
		FontStyleManager::clean();
	}

	/* create a new font if the font does not exists and returns a font name

	Note: the returned font name is actually equalled to psFontFamily
	*/
	WPXString findOrAdd(const char *psFontFamily);

	virtual void clean();
	virtual void write(OdfDocumentHandler *) const {}
	virtual void writeFontsDeclaration(OdfDocumentHandler *) const;


protected:
	std::map<WPXString, FontStyle *, ltstr> mHash;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
