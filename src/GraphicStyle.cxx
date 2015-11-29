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

#include <librevenge/librevenge.h>

#include "FilterInternal.hxx"
#include "FillManager.hxx"
#include "GraphicStyle.hxx"
#include "DocumentElement.hxx"

GraphicStyle::GraphicStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName, Style::Zone zone) :
	Style(psName, zone), mPropList(xPropList)
{
}

GraphicStyle::~GraphicStyle()
{
}

void GraphicStyle::write(OdfDocumentHandler *pHandler) const
{
	librevenge::RVNGPropertyList openElement;
	openElement.insert("style:name", getName());
	openElement.insert("style:family", "graphic");
	if (mPropList["style:parent-style-name"])
		openElement.insert("style:parent-style-name", mPropList["style:parent-style-name"]->getStr());
	else
		openElement.insert("style:parent-style-name", "standard");
	if (mPropList["style:display-name"])
		openElement.insert("style:display-name", mPropList["style:display-name"]->getStr());
	pHandler->startElement("style:style", openElement);

	librevenge::RVNGPropertyList graphicElement;
	librevenge::RVNGPropertyList::Iter i(mPropList);
	for (i.rewind(); i.next();)
	{
		if (strcmp(i.key(), "style:display-name")==0 || strcmp(i.key(), "style:parent-style-name") == 0 ||
		        strncmp(i.key(), "librevenge:", 11)==0)
			continue;
		graphicElement.insert(i.key(),i()->getStr());
	}
	pHandler->startElement("style:graphic-properties", graphicElement);
	pHandler->endElement("style:graphic-properties");

	if (mPropList["draw:show-unit"] && mPropList["draw:show-unit"]->getStr()=="true")
	{
		librevenge::RVNGPropertyList textElement;
		textElement.insert("fo:font-size", 12, librevenge::RVNG_POINT);
		pHandler->startElement("style:text-properties", textElement);
		pHandler->endElement("style:text-properties");
	}
	pHandler->endElement("style:style");
}

//
// manager
//
void GraphicStyleManager::clean()
{
	mStyles.resize(0);

	mMarkerStyles.clear();
	mStrokeDashStyles.clear();

	mMarkerNameMap.clear();
	mStrokeDashNameMap.clear();
	mStyleNameMap.clear();
}

void GraphicStyleManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	if (zone==Style::Z_Style)
	{
		for (size_t i=0; i < mMarkerStyles.size(); ++i)
			mMarkerStyles[i]->write(pHandler);
		for (size_t i=0; i<mStrokeDashStyles.size(); ++i)
			mStrokeDashStyles[i]->write(pHandler);
	}
	for (size_t i=0; i<mStyles.size(); ++i)
	{
		if (mStyles[i] && mStyles[i]->getZone()==zone)
			mStyles[i]->write(pHandler);
	}
}

librevenge::RVNGString GraphicStyleManager::findOrAdd(librevenge::RVNGPropertyList const &propList, Style::Zone zone)
{
	librevenge::RVNGPropertyList pList(propList);
	if (zone==Style::Z_Unknown)
		zone=Style::Z_ContentAutomatic;
	pList.insert("librevenge:zone-style", int(zone));

	librevenge::RVNGString hashKey = pList.getPropString();
	if (mStyleNameMap.find(hashKey) != mStyleNameMap.end())
		return mStyleNameMap.find(hashKey)->second;

	librevenge::RVNGString name;
	if (zone==Style::Z_StyleAutomatic)
		name.sprintf("gr_M%i", (int) mStyleNameMap.size());
	else if (zone==Style::Z_Style)
		name.sprintf("GraphicStyle_%i", (int) mStyleNameMap.size());
	else
		name.sprintf("gr_%i", (int) mStyleNameMap.size());

	mStyleNameMap[hashKey]=name;
	shared_ptr<GraphicStyle> style(new GraphicStyle(propList, name.cstr(), zone));
	mStyles.push_back(style);

	return name;
}
////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////
librevenge::RVNGString GraphicStyleManager::getStyleNameForMarker(librevenge::RVNGPropertyList const &style, bool startMarker)
{
	librevenge::RVNGPropertyList pList;
	if (startMarker)
	{
		if (!style["draw:marker-start-path"])
			return "";
		pList.insert("svg:d", style["draw:marker-start-path"]->getStr());
		if (style["draw:marker-start-viewbox"])
			pList.insert("svg:viewBox", style["draw:marker-start-viewbox"]->getStr());
	}
	else
	{
		if (!style["draw:marker-end-path"])
			return "";
		pList.insert("svg:d", style["draw:marker-end-path"]->getStr());
		if (style["draw:marker-end-viewbox"])
			pList.insert("svg:viewBox", style["draw:marker-end-viewbox"]->getStr());
	}
	librevenge::RVNGString hashKey = pList.getPropString();
	if (mMarkerNameMap.find(hashKey) != mMarkerNameMap.end())
		return mMarkerNameMap.find(hashKey)->second;

	librevenge::RVNGString name;
	name.sprintf("Marker_%i", (int) mMarkerNameMap.size());
	mMarkerNameMap[hashKey]=name;

	TagOpenElement *openElement = new TagOpenElement("draw:marker");
	openElement->addAttribute("draw:name", name);
	if (pList["svg:viewBox"])
		openElement->addAttribute("svg:viewBox", pList["svg:viewBox"]->getStr());
	openElement->addAttribute("svg:d", pList["svg:d"]->getStr());
	mMarkerStyles.push_back(openElement);
	mMarkerStyles.push_back(new TagCloseElement("draw:marker"));
	return name;
}

