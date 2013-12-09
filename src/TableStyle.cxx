/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
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

#include <math.h>
#include "FilterInternal.hxx"
#include "TableStyle.hxx"
#include "DocumentElement.hxx"

#ifdef _MSC_VER
#include <minmax.h>
#endif

#include <string.h>

TableCellStyle::TableCellStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName) :
	Style(psName),
	mPropList(xPropList)
{
}

void TableCellStyle::writeStyles(OdfDocumentHandler *pHandler, bool compatibleOdp) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table-cell");
	styleOpen.write(pHandler);

	// WLACH_REFACTORING: Only temporary.. a much better solution is to
	// generalize this sort of thing into the "Style" superclass
	librevenge::RVNGPropertyList stylePropList;
	librevenge::RVNGPropertyList::Iter i(mPropList);
	/* first set padding, so that mPropList can redefine, if
	   mPropList["fo:padding"] is defined */
	stylePropList.insert("fo:padding", "0.0382in");
	for (i.rewind(); i.next();)
	{
		if (strncmp(i.key(), "fo", 2) == 0)
			stylePropList.insert(i.key(), i()->clone());
		else if (strncmp(i.key(), "style:border-line-width", 23) == 0)
		{
			if (strcmp(i.key(), "style:border-line-width") == 0 ||
			        strcmp(i.key(), "style:border-line-width-left") == 0 ||
			        strcmp(i.key(), "style:border-line-width-right") == 0 ||
			        strcmp(i.key(), "style:border-line-width-top") == 0 ||
			        strcmp(i.key(), "style:border-line-width-bottom") == 0)
				stylePropList.insert(i.key(), i()->clone());
		}
		else if (strcmp(i.key(), "style:vertical-align")==0)
			stylePropList.insert(i.key(), i()->clone());
	}
	pHandler->startElement("style:table-cell-properties", stylePropList);
	pHandler->endElement("style:table-cell-properties");

	if (compatibleOdp)
	{
		librevenge::RVNGPropertyList pList;
		pList.insert("fo:padding", "0.0382in");
		if (mPropList["draw:fill"])
			pList.insert("draw:fill", mPropList["draw:fill"]->getStr());
		if (mPropList["draw:fill-color"])
			pList.insert("draw:fill-color", mPropList["draw:fill-color"]->getStr());
		if (mPropList["fo:padding"])
			pList.insert("fo:padding", mPropList["fo:padding"]->getStr());
		if (mPropList["draw:textarea-horizontal-align"])
			pList.insert("draw:textarea-horizontal-align", mPropList["draw:textarea-horizontal-align"]->getStr());
		pHandler->startElement("style:graphic-properties", pList);
		pHandler->endElement("style:graphic-properties");

		// HACK to get visible borders
		if (mPropList["fo:border"])
		{
			pList.clear();
			pList.insert("fo:border", mPropList["fo:border"]->getStr());
			pHandler->startElement("style:paragraph-properties", pList);
			pHandler->endElement("style:paragraph-properties");
		}
	}
	pHandler->endElement("style:style");
}

TableRowStyle::TableRowStyle(const librevenge::RVNGPropertyList &propList, const char *psName) :
	Style(psName),
	mPropList(propList)
{
}

void TableRowStyle::write(OdfDocumentHandler *pHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table-row");
	styleOpen.write(pHandler);

	TagOpenElement stylePropertiesOpen("style:table-row-properties");
	if (mPropList["style:min-row-height"])
		stylePropertiesOpen.addAttribute("style:min-row-height", mPropList["style:min-row-height"]->getStr());
	else if (mPropList["style:row-height"])
		stylePropertiesOpen.addAttribute("style:row-height", mPropList["style:row-height"]->getStr());
	stylePropertiesOpen.addAttribute("fo:keep-together", "auto");
	stylePropertiesOpen.write(pHandler);
	pHandler->endElement("style:table-row-properties");

	pHandler->endElement("style:style");
}


TableStyle::TableStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName) :
	Style(psName),
	mbRowOpened(false), mbRowHeaderOpened(false), mbCellOpened(false),
	mPropList(xPropList),
	mTableCellStyles(),
	mTableRowStyles()
{
}

TableStyle::~TableStyle()
{
	typedef std::vector<TableCellStyle *>::iterator TCSVIter;
	typedef std::vector<TableRowStyle *>::iterator TRSVIter;
	for (TCSVIter iterTableCellStyles = mTableCellStyles.begin() ; iterTableCellStyles != mTableCellStyles.end(); ++iterTableCellStyles)
		delete(*iterTableCellStyles);
	for (TRSVIter iterTableRowStyles = mTableRowStyles.begin() ; iterTableRowStyles != mTableRowStyles.end(); ++iterTableRowStyles)
		delete(*iterTableRowStyles);
}

int TableStyle::getNumColumns() const
{
	const librevenge::RVNGPropertyListVector *columns = mPropList.child("librevenge:table-columns");
	if (columns)
		return (int)(columns->count());
	return 0;
}

librevenge::RVNGString TableStyle::openRow(const librevenge::RVNGPropertyList &propList)
{
	if (mbRowOpened)
	{
		ODFGEN_DEBUG_MSG(("TableStyle::openRow: a row is already open\n"));
		return "";
	}

	librevenge::RVNGString rowName;
	rowName.sprintf("%s.Row%i", getName().cstr(), (int)mTableRowStyles.size());
	mTableRowStyles.push_back(new TableRowStyle(propList, rowName.cstr()));
	mbRowOpened=true;
	mbRowHeaderOpened=propList["librevenge:is-header-row"] &&
	                  propList["librevenge:is-header-row"]->getInt();
	return rowName;
}

