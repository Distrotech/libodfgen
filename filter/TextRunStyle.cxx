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
#include "FilterInternal.hxx"
#include "TextRunStyle.hxx"
#include "WriterProperties.hxx"
#include "DocumentElement.hxx"

#ifdef _MSC_VER
#include <minmax.h>
#endif

ParagraphStyle::ParagraphStyle(WPXPropertyList *pPropList, const vector<WPXTabStop> &xTabStops, const UTF8String &sName) :
	mpPropList(pPropList),
	mxTabStops(xTabStops),
	msName(sName)
{
}

ParagraphStyle::~ParagraphStyle()
{
	delete mpPropList;
}

void ParagraphStyle::write(DocumentHandler &xHandler) const
{
	WRITER_DEBUG_MSG(("Writing a paragraph style..\n"));

        WPXPropertyList propList;
	propList.insert("style:name", msName.getUTF8());
	propList.insert("style:family", "paragraph");
	propList.insert("style:parent-style-name", (*mpPropList)["style:parent-style-name"]->getStr());
	if ((*mpPropList)["style:master-page-name"])
		propList.insert("style:master-page-name", (*mpPropList)["style:master-page-name"]->getStr());
        xHandler.startElement("style:style", propList);

        propList.clear();
	WPXPropertyList::Iter i((*mpPropList));
	for (i; i.next(); )
	{
                if (i.key() == "list-style-name")
                        propList.insert("style:list-style-name", i()->getStr().getUTF8());

		if (i.key() == "margin-left")
			propList.insert("fo:margin-left", i()->getStr().getUTF8());
		if (i.key() == "margin-right")
			propList.insert("fo:margin-right", i()->getStr().getUTF8());
		if (i.key() == "text-indent")
			propList.insert("fo:text-indent", i()->getStr().getUTF8());
		if (i.key() == "margin-top")
			propList.insert("fo:margin-top", i()->getStr().getUTF8());
		if (i.key() == "margin-bottom")
			propList.insert("fo:margin-bottom", i()->getStr().getUTF8());
		if (i.key() == "line-spacing")
		{
			UTF8String sLineSpacing;
			sLineSpacing.sprintf("%.2f%%", i()->getFloat()*100.0f);
			propList.insert("fo:line-height", sLineSpacing.getUTF8());
		}
		if (i.key() == "column-break" && i()->getInt()) 
			propList.insert("fo:break-before", "column");	
		if (i.key() == "page-break" && i()->getInt()) 
			propList.insert("fo:break-before", "page");
		
		if (i.key() == "justification") 
		{
			switch (i()->getInt())
			{
			case WPX_PARAGRAPH_JUSTIFICATION_LEFT:
				// doesn't require a paragraph prop - it is the default, but, like, whatever
				propList.insert("fo:text-align", "left");
				break;
			case WPX_PARAGRAPH_JUSTIFICATION_CENTER:
				propList.insert("fo:text-align", "center");
				break;
			case WPX_PARAGRAPH_JUSTIFICATION_RIGHT:
				propList.insert("fo:text-align", "end");
				break;
			case WPX_PARAGRAPH_JUSTIFICATION_FULL:
				propList.insert("fo:text-align", "justify");
				break;
			case WPX_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES:
				propList.insert("fo:text-align", "justify");
				propList.insert("fo:text-align-last", "justify");
				break;
			}
		}
	}
	
	propList.insert("style:justify-single-word", "false");
	xHandler.startElement("style:properties", propList);

	if (mxTabStops.size() > 0)
	{
		TagOpenElement tabListOpen("style:tab-stops");
		tabListOpen.write(xHandler);
		for (int i=0; i<mxTabStops.size(); i++)
		{
			TagOpenElement tabStopOpen("style:tab-stop");
			UTF8String sPosition;
			sPosition.sprintf("%.4finch", mxTabStops[i].m_position);
			tabStopOpen.addAttribute("style:position", sPosition.getUTF8());
			WRITER_DEBUG_MSG(("Writing tab stops %s\n", sPosition.getUTF8()));
			switch (mxTabStops[i].m_alignment)
			{
			case RIGHT:
				tabStopOpen.addAttribute("style:type", "right");
				break;
			case CENTER:
				tabStopOpen.addAttribute("style:type", "center");
				break;
			case DECIMAL:
				tabStopOpen.addAttribute("style:type", "char");
				tabStopOpen.addAttribute("style:char", "."); // Assume a decimal point for the while
				break;
			default:  // Left alignment is the default one and BAR is not handled in OOo
				break;
			}

			if (mxTabStops[i].m_leaderCharacter != 0x0000)
			{
                                UTF8String sTempLeader;
                                sTempLeader.append(mxTabStops[i].m_leaderCharacter);
				tabStopOpen.addAttribute("style:leader-char", sTempLeader()); 
			}
			tabStopOpen.write(xHandler);
			xHandler.endElement("style:tab-stop");
			
		}
		xHandler.endElement("style:tab-stops");
	}

	xHandler.endElement("style:properties");
	xHandler.endElement("style:style");
}