librevenge::RVNGString GraphicStyleManager::getStyleNameForStrokeDash(librevenge::RVNGPropertyList const &style)
{
	librevenge::RVNGPropertyList pList;
	if (style["svg:stroke-linecap"])
		pList.insert("draw:style", style["svg:stroke-linecap"]->getStr());
	else
		pList.insert("draw:style", "rect");
	if (style["draw:distance"])
		pList.insert("draw:distance", style["draw:distance"]->getStr());
	if (style["draw:dots1"])
		pList.insert("draw:dots1", style["draw:dots1"]->getStr());
	if (style["draw:dots1-length"])
		pList.insert("draw:dots1-length", style["draw:dots1-length"]->getStr());
	if (style["draw:dots2"])
		pList.insert("draw:dots2", style["draw:dots2"]->getStr());
	if (style["draw:dots2-length"])
		pList.insert("draw:dots2-length", style["draw:dots2-length"]->getStr());
	librevenge::RVNGString hashKey = pList.getPropString();
	if (mStrokeDashNameMap.find(hashKey) != mStrokeDashNameMap.end())
		return mStrokeDashNameMap.find(hashKey)->second;

	librevenge::RVNGString name;
	name.sprintf("Dash_%i", (int) mStrokeDashNameMap.size());
	mStrokeDashNameMap[hashKey]=name;

	TagOpenElement *openElement = new TagOpenElement("draw:stroke-dash");
	openElement->addAttribute("draw:name", name);
	openElement->addAttribute("draw:style", pList["draw:style"]->getStr());
	if (pList["draw:distance"])
		openElement->addAttribute("draw:distance", pList["draw:distance"]->getStr());
	if (pList["draw:dots1"])
		openElement->addAttribute("draw:dots1", pList["draw:dots1"]->getStr());
	if (pList["draw:dots1-length"])
		openElement->addAttribute("draw:dots1-length", pList["draw:dots1-length"]->getStr());
	if (pList["draw:dots2"])
		openElement->addAttribute("draw:dots2", pList["draw:dots2"]->getStr());
	if (pList["draw:dots2-length"])
		openElement->addAttribute("draw:dots2-length", pList["draw:dots2-length"]->getStr());
	mStrokeDashStyles.push_back(openElement);
	mStrokeDashStyles.push_back(new TagCloseElement("draw:stroke-dash"));
	return name;
}

