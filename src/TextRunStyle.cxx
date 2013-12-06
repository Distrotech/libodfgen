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
#include "FilterInternal.hxx"
#include "TextRunStyle.hxx"
#include "DocumentElement.hxx"

#ifdef _MSC_VER
#include <minmax.h>
#endif

#include <string.h>

ParagraphStyle::ParagraphStyle(const librevenge::RVNGPropertyList &pPropList, const librevenge::RVNGString &sName) :
	mpPropList(pPropList),
	msName(sName)
{
}

ParagraphStyle::~ParagraphStyle()
{
}

bool ParagraphStyle::hasDisplayName() const
{
	return mpPropList["style:display-name"] && !mpPropList["style:display-name"]->getStr().empty();
}

void ParagraphStyle::write(OdfDocumentHandler *pHandler) const
{
	ODFGEN_DEBUG_MSG(("ParagraphStyle: Writing a paragraph style..\n"));

	librevenge::RVNGPropertyList propList;
	propList.insert("style:name", msName.cstr());
	propList.insert("style:family", "paragraph");
	if (mpPropList["style:display-name"])
		propList.insert("style:display-name", mpPropList["style:display-name"]->getStr());
	if (mpPropList["style:parent-style-name"])
		propList.insert("style:parent-style-name", mpPropList["style:parent-style-name"]->getStr());
	if (mpPropList["style:master-page-name"])
		propList.insert("style:master-page-name", mpPropList["style:master-page-name"]->getStr());
	pHandler->startElement("style:style", propList);

	propList.clear();
	librevenge::RVNGPropertyList::Iter i(mpPropList);
	for (i.rewind(); i.next();)
	{
		if (strncmp(i.key(), "fo:margin-",10) == 0)
		{
			if (strcmp(i.key(), "fo:margin-left") == 0 ||
			        strcmp(i.key(), "fo:margin-right") == 0 ||
			        strcmp(i.key(), "fo:margin-top") == 0)
				propList.insert(i.key(), i()->getStr());
			else if (strcmp(i.key(), "fo:margin-bottom") == 0)
			{
				if (i()->getDouble() > 0.0)
					propList.insert("fo:margin-bottom", i()->getStr());
				else
					propList.insert("fo:margin-bottom", 0.0);
			}
		}
		else if (strcmp(i.key(), "fo:text-indent") == 0)
			propList.insert("fo:text-indent", i()->getStr());
		else if (strcmp(i.key(), "fo:line-height") == 0)
			propList.insert("fo:line-height", i()->getStr());
		else if (strcmp(i.key(), "style:line-height-at-least") == 0)
			propList.insert("style:line-height-at-least", i()->getStr());
		else if (strcmp(i.key(), "fo:break-before") == 0)
			propList.insert("fo:break-before", i()->getStr());
		else if (strcmp(i.key(), "fo:text-align") == 0)
			propList.insert("fo:text-align", i()->getStr());
		else if (strcmp(i.key(), "fo:text-align-last") == 0)
			propList.insert("fo:text-align-last", i()->getStr());
		else if (strcmp(i.key(), "style:page-number") == 0)
			propList.insert("style:page-number", i()->getStr());
		else if (strcmp(i.key(), "fo:background-color") == 0)
			propList.insert("fo:background-color", i()->getStr());
		else if (strncmp(i.key(), "style:border-line-width", 23) == 0)
		{
			if (strcmp(i.key(), "style:border-line-width") == 0 ||
			        strcmp(i.key(), "style:border-line-width-left") == 0 ||
			        strcmp(i.key(), "style:border-line-width-right") == 0 ||
			        strcmp(i.key(), "style:border-line-width-top") == 0 ||
			        strcmp(i.key(), "style:border-line-width-bottom") == 0)
				propList.insert(i.key(), i()->getStr());
		}
		else if (strncmp(i.key(), "fo:border", 9) == 0)
		{
			if (strcmp(i.key(), "fo:border") == 0 ||
			        strcmp(i.key(), "fo:border-left") == 0 ||
			        strcmp(i.key(), "fo:border-right") == 0 ||
			        strcmp(i.key(), "fo:border-top") == 0 ||
			        strcmp(i.key(), "fo:border-bottom") == 0)
				propList.insert(i.key(), i()->getStr());
		}
		else if (strcmp(i.key(), "fo:keep-together") == 0)
			propList.insert("fo:keep-together", i()->getStr());
		else if (strcmp(i.key(), "fo:keep-with-next") == 0)
			propList.insert("fo:keep-with-next", i()->getStr());
	}

	propList.insert("style:justify-single-word", "false");
	pHandler->startElement("style:paragraph-properties", propList);

	const librevenge::RVNGPropertyListVector *pTabStops = mpPropList.child("style:tab-stops");
	if (pTabStops && pTabStops->count())
	{
		TagOpenElement tabListOpen("style:tab-stops");
		tabListOpen.write(pHandler);
		librevenge::RVNGPropertyListVector::Iter k(*pTabStops);
		for (k.rewind(); k.next();)
		{
			if (k()["style:position"] && k()["style:position"]->getDouble() < 0.0)
				continue;
			TagOpenElement tabStopOpen("style:tab-stop");

			librevenge::RVNGPropertyList::Iter j(k());
			for (j.rewind(); j.next();)
			{
				tabStopOpen.addAttribute(j.key(), j()->getStr().cstr());
			}
			tabStopOpen.write(pHandler);
			pHandler->endElement("style:tab-stop");
		}
		pHandler->endElement("style:tab-stops");
	}

	pHandler->endElement("style:paragraph-properties");
	pHandler->endElement("style:style");
}