#if 0
ParagraphStyle::ParagraphStyle(const uint8_t iParagraphJustification,
			       const float fMarginLeft, const float fMarginRight, const float fTextIndent, const float fLineSpacing,
			       const float fSpacingBeforeParagraph, const float fSpacingAfterParagraph, const vector<WPXTabStop> &tabStops,
			       const bool bColumnBreak, const bool bPageBreak, 
			       const char *psName, const char *psParentName) :
	Style(psName),
	msParentName(psParentName),
	mpsListStyleName(NULL),
	mfMarginLeft(fMarginLeft),
	mfMarginRight(fMarginRight),
	mfTextIndent(fTextIndent),
	mfLineSpacing(fLineSpacing),
	mfSpacingBeforeParagraph(fSpacingBeforeParagraph),
	mfSpacingAfterParagraph(fSpacingAfterParagraph),
	miParagraphJustification(iParagraphJustification),
	miNumTabStops(tabStops.size()),
	mbColumnBreak(bColumnBreak),
	mbPageBreak(bPageBreak)
{
	for (int i=0; i<miNumTabStops; i++)
		mTabStops.push_back(tabStops[i]);
}

ParagraphStyle::~ParagraphStyle()
{
	if (mpsListStyleName)
		delete mpsListStyleName;
}

