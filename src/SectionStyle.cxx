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

SectionStyle::SectionStyle(const librevenge::RVNGPropertyList &xPropList,
                           const char *psName, Style::Zone zone) :
	Style(psName, zone),
	mPropList(xPropList)
{
}

void SectionStyle::write(OdfDocumentHandler *pHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "section");
	styleOpen.write(pHandler);

	librevenge::RVNGPropertyList propList;
	librevenge::RVNGPropertyList::Iter p(mPropList);
	for (p.rewind(); p.next();)
	{
		if (strncmp(p.key(), "librevenge:", 11) && !p.child())
			propList.insert(p.key(), p()->getStr());
	}
	pHandler->startElement("style:section-properties", propList);

	// column properties
	librevenge::RVNGPropertyList columnProps;

	// if the number of columns is <= 1, we will never come here. This is only an additional check
	// style properties
	const librevenge::RVNGPropertyListVector *columns = mPropList.child("style:columns");
	if (columns && columns->count() > 1)
	{
		columnProps.insert("fo:column-count", (int)columns->count());
		pHandler->startElement("style:columns", columnProps);

		if (mPropList["librevenge:colsep-width"] && mPropList["librevenge:colsep-color"])
		{
			librevenge::RVNGPropertyList columnSeparator;
			columnSeparator.insert("style:width", mPropList["librevenge:colsep-width"]->getStr());
			columnSeparator.insert("style:color", mPropList["librevenge:colsep-color"]->getStr());
			if (mPropList["librevenge:colsep-height"])
				columnSeparator.insert("style:height", mPropList["librevenge:colsep-height"]->getStr());
			else
				columnSeparator.insert("style:height", "100%");
			if (mPropList["librevenge:colsep-vertical-align"])
				columnSeparator.insert("style:vertical-align", mPropList["librevenge:colsep-vertical-align"]->getStr());
			else
				columnSeparator.insert("style:vertical-align", "middle");
			pHandler->startElement("style:column-sep", columnSeparator);
			pHandler->endElement("style:column-sep");
		}
		librevenge::RVNGPropertyListVector::Iter i(*columns);
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

//
// the manager
//

void SectionStyleManager::clean()
{
	mStyleList.resize(0);
}

librevenge::RVNGString SectionStyleManager::add(const librevenge::RVNGPropertyList &propList, Style::Zone zone)
{
	if (zone==Style::Z_Unknown)
		zone=Style::Z_ContentAutomatic;
	librevenge::RVNGString name;
	if (zone==Style::Z_StyleAutomatic)
		name.sprintf("Section_M%i", mStyleList.size());
	else
		name.sprintf("Section%i", mStyleList.size());
	shared_ptr<SectionStyle> style(new SectionStyle(propList, name.cstr(), zone));
	mStyleList.push_back(style);
	return name;
}

void SectionStyleManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	for (size_t i=0; i<mStyleList.size(); ++i)
	{
		if (mStyleList[i] && mStyleList[i]->getZone()==zone)
			mStyleList[i]->write(pHandler);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
