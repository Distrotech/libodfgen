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
#include "ListStyle.hxx"
#include "DocumentElement.hxx"

OrderedListLevelStyle::OrderedListLevelStyle(const librevenge::RVNGPropertyList &xPropList) :
	mPropList(xPropList)
{
}

void OrderedListLevelStyle::write(OdfDocumentHandler *pHandler, int iLevel) const
{
	librevenge::RVNGString sLevel;
	sLevel.sprintf("%i", (iLevel+1));

	TagOpenElement listLevelStyleOpen("text:list-level-style-number");
	listLevelStyleOpen.addAttribute("text:level", sLevel);
	listLevelStyleOpen.addAttribute("text:style-name", "Numbering_Symbols");
	if (mPropList["style:num-prefix"])
	{
		librevenge::RVNGString sEscapedString(mPropList["style:num-prefix"]->getStr(), true);
		listLevelStyleOpen.addAttribute("style:num-prefix", sEscapedString);
	}
	if (mPropList["style:num-suffix"])
	{
		librevenge::RVNGString sEscapedString(mPropList["style:num-suffix"]->getStr(), true);
		listLevelStyleOpen.addAttribute("style:num-suffix", sEscapedString);
	}
	if (mPropList["style:num-format"])
		listLevelStyleOpen.addAttribute("style:num-format", mPropList["style:num-format"]->getStr());
	if (mPropList["text:start-value"])
	{
		// ODF as to the version 1.1 does require the text:start-value to be a positive integer, means > 0
		if (mPropList["text:start-value"]->getInt() > 0)
			listLevelStyleOpen.addAttribute("text:start-value", mPropList["text:start-value"]->getStr());
		else
			listLevelStyleOpen.addAttribute("text:start-value", "1");
	}
	if (mPropList["text:display-levels"])
		listLevelStyleOpen.addAttribute("text:display-levels", mPropList["text:display-levels"]->getStr());
	listLevelStyleOpen.write(pHandler);

	TagOpenElement stylePropertiesOpen("style:list-level-properties");
	if (mPropList["text:space-before"] && mPropList["text:space-before"]->getDouble() > 0.0)
		stylePropertiesOpen.addAttribute("text:space-before", mPropList["text:space-before"]->getStr());
	if (mPropList["text:min-label-width"] && mPropList["text:min-label-width"]->getDouble() > 0.0)
		stylePropertiesOpen.addAttribute("text:min-label-width", mPropList["text:min-label-width"]->getStr());
	if (mPropList["text:min-label-distance"] && mPropList["text:min-label-distance"]->getDouble() > 0.0)
		stylePropertiesOpen.addAttribute("text:min-label-distance", mPropList["text:min-label-distance"]->getStr());
	if (mPropList["fo:text-align"])
		stylePropertiesOpen.addAttribute("fo:text-align", mPropList["fo:text-align"]->getStr());
	stylePropertiesOpen.write(pHandler);

	pHandler->endElement("style:list-level-properties");
	pHandler->endElement("text:list-level-style-number");
}

UnorderedListLevelStyle::UnorderedListLevelStyle(const librevenge::RVNGPropertyList &xPropList)
	: mPropList(xPropList)
{
}

void UnorderedListLevelStyle::write(OdfDocumentHandler *pHandler, int iLevel) const
{
	librevenge::RVNGString sLevel;
	sLevel.sprintf("%i", (iLevel+1));
	TagOpenElement listLevelStyleOpen("text:list-level-style-bullet");
	listLevelStyleOpen.addAttribute("text:level", sLevel);
	listLevelStyleOpen.addAttribute("text:style-name", "Bullet_Symbols");
	if (mPropList["text:bullet-char"] && (mPropList["text:bullet-char"]->getStr().len()))
	{
		// The following is needed because the ODF format does not accept bullet chars longer than one character
		librevenge::RVNGString::Iter i(mPropList["text:bullet-char"]->getStr());
		i.rewind();
		librevenge::RVNGString sEscapedString(".");
		if (i.next())
			sEscapedString = librevenge::RVNGString(i(), true);
		listLevelStyleOpen.addAttribute("text:bullet-char", sEscapedString);

	}
	else
		listLevelStyleOpen.addAttribute("text:bullet-char", ".");
	if (mPropList["text:display-levels"])
		listLevelStyleOpen.addAttribute("text:display-levels", mPropList["text:display-levels"]->getStr());
	listLevelStyleOpen.write(pHandler);

	TagOpenElement stylePropertiesOpen("style:list-level-properties");
	if (mPropList["text:space-before"] && mPropList["text:space-before"]->getDouble() > 0.0)
		stylePropertiesOpen.addAttribute("text:space-before", mPropList["text:space-before"]->getStr());
	if (mPropList["text:min-label-width"] && mPropList["text:min-label-width"]->getDouble() > 0.0)
		stylePropertiesOpen.addAttribute("text:min-label-width", mPropList["text:min-label-width"]->getStr());
	if (mPropList["text:min-label-distance"] && mPropList["text:min-label-distance"]->getDouble() > 0.0)
		stylePropertiesOpen.addAttribute("text:min-label-distance", mPropList["text:min-label-distance"]->getStr());
	if (mPropList["fo:text-align"])
		stylePropertiesOpen.addAttribute("fo:text-align", mPropList["fo:text-align"]->getStr());
	stylePropertiesOpen.addAttribute("style:font-name", "OpenSymbol");
	stylePropertiesOpen.write(pHandler);

	pHandler->endElement("style:list-level-properties");
	pHandler->endElement("text:list-level-style-bullet");
}

ListStyle::ListStyle(const char *psName, const int iListID) :
	Style(psName),
	mDisplayName(""),
	mxListLevels(),
	miListID(iListID)
{
}

ListStyle::~ListStyle()
{
	for (std::map<int, ListLevelStyle *>::iterator iter = mxListLevels.begin();
	        iter != mxListLevels.end(); ++iter)
	{
		if (iter->second)
			delete(iter->second);
	}

}

bool ListStyle::isListLevelDefined(int iLevel) const
{
	std::map<int, ListLevelStyle *>::const_iterator iter = mxListLevels.find(iLevel);
	if (iter == mxListLevels.end() || !iter->second)
		return false;

	return true;
}

void ListStyle::setListLevel(int iLevel, ListLevelStyle *iListLevelStyle)
{
	// can't uncomment this next line without adding some extra logic.
	// figure out which is best: use the initial message, or constantly
	// update?
	if (!isListLevelDefined(iLevel))
		mxListLevels[iLevel] = iListLevelStyle;
}

void ListStyle::updateListLevel(const int iLevel, const librevenge::RVNGPropertyList &xPropList, bool ordered)
{
	if (iLevel < 0)
		return;
	if (!isListLevelDefined(iLevel))
	{
		if (ordered)
			setListLevel(iLevel, new OrderedListLevelStyle(xPropList));
		else
			setListLevel(iLevel, new UnorderedListLevelStyle(xPropList));
	}
}

void ListStyle::write(OdfDocumentHandler *pHandler) const
{
	TagOpenElement listStyleOpenElement("text:list-style");
	listStyleOpenElement.addAttribute("style:name", getName());
	if (!mDisplayName.empty())
		listStyleOpenElement.addAttribute("style:display-name", mDisplayName);
	listStyleOpenElement.write(pHandler);

	for (std::map<int, ListLevelStyle *>::const_iterator iter = mxListLevels.begin();
	        iter != mxListLevels.end(); ++iter)
	{
		if (iter->second)
			iter->second->write(pHandler, iter->first);
	}

	pHandler->endElement("text:list-style");
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
