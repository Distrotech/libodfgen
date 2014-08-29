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

PageSpan::PageSpan(const librevenge::RVNGPropertyList &xPropList, librevenge::RVNGString const &masterPageName, librevenge::RVNGString const &layoutName, librevenge::RVNGString const &pageDrawingName) :
	mxPropList(xPropList),
	msMasterPageName(masterPageName),
	msLayoutName(layoutName),
	msPageDrawingName(pageDrawingName),
	mpHeaderContent(0),
	mpFooterContent(0),
	mpHeaderLeftContent(0),
	mpFooterLeftContent(0),
	mpHeaderFirstContent(0),
	mpFooterFirstContent(0),
	mpHeaderLastContent(0),
	mpFooterLastContent(0)
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

	if (mpHeaderFirstContent)
	{
		for (DEVIter iterHeaderFirstContent = mpHeaderFirstContent->begin();
		        iterHeaderFirstContent != mpHeaderFirstContent->end();
		        ++iterHeaderFirstContent)
			delete(*iterHeaderFirstContent);
		delete mpHeaderFirstContent;
	}

	if (mpHeaderLastContent)
	{
		for (DEVIter iterHeaderLastContent = mpHeaderLastContent->begin();
		        iterHeaderLastContent != mpHeaderLastContent->end();
		        ++iterHeaderLastContent)
			delete(*iterHeaderLastContent);
		delete mpHeaderLastContent;
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

	if (mpFooterFirstContent)
	{
		for (DEVIter iterFooterFirstContent = mpFooterFirstContent->begin();
		        iterFooterFirstContent != mpFooterFirstContent->end();
		        ++iterFooterFirstContent)
			delete(*iterFooterFirstContent);
		delete mpFooterFirstContent;
	}

	if (mpFooterLastContent)
	{
		for (DEVIter iterFooterLastContent = mpFooterLastContent->begin();
		        iterFooterLastContent != mpFooterLastContent->end();
		        ++iterFooterLastContent)
			delete(*iterFooterLastContent);
		delete mpFooterLastContent;
	}
}

int PageSpan::getSpan() const
{
	if (mxPropList["librevenge:num-pages"])
		return mxPropList["librevenge:num-pages"]->getInt();

	return 0; // should never happen
}

void PageSpan::resetPageSizeAndMargins(double width, double height)
{
	mxPropList.insert("fo:page-width", width, librevenge::RVNG_INCH);
	mxPropList.insert("fo:page-height", height, librevenge::RVNG_INCH);
	mxPropList.insert("fo:margin-top", "0in");
	mxPropList.insert("fo:margin-bottom", "0in");
	mxPropList.insert("fo:margin-left", "0in");
	mxPropList.insert("fo:margin-right", "0in");
	mxPropList.insert("style:print-orientation", "portrait");
}

