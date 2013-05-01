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

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef _TABLESTYLE_HXX_
#define _TABLESTYLE_HXX_
#include <libwpd/libwpd.h>
#include <vector>

#include "Style.hxx"

class OdfDocumentHandler;

class TableCellStyle : public Style
{
public:
	virtual ~TableCellStyle() {};
	TableCellStyle(const WPXPropertyList &xPropList, const char *psName);
	virtual void write(OdfDocumentHandler *pHandler) const;
private:
	WPXPropertyList mPropList;
};

class TableRowStyle : public Style
{
public:
	virtual ~TableRowStyle() {};
	TableRowStyle(const WPXPropertyList &propList, const char *psName);
	virtual void write(OdfDocumentHandler *pHandler) const;
private:
	WPXPropertyList mPropList;
};

class TableStyle : public Style, public TopLevelElementStyle
{
public:
	TableStyle(const WPXPropertyList &xPropList, const WPXPropertyListVector &columns, const char *psName);
	virtual ~TableStyle();
	virtual void write(OdfDocumentHandler *pHandler) const;
	int getNumColumns() const
	{
		return (int)mColumns.count();
	}
	void addTableCellStyle(TableCellStyle *pTableCellStyle)
	{
		mTableCellStyles.push_back(pTableCellStyle);
	}
	int getNumTableCellStyles()
	{
		return (int)mTableCellStyles.size();
	}
	void addTableRowStyle(TableRowStyle *pTableRowStyle)
	{
		mTableRowStyles.push_back(pTableRowStyle);
	}
	int getNumTableRowStyles()
	{
		return (int)mTableRowStyles.size();
	}
private:
	WPXPropertyList mPropList;
	WPXPropertyListVector mColumns;
	std::vector<TableCellStyle *> mTableCellStyles;
	std::vector<TableRowStyle *> mTableRowStyles;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
