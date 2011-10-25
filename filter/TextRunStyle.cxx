/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* TextRunStyle: Stores (and writes) paragraph/span-style-based information
 * (e.g.: a paragraph might be bold) that is needed at the head of an OO
 * document.
 *
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004 Net Integration Technologies, Inc. (http://www.net-itech.com)
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
#include "FilterInternal.hxx"
#include "TextRunStyle.hxx"
#include "WriterProperties.hxx"
#include "DocumentElement.hxx"

#ifdef _MSC_VER
#include <minmax.h>
#endif

#include <string.h>

ParagraphStyle::ParagraphStyle(WPXPropertyList const &pPropList, const WPXPropertyListVector &xTabStops, const WPXString &sName) :
	mpPropList(pPropList),
	mxTabStops(xTabStops),
	msName(sName)
{
}

ParagraphStyle::~ParagraphStyle()
{
}

void ParagraphStyle::write(OdfDocumentHandler *pHandler) const
{
	WRITER_DEBUG_MSG(("Writing a paragraph style..\n"));

	WPXPropertyList propList;
	propList.insert("style:name", msName.cstr());
	propList.insert("style:family", "paragraph");
	if (mpPropList["style:parent-style-name"])
		propList.insert("style:parent-style-name", mpPropList["style:parent-style-name"]->getStr());
	if (mpPropList["style:master-page-name"])
		propList.insert("style:master-page-name", mpPropList["style:master-page-name"]->getStr());
	pHandler->startElement("style:style", propList);

	propList.clear();
	WPXPropertyList::Iter i(mpPropList);
	for (i.rewind(); i.next(); )
	{
#if 0
		if (strcmp(i.key(), "style:list-style-name") == 0)
			propList.insert("style:list-style-name", i()->getStr());
#endif
		if (strcmp(i.key(), "fo:margin-left") == 0)
			propList.insert("fo:margin-left", i()->getStr());
		if (strcmp(i.key(), "fo:margin-right") == 0)
			propList.insert("fo:margin-right", i()->getStr());
		if (strcmp(i.key(), "fo:text-indent") == 0)
			propList.insert("fo:text-indent", i()->getStr());
		if (strcmp(i.key(), "fo:margin-top") == 0)
			propList.insert("fo:margin-top", i()->getStr());
		if (strcmp(i.key(), "fo:margin-bottom") == 0)
		{
			if (i()->getDouble() > 0.0)
				propList.insert("fo:margin-bottom", i()->getStr());
			else
				propList.insert("fo:margin-bottom", 0.0);
		}
		if (strcmp(i.key(), "fo:line-height") == 0)
			propList.insert("fo:line-height", i()->getStr());
		if (strcmp(i.key(), "fo:break-before") == 0)
			propList.insert("fo:break-before", i()->getStr());
		if (strcmp(i.key(), "fo:text-align") == 0)
			propList.insert("fo:text-align", i()->getStr());
		if (strcmp(i.key(), "fo:text-align-last") == 0)
			propList.insert("fo:text-align-last", i()->getStr());
		if (strcmp(i.key(), "style:page-number") == 0)
			propList.insert("style:page-number", i()->getStr());

	}

	propList.insert("style:justify-single-word", "false");
	pHandler->startElement("style:paragraph-properties", propList);

	if (mxTabStops.count() > 0)
	{
		TagOpenElement tabListOpen("style:tab-stops");
		tabListOpen.write(pHandler);
		WPXPropertyListVector::Iter k(mxTabStops);
		for (k.rewind(); k.next();)
		{
			if (k()["style:position"] && k()["style:position"]->getDouble() < 0.0)
				continue;
			TagOpenElement tabStopOpen("style:tab-stop");

			WPXPropertyList::Iter j(k());
			for (j.rewind(); j.next(); )
			{
				tabStopOpen.addAttribute(j.key(), j()->getStr().cstr());
			}
			tabStopOpen.write(pHandler);
			pHandler->endElement("style:tab-stop");
		}
		pHandler->endElement("style:tab-stops");
	}

	pHandler->endElement("style:paragraph-properties");
	pHandler->endElement("style:style");
}

SpanStyle::SpanStyle(const char *psName, const WPXPropertyList &xPropList) :
	Style(psName),
	mPropList(xPropList)
{
}

void SpanStyle::write(OdfDocumentHandler *pHandler) const
{
	WRITER_DEBUG_MSG(("Writing a span style..\n"));
	WPXPropertyList styleOpenList;
	styleOpenList.insert("style:name", getName());
	styleOpenList.insert("style:family", "text");
	pHandler->startElement("style:style", styleOpenList);

	WPXPropertyList propList(mPropList);

	if (mPropList["style:font-name"])
	{
		propList.insert("style:font-name-asian", mPropList["style:font-name"]->getStr());
		propList.insert("style:font-name-complex", mPropList["style:font-name"]->getStr());
	}

	if (mPropList["fo:font-size"])
	{
		if (mPropList["fo:font-size"]->getDouble() > 0.0)
		{
			propList.insert("style:font-size-asian", mPropList["fo:font-size"]->getStr());
			propList.insert("style:font-size-complex", mPropList["fo:font-size"]->getStr());
		}
		else
			propList.remove("fo:font-size");
	}

	if (mPropList["fo:font-weight"])
	{
		propList.insert("style:font-weight-asian", mPropList["fo:font-weight"]->getStr());
		propList.insert("style:font-weight-complex", mPropList["fo:font-weight"]->getStr());
	}

	if (mPropList["fo:font-style"])
	{
		propList.insert("style:font-style-asian", mPropList["fo:font-style"]->getStr());
		propList.insert("style:font-style-complex", mPropList["fo:font-style"]->getStr());
	}

	pHandler->startElement("style:text-properties", propList);

	pHandler->endElement("style:text-properties");
	pHandler->endElement("style:style");
}

void ParagraphStyleManager::clean()
{
	for (std::map<WPXString, ParagraphStyle *, ltstr>::iterator iter = mHash.begin();
	        iter != mHash.end(); ++iter)
	{
		delete(iter->second);
	}
	mHash.clear();
}

void ParagraphStyleManager::write(OdfDocumentHandler *pHandler) const
{
	for (std::map<WPXString, ParagraphStyle *, ltstr>::const_iterator iter = mHash.begin();
	        iter != mHash.end(); ++iter)
	{
		if (strcmp(iter->second->getName().cstr(), "Standard") == 0)
			continue;
		(iter->second)->write(pHandler);
	}
}

WPXString ParagraphStyleManager::getKey(WPXPropertyList const &xPropList, WPXPropertyListVector const &tabStops) const
{
	WPXString sKey = propListToStyleKey(xPropList);

	WPXString sTabStops;
	sTabStops.sprintf("[num-tab-stops:%i]", tabStops.count());
	WPXPropertyListVector::Iter i(tabStops);
	for (i.rewind(); i.next();)
	{
		sTabStops.append(propListToStyleKey(i()));
	}
	sKey.append(sTabStops);

	return sKey;
}

WPXString ParagraphStyleManager::findOrAdd(WPXPropertyList const &propList, WPXPropertyListVector const &tabStops)
{
	WPXString hashKey = getKey(propList, tabStops);
	std::map<WPXString, ParagraphStyle *, ltstr>::const_iterator iter =
	    mHash.find(hashKey);
	if (iter!=mHash.end()) return iter->second->getName();

	// ok create a new list
	WRITER_DEBUG_MSG(("ParagraphStyleManager::findOrAdd: Paragraph Hash Key: %s\n", hashKey.cstr()));

	WPXString sName;
	sName.sprintf("S%i", mHash.size());
	ParagraphStyle *pStyle =
	    new ParagraphStyle(propList, tabStops, sName);
	mHash[hashKey] = pStyle;
	return sName;
}

void SpanStyleManager::clean()
{
	for (std::map<WPXString, SpanStyle *, ltstr>::iterator iter = mHash.begin();
	        iter != mHash.end(); ++iter)
	{
		delete(iter->second);
	}
	mHash.clear();
}

void SpanStyleManager::write(OdfDocumentHandler *pHandler) const
{
	for (std::map<WPXString, SpanStyle *, ltstr>::const_iterator iter = mHash.begin();
	        iter != mHash.end(); ++iter)
	{
		(iter->second)->write(pHandler);
	}
}

WPXString SpanStyleManager::findOrAdd(WPXPropertyList const &propList)
{
	WPXString hashKey = propListToStyleKey(propList);
	std::map<WPXString, SpanStyle *, ltstr>::const_iterator iter =
	    mHash.find(hashKey);
	if (iter!=mHash.end()) return iter->second->getName();

	// ok create a new list
	WRITER_DEBUG_MSG(("SpanStyleManager::findOrAdd: Span Hash Key: %s\n", hashKey.cstr()));

	WPXString sName;
	sName.sprintf("Span%i", mHash.size());
	SpanStyle *pStyle = new SpanStyle(sName.cstr(), propList);
	mHash[hashKey] = pStyle;
	return sName;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
