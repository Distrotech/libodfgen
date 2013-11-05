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
#include "PageSpan.hxx"
#include "DocumentElement.hxx"

PageSpan::PageSpan(const librevenge::RVNGPropertyList &xPropList) :
	mxPropList(xPropList),
	mpHeaderContent(0),
	mpFooterContent(0),
	mpHeaderLeftContent(0),
	mpFooterLeftContent(0)
{
}

namespace
{
typedef std::vector<DocumentElement *>::iterator DEVIter;
}

PageSpan::~PageSpan()
{
	if (mpHeaderContent)
	{
		for (DEVIter iterHeaderContent = mpHeaderContent->begin();
		        iterHeaderContent != mpHeaderContent->end();
		        ++iterHeaderContent)
			delete(*iterHeaderContent);
		delete mpHeaderContent;
	}

	if (mpHeaderLeftContent)
	{
		for (DEVIter iterHeaderLeftContent = mpHeaderLeftContent->begin();
		        iterHeaderLeftContent != mpHeaderLeftContent->end();
		        ++iterHeaderLeftContent)
			delete(*iterHeaderLeftContent);
		delete mpHeaderLeftContent;
	}

	if (mpFooterContent)
	{
		for (DEVIter iterFooterContent = mpFooterContent->begin();
		        iterFooterContent != mpFooterContent->end();
		        ++iterFooterContent)
			delete(*iterFooterContent);
		delete mpFooterContent;
	}

	if (mpFooterLeftContent)
	{
		for (DEVIter iterFooterLeftContent = mpFooterLeftContent->begin();
		        iterFooterLeftContent != mpFooterLeftContent->end();
		        ++iterFooterLeftContent)
			delete(*iterFooterLeftContent);
		delete mpFooterLeftContent;
	}
}

int PageSpan::getSpan() const
{
	if (mxPropList["librevenge:num-pages"])
		return mxPropList["librevenge:num-pages"]->getInt();

	return 0; // should never happen
}

void PageSpan::setHeaderContent(std::vector<DocumentElement *> *pHeaderContent)
{
	if (mpHeaderContent)
	{
		for (DEVIter iterHeaderContent = mpHeaderContent->begin();
		        iterHeaderContent != mpHeaderContent->end();
		        ++iterHeaderContent)
			delete(*iterHeaderContent);
		delete mpHeaderContent;
	}

	mpHeaderContent = pHeaderContent;
}

void PageSpan::setFooterContent(std::vector<DocumentElement *> *pFooterContent)
{
	if (mpFooterContent)
	{
		for (DEVIter iterFooterContent = mpFooterContent->begin();
		        iterFooterContent != mpFooterContent->end();
		        ++iterFooterContent)
			delete(*iterFooterContent);
		delete mpFooterContent;
	}

	mpFooterContent = pFooterContent;
}

void PageSpan::setHeaderLeftContent(std::vector<DocumentElement *> *pHeaderContent)
{
	if (mpHeaderLeftContent)
	{
		for (DEVIter iterHeaderLeftContent = mpHeaderLeftContent->begin();
		        iterHeaderLeftContent != mpHeaderLeftContent->end();
		        ++iterHeaderLeftContent)
			delete(*iterHeaderLeftContent);
		delete mpHeaderLeftContent;
	}

	mpHeaderLeftContent = pHeaderContent;
}

void PageSpan::setFooterLeftContent(std::vector<DocumentElement *> *pFooterContent)
{
	if (mpFooterLeftContent)
	{
		for (DEVIter iterFooterLeftContent = mpFooterLeftContent->begin();
		        iterFooterLeftContent != mpFooterLeftContent->end();
		        ++iterFooterLeftContent)
			delete(*iterFooterLeftContent);
		delete mpFooterLeftContent;
	}

	mpFooterLeftContent = pFooterContent;
}