librevenge::RVNGString PageSpan::protectString(librevenge::RVNGString const &orig)
{
	librevenge::RVNGString res("");
	char const *str=orig.cstr();
	for (int i=0; i<orig.len(); ++i)
		res.append(str[i]==' ' ? '_' : str[i]);
	return res;
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

void PageSpan::setHeaderFirstContent(std::vector<DocumentElement *> *pHeaderContent)
{
	if (mpHeaderFirstContent)
	{
		for (DEVIter iterHeaderFirstContent = mpHeaderFirstContent->begin();
		        iterHeaderFirstContent != mpHeaderFirstContent->end();
		        ++iterHeaderFirstContent)
			delete(*iterHeaderFirstContent);
		delete mpHeaderFirstContent;
	}

	mpHeaderFirstContent = pHeaderContent;
}

void PageSpan::setFooterFirstContent(std::vector<DocumentElement *> *pFooterContent)
{
	if (mpFooterFirstContent)
	{
		for (DEVIter iterFooterFirstContent = mpFooterFirstContent->begin();
		        iterFooterFirstContent != mpFooterFirstContent->end();
		        ++iterFooterFirstContent)
			delete(*iterFooterFirstContent);
		delete mpFooterFirstContent;
	}

	mpFooterFirstContent = pFooterContent;
}

void PageSpan::setHeaderLastContent(std::vector<DocumentElement *> *pHeaderContent)
{
	if (mpHeaderLastContent)
	{
		for (DEVIter iterHeaderLastContent = mpHeaderLastContent->begin();
		        iterHeaderLastContent != mpHeaderLastContent->end();
		        ++iterHeaderLastContent)
			delete(*iterHeaderLastContent);
		delete mpHeaderLastContent;
	}

	mpHeaderLastContent = pHeaderContent;
}

void PageSpan::setFooterLastContent(std::vector<DocumentElement *> *pFooterContent)
{
	if (mpFooterLastContent)
	{
		for (DEVIter iterFooterLastContent = mpFooterLastContent->begin();
		        iterFooterLastContent != mpFooterLastContent->end();
		        ++iterFooterLastContent)
			delete(*iterFooterLastContent);
		delete mpFooterLastContent;
	}

	mpFooterLastContent = pFooterContent;
}

void PageSpan::writePageStyle(OdfDocumentHandler *pHandler) const
{
	librevenge::RVNGPropertyList propList;

	propList.insert("style:name", getLayoutName());
	pHandler->startElement("style:page-layout", propList);

	librevenge::RVNGPropertyList tempPropList;
	tempPropList.insert("style:writing-mode", librevenge::RVNGString("lr-tb"));
	tempPropList.insert("style:footnote-max-height", librevenge::RVNGString("0in"));
	librevenge::RVNGPropertyList::Iter i(mxPropList);
	for (i.rewind(); i.next();)
	{
		if (i.child() || strncmp(i.key(), "librevenge:", 11)==0 || strncmp(i.key(), "svg:", 4)==0)
			continue;
		tempPropList.insert(i.key(), i()->clone());
	}
	pHandler->startElement("style:page-layout-properties", tempPropList);

	librevenge::RVNGPropertyList footnoteSepPropList;
	if (mxPropList.child("librevenge:footnote"))
	{
		librevenge::RVNGPropertyListVector const *footnoteVector=mxPropList.child("librevenge:footnote");
		if (footnoteVector->count()==1)
			footnoteSepPropList=(*footnoteVector)[0];
		else if (footnoteVector->count())
		{
			ODFGEN_DEBUG_MSG(("PageSpan::writePageStyle: the footnote property list seems bad\n"));
		}
	}
	else
	{
		footnoteSepPropList.insert("style:width", librevenge::RVNGString("0.0071in"));
		footnoteSepPropList.insert("style:distance-before-sep", librevenge::RVNGString("0.0398in"));
		footnoteSepPropList.insert("style:distance-after-sep", librevenge::RVNGString("0.0398in"));
		footnoteSepPropList.insert("style:adjustment", librevenge::RVNGString("left"));
		footnoteSepPropList.insert("style:rel-width", librevenge::RVNGString("25%"));
		footnoteSepPropList.insert("style:color", librevenge::RVNGString("#000000"));
	}
	pHandler->startElement("style:footnote-sep", footnoteSepPropList);

	pHandler->endElement("style:footnote-sep");
	pHandler->endElement("style:page-layout-properties");
	pHandler->endElement("style:page-layout");

	if (msPageDrawingName.empty())
		return;
	propList.clear();
	propList.insert("style:name", getPageDrawingName());
	propList.insert("style:family", "drawing-page");
	pHandler->startElement("style:style", propList);

	if (mxPropList.child("librevenge:drawing-page") && mxPropList.child("librevenge:drawing-page")->count()==1)
	{
		pHandler->startElement("style:drawing-page-properties", (*mxPropList.child("librevenge:drawing-page"))[0]);
		pHandler->endElement("style:drawing-page-properties");
	}
	else if (mxPropList.child("librevenge:drawing-page"))
	{
		ODFGEN_DEBUG_MSG(("PageSpan::writePageStyle: the drawing page property list seems bad\n"));
	}
	pHandler->endElement("style:style");
}

void PageSpan::writeMasterPages(OdfDocumentHandler *pHandler) const
{
	TagOpenElement masterPageOpen("style:master-page");
	librevenge::RVNGString sMasterPageDisplayName(msMasterPageName);
	librevenge::RVNGString sMasterPageName=protectString(sMasterPageDisplayName);
	librevenge::RVNGPropertyList propList;
	propList.insert("style:name", sMasterPageName);
	if (sMasterPageDisplayName!=sMasterPageName)
		propList.insert("style:display-name", sMasterPageDisplayName);
	if (!msPageDrawingName.empty())
		propList.insert("draw:style-name", getPageDrawingName());
	/* we do not set any next-style to avoid problem when the input is
	   OpenPageSpan("A")
	      ... : many pages of text without any page break
	   ClosePageSpan()
	   OpenPageSpan("B")
	      ...
	   ClosePageSpan()

	   ie. in this case, we need either to set the next-style of A to A or to do not set next-style
	      but not set next-style to B if we do not want the second page of the document to have
		  the layout B.
	 */
	propList.insert("style:page-layout-name", getLayoutName());
	pHandler->startElement("style:master-page", propList);

	if (mpHeaderContent)
	{
		_writeHeaderFooter("style:header", *mpHeaderContent, pHandler);
		pHandler->endElement("style:header");
	}
	else if (mpHeaderLeftContent || mpHeaderFirstContent /* || mpHeaderLastContent */)
	{
		TagOpenElement("style:header").write(pHandler);
		pHandler->endElement("style:header");
	}
	if (mpHeaderLeftContent)
	{
		_writeHeaderFooter("style:header-left", *mpHeaderLeftContent, pHandler);
		pHandler->endElement("style:header-left");
	}
	if (mpHeaderFirstContent)
	{
		_writeHeaderFooter("style:header-first", *mpHeaderFirstContent, pHandler);
		pHandler->endElement("style:header-first");
	}
	/*
	if (mpHeaderLastContent)
	{
	    _writeHeaderFooter("style:header-last", *mpHeaderLastContent, pHandler);
		pHandler->endElement("style:header-last");
	}
	*/

	if (mpFooterContent)
	{
		_writeHeaderFooter("style:footer", *mpFooterContent, pHandler);
		pHandler->endElement("style:footer");
	}
	else if (mpFooterLeftContent || mpFooterFirstContent /* || mpFooterLastContent */)
	{
		TagOpenElement("style:footer").write(pHandler);
		pHandler->endElement("style:footer");
	}
	if (mpFooterLeftContent)
	{
		_writeHeaderFooter("style:footer-left", *mpFooterLeftContent, pHandler);
		pHandler->endElement("style:footer-left");
	}
	if (mpFooterFirstContent)
	{
		_writeHeaderFooter("style:footer-first", *mpFooterFirstContent, pHandler);
		pHandler->endElement("style:footer-first");
	}
	/*
	if (mpFooterLastContent)
	{
	    _writeHeaderFooter("style:footer-last", *mpFooterLastContent, pHandler);
		pHandler->endElement("style:footer-last");
	}
	*/

	pHandler->endElement("style:master-page");
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

//
// the page manager
//

void PageSpanManager::clean()
{
	mpPageList.clear();
}

PageSpan *PageSpanManager::add(const librevenge::RVNGPropertyList &xPropList)
{
	librevenge::RVNGPropertyList propList(xPropList);
	// first find the master-page name
	librevenge::RVNGString masterPageName("");
	if (xPropList["librevenge:master-page-name"])
	{
		masterPageName.appendEscapedXML(xPropList["librevenge:master-page-name"]->getStr());
		propList.remove("librevenge:master-page-name");
	}
	if (masterPageName.empty())
	{
		do
			masterPageName.sprintf("Page Style %i", ++miCurrentPageMasterIndex);
		while (mpPageMasterNameSet.find(masterPageName) != mpPageMasterNameSet.end());
	}
	mpPageMasterNameSet.insert(masterPageName);
	// now find the layout page name
	librevenge::RVNGString layoutName("");
	if (xPropList["librevenge:layout-name"])
	{
		layoutName.appendEscapedXML(xPropList["librevenge:layout-name"]->getStr());
		propList.remove("librevenge:layout-name");
	}
	if (layoutName.empty())
	{
		do
			layoutName.sprintf("PM%i", miCurrentLayoutIndex++);
		while (mpLayoutNameSet.find(layoutName) != mpLayoutNameSet.end());
	}
	// now find the page drawing style (if needed)
	librevenge::RVNGString pageDrawingName("");
	if (xPropList["librevenge:page-drawing-name"])
	{
		layoutName.appendEscapedXML(xPropList["librevenge:page-drawing-name"]->getStr());
		propList.remove("librevenge:page-drawing--name");
	}
	if (pageDrawingName.empty() && xPropList.child("librevenge:drawing-page"))
	{
		do
			pageDrawingName.sprintf("dp%i", ++miCurrentPageDrawingIndex);
		while (mpPageDrawingNameSet.find(pageDrawingName) != mpPageDrawingNameSet.end());
	}
	shared_ptr<PageSpan> page(new PageSpan(propList, masterPageName, layoutName, pageDrawingName));
	mpPageList.push_back(page);
	return page.get();
}

PageSpan *PageSpanManager::getCurrentPageSpan()
{
	if (mpPageList.empty())
	{
		ODFGEN_DEBUG_MSG(("PageSpanManager::getCurrentPageSpan: can not find any page span\n"));
		return 0;
	}
	return mpPageList.back().get();
}

void PageSpanManager::writePageStyles(OdfDocumentHandler *pHandler) const
{
	for (size_t i=0; i<mpPageList.size(); ++i)
	{
		if (mpPageList[i])
			mpPageList[i]->writePageStyle(pHandler);
	}
}

void PageSpanManager::writeMasterPages(OdfDocumentHandler *pHandler) const
{
	for (size_t i=0; i<mpPageList.size(); ++i)
	{
		if (mpPageList[i])
			mpPageList[i]->writeMasterPages(pHandler);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
