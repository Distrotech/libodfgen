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
#include <map>
#include <set>
#include <vector>

#include "FilterInternal.hxx"

#include "Style.hxx"

class DocumentElement;
class OdfDocumentHandler;

class PageSpan
{
protected:
	enum ContentType
	{
		C_Header=0, C_HeaderFirst, C_HeaderLeft, C_HeaderLast,
		C_Footer, C_FooterFirst, C_FooterLeft, C_FooterLast,
		C_Master,
		C_NumContentTypes // keep this one last
	};
public:
	PageSpan(const librevenge::RVNGPropertyList &xPropList, librevenge::RVNGString const &masterPageName, librevenge::RVNGString const &layoutName, librevenge::RVNGString const &pageDrawingName, bool isMasterPage);
	virtual ~PageSpan();
	//! writes the page layout style: Z_StyleAutomatic and the page drawing style: Z_ContentAutomatic(if defined)
	void writePageStyle(OdfDocumentHandler *pHandler, Style::Zone zone) const;
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
	//! returns the page drawing name of the span's pageDrawing page
	librevenge::RVNGString getDisplayPageDrawingName() const
	{
		return msPageDrawingName;
	}
	//! returns the name of the span's page drawing
	librevenge::RVNGString getPageDrawingName() const
	{
		return protectString(msPageDrawingName);
	}
	/** reset the page size (used by Od[pg]Generator to reset the size to union size)
		and the margins to 0
		\note size are given in inch
	 */
	void resetPageSizeAndMargins(double width, double height);
	void setHeaderContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_Header, pContent);
	}
	void setFooterContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_Footer, pContent);
	}
	void setHeaderLeftContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_HeaderLeft, pContent);
	}
	void setFooterLeftContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_FooterLeft, pContent);
	}
	void setHeaderFirstContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_HeaderFirst, pContent);
	}
	void setFooterFirstContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_FooterFirst, pContent);
	}
	void setHeaderLastContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_HeaderLast, pContent);
	}
	void setFooterLastContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_FooterLast, pContent);
	}
	void setMasterContent(libodfgen::DocumentElementVector *pContent)
	{
		storeContent(C_Master, pContent);
	}

	/** small function which mainly replaced space by underscore */
	static librevenge::RVNGString protectString(librevenge::RVNGString const &orig);
protected:
	void storeContent(ContentType type, libodfgen::DocumentElementVector *pContent);
	void _writeContent(const char *contentTagName, const libodfgen::DocumentElementVector &content,
	                   OdfDocumentHandler *pHandler) const;
private:
	PageSpan(const PageSpan &);
	PageSpan &operator=(const PageSpan &);
	librevenge::RVNGPropertyList mxPropList;
	//! flag to know if this is a master page
	bool mbIsMasterPage;
	//! the page master display name
	librevenge::RVNGString msMasterPageName;
	//! the layout display name
	librevenge::RVNGString msLayoutName;
	//! the page drawing display name
	librevenge::RVNGString msPageDrawingName;
	libodfgen::DocumentElementVector *(mpContent[C_NumContentTypes]);
};

//! class used to store the list of created page
class PageSpanManager
{
public:
	//! constructor
	PageSpanManager() : mpPageList(), mpNameToMasterPageMap(),
		mpPageMasterNameSet(), miCurrentPageMasterIndex(0),
		mpLayoutNameSet(), miCurrentLayoutIndex(0),
		mpPageDrawingNameSet(), miCurrentPageDrawingIndex(0)
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
	PageSpan *add(const librevenge::RVNGPropertyList &xPropList, bool masterPage=false);
	//! return the page span which correspond to a master name
	PageSpan *get(librevenge::RVNGString const &name);
	//! write the pages' layouts (style automatic) or the pages' drawing styles(content automatic)
	void writePageStyles(OdfDocumentHandler *pHandler, Style::Zone zone) const;
	void writeMasterPages(OdfDocumentHandler *pHandler) const;

	/** reset all page sizes (used by OdgGenerator to reset the size to union size)
		and the margins to 0
		\note size are given in inch
	 */
	void resetPageSizeAndMargins(double width, double height);

protected:
	//! the list of page
	std::vector<shared_ptr<PageSpan> > mpPageList;
	//! a map master page name to pagespan
	std::map<librevenge::RVNGString, shared_ptr<PageSpan> > mpNameToMasterPageMap;
	//! the list of page master name
	std::set<librevenge::RVNGString> mpPageMasterNameSet;
	//! the current page master index (use to create a new page name)
	int miCurrentPageMasterIndex;
	//! the list of layout name
	std::set<librevenge::RVNGString> mpLayoutNameSet;
	//! the current layout index (use to create a new page name)
	int miCurrentLayoutIndex;
	//! the list of page drawing name
	std::set<librevenge::RVNGString> mpPageDrawingNameSet;
	//! the current page drawing index (use to create a new page name)
	int miCurrentPageDrawingIndex;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
