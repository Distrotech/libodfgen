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
#ifndef _FONTSTYLE_HXX_
#define _FONTSTYLE_HXX_
#include <map>
#include <vector>

#include <librevenge/librevenge.h>

#include "FilterInternal.hxx"

#include "Style.hxx"

class FontStyle : public Style
{
	struct EmbeddedInfo
	{
		EmbeddedInfo(const librevenge::RVNGString &mimeType, const librevenge::RVNGBinaryData &data);

		librevenge::RVNGString m_mimeType;
		librevenge::RVNGBinaryData m_data;
	};

public:
	FontStyle(const char *psName, const char *psFontFamily);
	~FontStyle();
	virtual void write(OdfDocumentHandler *pHandler) const;
	const librevenge::RVNGString &getFontFamily() const
	{
		return msFontFamily;
	}
	bool isEmbedded() const
	{
		return bool(m_embeddedInfo);
	}
	void setEmbedded(const librevenge::RVNGString &mimeType, const librevenge::RVNGBinaryData &data);

private:
	void writeEmbedded(OdfDocumentHandler *pHandler) const;

private:
	librevenge::RVNGString msFontFamily;
	shared_ptr<EmbeddedInfo> m_embeddedInfo;
};

class FontStyleManager : public StyleManager
{

public:
	FontStyleManager() : mStyleHash() {}
	virtual ~FontStyleManager()
	{
		clean();
	}

	/* create a new font if the font does not exists and returns a font name

	Note: the returned font name is actually equalled to psFontFamily
	*/
	librevenge::RVNGString findOrAdd(const char *psFontFamily);

	/** Set given font as embedded with given data.
	 */
	void setEmbedded(const librevenge::RVNGString &name, const librevenge::RVNGString &mimeType, const librevenge::RVNGBinaryData &data);

	virtual void clean();
	virtual void write(OdfDocumentHandler *) const {}
	virtual void write(OdfDocumentHandler *, Style::Zone zone) const;

protected:
	// style name -> SpanStyle
	std::map<librevenge::RVNGString, shared_ptr<FontStyle> > mStyleHash;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