SpanStyle::SpanStyle(const char *psName, const librevenge::RVNGPropertyList &xPropList) :
	Style(psName),
	mPropList(xPropList)
{
}

bool SpanStyle::hasDisplayName() const
{
	return mPropList["style:display-name"] && !mPropList["style:display-name"]->getStr().empty();
}

void SpanStyle::write(OdfDocumentHandler *pHandler) const
{
	librevenge::RVNGPropertyList propList(mPropList);

	ODFGEN_DEBUG_MSG(("SpanStyle: Writing a span style..\n"));
	librevenge::RVNGPropertyList styleOpenList;
	styleOpenList.insert("style:name", getName());
	if (mPropList["style:display-name"])
	{
		styleOpenList.insert("style:display-name", mPropList["style:display-name"]->getStr());
		propList.remove("style:display-name");
	}
	styleOpenList.insert("style:family", "text");
	pHandler->startElement("style:style", styleOpenList);


	if (mPropList["style:font-name"])
	{
		propList.insert("style:font-name-asian", mPropList["style:font-name"]->getStr());
		propList.insert("style:font-name-complex", mPropList["style:font-name"]->getStr());
	}

	if (mPropList["fo:font-size"])
	{
		if (mPropList["fo:font-size"]->getDouble() > 0.0)
		{
			propList.insert("style:font-size-asian", mPropList["fo:font-size"]->getStr());
			propList.insert("style:font-size-complex", mPropList["fo:font-size"]->getStr());
		}
		else
			propList.remove("fo:font-size");
	}

	if (mPropList["fo:font-weight"])
	{
		propList.insert("style:font-weight-asian", mPropList["fo:font-weight"]->getStr());
		propList.insert("style:font-weight-complex", mPropList["fo:font-weight"]->getStr());
	}

	if (mPropList["fo:font-style"])
	{
		propList.insert("style:font-style-asian", mPropList["fo:font-style"]->getStr());
		propList.insert("style:font-style-complex", mPropList["fo:font-style"]->getStr());
	}
	pHandler->startElement("style:text-properties", propList);

	pHandler->endElement("style:text-properties");
	pHandler->endElement("style:style");
}

void ParagraphStyleManager::clean()
{
	mHashNameMap.clear();
	mStyleHash.clear();
	mDisplayNameMap.clear();
}

