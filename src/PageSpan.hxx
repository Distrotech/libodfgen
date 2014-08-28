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

#include <librevenge/librevenge.h>
#include <set>
#include <vector>

class DocumentElement;
class OdfDocumentHandler;

class PageSpan
{
public:
	PageSpan(const librevenge::RVNGPropertyList &xPropList, librevenge::RVNGString const &masterPageName, librevenge::RVNGString const &layoutName);
	virtual ~PageSpan();
	void writePageLayout(OdfDocumentHandler *pHandler) const;
	void writeMasterPages(OdfDocumentHandler *pHandler) const;
	int getSpan() const;
	//! returns the display name of the span's master page
	librevenge::RVNGString getDisplayMasterName() const
	{
		return msMasterPageName;
	}
	//! returns the name of the span's master page
	librevenge::RVNGString getMasterName() const
	{
		return protectString(msMasterPageName);
	}
	//! returns the display name of the span's layout page
	librevenge::RVNGString getDisplayLayoutName() const
	{
		return msLayoutName;
	}
	//! returns the name of the span's layout page
	librevenge::RVNGString getLayoutName() const
	{
		return protectString(msLayoutName);
	}

	void setHeaderContent(std::vector<DocumentElement *> *pHeaderContent);
	void setFooterContent(std::vector<DocumentElement *> *pFooterContent);
	void setHeaderLeftContent(std::vector<DocumentElement *> *pHeaderContent);
	void setFooterLeftContent(std::vector<DocumentElement *> *pFooterContent);
	void setHeaderFirstContent(std::vector<DocumentElement *> *pHeaderContent);
	void setFooterFirstContent(std::vector<DocumentElement *> *pFooterContent);
	void setHeaderLastContent(std::vector<DocumentElement *> *pHeaderContent);
	void setFooterLastContent(std::vector<DocumentElement *> *pFooterContent);

	/** small function which mainly replaced space by underscore */
	static librevenge::RVNGString protectString(librevenge::RVNGString const &orig);
protected:
	void _writeHeaderFooter(const char *headerFooterTagName, const std::vector<DocumentElement *> &headerFooterContent,
	                        OdfDocumentHandler *pHandler) const;
private:
	PageSpan(const PageSpan &);
	PageSpan &operator=(const PageSpan &);
	librevenge::RVNGPropertyList mxPropList;
	//! the page master display name
	librevenge::RVNGString msMasterPageName;
	//! the layout display name
	librevenge::RVNGString msLayoutName;
	std::vector<DocumentElement *> *mpHeaderContent;
	std::vector<DocumentElement *> *mpFooterContent;
	std::vector<DocumentElement *> *mpHeaderLeftContent;
	std::vector<DocumentElement *> *mpFooterLeftContent;
	std::vector<DocumentElement *> *mpHeaderFirstContent;
	std::vector<DocumentElement *> *mpFooterFirstContent;
	std::vector<DocumentElement *> *mpHeaderLastContent;
	std::vector<DocumentElement *> *mpFooterLastContent;
};

//! class used to store the list of created page
class PageSpanManager
{
public:
	//! constructor
	PageSpanManager() : mpPageList(),
		mpPageMasterNameSet(), miCurrentPageMasterIndex(0),
		mpLayoutNameSet(), miCurrentLayoutIndex(0)
	{
	}
	//! destructor
	~PageSpanManager()
	{
		clean();
	}
	//! clean data
	void clean();
	//! create a new page and set it to current. Returns a pointer to this new page
	PageSpan *add(const librevenge::RVNGPropertyList &xPropList);
	//! returns the current page span or 0
	PageSpan *getCurrentPageSpan();

	void writePageLayout(OdfDocumentHandler *pHandler) const;
	void writeMasterPages(OdfDocumentHandler *pHandler) const;


protected:
	//! the list of page
	std::vector<shared_ptr<PageSpan> > mpPageList;
	//! the list of page master name
	std::set<librevenge::RVNGString> mpPageMasterNameSet;
	//! the current page master index (use to create a new page name)
	int miCurrentPageMasterIndex;
	//! the list of layout name
	std::set<librevenge::RVNGString> mpLayoutNameSet;
	//! the current layout index (use to create a new page name)
	int miCurrentLayoutIndex;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
