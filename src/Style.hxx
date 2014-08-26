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

#ifndef _STYLE_HXX_
#define _STYLE_HXX_
#include <librevenge/librevenge.h>

#include "DocumentElement.hxx"

class Style
{
public:
	/** the different zone ( Z_ContentAutomatic: automatic zone, Z_Style:
		must be stored in the styles, Z_StyleAutomatic: must be
		stored in the content automatic zone, Z_Font: must be store in
		the font declaration part) */
	enum Zone { Z_ContentAutomatic, Z_Style, Z_StyleAutomatic, Z_Font, Z_Unknown };
	Style(const librevenge::RVNGString &psName, Zone type=Z_ContentAutomatic) : msName(psName), mZone(type) {}
	virtual ~Style() {}
	//! return the zone
	Zone getZone() const
	{
		return mZone;
	}
	virtual void write(OdfDocumentHandler *) const {};
	const librevenge::RVNGString &getName() const
	{
		return msName;
	}

private:
	//! the style name
	librevenge::RVNGString msName;
	//! the style zone
	Zone mZone;
};

class StyleManager
{
public:
	StyleManager() {}
	virtual ~StyleManager() {}

	virtual void clean() {};
	virtual void write(OdfDocumentHandler *) const = 0;

private:
	// forbide copy constructor/operator
	StyleManager(const StyleManager &);
	StyleManager &operator=(const StyleManager &);
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
