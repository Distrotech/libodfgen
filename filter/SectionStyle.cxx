/* SectionStyle: Stores (and writes) section-based information (e.g.: a column
 * break needs a new section) that is needed at the head of an OO document and
 * is referenced throughout the entire document
 *
 * Copyright (C) 2002-2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (c) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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
#include "SectionStyle.hxx"
#include "DocumentElement.hxx"
#include <math.h>

#ifdef _MSC_VER
double rint(double x);
#endif /* _WIN32 */

const float fDefaultSideMargin = 1.0f; // inches
const float fDefaultPageWidth = 8.5f; // inches (OOo required default: we will handle this later)
const float fDefaultPageHeight = 11.0f; // inches

SectionStyle::SectionStyle(const int iNumColumns, const vector<WPXColumnDefinition> &columns, const char *psName) : Style(psName),
	miNumColumns(iNumColumns)
{

	for (int i=0; i<columns.size(); i++)
		mColumns.push_back(columns[i]);
	WRITER_DEBUG_MSG(("WriterWordPerfect: Created a new set of section props with this no. of columns: %i and this name: %s\n", 
	       (int)miNumColumns, (const char *)getName()));	
}

void SectionStyle::write(DocumentHandler &xHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "section");
	styleOpen.write(xHandler);

	// if miNumColumns <= 1, we will never come here. This is only additional check
	if (miNumColumns > 1)
	{		
		// style properties
		TagOpenElement stylePropertiesOpen("style:properties");
		stylePropertiesOpen.addAttribute("text:dont-balance-text-columns", "false");
		stylePropertiesOpen.write(xHandler);

		// column properties
		TagOpenElement columnsOpen("style:columns");
		UTF8String sColumnCount;
		sColumnCount.sprintf("%i", miNumColumns);
		columnsOpen.addAttribute("fo:column-count", sColumnCount.getUTF8());
		columnsOpen.write(xHandler);
	
		UTF8String sRelWidth, sMarginLeft, sMarginRight;
		for (int i=0; i<miNumColumns; i++)
		{
			TagOpenElement columnOpen("style:column");
			// The "style:rel-width" is expressed in twips (1440 twips per inch) and includes the left and right Gutter
			sRelWidth.sprintf("%i*", (int)rint(mColumns[i].m_width * 1440.0f));
			columnOpen.addAttribute("style:rel-width", sRelWidth.getUTF8());
			sMarginLeft.sprintf("%.4finch", mColumns[i].m_leftGutter);
			columnOpen.addAttribute("fo:margin-left", sMarginLeft.getUTF8());
			sMarginRight.sprintf("%.4finch", mColumns[i].m_rightGutter);
			columnOpen.addAttribute("fo:margin-right", sMarginRight.getUTF8());
			columnOpen.write(xHandler);
			
			TagCloseElement columnClose("style:column");
			columnClose.write(xHandler);
		}
	}

	xHandler.endElement(UTF8String::createFromAscii("style:columns"));
	xHandler.endElement(UTF8String::createFromAscii("style:properties"));
	xHandler.endElement(UTF8String::createFromAscii("style:style"));
}
