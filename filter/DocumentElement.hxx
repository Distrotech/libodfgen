/* DocumentElement: The items we are collecting to be put into the Writer
 * document: paragraph and spans of text, as well as section breaks.
 *
 * Copyright (C) 2002-2003 William Lachance (william.lachance@sympatico.ca)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * For further information visit http://libwpd.sourceforge.net
 *
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef _DOCUMENTELEMENT_H
#define _DOCUMENTELEMENT_H
#include <libwpd/libwpd.h>
#include <libwpd/WPXProperty.h>
#include <libwpd/libwpd_support.h>
#include <vector>

using namespace std;

class DocumentHandler
{
public:
        virtual void startDocument() = 0;
        virtual void endDocument() = 0;
        virtual void startElement(const UTF8String &sName, const WPXPropertyList &xPropList) = 0;
        virtual void endElement(const UTF8String &sName) = 0;
        virtual void characters(const UTF8String &sCharacters) = 0;
};

class DocumentElement
{
public:	
	virtual ~DocumentElement() {}
	virtual void write(DocumentHandler &xHandler) const = 0;
	virtual void print() const {}
};

class TagElement : public DocumentElement
{
public:
	TagElement(const char *szTagName) : msTagName(szTagName) {}
	const UTF8String & getTagName() const { return msTagName; }
	virtual void print() const;
private:
	UTF8String msTagName;
};

class TagOpenElement : public TagElement
{
public:
	TagOpenElement(const char *szTagName) : TagElement(szTagName) {}
	~TagOpenElement() {}
	void addAttribute(const char *szAttributeName, const UTF8String &sAttributeValue);
	virtual void write(DocumentHandler &xHandler) const;
	virtual void print () const;
private:
	WPXPropertyList maAttrList;
};

class TagCloseElement : public TagElement
{
public:
	TagCloseElement(const char *szTagName) : TagElement(szTagName) {}
	virtual void write(DocumentHandler &xHandler) const;
};

class CharDataElement : public DocumentElement
{
public:
	CharDataElement(const char *sData) : DocumentElement(), msData(sData) {}
	virtual void write(DocumentHandler &xHandler) const;
private:
	UTF8String msData;
};

class TextElement : public DocumentElement
{
public:
	TextElement(const UTF8String & sTextBuf);
	virtual void write(DocumentHandler &xHandler) const;

private:
	UTF8String msTextBuf;
};
 
#endif