void ParagraphStyle::write(DocumentHandler &xHandler) const
{
	WRITER_DEBUG_MSG(("Writing a paragraph style..\n"));
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "paragraph");
	styleOpen.addAttribute("style:parent-style-name", msParentName);
	if (getMasterPageName())
		styleOpen.addAttribute("style:master-page-name", getMasterPageName().getUTF8());
	if (mpsListStyleName)
		styleOpen.addAttribute("style:list-style-name", mpsListStyleName.getUTF8());
	styleOpen.write(xHandler);

	TagOpenElement stylePropertiesOpen("style:properties");
	// margin properties
	if (mfMarginLeft != 0.0f || mfMarginRight != 0.0f || mfTextIndent != 0.0f)
	{
		UTF8String sMarginLeft;
		sMarginLeft.sprintf("%finch", mfMarginLeft);
		UTF8String sMarginRight;
		sMarginRight.sprintf("%finch", mfMarginRight);
		UTF8String sTextIndent;
		sTextIndent.sprintf("%finch", mfTextIndent);
		propList.insert("fo:margin-left", sMarginLeft.getUTF8());
		propList.insert("fo:margin-right", sMarginRight.getUTF8());
		propList.insert("fo:text-indent", sTextIndent.getUTF8());
	}
	// line spacing
	if (mfLineSpacing != 1.0f) {
		UTF8String sLineSpacing;
		sLineSpacing.sprintf("%.2f%%", mfLineSpacing*100.0f);
		propList.insert("fo:line-height", sLineSpacing.getUTF8());
	}
	if (mfSpacingAfterParagraph != 0.0f || mfSpacingBeforeParagraph != 0.0f) {
		UTF8String sSpacingAfterParagraph;
		sSpacingAfterParagraph.sprintf("%finch", mfSpacingAfterParagraph);
		UTF8String sSpacingBeforeParagraph;
		sSpacingBeforeParagraph.sprintf("%finch", mfSpacingBeforeParagraph);
		propList.insert("fo:margin-top", sSpacingBeforeParagraph.getUTF8());
		propList.insert("fo:margin-bottom", sSpacingAfterParagraph.getUTF8());
	}

	// column break
	if (mbColumnBreak) {
		propList.insert("fo:break-before", "column");
	}

	if (mbPageBreak) {
		propList.insert("fo:break-before", "page");
	}

	WRITER_DEBUG_MSG(("WriterWordPerfect: Adding justification style props: %i\n", miParagraphJustification));
	switch (miParagraphJustification)
		{
		case WPX_PARAGRAPH_JUSTIFICATION_LEFT:
			// doesn't require a paragraph prop - it is the default, but, like, whatever
			propList.insert("fo:text-align", "left");
			break;
		case WPX_PARAGRAPH_JUSTIFICATION_CENTER:
			propList.insert("fo:text-align", "center");
			break;
		case WPX_PARAGRAPH_JUSTIFICATION_RIGHT:
			propList.insert("fo:text-align", "end");
			break;
		case WPX_PARAGRAPH_JUSTIFICATION_FULL:
			propList.insert("fo:text-align", "justify");
			break;
		case WPX_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES:
			propList.insert("fo:text-align", "justify");
			propList.insert("fo:text-align-last", "justify");
			break;
	}
	propList.insert("style:justify-single-word", "false");
	stylePropertiesOpen.write(xHandler);
	WRITER_DEBUG_MSG(("Writing %i tab stops\n", miNumTabStops));
	if (miNumTabStops > 0)
	{
		TagOpenElement tabListOpen("style:tab-stops");
		tabListOpen.write(xHandler);
		for (int i=0; i<miNumTabStops; i++)
		{
			TagOpenElement tabStopOpen("style:tab-stop");
			UTF8String sPosition;
			sPosition.sprintf("%.4finch", mTabStops[i].m_position);
			tabStopOpen.addAttribute("style:position", sPosition.getUTF8());
			WRITER_DEBUG_MSG(("Writing tab stops %s\n", sPosition.getUTF8()));
			switch (mTabStops[i].m_alignment)
			{
			case RIGHT:
				tabStopOpen.addAttribute("style:type", "right");
				break;
			case CENTER:
				tabStopOpen.addAttribute("style:type", "center");
				break;
			case DECIMAL:
				tabStopOpen.addAttribute("style:type", "char");
				tabStopOpen.addAttribute("style:char", "."); // Assume a decimal point for the while
				break;
			default:  // Left alignment is the default one and BAR is not handled in OOo
				break;
			}
			UCSString tempLeaderCharacter;
			tempLeaderCharacter.clear();
			if (mTabStops[i].m_leaderCharacter != 0x0000)
			{
				tempLeaderCharacter.append((uint32_t) mTabStops[i].m_leaderCharacter);
				UTF8String leaderCharacter(tempLeaderCharacter);
				tabStopOpen.addAttribute("style:leader-char", leaderCharacter.getUTF8()); 
			}
			tabStopOpen.write(xHandler);
			xHandler.endElement("style:tab-stop");
			
		}
		xHandler.endElement("style:tab-stops");
	}

	xHandler.endElement("style:properties");
	xHandler.endElement("style:style");
}
#endif
SpanStyle::SpanStyle(const uint32_t iTextAttributeBits, const char *pFontName, const float fFontSize,
		     const char *pFontColor, const char *pHighlightColor, const char *psName) :
	Style(psName),
	miTextAttributeBits(iTextAttributeBits),
	msFontName(pFontName),
	mfFontSize(fFontSize),
	msFontColor(pFontColor),
	msHighlightColor(pHighlightColor)
{
}