void GraphicStyleManager::addGraphicProperties(librevenge::RVNGPropertyList const &style, librevenge::RVNGPropertyList &element)
{
	if (style["draw:stroke"] && style["draw:stroke"]->getStr() == "none")
		element.insert("draw:stroke", "none");
	else
	{
		if (style["svg:stroke-width"])
			element.insert("svg:stroke-width", style["svg:stroke-width"]->getStr());
		if (style["svg:stroke-color"])
			element.insert("svg:stroke-color", style["svg:stroke-color"]->getStr());
		if (style["svg:stroke-opacity"])
			element.insert("svg:stroke-opacity", style["svg:stroke-opacity"]->getStr());
		if (style["svg:stroke-linejoin"])
			element.insert("draw:stroke-linejoin", style["svg:stroke-linejoin"]->getStr());
		if (style["svg:stroke-linecap"])
			element.insert("svg:stroke-linecap", style["svg:stroke-linecap"]->getStr());

		librevenge::RVNGString name("");
		if (style["draw:stroke"] && style["draw:stroke"]->getStr() == "dash")
			name=getStyleNameForStrokeDash(style);
		if (!name.empty())
		{
			element.insert("draw:stroke", "dash");
			element.insert("draw:stroke-dash", name);
		}
		else
			element.insert("draw:stroke", "solid");
	}

	if (style["draw:color-mode"] && style["draw:color-mode"]->getStr().len() > 0)
		element.insert("draw:color-mode", style["draw:color-mode"]->getStr());
	if (style["draw:luminance"] && style["draw:luminance"]->getStr().len() > 0)
		element.insert("draw:luminance", style["draw:luminance"]->getStr());
	if (style["draw:contrast"] && style["draw:contrast"]->getStr().len() > 0)
		element.insert("draw:contrast", style["draw:contrast"]->getStr());
	if (style["draw:gamma"] && style["draw:gamma"]->getStr().len() > 0)
		element.insert("draw:gamma", style["draw:gamma"]->getStr());
	if (style["draw:red"] && style["draw:red"]->getStr().len() > 0)
		element.insert("draw:red", style["draw:red"]->getStr());
	if (style["draw:green"] && style["draw:green"]->getStr().len() > 0)
		element.insert("draw:green", style["draw:green"]->getStr());
	if (style["draw:blue"] && style["draw:blue"]->getStr().len() > 0)
		element.insert("draw:blue", style["draw:blue"]->getStr());

	// only append shadow props if the shape is filled
	if (!style["draw:fill"] || style["draw:fill"]->getStr() != "none")
	{
		if (style["draw:shadow"])
			element.insert("draw:shadow", style["draw:shadow"]->getStr());
		else
			element.insert("draw:shadow", "hidden");
		if (style["draw:shadow-offset-x"])
			element.insert("draw:shadow-offset-x", style["draw:shadow-offset-x"]->getStr());
		if (style["draw:shadow-offset-y"])
			element.insert("draw:shadow-offset-y", style["draw:shadow-offset-y"]->getStr());
		if (style["draw:shadow-color"])
			element.insert("draw:shadow-color", style["draw:shadow-color"]->getStr());
		if (style["draw:shadow-opacity"])
			element.insert("draw:shadow-opacity", style["draw:shadow-opacity"]->getStr());
		if (style["svg:fill-rule"])
			element.insert("svg:fill-rule", style["svg:fill-rule"]->getStr());
	}

	mFillManager.addProperties(style, element);

	// marker
	if (style["draw:marker-start-path"])
	{
		librevenge::RVNGString marker=getStyleNameForMarker(style, true);
		if (!marker.empty())
		{
			element.insert("draw:marker-start", marker);
			if (style["draw:marker-start-center"])
				element.insert("draw:marker-start-center", style["draw:marker-start-center"]->getStr());
			if (style["draw:marker-start-width"])
				element.insert("draw:marker-start-width", style["draw:marker-start-width"]->getStr());
			else
				element.insert("draw:marker-start-width", "0.118in");
		}
	}
	if (style["draw:marker-end-path"])
	{
		librevenge::RVNGString marker=getStyleNameForMarker(style, false);
		if (!marker.empty())
		{
			element.insert("draw:marker-end", marker);
			if (style["draw:marker-end-center"])
				element.insert("draw:marker-end-center", style["draw:marker-end-center"]->getStr());
			if (style["draw:marker-end-width"])
				element.insert("draw:marker-end-width", style["draw:marker-end-width"]->getStr());
			else
				element.insert("draw:marker-end-width", "0.118in");
		}
	}
	// other
	char const *(others[])=
	{
		"draw:ole-draw-aspect",
		"draw:show-unit",
		"fo:background-color",
		"fo:border","fo:border-top","fo:border-left","fo:border-bottom","fo:border-right",
		"fo:clip",
		"style:background-transparency",
		"style:border-line-width","style:border-line-width-top","style:border-line-width-left",
		"style:border-line-width-bottom","style:border-line-width-right",
		"style:mirror", "style:parent-style-name",
		"style:run-through", "style:wrap"
	};
	for (unsigned b = 0; b < ODFGEN_N_ELEMENTS(others); b++)
	{
		if (style[others[b]])
			element.insert(others[b], style[others[b]]->getStr());
	}
}

void GraphicStyleManager::addFrameProperties(librevenge::RVNGPropertyList const &propList, librevenge::RVNGPropertyList &element)
{
	element.insert("fo:min-width", "1in");
	char const *attrib[]=
	{
		"fo:min-width", "fo:min-height", "fo:max-width", "fo:max-height", "fo:padding-top", "fo:padding-bottom",
		"fo:padding-left", "fo:padding-right", "draw:textarea-vertical-align", "draw:fill", "draw:fill-color"
		// checkme
		// "draw:z-index",
		// "svg:x", "svg:y", "svg:width", "svg:height", "style:wrap", "style:run-through",
		// "text:anchor-type", "text:anchor-page-number"
	};
	for (unsigned i=0; i<ODFGEN_N_ELEMENTS(attrib); ++i)
	{
		if (propList[attrib[i]])
			element.insert(attrib[i], propList[attrib[i]]->getStr());
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
