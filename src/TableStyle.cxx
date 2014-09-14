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
	stylePropList.insert("fo:padding", 0.0382);
	for (i.rewind(); i.next();)
	{
		if (!strncmp(i.key(), "fo", 2))
			stylePropList.insert(i.key(), i()->clone());
		else if (!strncmp(i.key(), "style:border-line-width", 23))
		{
			if (!strcmp(i.key(), "style:border-line-width") ||
			        !strcmp(i.key(), "style:border-line-width-left") ||
			        !strcmp(i.key(), "style:border-line-width-right") ||
			        !strcmp(i.key(), "style:border-line-width-top") ||
			        !strcmp(i.key(), "style:border-line-width-bottom"))
				stylePropList.insert(i.key(), i()->clone());
		}
		else if (!strcmp(i.key(), "style:vertical-align"))
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


Table::Table(const librevenge::RVNGPropertyList &xPropList, const char *psName, Style::Zone zone) :
	Style(psName, zone), mPropList(xPropList),
	mbRowOpened(false), mbRowHeaderOpened(false), mbCellOpened(false),
	mRowNameHash(), mRowStyleHash(), mCellNameHash(), mCellStyleHash()
{
}

Table::~Table()
{
}

int Table::getNumColumns() const
{
	const librevenge::RVNGPropertyListVector *columns = mPropList.child("librevenge:table-columns");
	if (columns)
		return (int)(columns->count());
	return 0;
}

librevenge::RVNGString Table::openRow(const librevenge::RVNGPropertyList &propList)
{
	if (mbRowOpened)
	{
		ODFGEN_DEBUG_MSG(("Table::openRow: a row is already open\n"));
		return "";
	}
	mbRowOpened=true;
	mbRowHeaderOpened=propList["librevenge:is-header-row"] &&
	                  propList["librevenge:is-header-row"]->getInt();
	// first remove unused data
	librevenge::RVNGPropertyList pList;
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (strncmp(i.key(), "librevenge:", 11)==0)
			continue;
		if (i.child())
			continue;
		pList.insert(i.key(),i()->clone());
	}
	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mRowNameHash.find(hashKey);
	if (iter!=mRowNameHash.end()) return iter->second;

	librevenge::RVNGString name;
	name.sprintf("%s_row%i", getName().cstr(), (int) mRowStyleHash.size());
	mRowNameHash[hashKey]=name;
	mRowStyleHash[name]=shared_ptr<TableRowStyle>(new TableRowStyle(propList, name.cstr()));
	return name;
}

bool Table::closeRow()
{
	if (!mbRowOpened)
	{
		ODFGEN_DEBUG_MSG(("Table::closeRow: no row is open\n"));
		return false;
	}
	mbRowOpened=mbRowHeaderOpened=false;
	return true;
}

librevenge::RVNGString Table::openCell(const librevenge::RVNGPropertyList &propList)
{
	if (!mbRowOpened || mbCellOpened)
	{
		ODFGEN_DEBUG_MSG(("Table::openCell: can not open a cell\n"));
		return "";
	}
	mbCellOpened=true;
	// first remove unused data
	librevenge::RVNGPropertyList pList;
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (!strncmp(i.key(), "librevenge:", 11) &&
		        strncmp(i.key(), "librevenge:numbering-name", 24))
			continue;
		if (i.child())
			continue;
		pList.insert(i.key(),i()->clone());
	}
	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mCellNameHash.find(hashKey);
	if (iter!=mCellNameHash.end()) return iter->second;

	librevenge::RVNGString name;
	name.sprintf("%s_cell%i", getName().cstr(), (int) mCellStyleHash.size());
	mCellNameHash[hashKey]=name;
	mCellStyleHash[name]=shared_ptr<TableCellStyle>(new TableCellStyle(propList, name.cstr()));
	return name;
}

bool Table::closeCell()
{
	if (!mbCellOpened)
	{
		ODFGEN_DEBUG_MSG(("Table::closeCell: no cell are opened\n"));
		return false;
	}
	mbCellOpened=false;
	return true;
}

bool Table::insertCoveredCell(const librevenge::RVNGPropertyList &)
{
	if (!mbRowOpened || mbCellOpened)
	{
		ODFGEN_DEBUG_MSG(("Table::insertCoveredCell: can not open a cell\n"));
		return false;
	}
	return true;
}

void Table::write(OdfDocumentHandler *pHandler) const
{
	ODFGEN_DEBUG_MSG(("Table::write: default function must not be called\n"));
	write(pHandler, false);
}

void Table::write(OdfDocumentHandler *pHandler, bool compatibleOdp) const
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

	std::map<librevenge::RVNGString, shared_ptr<TableRowStyle> >::const_iterator rIt;
	for (rIt=mRowStyleHash.begin(); rIt!=mRowStyleHash.end(); ++rIt)
	{
		if (!rIt->second) continue;
		rIt->second->write(pHandler);
	}

	std::map<librevenge::RVNGString, shared_ptr<TableCellStyle> >::const_iterator cIt;
	for (cIt=mCellStyleHash.begin(); cIt!=mCellStyleHash.end(); ++cIt)
	{
		if (!cIt->second) continue;
		cIt->second->writeStyles(pHandler, compatibleOdp);
	}
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

bool TableManager::openTable(const librevenge::RVNGPropertyList &xPropList, Style::Zone zone)
{
	librevenge::RVNGString sTableName;
	if (zone==Style::Z_Unknown)
		zone=Style::Z_ContentAutomatic;
	if (zone==Style::Z_StyleAutomatic)
		sTableName.sprintf("Table_M%i", (int) mTableStyles.size());
	else
		sTableName.sprintf("Table%i", (int) mTableStyles.size());

	shared_ptr<Table> table(new Table(xPropList, sTableName.cstr(), zone));
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

void TableManager::write(OdfDocumentHandler *pHandler, Style::Zone zone, bool compatibleOdp) const
{
	for (size_t i=0; i < mTableStyles.size(); ++i)
	{
		if (mTableStyles[i] && mTableStyles[i]->getZone()==zone)
			mTableStyles[i]->write(pHandler, compatibleOdp);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
