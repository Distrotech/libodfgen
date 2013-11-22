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

#ifndef _SHEETSTYLE_HXX_
#define _SHEETSTYLE_HXX_
#include <librevenge/librevenge.h>
#include <map>
#include <string>
#include <vector>

#include "Style.hxx"

class OdfDocumentHandler;
class SheetManager;
class SheetStyle;

class SheetNumberingStyle : public Style
{
public:
	SheetNumberingStyle(const librevenge::RVNGPropertyList &xPropList, const librevenge::RVNGString &psName);
	virtual ~SheetNumberingStyle() {};
	void addCondition(const librevenge::RVNGPropertyList &condition)
	{
		mConditionsList.push_back(condition);
	}
	void writeStyle(OdfDocumentHandler *pHandler, SheetStyle const &sheet) const;

private:
	void writeCondition(librevenge::RVNGPropertyList const &propList, OdfDocumentHandler *pHandler, SheetStyle const &sheet) const;

	librevenge::RVNGPropertyList mPropList;
	std::vector<librevenge::RVNGPropertyList> mConditionsList;
};

class SheetCellStyle : public Style
{
public:
	virtual ~SheetCellStyle() {};
	SheetCellStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName);
	virtual void writeStyle(OdfDocumentHandler *pHandler, SheetStyle const &manager) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class SheetRowStyle : public Style
{
public:
	virtual ~SheetRowStyle() {};
	SheetRowStyle(const librevenge::RVNGPropertyList &propList, const char *psName);
	virtual void write(OdfDocumentHandler *pHandler) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class SheetStyle : public Style, public TopLevelElementStyle
{
public:
	SheetStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName);
	virtual ~SheetStyle();
	virtual void write(OdfDocumentHandler *pHandler) const;
	int getNumColumns() const
	{
		return mColumns ? (int)mColumns->count() : 0;
	}

	librevenge::RVNGString addCell(const librevenge::RVNGPropertyList &propList);
	librevenge::RVNGString addRow(const librevenge::RVNGPropertyList &propList);

	void addCondition(const librevenge::RVNGPropertyList &xPropList);

	void addNumberingStyle(const librevenge::RVNGPropertyList &xPropList);
	librevenge::RVNGString getNumberingStyleName(librevenge::RVNGString const &localName) const;

private:
	librevenge::RVNGPropertyList mPropList;
	librevenge::RVNGPropertyListVector const *mColumns;

	// hash key -> row style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mRowNameHash;
	// style name -> SheetRowStyle
	std::map<librevenge::RVNGString, shared_ptr<SheetRowStyle>, ltstr> mRowStyleHash;
	// hash key -> cell style name
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr> mCellNameHash;
	// style name -> SheetCellStyle
	std::map<librevenge::RVNGString, shared_ptr<SheetCellStyle>, ltstr> mCellStyleHash;
	// style name -> NumberingStyle
	std::map<librevenge::RVNGString, shared_ptr<SheetNumberingStyle>, ltstr> mNumberingHash;

	// Disable copying
	SheetStyle(const SheetStyle&);
	SheetStyle &operator=(const SheetStyle&);
};

class SheetManager
{
public:
	SheetManager();
	virtual ~SheetManager();
	//! clean all data
	void clean();
	void write(OdfDocumentHandler *pHandler) const;

	bool isSheetOpened() const
	{
		return mbSheetOpened;
	}
	SheetStyle *actualSheet()
	{
		if (!mbSheetOpened || !mSheetStyles.back()) return 0;
		return mSheetStyles.back().get();
	}
	//! open a sheet and update the list of elements
	bool openSheet(const librevenge::RVNGPropertyList &xPropList);
	bool closeSheet();

	static librevenge::RVNGString convertFormula(const librevenge::RVNGPropertyListVector &formatsList);

private:
	bool mbSheetOpened;
	std::vector<shared_ptr<SheetStyle> > mSheetStyles;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
