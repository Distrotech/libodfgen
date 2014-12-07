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

ParagraphStyle::ParagraphStyle(const librevenge::RVNGPropertyList &pPropList, const librevenge::RVNGString &sName, Style::Zone zone) : Style(sName, zone),
	mpPropList(pPropList)
{
}

ParagraphStyle::~ParagraphStyle()
{
}

void ParagraphStyle::write(OdfDocumentHandler *pHandler) const
{
	ODFGEN_DEBUG_MSG(("ParagraphStyle: Writing a paragraph style..\n"));

	librevenge::RVNGPropertyList propList;
	propList.insert("style:name", getName());
	propList.insert("style:family", "paragraph");
	if (mpPropList["style:display-name"])
		propList.insert("style:display-name", mpPropList["style:display-name"]->clone());
	if (mpPropList["style:parent-style-name"])
		propList.insert("style:parent-style-name", mpPropList["style:parent-style-name"]->clone());
	if (mpPropList["style:master-page-name"])
		propList.insert("style:master-page-name", mpPropList["style:master-page-name"]->clone());
	if (mpPropList["style:default-outline-level"] && mpPropList["style:default-outline-level"]->getInt()>0)
		propList.insert("style:default-outline-level", mpPropList["style:default-outline-level"]->clone());
	pHandler->startElement("style:style", propList);

	propList.clear();
	librevenge::RVNGPropertyList::Iter i(mpPropList);
	for (i.rewind(); i.next();)
	{
		if (i.child() ||
		        !strcmp(i.key(), "style:display-name") ||
		        !strcmp(i.key(), "style:parent-style-name") ||
		        !strcmp(i.key(), "style:master-page-name") ||
		        !strcmp(i.key(), "style:default-outline-level") ||
		        !strncmp(i.key(), "librevenge:",11))
			continue;
		else if (!strncmp(i.key(), "fo:margin-",10))
		{
			if (!strcmp(i.key(), "fo:margin-left") ||
			        !strcmp(i.key(), "fo:margin-right") ||
			        !strcmp(i.key(), "fo:margin-top"))
				propList.insert(i.key(), i()->clone());
			else if (!strcmp(i.key(), "fo:margin-bottom"))
			{
				if (i()->getDouble() > 0.0)
					propList.insert("fo:margin-bottom", i()->clone());
				else
					propList.insert("fo:margin-bottom", 0.0);
			}
		}
		else if (!strncmp(i.key(), "style:border-line-width", 23))
		{
			if (!strcmp(i.key(), "style:border-line-width") ||
			        !strcmp(i.key(), "style:border-line-width-left") ||
			        !strcmp(i.key(), "style:border-line-width-right") ||
			        !strcmp(i.key(), "style:border-line-width-top") ||
			        !strcmp(i.key(), "style:border-line-width-bottom"))
				propList.insert(i.key(), i()->clone());
		}
		else if (!strncmp(i.key(), "fo:border", 9))
		{
			if (!strcmp(i.key(), "fo:border") ||
			        !strcmp(i.key(), "fo:border-left") ||
			        !strcmp(i.key(), "fo:border-right") ||
			        !strcmp(i.key(), "fo:border-top") ||
			        !strcmp(i.key(), "fo:border-bottom"))
				propList.insert(i.key(), i()->clone());
		}
		else if (!strcmp(i.key(), "text:outline-level"))
			continue;
		else
			propList.insert(i.key(), i()->clone());
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

SpanStyle::SpanStyle(const char *psName, const librevenge::RVNGPropertyList &xPropList, Style::Zone zone) :
	Style(psName, zone),
	mPropList(xPropList)
{
}

void SpanStyle::write(OdfDocumentHandler *pHandler) const
{
	ODFGEN_DEBUG_MSG(("SpanStyle: Writing a span style..\n"));
	librevenge::RVNGPropertyList styleOpenList;
	styleOpenList.insert("style:name", getName());
	if (mPropList["style:display-name"])
		styleOpenList.insert("style:display-name", mPropList["style:display-name"]->clone());
	styleOpenList.insert("style:family", "text");
	pHandler->startElement("style:style", styleOpenList);

	librevenge::RVNGPropertyList style;
	SpanStyleManager::addSpanProperties(mPropList, style);
	pHandler->startElement("style:text-properties", style);
	pHandler->endElement("style:text-properties");
	pHandler->endElement("style:style");
}

void ParagraphStyleManager::clean()
{
	mHashNameMap.clear();
	mStyleHash.clear();
	mDisplayNameMap.clear();
}

void ParagraphStyleManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	for (std::map<librevenge::RVNGString, shared_ptr<ParagraphStyle> >::const_iterator iter = mStyleHash.begin();
	        iter != mStyleHash.end(); ++iter)
	{
		if (iter->second && iter->second->getZone()==zone)
			(iter->second)->write(pHandler);
	}
}

librevenge::RVNGString ParagraphStyleManager::findOrAdd(const librevenge::RVNGPropertyList &propList, Style::Zone zone)
{
	librevenge::RVNGPropertyList pList(propList);

	// first check if we need to store the style as style or as automatic style
	bool deferMasterPageNameInsertion=false;
	Style::Zone currentZone=zone;
	if (propList["style:display-name"])
	{
		if (propList["style:master-page-name"])
		{
			deferMasterPageNameInsertion=true;
			pList.remove("style:master-page-name");
		}
		currentZone=Style::Z_Style;
	}
	else if (currentZone==Style::Z_Unknown)
		currentZone=Style::Z_ContentAutomatic;
	pList.insert("librevenge:zone-style", int(currentZone));

	// look if we have already create this style
	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mHashNameMap.find(hashKey);
	librevenge::RVNGString sName("");
	if (iter!=mHashNameMap.end())
	{
		if (!deferMasterPageNameInsertion)
			return iter->second;
		sName=iter->second;
	}
	else
	{
		ODFGEN_DEBUG_MSG(("ParagraphStyleManager::findOrAdd: Paragraph Hash Key: %s\n", hashKey.cstr()));

		// ok create a new style
		if (currentZone==Style::Z_Style)
			sName.sprintf("S_N%i", mStyleHash.size());
		else if (currentZone==Style::Z_StyleAutomatic)
			sName.sprintf("S_M%i", mStyleHash.size());
		else
			sName.sprintf("S%i", mStyleHash.size());
		if (propList["style:display-name"])
		{
			librevenge::RVNGString name(propList["style:display-name"]->getStr());
			if (mDisplayNameMap.find(name) != mDisplayNameMap.end())
			{
				ODFGEN_DEBUG_MSG(("ParagraphStyleManager::findOrAdd: a paragraph with name %s already exists\n", name.cstr()));
				pList.remove("style:display-name");
			}
			else
				mDisplayNameMap[name]=sName;
		}
		shared_ptr<ParagraphStyle> parag(new ParagraphStyle(pList, sName, currentZone));
		mStyleHash[sName] =parag;
		mHashNameMap[hashKey] = sName;
		if (!deferMasterPageNameInsertion)
			return sName;
	}

	//
	// we must now create the style with master-page-name attribute : let inherete from the named style
	//
	pList=propList;
	pList.remove("style:display-name");
	pList.insert("style:parent-style-name", sName);
	return findOrAdd(pList, zone);
}

shared_ptr<ParagraphStyle> const ParagraphStyleManager::get(const librevenge::RVNGString &name) const
{
	std::map<librevenge::RVNGString, shared_ptr<ParagraphStyle> >::const_iterator iter
	    = mStyleHash.find(name);
	if (iter == mStyleHash.end()) return shared_ptr<ParagraphStyle>();
	return iter->second;
}

////////////////////////////////////////////////////////////
// span manager
////////////////////////////////////////////////////////////
void SpanStyleManager::clean()
{
	mHashNameMap.clear();
	mStyleHash.clear();
	mDisplayNameMap.clear();
}

void SpanStyleManager::addSpanProperties(librevenge::RVNGPropertyList const &style, librevenge::RVNGPropertyList &element)
{
	librevenge::RVNGPropertyList::Iter i(style);
	for (i.rewind(); i.next();)
	{
		if (i.child()) continue;
		switch (i.key()[0])
		{
		case 'f':
			if (!strcmp(i.key(), "fo:font-size"))
			{
				if (style["fo:font-size"]->getDouble() > 0.0)
				{
					element.insert("fo:font-size", style["fo:font-size"]->clone());
					element.insert("style:font-size-asian", style["fo:font-size"]->clone());
					element.insert("style:font-size-complex", style["fo:font-size"]->clone());
				}
				break;
			}
			if (!strcmp(i.key(), "fo:font-weight"))
			{
				element.insert("fo:font-weight", style["fo:font-weight"]->clone());
				element.insert("style:font-weight-asian", style["fo:font-weight"]->clone());
				element.insert("style:font-weight-complex", style["fo:font-weight"]->clone());
				break;
			}
			if (!strcmp(i.key(), "fo:font-style"))
			{
				element.insert("fo:font-style", style["fo:font-style"]->clone());
				element.insert("style:font-style-asian", style["fo:font-style"]->clone());
				element.insert("style:font-style-complex", style["fo:font-style"]->clone());
				break;
			}
			if (!strcmp(i.key(), "fo:background-color") || !strcmp(i.key(), "fo:color") ||
			        !strcmp(i.key(), "fo:country") || !strncmp(i.key(), "fo:font", 7) ||
			        !strncmp(i.key(), "fo:hyphen", 9) || !strncmp(i.key(), "fo:text", 7) ||
			        !strcmp(i.key(), "fo:language") || !strcmp(i.key(), "fo:letter-spacing") ||
			        !strcmp(i.key(), "fo:script"))
				element.insert(i.key(),i()->clone());
			break;
		case 's':
			if (!strcmp(i.key(), "style:font-name"))
			{
				element.insert("style:font-name", style["style:font-name"]->clone());
				element.insert("style:font-name-asian", style["style:font-name"]->clone());
				element.insert("style:font-name-complex", style["style:font-name"]->clone());
				break;
			}
			if (!strncmp(i.key(), "style:country", 13) || !strncmp(i.key(), "style:font", 10) ||
			        !strncmp(i.key(), "style:language", 14) || !strncmp(i.key(), "style:letter", 12) ||
			        !strncmp(i.key(), "style:rfc-", 10) || !strncmp(i.key(), "style:script", 12) ||
			        !strncmp(i.key(), "style:text", 10))
				element.insert(i.key(),i()->clone());
			// style:writing-mode is ignored
			break;
		case 't':
			if (!strcmp(i.key(), "text:display"))
				element.insert(i.key(),i()->clone());
			break;
		default:
			break;
		}
	}
}

void SpanStyleManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	for (std::map<librevenge::RVNGString, shared_ptr<SpanStyle> >::const_iterator iter = mStyleHash.begin();
	        iter != mStyleHash.end(); ++iter)
	{
		if (iter->second && iter->second->getZone()==zone)
			(iter->second)->write(pHandler);
	}
}

