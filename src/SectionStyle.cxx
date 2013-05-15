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
 * Copyright (c) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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
#include "SectionStyle.hxx"
#include "DocumentElement.hxx"
#include <math.h>

#ifdef _MSC_VER
double rint(double x);
#endif /* _WIN32 */

SectionStyle::SectionStyle(const WPXPropertyList &xPropList,
                           const WPXPropertyListVector &xColumns,
                           const char *psName) :
	Style(psName),
	mPropList(xPropList),
	mColumns(xColumns)
{
}

void SectionStyle::write(OdfDocumentHandler *pHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "section");
	styleOpen.write(pHandler);

	WPXPropertyList propList;
	WPXPropertyList::Iter p(mPropList);
	for (p.rewind(); p.next(); )
	{
		if (strncmp(p.key(), "libwpd:", 7) != 0)
			propList.insert(p.key(), p()->getStr());
	}
	pHandler->startElement("style:section-properties", propList);

	// column properties
	WPXPropertyList columnProps;

	// if the number of columns is <= 1, we will never come here. This is only an additional check
	// style properties
	if (mColumns.count() > 1)
	{
		columnProps.insert("fo:column-count", (int)mColumns.count());
		pHandler->startElement("style:columns", columnProps);

		if (mPropList["libwpd:colsep-width"] && mPropList["libwpd:colsep-color"])
		{
			WPXPropertyList columnSeparator;
			columnSeparator.insert("style:width", mPropList["libwpd:colsep-width"]->getStr());
			columnSeparator.insert("style:color", mPropList["libwpd:colsep-color"]->getStr());
			if (mPropList["libwpd:colsep-height"])
				columnSeparator.insert("style:height", mPropList["libwpd:colsep-height"]->getStr());
			else
				columnSeparator.insert("style:height", "100%");
			if (mPropList["libwpd:colsep-vertical-align"])
				columnSeparator.insert("style:vertical-align", mPropList["libwpd:colsep-vertical-align"]->getStr());
			else
				columnSeparator.insert("style:vertical-align", "middle");
			pHandler->startElement("style:column-sep", columnSeparator);
			pHandler->endElement("style:column-sep");
		}
		WPXPropertyListVector::Iter i(mColumns);
		for (i.rewind(); i.next();)
		{
			pHandler->startElement("style:column", i());
			pHandler->endElement("style:column");
		}
	}
	else
	{
		columnProps.insert("fo:column-count", 0);
		columnProps.insert("fo:column-gap", 0.0);
		pHandler->startElement("style:columns", columnProps);
	}

	pHandler->endElement("style:columns");


	pHandler->endElement("style:section-properties");

	pHandler->endElement("style:style");
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
