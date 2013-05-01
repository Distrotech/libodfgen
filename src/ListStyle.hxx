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
#include <libwpd/libwpd.h>

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
	OrderedListLevelStyle(const WPXPropertyList &xPropList);
	void write(OdfDocumentHandler *pHandler, int iLevel) const;
private:
	WPXPropertyList mPropList;
};

class UnorderedListLevelStyle : public ListLevelStyle
{
public:
	UnorderedListLevelStyle(const WPXPropertyList &xPropList);
	void write(OdfDocumentHandler *pHandler, int iLevel) const;
private:
	WPXPropertyList mPropList;
};

class ListStyle : public Style
{
public:
	ListStyle(const char *psName, const int iListID);
	virtual ~ListStyle();
	void updateListLevel(const int iLevel, const WPXPropertyList &xPropList, bool ordered);
	virtual void write(OdfDocumentHandler *pHandler) const;
	int getListID()
	{
		return miListID;
	}
	bool isListLevelDefined(int iLevel) const;

protected:
	void setListLevel(int iLevel, ListLevelStyle *iListLevelStyle);

private:
	ListStyle(const ListStyle &);
	ListStyle &operator=(const ListStyle &);
	std::map<int, ListLevelStyle *> mxListLevels;
	const int miListID;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
