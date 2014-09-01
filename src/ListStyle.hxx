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
#ifndef _LISTSTYLE_HXX_
#define _LISTSTYLE_HXX_

#include <map>
#include <stack>
#include <vector>
#include <librevenge/librevenge.h>

#include "Style.hxx"

class ListLevelStyle
{
public:
	virtual ~ListLevelStyle() {};
	virtual void write(OdfDocumentHandler *pHandler, int iLevel) const = 0;
};

class OrderedListLevelStyle : public ListLevelStyle
{
public:
	OrderedListLevelStyle(const librevenge::RVNGPropertyList &xPropList);
	void write(OdfDocumentHandler *pHandler, int iLevel) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class UnorderedListLevelStyle : public ListLevelStyle
{
public:
	UnorderedListLevelStyle(const librevenge::RVNGPropertyList &xPropList);
	void write(OdfDocumentHandler *pHandler, int iLevel) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class ListStyle : public Style
{
public:
	ListStyle(const char *psName, const int iListID, Style::Zone zone);
	virtual ~ListStyle();
	void updateListLevel(const int iLevel, const librevenge::RVNGPropertyList &xPropList, bool ordered);
	virtual void write(OdfDocumentHandler *pHandler) const;
	int getListID()
	{
		return miListID;
	}
	bool isListLevelDefined(int iLevel) const;
	librevenge::RVNGString getDisplayName() const
	{
		return mDisplayName;
	}
	void setDisplayName(const char *displayName=0)
	{
		if (!displayName || !*displayName)
			mDisplayName="";
		else
			mDisplayName = displayName;
	}

protected:
	void setListLevel(int iLevel, ListLevelStyle *iListLevelStyle);

private:
	ListStyle(const ListStyle &);
	ListStyle &operator=(const ListStyle &);
	//! the display name ( if defined)
	librevenge::RVNGString mDisplayName;
	//! the list id
	const int miListID;
	std::map<int, ListLevelStyle *> mxListLevels;
};

/** a list manager */
class ListManager
{
public:
	// list state
	struct State
	{
		State();
		State(const State &state);

		ListStyle *mpCurrentListStyle;
		unsigned int miCurrentListLevel;
		unsigned int miLastListLevel;
		unsigned int miLastListNumber;
		bool mbListContinueNumbering;
		bool mbListElementParagraphOpened;
		std::stack<bool> mbListElementOpened;
	private:
		State &operator=(const State &state);
	};

public:
	//! constructor
	ListManager();
	//! destructor
	virtual ~ListManager();

	/// call to define a list level
	void defineLevel(const librevenge::RVNGPropertyList &propList, bool ordered, Style::Zone zone);

	/// write all
	virtual void write(OdfDocumentHandler *pHandler) const
	{
		write(pHandler, Style::Z_Style);
		write(pHandler, Style::Z_StyleAutomatic);
		write(pHandler, Style::Z_ContentAutomatic);
	}
	// write automatic/named style
	void write(OdfDocumentHandler *pHandler, Style::Zone zone) const;

	/// access to the current list state
	State &getState();
	/// pop the list state (if possible)
	void popState();
	/// push the list state by adding an empty value
	void pushState();

protected:
	// list styles
	unsigned int miNumListStyles;
	// list styles
	std::vector<ListStyle *> mListStylesVector;
	// a map id -> last list style defined with id
	std::map<int, ListStyle *> mIdListStyleMap;

	//! list states
	std::stack<State> mStatesStack;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
