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
	msPageDrawingName(pageDrawingName)
{
	for (int i=0; i<C_NumContentTypes; ++i) mpContent[i]=0;
}

namespace
{
typedef std::vector<DocumentElement *>::iterator DEVIter;
}

PageSpan::~PageSpan()
{
	for (int i=0; i<C_NumContentTypes; ++i)
	{
		if (!mpContent[i]) continue;
		for (DEVIter iterHeaderContent = mpContent[i]->begin();
		        iterHeaderContent != mpContent[i]->end();
		        ++iterHeaderContent)
			delete(*iterHeaderContent);
		delete mpContent[i];
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

void PageSpan::storeContent(ContentType type, std::vector<DocumentElement *> *pContent)
{
	if (type<0||type>=C_NumContentTypes)
	{
		ODFGEN_DEBUG_MSG(("PageSpan::storeContent: the type seems bad\n"));
		// probably better to do not delete it
		return;
	}
	if (mpContent[type])
	{
		for (DEVIter iterHeaderContent = mpContent[type]->begin();
		        iterHeaderContent != mpContent[type]->end();
		        ++iterHeaderContent)
			delete(*iterHeaderContent);
		delete mpContent[type];
	}
	mpContent[type]=pContent;
}

void PageSpan::writePageStyle(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	librevenge::RVNGPropertyList propList;
	if (zone==Style::Z_ContentAutomatic)
	{
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
	}

	if (zone==Style::Z_StyleAutomatic && !msPageDrawingName.empty())
	{
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

	if (mpContent[C_Header])
		_writeContent("style:header", *mpContent[C_Header], pHandler);
	else if (mpContent[C_HeaderLeft] || mpContent[C_HeaderFirst] /* || mpContent[C_HeaderLast] */)
	{
		TagOpenElement("style:header").write(pHandler);
		TagCloseElement("style:header").write(pHandler);
	}
	if (mpContent[C_HeaderLeft])
		_writeContent("style:header-left", *mpContent[C_HeaderLeft], pHandler);
	if (mpContent[C_HeaderFirst])
		_writeContent("style:header-first", *mpContent[C_HeaderFirst], pHandler);
	/*
	if (mpContent[C_HeaderLast])
	    _writeContent("style:header-last", *mpContent[C_HeaderLast], pHandler);
	*/

	if (mpContent[C_Footer])
		_writeContent("style:footer", *mpContent[C_Footer], pHandler);
	else if (mpContent[C_FooterLeft] || mpContent[C_FooterFirst] /* || mpContent[C_FooterLast] */)
	{
		TagOpenElement("style:footer").write(pHandler);
		TagCloseElement("style:footer").write(pHandler);
	}
	if (mpContent[C_FooterLeft])
		_writeContent("style:footer-left", *mpContent[C_FooterLeft], pHandler);
	if (mpContent[C_FooterFirst])
		_writeContent("style:footer-first", *mpContent[C_FooterFirst], pHandler);
	/*
	if (mpContent[C_FooterLast])
	    _writeContent("style:footer-last", *mpContent[C_FooterLast], pHandler);
	*/

	if (mpContent[C_Master])
		_writeContent(0, *mpContent[C_Master], pHandler);
	pHandler->endElement("style:master-page");
}

void PageSpan::_writeContent(const char *contentTagName,
                             const std::vector<DocumentElement *> &content,
                             OdfDocumentHandler *pHandler) const
{
	bool hasTagName=contentTagName && strlen(contentTagName);
	if (hasTagName)
		TagOpenElement(contentTagName).write(pHandler);
	for (std::vector<DocumentElement *>::const_iterator iter = content.begin();
	        iter != content.end();
	        ++iter)
	{
		(*iter)->write(pHandler);
	}
	if (hasTagName)
		TagCloseElement(contentTagName).write(pHandler);
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
		propList.remove("librevenge:page-drawing-name");
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

void PageSpanManager::writePageStyles(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	/* We need to send each style only one time.

	   Actually, nobody defines the layout or the page-drawing-name, so can send the first data
	 */
	std::set<librevenge::RVNGString> done;
	for (size_t i=0; i<mpPageList.size(); ++i)
	{
		if (!mpPageList[i]) continue;
		librevenge::RVNGString name;
		if (zone==Style::Z_StyleAutomatic)
			name=mpPageList[i]->getLayoutName();
		else
			name=mpPageList[i]->getPageDrawingName();
		if (done.find(name)!=done.end())
			continue;
		done.insert(name);
		mpPageList[i]->writePageStyle(pHandler, zone);
	}
}

void PageSpanManager::writeMasterPages(OdfDocumentHandler *pHandler) const
{
	/* We need to send each master-page only one time.

	   As a master page must be open before a page, we can send the first page
	 */
	std::set<librevenge::RVNGString> done;
	for (size_t i=0; i<mpPageList.size(); ++i)
	{
		if (!mpPageList[i]) continue;
		librevenge::RVNGString name(mpPageList[i]->getMasterName());
		if (done.find(name)!=done.end())
			continue;
		done.insert(name);
		mpPageList[i]->writeMasterPages(pHandler);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
