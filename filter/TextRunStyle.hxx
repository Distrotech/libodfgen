/* TextRunStyle: Stores (and writes) paragraph/span-style-based information
 * (e.g.: a paragraph might be bold) that is needed at the head of an OO
 * document.
 *
 * Copyright (C) 2002-2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#ifndef _TEXTRUNSTYLE_H
#define _TEXTRUNSTYLE_H
#include <libwpd/libwpd.h>

#include "Style.hxx"

class TagOpenElement;
class DocumentElement;
class DocumentHandler;

class SpanStyle : public Style
{
public:
	SpanStyle(const uint32_t iTextAttributeBits, const char *pFontName, const float fFontSize, const char *pFontColor,
		  const char *pBackgroundColor, const char *psName);
	virtual void write(DocumentHandler &xHandler) const;
	const int getTextAttributeBits() const { return miTextAttributeBits; }
	const UTF8String & getFontName() const { return msFontName; }
	const float getFontSize() const { return mfFontSize; }

	void _addTextProperties(TagOpenElement *pStylePropertiesOpenElement) const;

private:
	int miTextAttributeBits;
	UTF8String msFontName;
	float mfFontSize;
	UTF8String msFontColor;
	UTF8String msHighlightColor;
};
#if 0
class ParagraphStyle : public Style, public TopLevelElementStyle
{
public:
	ParagraphStyle(const uint8_t iParagraphJustification,
			const float fMarginLeft, const float fMarginRight, const float fTextIndent, const float fLineSpacing,
			const float fSpacingBeforeParagraph, const float fSpacingAfterParagraph, const vector<WPXTabStop> &tabStops, 
			const bool bColumnBreak, const bool bPageBreak, const char *psName, const char *psParentName);

	virtual ~ParagraphStyle();

	void setListStyleName(UTF8String &sListStyleName) { delete mpsListStyleName ; mpsListStyleName = new UTF8String(sListStyleName); }
	virtual void write(DocumentHandler &xHandler) const;
	const virtual bool isParagraphStyle() const { return true; }

private:
	UTF8String msParentName;
	UTF8String *mpsListStyleName;
	float mfMarginLeft;
	float mfMarginRight;
	float mfTextIndent;
	float mfLineSpacing;
	float mfSpacingBeforeParagraph;
	float mfSpacingAfterParagraph;
	uint8_t miParagraphJustification;
	vector<WPXTabStop> mTabStops;
	int miNumTabStops;
	bool mbColumnBreak;
	bool mbPageBreak;
};
#endif

class ParagraphStyle
{
public:
	ParagraphStyle(WPXPropertyList *propList, const vector<WPXTabStop> &tabStops, const UTF8String &sName);
	virtual ~ParagraphStyle();
	virtual void write(DocumentHandler &xHandler) const;
	UTF8String getName() const { return msName; }
private:
	WPXPropertyList *mpPropList;
	vector<WPXTabStop> mxTabStops;
	UTF8String msName;
};
#endif
