/* TableStyle: Stores (and writes) table-based information that is 
 * needed at the head of an OO document.
 *
 * Copyright (C) 2002-2004 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2004 Net Integration Technologies, Inc. (http://www.net-itech.com)
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
#include <math.h>
#include "FilterInternal.hxx"
#include "TableStyle.hxx"
#include "DocumentElement.hxx"

#ifdef _MSC_VER
#include <minmax.h>
#endif

TableCellStyle::TableCellStyle(const float fLeftBorderThickness, const float fRightBorderThickness, 
				const float fTopBorderThickness, const float fBottomBorderThickness, 
				const UTF8String &sColor, const UTF8String &sBorderColor,
				const WPXVerticalAlignment cellVerticalAlignment, const char *psName) :
	Style(psName),
	mfLeftBorderThickness(fLeftBorderThickness),
	mfRightBorderThickness(fRightBorderThickness),
	mfTopBorderThickness(fTopBorderThickness),
	mfBottomBorderThickness(fBottomBorderThickness),
	mCellVerticalAlignment(cellVerticalAlignment),	
	msColor(sColor),
	msBorderColor(sBorderColor)
{
}

void TableCellStyle::write(DocumentHandler &xHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table-cell");
	styleOpen.write(xHandler);

	TagOpenElement stylePropertiesOpen("style:properties");
	switch (mCellVerticalAlignment)
	{
	case TOP:
		stylePropertiesOpen.addAttribute("fo:vertical-align", "top");
		break;
	case MIDDLE:
		stylePropertiesOpen.addAttribute("fo:vertical-align", "middle");
		break;
	case BOTTOM:
		stylePropertiesOpen.addAttribute("fo:vertical-align", "bottom");
		break;
	case FULL: // OOo does not have the full vertical alignment
		break;
	default:
		break;
	}
	
	stylePropertiesOpen.addAttribute("fo:background-color", msColor.getUTF8());
	stylePropertiesOpen.addAttribute("fo:padding", "0.0382inch");
	
	UTF8String sBorderLeft;
	sBorderLeft.sprintf("%finch solid %s", mfLeftBorderThickness, msBorderColor.getUTF8());
	stylePropertiesOpen.addAttribute("fo:border-left", sBorderLeft.getUTF8());
	UTF8String sBorderRight;
	sBorderRight.sprintf("%finch solid %s", mfRightBorderThickness, msBorderColor.getUTF8());
	stylePropertiesOpen.addAttribute("fo:border-right", sBorderRight.getUTF8());
	UTF8String sBorderTop;
	sBorderTop.sprintf("%finch solid %s", mfTopBorderThickness, msBorderColor.getUTF8());
	stylePropertiesOpen.addAttribute("fo:border-top", sBorderTop.getUTF8());
	UTF8String sBorderBottom;
	sBorderBottom.sprintf("%finch solid %s", mfBottomBorderThickness, msBorderColor.getUTF8());
	stylePropertiesOpen.addAttribute("fo:border-bottom", sBorderBottom.getUTF8());
	stylePropertiesOpen.write(xHandler);
	xHandler.endElement("style:properties");

	xHandler.endElement("style:style");	
}

TableRowStyle::TableRowStyle(const float fHeight, const bool bIsMinimumHeight, const bool bIsHeaderRow, const char *psName):
	Style(psName),
	mfHeight(fHeight),
	mbIsMinimumHeight(bIsMinimumHeight),
	mbIsHeaderRow(bIsHeaderRow)
{
}

void TableRowStyle::write(DocumentHandler &xHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table-row");
	styleOpen.write(xHandler);
	
	if ((mfHeight != 0.0f) || (!mbIsMinimumHeight))
	{
		TagOpenElement stylePropertiesOpen("style:properties");
		UTF8String sRowHeight;
		sRowHeight.sprintf("%finch", mfHeight);
		if (mbIsMinimumHeight)
			stylePropertiesOpen.addAttribute("style:min-row-height", sRowHeight.getUTF8());
		else
			stylePropertiesOpen.addAttribute("style:row-height", sRowHeight.getUTF8());
		stylePropertiesOpen.write(xHandler);
		xHandler.endElement("style:properties");
	}
	
	xHandler.endElement("style:style");
		
}
	

TableStyle::TableStyle(const float fDocumentMarginLeft, const float fDocumentMarginRight, 
		       const float fMarginLeftOffset, const float fMarginRightOffset,
		       const uint8_t iTablePositionBits, const float fLeftOffset,
		       const vector < WPXColumnDefinition > &columns, const char *psName) : 
	Style(psName),
	mfDocumentMarginLeft(fDocumentMarginLeft),
	mfDocumentMarginRight(fDocumentMarginRight),
	mfMarginLeftOffset(fMarginLeftOffset),
	mfMarginRightOffset(fMarginRightOffset),
	miTablePositionBits(iTablePositionBits),
	mfLeftOffset(fLeftOffset),
	miNumColumns(columns.size())

{
	WRITER_DEBUG_MSG(("WriterWordPerfect: Created a new set of table props with this no. of columns repeated: %i and this name: %s\n",
	       (int)miNumColumns, (const char *)getName()));

	typedef vector<WPXColumnDefinition>::const_iterator CDVIter;
	for (CDVIter iterColumns = columns.begin() ; iterColumns != columns.end(); iterColumns++)
	{
		mColumns.push_back((*iterColumns));
	}
}

TableStyle::~TableStyle()
{
	typedef vector<TableCellStyle *>::iterator TCSVIter;
	for (TCSVIter iterTableCellStyles = mTableCellStyles.begin() ; iterTableCellStyles != mTableCellStyles.end(); iterTableCellStyles++)
		delete(*iterTableCellStyles);

}

void TableStyle::write(DocumentHandler &xHandler) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table");
	if (getMasterPageName())
		styleOpen.addAttribute("style:master-page-name", getMasterPageName()->getUTF8());
	styleOpen.write(xHandler);

	TagOpenElement stylePropertiesOpen("style:properties");

	UTF8String sTableMarginLeft;
	UTF8String sTableMarginRight;
	UTF8String sTableAlignment;
	char *pTableAlignment = NULL;
	if (miTablePositionBits == WPX_TABLE_POSITION_ALIGN_WITH_LEFT_MARGIN) {
		sTableAlignment.sprintf("left");
		sTableMarginLeft.sprintf("0inch");
	}
	else if (miTablePositionBits == WPX_TABLE_POSITION_ALIGN_WITH_RIGHT_MARGIN) {
		sTableAlignment.sprintf("right");
	}
	else if (miTablePositionBits == WPX_TABLE_POSITION_CENTER_BETWEEN_MARGINS) {
		sTableAlignment.sprintf("center");
	}
 	else if (miTablePositionBits == WPX_TABLE_POSITION_ABSOLUTE_FROM_LEFT_MARGIN) {
		sTableAlignment.sprintf("left");
		sTableMarginLeft.sprintf("%finch", (mfLeftOffset-mfDocumentMarginLeft+mfMarginLeftOffset));
 	}
	else if (miTablePositionBits == WPX_TABLE_POSITION_FULL) {
		sTableAlignment.sprintf("margins");
		sTableMarginLeft.sprintf("%finch", mfMarginLeftOffset);
		sTableMarginRight.sprintf("%finch", mfMarginRightOffset);
	}		
	stylePropertiesOpen.addAttribute("table:align", sTableAlignment.getUTF8());
	if (sTableMarginLeft.getUTF8())
		stylePropertiesOpen.addAttribute("fo:margin-left", sTableMarginLeft.getUTF8());
	if (sTableMarginRight.getUTF8())
		stylePropertiesOpen.addAttribute("fo:margin-right", sTableMarginRight.getUTF8());

 	float fTableWidth = 0;
 	typedef vector<WPXColumnDefinition>::const_iterator CDVIter;
 	for (CDVIter iterColumns2 = mColumns.begin() ; iterColumns2 != mColumns.end(); iterColumns2++)
 	{
 		fTableWidth += (*iterColumns2).m_width;
 	}
	UTF8String sTableWidth;
	sTableWidth.sprintf("%finch", fTableWidth);
	stylePropertiesOpen.addAttribute("style:width", sTableWidth.getUTF8());
	stylePropertiesOpen.write(xHandler);

	xHandler.endElement("style:properties");

	xHandler.endElement("style:style");
		
	int i=1;
	typedef vector<WPXColumnDefinition>::const_iterator CDVIter;
	for (CDVIter iterColumns = mColumns.begin() ; iterColumns != mColumns.end(); iterColumns++)
	{
		TagOpenElement styleOpen("style:style");
		UTF8String sColumnName;
		sColumnName.sprintf("%s.Column%i", getName()(), i);
		styleOpen.addAttribute("style:name", sColumnName);
		styleOpen.addAttribute("style:family", "table-column");
		styleOpen.write(xHandler);

		TagOpenElement stylePropertiesOpen("style:properties");
		UTF8String sColumnWidth;
		sColumnWidth.sprintf("%finch", (*iterColumns).m_width);
		stylePropertiesOpen.addAttribute("style:column-width", sColumnWidth.getUTF8());
		stylePropertiesOpen.write(xHandler);
		xHandler.endElement("style:properties");

		xHandler.endElement("style:style");

		i++;
	}

	typedef vector<TableRowStyle *>::const_iterator TRSVIter;
	for (TRSVIter iterTableRow = mTableRowStyles.begin() ; iterTableRow != mTableRowStyles.end(); iterTableRow++)
		(*iterTableRow)->write(xHandler);

	typedef vector<TableCellStyle *>::const_iterator TCSVIter;
	for (TCSVIter iterTableCell = mTableCellStyles.begin() ; iterTableCell != mTableCellStyles.end(); iterTableCell++)
		(*iterTableCell)->write(xHandler);
}