bool TableStyle::closeRow()
{
	if (!mbRowOpened)
	{
		ODFGEN_DEBUG_MSG(("TableStyle::closeRow: no row is open\n"));
		return false;
	}
	mbRowOpened=mbRowHeaderOpened=false;
	return true;
}

librevenge::RVNGString TableStyle::openCell(const librevenge::RVNGPropertyList &propList)
{
	if (!mbRowOpened || mbCellOpened)
	{
		ODFGEN_DEBUG_MSG(("TableStyle::openCell: can not open a cell\n"));
		return "";
	}

	librevenge::RVNGString cellName;
	cellName.sprintf("%s.Cell%i", getName().cstr(), (int) mTableCellStyles.size());
	mTableCellStyles.push_back(new TableCellStyle(propList, cellName.cstr()));
	mbCellOpened=true;
	return cellName;
}

bool TableStyle::closeCell()
{
	if (!mbCellOpened)
	{
		ODFGEN_DEBUG_MSG(("TableStyle::closeCell: no cell are opened\n"));
		return false;
	}
	mbCellOpened=false;
	return true;
}

bool TableStyle::insertCoveredCell(const librevenge::RVNGPropertyList &)
{
	if (!mbRowOpened || mbCellOpened)
	{
		ODFGEN_DEBUG_MSG(("TableStyle::insertCoveredCell: can not open a cell\n"));
		return false;
	}
	return true;
}

void TableStyle::writeStyles(OdfDocumentHandler *pHandler, bool compatibleOdp) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table");
	if (mPropList["style:master-page-name"])
		styleOpen.addAttribute("style:master-page-name", mPropList["style:master-page-name"]->getStr());
	styleOpen.write(pHandler);

	TagOpenElement stylePropertiesOpen("style:table-properties");
	if (mPropList["table:align"])
		stylePropertiesOpen.addAttribute("table:align", mPropList["table:align"]->getStr());
	if (mPropList["fo:margin-left"])
		stylePropertiesOpen.addAttribute("fo:margin-left", mPropList["fo:margin-left"]->getStr());
	if (mPropList["fo:margin-right"])
		stylePropertiesOpen.addAttribute("fo:margin-right", mPropList["fo:margin-right"]->getStr());
	if (mPropList["style:width"])
		stylePropertiesOpen.addAttribute("style:width", mPropList["style:width"]->getStr());
	if (mPropList["fo:break-before"])
		stylePropertiesOpen.addAttribute("fo:break-before", mPropList["fo:break-before"]->getStr());
	if (mPropList["table:border-model"])
		stylePropertiesOpen.addAttribute("table:border-model", mPropList["table:border-model"]->getStr());
	stylePropertiesOpen.write(pHandler);

	pHandler->endElement("style:table-properties");

	pHandler->endElement("style:style");

	const librevenge::RVNGPropertyListVector *columns = mPropList.child("librevenge:table-columns");
	if (columns && columns->count())
	{
		librevenge::RVNGPropertyListVector::Iter j(*columns);

		int i=1;
		for (j.rewind(); j.next(); ++i)
		{
			TagOpenElement columnStyleOpen("style:style");
			librevenge::RVNGString sColumnName;
			sColumnName.sprintf("%s.Column%i", getName().cstr(), i);
			columnStyleOpen.addAttribute("style:name", sColumnName);
			columnStyleOpen.addAttribute("style:family", "table-column");
			columnStyleOpen.write(pHandler);

			pHandler->startElement("style:table-column-properties", j());
			pHandler->endElement("style:table-column-properties");

			pHandler->endElement("style:style");
		}
	}
	else
	{
		TagOpenElement columnStyleOpen("style:style");
		librevenge::RVNGString sColumnName;
		sColumnName.sprintf("%s.Column0", getName().cstr());
		columnStyleOpen.addAttribute("style:name", sColumnName);
		columnStyleOpen.addAttribute("style:family", "table-column");
		columnStyleOpen.write(pHandler);

		pHandler->startElement("style:table-column-properties", librevenge::RVNGPropertyList());
		pHandler->endElement("style:table-column-properties");

		pHandler->endElement("style:style");
	}


	typedef std::vector<TableRowStyle *>::const_iterator TRSVIter;
	for (TRSVIter iterTableRow = mTableRowStyles.begin() ; iterTableRow != mTableRowStyles.end(); ++iterTableRow)
		(*iterTableRow)->write(pHandler);

	typedef std::vector<TableCellStyle *>::const_iterator TCSVIter;
	for (TCSVIter iterTableCell = mTableCellStyles.begin() ; iterTableCell != mTableCellStyles.end(); ++iterTableCell)
		(*iterTableCell)->writeStyles(pHandler, compatibleOdp);
}

TableManager::TableManager() : mTableOpened(), mTableStyles()
{
}

TableManager::~TableManager()
{
}

void TableManager::clean()
{
	mTableOpened.clear();
	mTableStyles.clear();
}

bool TableManager::openTable(const librevenge::RVNGPropertyList &xPropList)
{
	librevenge::RVNGString sTableName;
	sTableName.sprintf("Table%i", (int) mTableStyles.size());
	shared_ptr<TableStyle> table(new TableStyle(xPropList, sTableName.cstr()));
	mTableOpened.push_back(table);
	mTableStyles.push_back(table);
	return true;
}

bool TableManager::closeTable()
{
	if (mTableOpened.empty())
	{
		ODFGEN_DEBUG_MSG(("TableManager::oops: no table are opened\n"));
		return false;
	}
	mTableOpened.pop_back();
	return true;
}

void TableManager::write(OdfDocumentHandler *pHandler, bool compatibleOdp) const
{
	for (size_t i=0; i < mTableStyles.size(); ++i)
	{
		if (mTableStyles[i])
			mTableStyles[i]->writeStyles(pHandler, compatibleOdp);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
