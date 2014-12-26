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

#include "FontStyle.hxx"

#include "DocumentElement.hxx"

FontStyle::EmbeddedInfo::EmbeddedInfo(const librevenge::RVNGString &mimeType, const librevenge::RVNGBinaryData &data)
	: m_mimeType(mimeType)
	, m_data(data)
{
}

FontStyle::FontStyle(const char *psName, const char *psFontFamily) : Style(psName, Style::Z_Font),
	msFontFamily()
	, m_embeddedInfo()
{
	msFontFamily.appendEscapedXML(psFontFamily);
}

FontStyle::~FontStyle()
{
}

void FontStyle::write(OdfDocumentHandler *pHandler) const
{
	TagOpenElement styleOpen("style:font-face");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("svg:font-family", msFontFamily);
	styleOpen.write(pHandler);
	if (bool(m_embeddedInfo))
		writeEmbedded(pHandler);
	TagCloseElement styleClose("style:font-face");
	styleClose.write(pHandler);
}

void FontStyle::writeEmbedded(OdfDocumentHandler *const pHandler) const
{
	assert(bool(m_embeddedInfo));

	TagOpenElement("svg:font-face-src").write(pHandler);
	TagOpenElement("svg:font-face-uri").write(pHandler);

	librevenge::RVNGString type;
	if (m_embeddedInfo->m_mimeType == "application/x-font-ttf")
		type = "truetype";
	else if (m_embeddedInfo->m_mimeType == "application/vnd.ms-fontobject")
		type = "embedded-opentype";
	if (!type.empty())
	{
		TagOpenElement fontFaceFormatOpen("svg:font-face-format");
		fontFaceFormatOpen.addAttribute("svg:string", type);
		fontFaceFormatOpen.write(pHandler);
		TagCloseElement("svg:font-face-format").write(pHandler);
	}

	TagOpenElement("office:binary-data").write(pHandler);
	try
	{
		CharDataElement(m_embeddedInfo->m_data.getBase64Data()).write(pHandler);
	}
	catch (...)
	{
		ODFGEN_DEBUG_MSG(("FontStyle::writeEmbedded: ARGHH, catch an exception when encoding font!!!\n"));
	}
	TagCloseElement("office:binary-data").write(pHandler);

	TagCloseElement("svg:font-face-uri").write(pHandler);
	TagCloseElement("svg:font-face-src").write(pHandler);
}

void FontStyle::setEmbedded(const librevenge::RVNGString &mimeType, const librevenge::RVNGBinaryData &data)
{
	if (!mimeType.empty() && !data.empty())
		m_embeddedInfo.reset(new EmbeddedInfo(mimeType, data));
}

void FontStyleManager::clean()
{
	mStyleHash.clear();
}

void FontStyleManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	std::map<librevenge::RVNGString, shared_ptr<FontStyle> >::const_iterator iter;
	for (iter = mStyleHash.begin(); iter != mStyleHash.end(); ++iter)
	{
		if (iter->second->getZone()==zone)
			(iter->second)->write(pHandler);
	}

	if (zone!=Style::Z_Font)
		return;
	TagOpenElement symbolFontOpen("style:font-face");
	symbolFontOpen.addAttribute("style:name", "StarSymbol");
	symbolFontOpen.addAttribute("svg:font-family", "StarSymbol");
	symbolFontOpen.addAttribute("style:font-charset", "x-symbol");
	symbolFontOpen.write(pHandler);
	pHandler->endElement("style:font-face");
}

librevenge::RVNGString FontStyleManager::findOrAdd(const char *psFontFamily)
{
	std::map<librevenge::RVNGString, shared_ptr<FontStyle> >::const_iterator iter =
	    mStyleHash.find(psFontFamily);
	if (iter!=mStyleHash.end()) return psFontFamily;

	// ok create a new font
	shared_ptr<FontStyle> font(new FontStyle(psFontFamily, psFontFamily));
	mStyleHash[psFontFamily] = font;
	return psFontFamily;
}

void FontStyleManager::setEmbedded(const librevenge::RVNGString &name, const librevenge::RVNGString &mimeType, const librevenge::RVNGBinaryData &data)
{
	findOrAdd(name.cstr());
	mStyleHash[name]->setEmbedded(mimeType, data);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