librevenge::RVNGString SpanStyleManager::findOrAdd(const librevenge::RVNGPropertyList &propList, Style::Zone zone)
{
	librevenge::RVNGPropertyList pList(propList);

	// first check if we need to store the style as style or as automatic style
	if (propList["style:display-name"] && !propList["style:master-page-name"])
		zone=Style::Z_Style;
	else if (zone==Style::Z_Unknown)
		zone=Style::Z_ContentAutomatic;
	pList.insert("librevenge:zone-style", int(zone));

	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mHashNameMap.find(hashKey);
	if (iter!=mHashNameMap.end()) return iter->second;

	// ok create a new list
	ODFGEN_DEBUG_MSG(("SpanStyleManager::findOrAdd: Span Hash Key: %s\n", hashKey.cstr()));

	librevenge::RVNGString sName("");
	if (zone==Style::Z_Style)
		sName.sprintf("Span_N%i", mStyleHash.size());
	else if (zone==Style::Z_StyleAutomatic)
		sName.sprintf("Span_M%i", mStyleHash.size());
	else
		sName.sprintf("Span%i", mStyleHash.size());
	shared_ptr<SpanStyle> span(new SpanStyle(sName.cstr(), propList, zone));
	mStyleHash[sName] = span;
	mHashNameMap[hashKey] = sName;
	if (propList["style:display-name"] && !propList["style:display-name"]->getStr().empty())
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
	std::map<librevenge::RVNGString, shared_ptr<SpanStyle> >::const_iterator iter
	    = mStyleHash.find(name);
	if (iter == mStyleHash.end()) return shared_ptr<SpanStyle>();
	return iter->second;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
