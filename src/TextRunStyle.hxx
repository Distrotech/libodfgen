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

#ifndef _TEXTRUNSTYLE_HXX_
#define _TEXTRUNSTYLE_HXX_

#include <map>

#include <librevenge/librevenge.h>

#include "FilterInternal.hxx"

#include "Style.hxx"

class OdfDocumentHandler;

class ParagraphStyle : public Style
{
public:
	ParagraphStyle(librevenge::RVNGPropertyList const &propList, const librevenge::RVNGString &sName, Style::Zone zone);
	virtual ~ParagraphStyle();
	virtual void write(OdfDocumentHandler *pHandler) const;

private:
	librevenge::RVNGPropertyList mpPropList;
};


class SpanStyle : public Style
{
public:
	SpanStyle(const char *psName, const librevenge::RVNGPropertyList &xPropList, Style::Zone zone);
	virtual void write(OdfDocumentHandler *pHandler) const;
private:
	librevenge::RVNGPropertyList mPropList;
};

class ParagraphStyleManager : public StyleManager
{
public:
	ParagraphStyleManager() : mHashNameMap(), mStyleHash(), mDisplayNameMap() {}
	virtual ~ParagraphStyleManager()
	{
		clean();
	}

	/* create a new style if it does not exists. In all case, returns the name of the style

	Note: using S%i(or S_M%i) as new name*/
	librevenge::RVNGString findOrAdd(const librevenge::RVNGPropertyList &xPropList, Style::Zone zone=Style::Z_Unknown);

	/* returns the style corresponding to a given name ( if it exists ) */
	shared_ptr<ParagraphStyle> const get(const librevenge::RVNGString &name) const;
	//! return the file name corresponding to a display name
	librevenge::RVNGString getFinalDisplayName(const librevenge::RVNGString &displayName);

	virtual void clean();
	// write all
	virtual void write(OdfDocumentHandler *pHandler) const
	{
		write(pHandler, Style::Z_Style);
		write(pHandler, Style::Z_StyleAutomatic);
		write(pHandler, Style::Z_ContentAutomatic);
	}
	// write automatic/named style
	void write(OdfDocumentHandler *pHandler, Style::Zone zone) const;

protected:
	// hash key -> name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mHashNameMap;
	// style name -> paragraph style
	std::map<librevenge::RVNGString, shared_ptr<ParagraphStyle> > mStyleHash;
	// display name -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mDisplayNameMap;
};

class SpanStyleManager : public StyleManager
{
public:
	SpanStyleManager() : mHashNameMap(), mStyleHash(), 	mDisplayNameMap() {}
	virtual ~SpanStyleManager()
	{
		clean();
	}

	/* create a new style if it does not exists. In all case, returns the name of the style

	Note: using Span%i (or Span_M%i) as new name*/
	librevenge::RVNGString findOrAdd(const librevenge::RVNGPropertyList &xPropList, Style::Zone zone=Style::Z_Unknown);
	/* returns the style corresponding to a given name ( if it exists ) */
	shared_ptr<SpanStyle> const get(const librevenge::RVNGString &name) const;
	/** append the span in the element, ie. the stroke, pattern, bitmap, marker properties */
	static void addSpanProperties(librevenge::RVNGPropertyList const &style, librevenge::RVNGPropertyList &element);
	//! return the file name corresponding to a display name
	librevenge::RVNGString getFinalDisplayName(const librevenge::RVNGString &displayName);

	virtual void clean();
	// write all
	virtual void write(OdfDocumentHandler *pHandler) const
	{
		write(pHandler, Style::Z_Style);
		write(pHandler, Style::Z_StyleAutomatic);
		write(pHandler, Style::Z_ContentAutomatic);
	}
	// write automatic/named style
	void write(OdfDocumentHandler *pHandler, Style::Zone zone) const;

protected:
	// hash key -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mHashNameMap;
	// style name -> SpanStyle
	std::map<librevenge::RVNGString, shared_ptr<SpanStyle> > mStyleHash;
	// display name -> style name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mDisplayNameMap;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
