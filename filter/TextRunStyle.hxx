/* TextRunStyle: Stores (and writes) paragraph/span-style-based information
 * (e.g.: a paragraph might be bold) that is needed at the head of an OO
 * document.
 *
 * Copyright (C) 2002-2003 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#ifndef _TEXTRUNSTYLE_H
#define _TEXTRUNSTYLE_H

#include <map>

#include <libwpd/libwpd.h>

#include "Style.hxx"

class TagOpenElement;
class DocumentElement;
class OdfDocumentHandler;

class ParagraphStyle
{
public:
	ParagraphStyle(WPXPropertyList const &propList, const WPXPropertyListVector &tabStops, const WPXString &sName);
	virtual ~ParagraphStyle();
	virtual void write(OdfDocumentHandler *pHandler) const;
	WPXString getName() const
	{
		return msName;
	}
private:
	WPXPropertyList mpPropList;
	WPXPropertyListVector mxTabStops;
	WPXString msName;
};


class SpanStyle : public Style
{
public:
	SpanStyle(const char *psName, const WPXPropertyList &xPropList);
	virtual void write(OdfDocumentHandler *pHandler) const;

private:
	WPXPropertyList mPropList;
};

class ParagraphStyleManager : public StyleManager
{
public:
	ParagraphStyleManager() : mHash() {}
	virtual ~ParagraphStyleManager()
	{
		ParagraphStyleManager::clean();
	}

	/* create a new style if it does not exists. In all case, returns the name of the style

	Note: using S%i as new name*/
	WPXString findOrAdd(const WPXPropertyList &xPropList, const WPXPropertyListVector &tabStops);

	virtual void clean();
	virtual void write(OdfDocumentHandler *) const;


protected:
	// return a unique key
	WPXString getKey(WPXPropertyList const &xPropList, WPXPropertyListVector const &tabStops) const;

	// paragraph styles
	std::map<WPXString, ParagraphStyle *, ltstr> mHash;
};

class SpanStyleManager : public StyleManager
{
public:
	SpanStyleManager() : mHash() {}
	virtual ~SpanStyleManager()
	{
		SpanStyleManager::clean();
	}

	/* create a new style if it does not exists. In all case, returns the name of the style

	Note: using Span%i as new name*/
	WPXString findOrAdd(WPXPropertyList const &xPropList);


	virtual void clean();
	virtual void write(OdfDocumentHandler *) const;

protected:
	// span styles
	std::map<WPXString, SpanStyle *, ltstr> mHash;
};
#endif
