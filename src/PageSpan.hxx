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
#ifndef _PAGESPAN_HXX_
#define _PAGESPAN_HXX_

#include <libwpd/libwpd.h>
#include <vector>

class DocumentElement;
class OdfDocumentHandler;

class PageSpan
{
public:
	PageSpan(const WPXPropertyList &xPropList);
	virtual ~PageSpan();
	void writePageLayout(const int iNum, OdfDocumentHandler *pHandler) const;
	void writeMasterPages(const int iStartingNum, const int iPageLayoutNum, const bool bLastPageSpan, OdfDocumentHandler *pHandler) const;
	int getSpan() const;

	void setHeaderContent(std::vector<DocumentElement *> *pHeaderContent);
	void setFooterContent(std::vector<DocumentElement *> *pFooterContent);
	void setHeaderLeftContent(std::vector<DocumentElement *> *pHeaderContent);
	void setFooterLeftContent(std::vector<DocumentElement *> *pFooterContent);
protected:
	void _writeHeaderFooter(const char *headerFooterTagName, const std::vector<DocumentElement *> &headerFooterContent,
	                        OdfDocumentHandler *pHandler) const;
private:
	PageSpan(const PageSpan &);
	PageSpan &operator=(const PageSpan &);
	WPXPropertyList mxPropList;
	std::vector<DocumentElement *> *mpHeaderContent;
	std::vector<DocumentElement *> *mpFooterContent;
	std::vector<DocumentElement *> *mpHeaderLeftContent;
	std::vector<DocumentElement *> *mpFooterLeftContent;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
