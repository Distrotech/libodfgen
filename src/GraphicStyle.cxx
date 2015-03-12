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

	pHandler->endElement("style:style");
}

//
// manager
//
void GraphicStyleManager::clean()
{
	mStyles.resize(0);

	mBitmapStyles.clear();
	mGradientStyles.clear();
	mMarkerStyles.clear();
	mOpacityStyles.clear();
	mStrokeDashStyles.clear();

	mBitmapNameMap.clear();
	mGradientNameMap.clear();
	mMarkerNameMap.clear();
	mOpacityNameMap.clear();
	mStrokeDashNameMap.clear();
	mStyleNameMap.clear();
}

void GraphicStyleManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	if (zone==Style::Z_Style)
	{
		for (size_t i=0; i < mBitmapStyles.size(); ++i)
			mBitmapStyles[i]->write(pHandler);
		for (size_t i=0; i < mGradientStyles.size(); ++i)
			mGradientStyles[i]->write(pHandler);
		for (size_t i=0; i < mMarkerStyles.size(); ++i)
			mMarkerStyles[i]->write(pHandler);
		for (size_t i=0; i < mOpacityStyles.size(); ++i)
			mOpacityStyles[i]->write(pHandler);
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
librevenge::RVNGString GraphicStyleManager::getStyleNameForBitmap(librevenge::RVNGString const &bitmap)
{
	if (bitmap.empty())
		return "";
	if (mBitmapNameMap.find(bitmap) != mBitmapNameMap.end())
		return mBitmapNameMap.find(bitmap)->second;

	librevenge::RVNGString name;
	name.sprintf("Bitmap_%i", (int) mBitmapNameMap.size());
	mBitmapNameMap[bitmap]=name;

	TagOpenElement *openElement = new TagOpenElement("draw:fill-image");
	openElement->addAttribute("draw:name", name);
	mBitmapStyles.push_back(openElement);
	mBitmapStyles.push_back(new TagOpenElement("office:binary-data"));
	mBitmapStyles.push_back(new CharDataElement(bitmap));
	mBitmapStyles.push_back(new TagCloseElement("office:binary-data"));
	mBitmapStyles.push_back(new TagCloseElement("draw:fill-image"));
	return name;
}

librevenge::RVNGString GraphicStyleManager::getStyleNameForGradient(librevenge::RVNGPropertyList const &style,
                                                                    bool &needCreateOpacityStyle)
{
	needCreateOpacityStyle=false;

	librevenge::RVNGPropertyList pList;
	// default value
	pList.insert("draw:style", "linear");
	pList.insert("draw:border", "0%");
	pList.insert("draw:start-intensity", "100%");
	pList.insert("draw:end-intensity", "100%");
	// property rename
	if (style["svg:cx"])
		pList.insert("draw:cx", style["svg:cx"]->getStr());
	if (style["svg:cy"])
		pList.insert("draw:cy", style["svg:cy"]->getStr());
	// prepare angle: ODG angle unit is 0.1 degree
	double angle = style["draw:angle"] ? style["draw:angle"]->getDouble() : 0.0;
	while (angle < 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;
	librevenge::RVNGString sValue;
	sValue.sprintf("%i", (unsigned)(angle*10));
	pList.insert("draw:angle", sValue);
	// gradient vector
	const librevenge::RVNGPropertyListVector *gradient = style.child("svg:linearGradient");
	if (!gradient)
		gradient = style.child("svg:radialGradient");
	if (gradient) pList.insert("svg:linearGradient", *gradient);
	static char const *(wh[]) =
	{
		"draw:border", "draw:cx", "draw:cy", "draw:end-color", "draw:end-intensity",
		"draw:start-color", "draw:start-intensity", "draw:style"
	};
	for (int i=0; i<8; ++i)
	{
		if (style[wh[i]])
			pList.insert(wh[i], style[wh[i]]->getStr());
	}
	librevenge::RVNGString hashKey = pList.getPropString();
	if (mGradientNameMap.find(hashKey) != mGradientNameMap.end())
		return mGradientNameMap.find(hashKey)->second;

	librevenge::RVNGString name;
	name.sprintf("Gradient_%i", (int) mGradientNameMap.size());
	mGradientNameMap[hashKey]=name;

	TagOpenElement *openElement = new TagOpenElement("draw:gradient");
	openElement->addAttribute("draw:name", name);
	openElement->addAttribute("draw:style", pList["draw:style"]->getStr());
	openElement->addAttribute("draw:angle", sValue);
	if (pList["draw:cx"])
		openElement->addAttribute("draw:cx", pList["draw:cx"]->getStr());
	if (pList["draw:cy"])
		openElement->addAttribute("draw:cy", pList["draw:cy"]->getStr());

	if (!gradient || !gradient->count())
	{
		if (pList["draw:start-color"])
			openElement->addAttribute("draw:start-color", pList["draw:start-color"]->getStr());
		if (pList["draw:end-color"])
			openElement->addAttribute("draw:end-color", pList["draw:end-color"]->getStr());
		openElement->addAttribute("draw:border", pList["draw:border"]->getStr());

		openElement->addAttribute("draw:start-intensity", pList["draw:start-intensity"]->getStr());
		openElement->addAttribute("draw:end-intensity", pList["draw:end-intensity"]->getStr());

		// Work around a mess in LibreOffice where both opacities of 100% are interpreted as complete transparency
		// Nevertheless, when one is different, immediately, they are interpreted correctly
		if (style["librevenge:start-opacity"] && style["librevenge:end-opacity"]
		        && (style["librevenge:start-opacity"]->getDouble() < 1.0 || style["librevenge:end-opacity"]->getDouble() < 1.0))
			needCreateOpacityStyle=true;
	}
	else if (gradient->count() >= 2)
	{
		if ((*gradient)[1]["svg:stop-color"])
			openElement->addAttribute("draw:start-color", (*gradient)[1]["svg:stop-color"]->getStr());
		if ((*gradient)[0]["svg:stop-color"])
			openElement->addAttribute("draw:end-color", (*gradient)[0]["svg:stop-color"]->getStr());
		if ((*gradient)[0]["svg:stop-opacity"] || (*gradient)[1]["svg:stop-opacity"])
			needCreateOpacityStyle=true;
		openElement->addAttribute("draw:border", "0%");
	}
	else
	{
		delete openElement;
		return "";
	}

	mGradientStyles.push_back(openElement);
	mGradientStyles.push_back(new TagCloseElement("draw:gradient"));
	return name;
}

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

librevenge::RVNGString GraphicStyleManager::getStyleNameForOpacity(librevenge::RVNGPropertyList const &style)
{
	librevenge::RVNGPropertyList pList;
	// default value
	pList.insert("draw:border", "0%");
	pList.insert("draw:start", "100%");
	pList.insert("draw:end", "100%");
	// property rename
	if (style["svg:cx"])
		pList.insert("draw:cx", style["svg:cx"]->getStr());
	if (style["svg:cy"])
		pList.insert("draw:cy", style["svg:cy"]->getStr());
	if (style["draw:start-intensity"])
		pList.insert("draw:start", style["draw:start-intensity"]->getStr());
	if (style["draw:end-intensity"])
		pList.insert("draw:end", style["draw:end-intensity"]->getStr());
	// data in gradient vector
	const librevenge::RVNGPropertyListVector *gradient = style.child("svg:linearGradient");
	if (!gradient)
		gradient = style.child("svg:radialGradient");
	if (gradient && gradient->count() >= 2)
	{
		if ((*gradient)[1]["svg:stop-opacity"])
			pList.insert("draw:start", (*gradient)[1]["svg:stop-opacity"]->getStr());
		if ((*gradient)[0]["svg:stop-opacity"])
			pList.insert("draw:end", (*gradient)[0]["svg:stop-opacity"]->getStr());
	}
	// prepare angle: ODG angle unit is 0.1 degree
	double angle = style["draw:angle"] ? style["draw:angle"]->getDouble() : 0.0;
	while (angle < 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;
	librevenge::RVNGString sValue;
	sValue.sprintf("%i", (unsigned)(angle*10));
	pList.insert("draw:angle", sValue);
	// basic data
	static char const *(wh[]) = { "draw:border", "draw:cx", "draw:cy"	};
	for (int i=0; i<3; ++i)
	{
		if (style[wh[i]])
			pList.insert(wh[i], style[wh[i]]->getStr());
	}

	librevenge::RVNGString hashKey = pList.getPropString();
	if (mOpacityNameMap.find(hashKey) != mOpacityNameMap.end())
		return mOpacityNameMap.find(hashKey)->second;

	librevenge::RVNGString name;
	name.sprintf("Transparency_%i", (int) mOpacityNameMap.size());
	mOpacityNameMap[hashKey]=name;

	TagOpenElement *openElement = new TagOpenElement("draw:opacity");
	openElement->addAttribute("draw:name", name);
	openElement->addAttribute("draw:angle", sValue);
	if (pList["draw:border"])
		openElement->addAttribute("draw:border", pList["draw:border"]->getStr());
	if (pList["draw:cx"])
		openElement->addAttribute("draw:cx", pList["draw:cx"]->getStr());
	if (pList["draw:cy"])
		openElement->addAttribute("draw:cy", pList["draw:cy"]->getStr());
	if (pList["draw:start"])
		openElement->addAttribute("draw:start", pList["draw:start"]->getStr());
	if (pList["draw:end"])
		openElement->addAttribute("draw:end", pList["draw:end"]->getStr());

	mOpacityStyles.push_back(openElement);
	mOpacityStyles.push_back(new TagCloseElement("draw:opacity"));
	return name;
}

librevenge::RVNGString GraphicStyleManager::getStyleNameForStrokeDash(librevenge::RVNGPropertyList const &style)
{
	librevenge::RVNGPropertyList pList;
	if (style["svg:stoke-linecap"])
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
			element.insert("svg:stoke-linecap", style["svg:stroke-linecap"]->getStr());

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

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "none")
		element.insert("draw:fill", "none");
	else
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

	if (style["draw:fill"] && style["draw:fill"]->getStr() == "bitmap" &&
	        style["draw:fill-image"] && style["librevenge:mime-type"])
	{
		librevenge::RVNGString name=getStyleNameForBitmap(style["draw:fill-image"]->getStr());
		if (!name.empty())
		{
			element.insert("draw:fill", "bitmap");
			element.insert("draw:fill-image-name", name);
			if (style["draw:fill-image-width"])
				element.insert("draw:fill-image-width", style["draw:fill-image-width"]->getStr());
			else if (style["svg:width"])
				element.insert("draw:fill-image-width", style["svg:width"]->getStr());
			if (style["draw:fill-image-height"])
				element.insert("draw:fill-image-height", style["draw:fill-image-height"]->getStr());
			else if (style["svg:height"])
				element.insert("draw:fill-image-height", style["svg:height"]->getStr());
			if (style["style:repeat"])
				element.insert("style:repeat", style["style:repeat"]->getStr());
			if (style["draw:fill-image-ref-point"])
				element.insert("draw:fill-image-ref-point", style["draw:fill-image-ref-point"]->getStr());
			if (style["draw:fill-image-ref-point-x"])
				element.insert("draw:fill-image-ref-point-x", style["draw:fill-image-ref-point-x"]->getStr());
			if (style["draw:fill-image-ref-point-y"])
				element.insert("draw:fill-image-ref-point-y", style["draw:fill-image-ref-point-y"]->getStr());
		}
		else
			element.insert("draw:fill", "none");
	}
	else if (style["draw:fill"] && style["draw:fill"]->getStr() == "gradient")
	{
		librevenge::RVNGString gradientName(""), opacityName("");
		bool bUseOpacityGradient = false;
		gradientName=getStyleNameForGradient(style, bUseOpacityGradient);
		if (!gradientName.empty())
		{
			element.insert("draw:fill", "gradient");
			element.insert("draw:fill-gradient-name", gradientName);
			if (bUseOpacityGradient)
			{
				opacityName=getStyleNameForOpacity(style);
				if (!opacityName.empty())
					element.insert("draw:opacity-name", opacityName);
			}
		}
		else
		{
			element.insert("draw:fill", "solid");
			// try to use the gradient to define the color
			const librevenge::RVNGPropertyListVector *gradient = style.child("svg:linearGradient");
			if (!gradient)
				gradient = style.child("svg:radialGradient");
			if (gradient && gradient->count() >= 1 && (*gradient)[0]["svg:stop-color"])
				element.insert("draw:fill-color", (*gradient)[0]["svg:stop-color"]->getStr());
		}
	}
	else if (style["draw:fill"] && style["draw:fill"]->getStr() == "solid")
	{
		element.insert("draw:fill", "solid");
		if (style["draw:fill-color"])
			element.insert("draw:fill-color", style["draw:fill-color"]->getStr());
		if (style["draw:opacity"])
			element.insert("draw:opacity", style["draw:opacity"]->getStr());
	}

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
	static char const *(others[])=
	{
		"draw:ole-draw-aspect",
		"fo:background-color",
		"fo:border","fo:border-top","fo:border-left","fo:border-bottom","fo:border-right",
		"fo:clip",
		"style:background-transparency",
		"style:border-line-width","style:border-line-width-top","style:border-line-width-left",
		"style:border-line-width-bottom","style:border-line-width-right",
		"style:mirror", "style:parent-style-name",
		"style:run-through", "style:wrap"
	};
	for (int b = 0; b < 18; b++)
	{
		if (style[others[b]])
			element.insert(others[b], style[others[b]]->getStr());
	}
}

void GraphicStyleManager::addFrameProperties(librevenge::RVNGPropertyList const &propList, librevenge::RVNGPropertyList &element)
{
	element.insert("fo:min-width", "1in");
	static char const *attrib[]=
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
