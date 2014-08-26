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
		librevenge::RVNGString sEscapedString;
		sEscapedString.appendEscapedXML(mPropList["style:num-prefix"]->getStr());
		listLevelStyleOpen.addAttribute("style:num-prefix", sEscapedString);
	}
	if (mPropList["style:num-suffix"])
	{
		librevenge::RVNGString sEscapedString;
		sEscapedString.appendEscapedXML(mPropList["style:num-suffix"]->getStr());
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
		librevenge::RVNGString sEscapedString;
		if (i.next())
			sEscapedString.appendEscapedXML(i());
		else
			sEscapedString.append('.');
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

ListStyle::ListStyle(const char *psName, const int iListID, Style::Zone zone) :
	Style(psName, zone),
	mDisplayName(""),
	miListID(iListID),
	mxListLevels()
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

//
// list manager
//

ListManager::State::State() :
	mpCurrentListStyle(0),
	miCurrentListLevel(0),
	miLastListLevel(0),
	miLastListNumber(0),
	mbListContinueNumbering(false),
	mbListElementParagraphOpened(false),
	mbListElementOpened()
{
}

ListManager::State::State(const ListManager::State &state) :
	mpCurrentListStyle(state.mpCurrentListStyle),
	miCurrentListLevel(state.miCurrentListLevel),
	miLastListLevel(state.miCurrentListLevel),
	miLastListNumber(state.miLastListNumber),
	mbListContinueNumbering(state.mbListContinueNumbering),
	mbListElementParagraphOpened(state.mbListElementParagraphOpened),
	mbListElementOpened(state.mbListElementOpened)
{
}
ListManager::State &ListManager::getState()
{
	if (!mStatesStack.empty()) return mStatesStack.top();
	ODFGEN_DEBUG_MSG(("ListManager::getState: call with no state\n"));
	static ListManager::State bad;
	return bad;
}

void ListManager::popState()
{
	if (mStatesStack.size()>1)
		mStatesStack.pop();
}

void ListManager::pushState()
{
	mStatesStack.push(State());
}

//

ListManager::ListManager() : miNumListStyles(0), mListStylesVector(), mIdListStyleMap(), mStatesStack()
{
	mStatesStack.push(State());
}

ListManager::~ListManager()
{
	for (std::vector<ListStyle *>::iterator iterListStyles = mListStylesVector.begin();
	        iterListStyles != mListStylesVector.end(); ++iterListStyles)
		delete(*iterListStyles);
}

void ListManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	for (std::vector<ListStyle *>::const_iterator iterListStyles = mListStylesVector.begin(); iterListStyles != mListStylesVector.end(); ++iterListStyles)
	{
		if ((*iterListStyles)->getZone() == zone)
			(*iterListStyles)->write(pHandler);
	}

}

void ListManager::defineLevel(const librevenge::RVNGPropertyList &propList, bool ordered, Style::Zone zone)
{
	int id = -1;
	if (propList["librevenge:list-id"])
		id = propList["librevenge:list-id"]->getInt();

	ListStyle *pListStyle = 0;
	State &state=getState();
	// as all direct list have the same id:-1, we reused the last list
	// excepted at level 0 where we force the redefinition of a new list
	if ((id!=-1 || !state.mbListElementOpened.empty()) &&
	        state.mpCurrentListStyle && state.mpCurrentListStyle->getListID() == id)
		pListStyle = state.mpCurrentListStyle;

	// this rather appalling conditional makes sure we only start a
	// new list (rather than continue an old one) if: (1) we have no
	// prior list or the prior list has another listId OR (2) we can
	// tell that the user actually is starting a new list at level 1
	// (and only level 1)
	if (pListStyle == 0 ||
	        (ordered && propList["librevenge:level"] && propList["librevenge:level"]->getInt()==1 &&
	         (propList["text:start-value"] && propList["text:start-value"]->getInt() != int(state.miLastListNumber+1))))
	{
		// first retrieve the displayname
		librevenge::RVNGString displayName("");
		if (propList["style:display-name"])
			displayName=propList["style:display-name"]->getStr();
		else if (pListStyle)
			displayName=pListStyle->getDisplayName();

		ODFGEN_DEBUG_MSG(("ListManager:defineLevel Attempting to create a new list style (listid: %i)\n", id));
		// first check if we need to store the style as style or as automatic style
		if (propList["style:display-name"] && !propList["style:master-page-name"])
			zone=Style::Z_Style;
		else if (zone==Style::Z_Unknown)
			zone=Style::Z_ContentAutomatic;
		librevenge::RVNGString sName;
		if (zone==Style::Z_Style)
			sName.sprintf(ordered ? "OL_N%i" : "UL_N%i", miNumListStyles);
		else if (zone==Style::Z_StyleAutomatic)
			sName.sprintf(ordered ? "OL_M%i" : "UL_M%i", miNumListStyles);
		else
			sName.sprintf(ordered ? "OL%i" : "UL%i", miNumListStyles);
		miNumListStyles++;

		pListStyle = new ListStyle(sName.cstr(), id, zone);
		if (!displayName.empty())
			pListStyle->setDisplayName(displayName.cstr());

		mListStylesVector.push_back(pListStyle);
		state.mpCurrentListStyle = pListStyle;
		mIdListStyleMap[pListStyle->getListID()]=pListStyle;

		if (ordered)
		{
			state.mbListContinueNumbering = false;
			state.miLastListNumber = 0;
		}
	}
	else if (ordered)
		state.mbListContinueNumbering = true;

	if (!propList["librevenge:level"])
		return;
	// Iterate through ALL list styles with the same WordPerfect list id and define a level if it is not already defined
	// This solves certain problems with lists that start and finish without reaching certain levels and then begin again
	// and reach those levels. See gradguide0405_PC.wpd in the regression suite
	for (std::vector<ListStyle *>::iterator iterListStyles = mListStylesVector.begin(); iterListStyles != mListStylesVector.end(); ++iterListStyles)
	{
		if ((* iterListStyles) && (* iterListStyles)->getListID() == id)
			(* iterListStyles)->updateListLevel((propList["librevenge:level"]->getInt() - 1), propList, ordered);
	}
}


/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