void PageSpan::writePageLayout(const int iNum, OdfDocumentHandler *pHandler) const
{
	librevenge::RVNGPropertyList propList;

	librevenge::RVNGString sPageLayoutName;
	sPageLayoutName.sprintf("PM%i", iNum+2);
	propList.insert("style:name", sPageLayoutName);
	pHandler->startElement("style:page-layout", propList);

	librevenge::RVNGPropertyList tempPropList = mxPropList;
	if (!tempPropList["style:writing-mode"])
		tempPropList.insert("style:writing-mode", librevenge::RVNGString("lr-tb"));
	if (!tempPropList["style:footnote-max-height"])
		tempPropList.insert("style:footnote-max-height", librevenge::RVNGString("0in"));
	pHandler->startElement("style:page-layout-properties", tempPropList);

	librevenge::RVNGPropertyList footnoteSepPropList;
	footnoteSepPropList.insert("style:width", librevenge::RVNGString("0.0071in"));
	footnoteSepPropList.insert("style:distance-before-sep", librevenge::RVNGString("0.0398in"));
	footnoteSepPropList.insert("style:distance-after-sep", librevenge::RVNGString("0.0398in"));
	footnoteSepPropList.insert("style:adjustment", librevenge::RVNGString("left"));
	footnoteSepPropList.insert("style:rel-width", librevenge::RVNGString("25%"));
	footnoteSepPropList.insert("style:color", librevenge::RVNGString("#000000"));
	pHandler->startElement("style:footnote-sep", footnoteSepPropList);

	pHandler->endElement("style:footnote-sep");
	pHandler->endElement("style:page-layout-properties");
	pHandler->endElement("style:page-layout");
}

void PageSpan::writeMasterPages(const int iStartingNum, const int iPageLayoutNum, const bool bLastPageSpan,
                                OdfDocumentHandler *pHandler) const
{
	int iSpan = 0;
	(bLastPageSpan) ? iSpan = 1 : iSpan = getSpan();

	for (int i=iStartingNum; i<(iStartingNum+iSpan); ++i)
	{
		TagOpenElement masterPageOpen("style:master-page");
		librevenge::RVNGString sMasterPageName, sMasterPageDisplayName;
		sMasterPageName.sprintf("Page_Style_%i", i);
		sMasterPageDisplayName.sprintf("Page Style %i", i);
		librevenge::RVNGString sPageLayoutName;
		librevenge::RVNGPropertyList propList;
		sPageLayoutName.sprintf("PM%i", iPageLayoutNum+2);
		propList.insert("style:name", sMasterPageName);
		propList.insert("style:display-name", sMasterPageDisplayName);
		propList.insert("style:page-layout-name", sPageLayoutName);
		if (!bLastPageSpan)
		{
			librevenge::RVNGString sNextMasterPageName;
			sNextMasterPageName.sprintf("Page_Style_%i", (i+1));
			propList.insert("style:next-style-name", sNextMasterPageName);
		}
		pHandler->startElement("style:master-page", propList);

		if (mpHeaderContent)
		{
			_writeHeaderFooter("style:header", *mpHeaderContent, pHandler);
			pHandler->endElement("style:header");
			if (mpHeaderLeftContent)
			{
				_writeHeaderFooter("style:header-left", *mpHeaderLeftContent, pHandler);
				pHandler->endElement("style:header-left");
			}
		}
		else if (mpHeaderLeftContent)
		{
			TagOpenElement("style:header").write(pHandler);
			pHandler->endElement("style:header");
			_writeHeaderFooter("style:header-left", *mpHeaderLeftContent, pHandler);
			pHandler->endElement("style:header-left");
		}

		if (mpFooterContent)
		{
			_writeHeaderFooter("style:footer", *mpFooterContent, pHandler);
			pHandler->endElement("style:footer");
			if (mpFooterLeftContent)
			{
				_writeHeaderFooter("style:footer-left", *mpFooterLeftContent, pHandler);
				pHandler->endElement("style:footer-left");
			}
		}
		else if (mpFooterLeftContent)
		{
			TagOpenElement("style:footer").write(pHandler);
			pHandler->endElement("style:footer");
			_writeHeaderFooter("style:footer-left", *mpFooterLeftContent, pHandler);
			pHandler->endElement("style:footer-left");
		}

		pHandler->endElement("style:master-page");
	}
}

void PageSpan::_writeHeaderFooter(const char *headerFooterTagName,
                                  const std::vector<DocumentElement *> &headerFooterContent,
                                  OdfDocumentHandler *pHandler) const
{
	TagOpenElement headerFooterOpen(headerFooterTagName);
	headerFooterOpen.write(pHandler);
	for (std::vector<DocumentElement *>::const_iterator iter = headerFooterContent.begin();
	        iter != headerFooterContent.end();
	        ++iter)
	{
		(*iter)->write(pHandler);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