void SpanStyle::write(DocumentHandler &xHandler) const
{
	WRITER_DEBUG_MSG(("Writing a span style..\n"));
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "text");
	styleOpen.write(xHandler);

	TagOpenElement stylePropertiesOpen("style:properties");
 	_addTextProperties(&stylePropertiesOpen);
	stylePropertiesOpen.write(xHandler);

	xHandler.endElement("style:properties");
	xHandler.endElement("style:style");
}

void SpanStyle::_addTextProperties(TagOpenElement *pStylePropertiesOpenElement) const
{
 	if (miTextAttributeBits & WPX_SUPERSCRIPT_BIT) {
		UTF8String sSuperScript;
		sSuperScript.sprintf("super %s", IMP_DEFAULT_SUPER_SUB_SCRIPT);
		pStylePropertiesOpenElement->addAttribute("style:text-position", sSuperScript.getUTF8());
	}
 	if (miTextAttributeBits & WPX_SUBSCRIPT_BIT) {
		UTF8String sSubScript;
		sSubScript.sprintf("sub %s", IMP_DEFAULT_SUPER_SUB_SCRIPT);
		pStylePropertiesOpenElement->addAttribute("style:text-position", sSubScript.getUTF8());
	}
	if (miTextAttributeBits & WPX_ITALICS_BIT) {
		pStylePropertiesOpenElement->addAttribute("fo:font-style", "italic");
	}
	if (miTextAttributeBits & WPX_BOLD_BIT) {
		pStylePropertiesOpenElement->addAttribute("fo:font-weight", "bold");
	}
	if (miTextAttributeBits & WPX_STRIKEOUT_BIT) {
		pStylePropertiesOpenElement->addAttribute("style:text-crossing-out", "single-line");
	}
 	if (miTextAttributeBits & WPX_UNDERLINE_BIT) {
		pStylePropertiesOpenElement->addAttribute("style:text-underline", "single");
	}
	if (miTextAttributeBits & WPX_DOUBLE_UNDERLINE_BIT) {
		pStylePropertiesOpenElement->addAttribute("style:text-underline", "double");
	}
	if (miTextAttributeBits & WPX_OUTLINE_BIT) {
		pStylePropertiesOpenElement->addAttribute("style:text-outline", "true");
	}
	if (miTextAttributeBits & WPX_SMALL_CAPS_BIT) {
		pStylePropertiesOpenElement->addAttribute("fo:font-variant", "small-caps");
	}
	if (miTextAttributeBits & WPX_BLINK_BIT) {
		pStylePropertiesOpenElement->addAttribute("style:text-blinking", "true");
	}
	if (miTextAttributeBits & WPX_SHADOW_BIT) {
		pStylePropertiesOpenElement->addAttribute("fo:text-shadow", "1pt 1pt");
	}

	pStylePropertiesOpenElement->addAttribute("style:font-name", msFontName.getUTF8());
	UTF8String sFontSize;
	sFontSize.sprintf("%ipt", (int)mfFontSize);
	pStylePropertiesOpenElement->addAttribute("fo:font-size", sFontSize.getUTF8());

	if (!(miTextAttributeBits & WPX_REDLINE_BIT))
	// Here we give the priority to the redline bit over the font color. This is how WordPerfect behaves:
	// redline overrides font color even if the color is changed when redline was already defined.
	// When redline finishes, the color is back.
	{
		pStylePropertiesOpenElement->addAttribute("fo:color", msFontColor.getUTF8());
	}
	else // redlining applies
	{
		pStylePropertiesOpenElement->addAttribute("fo:color", "#ff3333"); // #ff3333 = a nice bright red
	}

	if (!(strcmp(msHighlightColor.getUTF8(), "#FFFFFF")) && !(strcmp(msHighlightColor.getUTF8(), "#ffffff"))) // do not apply the highlight if it is white (WLACH_REFACTORING: libwpd should not pass it at all..)
	{
		pStylePropertiesOpenElement->addAttribute("style:text-background-color", msHighlightColor.getUTF8());
	}
	else
		pStylePropertiesOpenElement->addAttribute("style:text-background-color", "transparent");

}