void ParagraphStyleManager::write(OdfDocumentHandler *pHandler, bool automatic) const
{
	for (std::map<librevenge::RVNGString, shared_ptr<ParagraphStyle>, ltstr>::const_iterator iter = mStyleHash.begin();
	        iter != mStyleHash.end(); ++iter)
	{
		if (iter->second && iter->second->hasDisplayName()!=automatic)
			(iter->second)->write(pHandler);
	}
}

librevenge::RVNGString ParagraphStyleManager::findOrAdd(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGString hashKey = propList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr>::const_iterator iter =
	    mHashNameMap.find(hashKey);
	if (iter!=mHashNameMap.end()) return iter->second;

	// ok create a new list
	librevenge::RVNGString sName("");
	ODFGEN_DEBUG_MSG(("ParagraphStyleManager::findOrAdd: Paragraph Hash Key: %s\n", hashKey.cstr()));
	sName.sprintf("S%i", mStyleHash.size());
	shared_ptr<ParagraphStyle> parag(new ParagraphStyle(propList, sName));
	mStyleHash[sName] =parag;
	mHashNameMap[hashKey] = sName;
	if (parag->hasDisplayName())
		mDisplayNameMap[propList["style:display-name"]->getStr()]=sName;

	return sName;
}

shared_ptr<ParagraphStyle> const ParagraphStyleManager::get(const librevenge::RVNGString &name) const
{
	std::map<librevenge::RVNGString, shared_ptr<ParagraphStyle>, ltstr>::const_iterator iter
	    = mStyleHash.find(name);
	if (iter == mStyleHash.end()) return shared_ptr<ParagraphStyle>();
	return iter->second;
}

void SpanStyleManager::clean()
{
	mHashNameMap.clear();
	mStyleHash.clear();
	mDisplayNameMap.clear();
}

void SpanStyleManager::write(OdfDocumentHandler *pHandler, bool automatic) const
{
	for (std::map<librevenge::RVNGString, shared_ptr<SpanStyle>, ltstr>::const_iterator iter = mStyleHash.begin();
	        iter != mStyleHash.end(); ++iter)
	{
		if (iter->second && iter->second->hasDisplayName()!=automatic)
			(iter->second)->write(pHandler);
	}
}

librevenge::RVNGString SpanStyleManager::findOrAdd(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGString hashKey = propList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString, ltstr>::const_iterator iter =
	    mHashNameMap.find(hashKey);
	if (iter!=mHashNameMap.end()) return iter->second;

	// ok create a new list
	ODFGEN_DEBUG_MSG(("SpanStyleManager::findOrAdd: Span Hash Key: %s\n", hashKey.cstr()));

	librevenge::RVNGString sName("");
	sName.sprintf("Span%i", mStyleHash.size());
	shared_ptr<SpanStyle> span(new SpanStyle(sName.cstr(), propList));
	mStyleHash[sName] = span;
	mHashNameMap[hashKey] = sName;
	if (span->hasDisplayName())
		mDisplayNameMap[propList["style:display-name"]->getStr()]=sName;
	return sName;
}

librevenge::RVNGString SpanStyleManager::getFinalDisplayName(const librevenge::RVNGString &displayName)
{
	if (mDisplayNameMap.find(displayName) != mDisplayNameMap.end())
		return mDisplayNameMap.find(displayName)->second;
	ODFGEN_DEBUG_MSG(("SpanStyleManager::getName: can not find style with display name: %s\n", displayName.cstr()));
	return librevenge::RVNGString("");
}

shared_ptr<SpanStyle> const SpanStyleManager::get(const librevenge::RVNGString &name) const
{
	std::map<librevenge::RVNGString, shared_ptr<SpanStyle>, ltstr>::const_iterator iter
	    = mStyleHash.find(name);
	if (iter == mStyleHash.end()) return shared_ptr<SpanStyle>();
	return iter->second;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
