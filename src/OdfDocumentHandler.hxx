/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2004 William Lachance (wlach@interlog.com)
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
#ifndef _ODFDOCUMENTHANDLER_HXX_
#define _ODFDOCUMENTHANDLER_HXX_
#include <libwpd/libwpd.h>

enum OdfStreamType { ODF_FLAT_XML, ODF_CONTENT_XML, ODF_STYLES_XML, ODF_SETTINGS_XML, ODF_META_XML };

class OdfDocumentHandler
{
public:
	OdfDocumentHandler() {};
	virtual ~OdfDocumentHandler() {};

	virtual void startDocument() = 0;
	virtual void endDocument() = 0;
	virtual void startElement(const char *psName, const WPXPropertyList &xPropList) = 0;
	virtual void endElement(const char *psName) = 0;
	virtual void characters(const WPXString &sCharacters) = 0;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
