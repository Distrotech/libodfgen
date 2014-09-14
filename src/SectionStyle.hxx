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
#ifndef _SECTIONSTYLE_HXX_
#define _SECTIONSTYLE_HXX_

#include <vector>

#include <librevenge/librevenge.h>

#include "Style.hxx"


class SectionStyle : public Style
{
public:
	SectionStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName, Style::Zone zone);
	virtual void write(OdfDocumentHandler *pHandler) const;

private:
	librevenge::RVNGPropertyList mPropList;
};


class SectionStyleManager : public StyleManager
{
public:
	SectionStyleManager() : mStyleList() {}
	virtual ~SectionStyleManager()
	{
		clean();
	}

	/* creates a new style and returns the name of the style

	Note: using Section%i (or Section_M%i) as new name*/
	librevenge::RVNGString add(const librevenge::RVNGPropertyList &xPropList, Style::Zone zone=Style::Z_Unknown);

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
	// the list of section
	std::vector<shared_ptr<SectionStyle> > mStyleList;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
