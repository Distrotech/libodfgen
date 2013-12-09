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
#include <librevenge/librevenge.h>

#include <vector>

// for shared_ptr
#include "FilterInternal.hxx"

#include "Style.hxx"

class OdfDocumentHandler;

class TableCellStyle : public Style
{
public:
	virtual ~TableCellStyle() {};
	TableCellStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName);
	virtual void writeStyles(OdfDocumentHandler *pHandler, bool odpCompat=false) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class TableRowStyle : public Style
{
public:
	virtual ~TableRowStyle() {};
	TableRowStyle(const librevenge::RVNGPropertyList &propList, const char *psName);
	virtual void write(OdfDocumentHandler *pHandler) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class TableStyle : public Style, public TopLevelElementStyle
{
public:
	TableStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName);
	virtual ~TableStyle();
	virtual void writeStyles(OdfDocumentHandler *pHandler, bool compatibleOdp=false) const;
	int getNumColumns() const;

	librevenge::RVNGString openRow(const librevenge::RVNGPropertyList &propList);
	bool closeRow();
	bool isRowOpened(bool &inHeaderRow) const
	{
		inHeaderRow=mbRowHeaderOpened;
		return mbRowOpened;
	}
	librevenge::RVNGString openCell(const librevenge::RVNGPropertyList &propList);
	bool closeCell();
	bool insertCoveredCell(const librevenge::RVNGPropertyList &propList);
	bool isCellOpened() const
	{
		return mbCellOpened;
	}

private:
	bool mbRowOpened, mbRowHeaderOpened, mbCellOpened;
	librevenge::RVNGPropertyList mPropList;
	std::vector<TableCellStyle *> mTableCellStyles;
	std::vector<TableRowStyle *> mTableRowStyles;
};

class TableManager
{
public:
	TableManager();
	virtual ~TableManager();
	//! clean all data
	void clean();
	void write(OdfDocumentHandler *pHandler, bool compatibleOdp=false) const;

	bool isTableOpened() const
	{
		return !mTableOpened.empty();
	}
	TableStyle *getActualTable()
	{
		if (mTableOpened.empty()) return 0;
		return mTableOpened.back().get();
	}
	TableStyle const *getActualTable() const
	{
		if (mTableOpened.empty()) return 0;
		return mTableOpened.back().get();
	}
	//! open a table and update the list of elements
	bool openTable(const librevenge::RVNGPropertyList &xPropList);
	bool closeTable();

private:
	std::vector<shared_ptr<TableStyle> > mTableOpened;
	std::vector<shared_ptr<TableStyle> > mTableStyles;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
