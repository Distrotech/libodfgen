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

#include <string.h>

//
// page drawing style
//
PageDrawingStyle::PageDrawingStyle(const librevenge::RVNGPropertyList &pPropList, const librevenge::RVNGString &sName, Style::Zone zone) : Style(sName, zone),
	mpPropList(pPropList)
{
}

PageDrawingStyle::~PageDrawingStyle()
{
}

void PageDrawingStyle::write(OdfDocumentHandler *pHandler) const
{
	librevenge::RVNGPropertyList propList;
	propList.insert("style:name", getName());
	if (mpPropList["style:display-name"])
		propList.insert("style:display-name", mpPropList["style:display-name"]);
	propList.insert("style:family", "drawing-page");
	pHandler->startElement("style:style", propList);

	propList.clear();
	librevenge::RVNGPropertyList::Iter i(mpPropList);
	for (i.rewind(); i.next();)
	{
		if (i.child() || strcmp(i.key(), "style:display-name")==0 ||
		        strncmp(i.key(), "librevenge:",11)==0) continue;
		propList.insert(i.key(), i()->clone());
	}
	pHandler->startElement("style:drawing-page-properties", propList);
	pHandler->endElement("style:drawing-page-properties");

	pHandler->endElement("style:style");
}

//
// page layout style
//
PageLayoutStyle::PageLayoutStyle(const librevenge::RVNGPropertyList &pPropList, const librevenge::RVNGString &sName, Style::Zone zone) : Style(sName, zone),
	mpPropList(pPropList)
{
}

PageLayoutStyle::~PageLayoutStyle()
{
}

