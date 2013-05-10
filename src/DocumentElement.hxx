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

#ifndef _DOCUMENTELEMENT_HXX_
#define _DOCUMENTELEMENT_HXX_

#include <libwpd/libwpd.h>
#include <libwpd/WPXProperty.h>
#include <libwpd/WPXString.h>
#include <vector>

#include <libodfgen/libodfgen.hxx>

class DocumentElement
{
public:
	virtual ~DocumentElement() {}
	virtual void write(OdfDocumentHandler *pHandler) const = 0;
	virtual void print() const {}
};

class TagElement : public DocumentElement
{
public:
	virtual ~TagElement() {}
	TagElement(const WPXString &szTagName) : msTagName(szTagName) {}
	const WPXString &getTagName() const
	{
		return msTagName;
	}
	virtual void print() const;
private:
	WPXString msTagName;
};

class TagOpenElement : public TagElement
{
public:
	TagOpenElement(const WPXString &szTagName) : TagElement(szTagName), maAttrList() {}
	virtual ~TagOpenElement() {}
	void addAttribute(const WPXString &szAttributeName, const WPXString &sAttributeValue);
	virtual void write(OdfDocumentHandler *pHandler) const;
	virtual void print () const;
private:
	WPXPropertyList maAttrList;
};

class TagCloseElement : public TagElement
{
public:
	TagCloseElement(const WPXString &szTagName) : TagElement(szTagName) {}
	virtual ~TagCloseElement() {}
	virtual void write(OdfDocumentHandler *pHandler) const;
};

class CharDataElement : public DocumentElement
{
public:
	CharDataElement(const WPXString &sData) : DocumentElement(), msData(sData) {}
	virtual ~CharDataElement() {}
	virtual void write(OdfDocumentHandler *pHandler) const;
private:
	WPXString msData;
};

class TextElement : public DocumentElement
{
public:
	TextElement(const WPXString &sTextBuf) : DocumentElement(), msTextBuf(sTextBuf, false) {}
	virtual ~TextElement() {}
	virtual void write(OdfDocumentHandler *pHandler) const;

private:
	WPXString msTextBuf;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