void PageLayoutStyle::write(OdfDocumentHandler *pHandler) const
{
	librevenge::RVNGPropertyList propList;
	propList.insert("style:name", getName());
	if (mpPropList["style:display-name"])
		propList.insert("style:display-name", mpPropList["style:display-name"]);
	pHandler->startElement("style:page-layout", propList);

	librevenge::RVNGPropertyList tempPropList;
	tempPropList.insert("style:writing-mode", librevenge::RVNGString("lr-tb"));
	tempPropList.insert("style:footnote-max-height", librevenge::RVNGString("0in"));
	librevenge::RVNGPropertyList::Iter i(mpPropList);
	for (i.rewind(); i.next();)
	{
		if (i.child() || strncmp(i.key(), "librevenge:", 11)==0 || strncmp(i.key(), "svg:", 4)==0)
			continue;
		tempPropList.insert(i.key(), i()->clone());
	}
	pHandler->startElement("style:page-layout-properties", tempPropList);

	librevenge::RVNGPropertyList footnoteSepPropList;
	if (mpPropList.child("librevenge:footnote"))
	{
		librevenge::RVNGPropertyListVector const *footnoteVector=mpPropList.child("librevenge:footnote");
		if (footnoteVector->count()==1)
			footnoteSepPropList=(*footnoteVector)[0];
		else if (footnoteVector->count())
		{
			ODFGEN_DEBUG_MSG(("PageLayoutStyle::write: the footnote property list seems bad\n"));
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

//
// page span
//
PageSpan::PageSpan(const librevenge::RVNGPropertyList &xPropList, librevenge::RVNGString const &masterName, librevenge::RVNGString const &masterDisplay, bool isMasterPage) :
	mxPropList(xPropList),
	mbIsMasterPage(isMasterPage),
	msMasterName(masterName),
	msMasterDisplay(masterDisplay),
	msLayoutName(""),
	msDrawingName("")
{
	for (int i=0; i<C_NumContentTypes; ++i) mpContent[i]=0;
}

PageSpan::~PageSpan()
{
	for (int i=0; i<C_NumContentTypes; ++i)
	{
		if (mpContent[i]) delete mpContent[i];
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

void PageSpan::storeContent(ContentType type, libodfgen::DocumentElementVector *pContent)
{
	if (type<0||type>=C_NumContentTypes)
	{
		ODFGEN_DEBUG_MSG(("PageSpan::storeContent: the type seems bad\n"));
		// probably better to do not delete it
		return;
	}
	if (mpContent[type])
		delete mpContent[type];
	mpContent[type]=pContent;
}

void PageSpan::writeMasterPages(OdfDocumentHandler *pHandler) const
{
	TagOpenElement masterOpen("style:master-page");
	librevenge::RVNGPropertyList propList;
	propList.insert("style:name", msMasterName);
	if (!msMasterDisplay.empty() && msMasterDisplay!=msMasterName)
		propList.insert("style:display-name", msMasterDisplay);
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
	if (!msDrawingName.empty())
		propList.insert("draw:style-name", getDrawingName());
	if (!msLayoutName.empty())
		propList.insert("style:page-layout-name", msLayoutName);
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

	if (mpContent[C_Master] && mbIsMasterPage)
		_writeContent(0, *mpContent[C_Master], pHandler);
	pHandler->endElement("style:master-page");
}

void PageSpan::_writeContent(const char *contentTagName,
                             const libodfgen::DocumentElementVector &content,
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

PageSpan *PageSpanManager::get(librevenge::RVNGString const &name)
{
	librevenge::RVNGString masterName("");
	masterName.appendEscapedXML(name);
	if (mpNameToMasterMap.find(masterName)==mpNameToMasterMap.end())
	{
		ODFGEN_DEBUG_MSG(("PageSpan::get: can not find a master page name\n"));
		return 0;
	}
	return mpNameToMasterMap.find(masterName)->second.get();
}

PageSpan *PageSpanManager::add(const librevenge::RVNGPropertyList &xPropList, bool isMasterPage)
{
	librevenge::RVNGPropertyList propList(xPropList);
	// first find the master-page name
	librevenge::RVNGString masterName("");
	if (xPropList["librevenge:master-page-name"])
	{
		masterName.appendEscapedXML(xPropList["librevenge:master-page-name"]->getStr());
		propList.remove("librevenge:master-page-name");
	}
	if (isMasterPage)
	{
		if (masterName.empty())
		{
			ODFGEN_DEBUG_MSG(("PageSpan::add: can not find the master page name\n"));
			return 0;
		}
		if (mpNameToMasterMap.find(masterName)!=mpNameToMasterMap.end())
		{
			ODFGEN_DEBUG_MSG(("PageSpan::add: a master page already exists with the same name\n"));
			return 0;
		}
	}
	if (masterName.empty())
	{
		do
			masterName.sprintf("Page Style %i", ++miCurrentMasterIndex);
		while (mpMasterNameSet.find(masterName) != mpMasterNameSet.end());
	}
	mpMasterNameSet.insert(masterName);

	shared_ptr<PageSpan> page(new PageSpan(propList, PageSpan::protectString(masterName), masterName, isMasterPage));
	mpPageList.push_back(page);
	if (isMasterPage)
		mpNameToMasterMap[masterName]=page;

	// now find the layout page name
	page->setLayoutName(findOrAddLayout(xPropList));
	// now find the page drawing style (if needed)
	librevenge::RVNGString drawingName=findOrAddDrawing(xPropList, isMasterPage);
	if (!drawingName.empty())
		page->setDrawingName(drawingName);
	return page.get();
}

librevenge::RVNGString PageSpanManager::findOrAddDrawing(const librevenge::RVNGPropertyList &propList, bool isMaster)
{
	if (!propList["librevenge:drawing-name"] && !propList.child("librevenge:drawing-page"))
		return 0;

	librevenge::RVNGString drawingName("");
	Style::Zone zone=isMaster ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic;
	if (propList["librevenge:drawing-name"])
	{
		drawingName.appendEscapedXML(propList["librevenge:drawing-name"]->getStr());
		if (mpNameToDrawingMap.find(drawingName)!=mpNameToDrawingMap.end()
		        && mpNameToDrawingMap.find(drawingName)->second)
			return mpNameToDrawingMap.find(drawingName)->second->getName();
		zone=Style::Z_Style;
	}

	librevenge::RVNGPropertyList drawingList;
	if (!propList.child("librevenge:drawing-page"))
	{
		ODFGEN_DEBUG_MSG(("PageSpanManager::findOrAddDrawing: can not find the drawing definition"));
	}
	else if (propList.child("librevenge:drawing-page")->count()>=1)
		drawingList=(*propList.child("librevenge:drawing-page"))[0];
	if (!drawingName.empty())
		drawingList.insert("style:display-name", drawingName);
	drawingList.insert("librevenge:zone-style", int(zone));

	librevenge::RVNGString hashKey = drawingList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mHashDrawingMap.find(hashKey);
	if (iter!=mHashDrawingMap.end()) return iter->second;

	librevenge::RVNGString finalName("");
	finalName.sprintf("DP%i", (int) mpDrawingList.size()+1);
	mHashDrawingMap[hashKey]=finalName;
	shared_ptr<PageDrawingStyle> style(new PageDrawingStyle(drawingList, finalName, zone));
	mpDrawingList.push_back(style);
	if (!drawingName.empty())
		mpNameToDrawingMap[drawingName]=style;
	return finalName;
}

librevenge::RVNGString PageSpanManager::findOrAddLayout(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGString layoutName("");
	Style::Zone zone=Style::Z_StyleAutomatic;
	if (propList["librevenge:layout-name"])
	{
		layoutName.appendEscapedXML(propList["librevenge:layout-name"]->getStr());
		if (mpNameToLayoutMap.find(layoutName)!=mpNameToLayoutMap.end()
		        && mpNameToLayoutMap.find(layoutName)->second)
			return mpNameToLayoutMap.find(layoutName)->second->getName();
		zone=Style::Z_Style;
	}

	librevenge::RVNGPropertyList layoutList;
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (i.child() || strcmp(i.key(), "style:display-name")==0 ||
		        strncmp(i.key(), "librevenge:",11)==0) continue;
		layoutList.insert(i.key(), i()->clone());
	}
	if (propList.child("librevenge:footnote"))
		layoutList.insert("librevenge:footnote", *propList.child("librevenge:footnote"));

	if (!layoutName.empty())
		layoutList.insert("style:display-name", layoutName);
	layoutList.insert("librevenge:zone-style", int(zone));

	librevenge::RVNGString hashKey = layoutList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mHashLayoutMap.find(hashKey);
	if (iter!=mHashLayoutMap.end()) return iter->second;

	librevenge::RVNGString finalName("");
	finalName.sprintf("PL%i", (int) mpLayoutList.size());
	mHashLayoutMap[hashKey]=finalName;
	shared_ptr<PageLayoutStyle> style(new PageLayoutStyle(layoutList, finalName, zone));
	mpLayoutList.push_back(style);
	if (!layoutName.empty())
		mpNameToLayoutMap[layoutName]=style;
	return finalName;
}

void PageSpanManager::writePageStyles(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	/* We need to send each style only one time.

	   Actually, nobody defines the layout or the page-drawing-name, so can send the first data
	 */

	// first the layout
	for (size_t i=0; i<mpLayoutList.size(); ++i)
	{
		if (!mpLayoutList[i] || mpLayoutList[i]->getZone()!=zone) continue;
		mpLayoutList[i]->write(pHandler);
	}
	// now the drawing style
	for (size_t i=0; i<mpDrawingList.size(); ++i)
	{
		if (!mpDrawingList[i] || mpDrawingList[i]->getZone()!=zone) continue;
		mpDrawingList[i]->write(pHandler);
	}
}

void PageSpanManager::writeMasterPages(OdfDocumentHandler *pHandler) const
{
	/* We need to send each master-page only one time. */
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

void PageSpanManager::resetPageSizeAndMargins(double width, double height)
{
	for (size_t i=0; i<mpPageList.size(); ++i)
	{
		if (!mpPageList[i]) continue;
		mpPageList[i]->resetPageSizeAndMargins(width, height);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
