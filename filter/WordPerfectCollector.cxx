/* WordPerfectCollector: Collects sections and runs of text from a
 * wordperfect file (and styles to go along with them) and writes them
 * to a Writer target document
 *
 * Copyright (C) 2002-2004 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003-2004 Net Integration Technologies (http://www.net-itech.com)
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

#include <libwpd/libwpd.h>
#include <string.h> // for strcmp

#include "WordPerfectCollector.hxx"
#include "DocumentElement.hxx"
#include "TextRunStyle.hxx"
#include "FontStyle.hxx"
#include "ListStyle.hxx"
#include "PageSpan.hxx"
#include "SectionStyle.hxx"
#include "TableStyle.hxx"
#include "FilterInternal.hxx"
#include "WriterProperties.hxx"

_WriterDocumentState::_WriterDocumentState() :
	mbFirstElement(true),
	mbInFakeSection(false),
	mbListElementOpenedAtCurrentLevel(false),
	mbTableCellOpened(false),
	mbHeaderRow(false)
{
}

WordPerfectCollector::WordPerfectCollector() :
	mbUsed(false),
	miNumSections(0),
	miCurrentNumColumns(1),
	mfSectionSpaceAfter(0.0f),
	miNumTables(0),
	miNumListStyles(0),
	mpCurrentContentElements(&mBodyElements),
	mpCurrentPageSpan(NULL),
	miNumPageStyles(0),
	mpCurrentListStyle(NULL),
	miCurrentListLevel(0),
	miLastListLevel(0),
	miLastListNumber(0),
	mbListContinueNumbering(false),
	mbListElementParagraphOpened(false),
	mbListElementOpened(false)
{
}

WordPerfectCollector::~WordPerfectCollector()
{
}

bool WordPerfectCollector::filter(WPXInputStream &input, DocumentHandler &xHandler)
{
	// The contract for WordPerfectCollector is that it will only be used once after it is
	// instantiated
	if (mbUsed)
		return false;

	mbUsed = true;

	// parse & write
 	if (!_parseSourceDocument(input))
		return false;
	if (!_writeTargetDocument(xHandler))
		return false;

 	// clean up the mess we made
 	WRITER_DEBUG_MSG(("WriterWordPerfect: Cleaning up our mess..\n"));

	WRITER_DEBUG_MSG(("Destroying the body elements\n"));
	for (vector<DocumentElement *>::iterator iterBody = mBodyElements.begin(); iterBody != mBodyElements.end(); iterBody++) {
		delete((*iterBody));
		(*iterBody) = NULL;
	}

	WRITER_DEBUG_MSG(("Destroying the styles elements\n"));
	for (vector<DocumentElement *>::iterator iterStyles = mStylesElements.begin(); iterStyles != mStylesElements.end(); iterStyles++) {
 		delete (*iterStyles);
		(*iterStyles) = NULL; // we may pass over the same element again (in the case of headers/footers spanning multiple pages)
				      // so make sure we don't do a double del
	}

	WRITER_DEBUG_MSG(("Destroying the rest of the styles elements\n"));
	for (map<UTF8String, ParagraphStyle *, ltstr>::iterator iterTextStyle = mTextStyleHash.begin(); iterTextStyle != mTextStyleHash.end(); iterTextStyle++) {
		delete(iterTextStyle->second);
	}
	for (map<UTF8String, SpanStyle *, ltstr>::iterator iterSpanStyle = mSpanStyleHash.begin(); iterSpanStyle != mSpanStyleHash.end(); iterSpanStyle++) {
		delete(iterSpanStyle->second);
	}
	for (map<UTF8String, FontStyle *, ltstr>::iterator iterFont = mFontHash.begin(); iterFont != mFontHash.end(); iterFont++) {
		delete(iterFont->second);
	}

	for (vector<ListStyle *>::iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); iterListStyles++) {
		delete((*iterListStyles));
	}
	for (vector<SectionStyle *>::iterator iterSectionStyles = mSectionStyles.begin(); iterSectionStyles != mSectionStyles.end(); iterSectionStyles++) {
		delete((*iterSectionStyles));
	}
	for (vector<TableStyle *>::iterator iterTableStyles = mTableStyles.begin(); iterTableStyles != mTableStyles.end(); iterTableStyles++) {
		delete((*iterTableStyles));
	}

	for (vector<PageSpan *>::iterator iterPageSpans = mPageSpans.begin(); iterPageSpans != mPageSpans.end(); iterPageSpans++) {
		delete((*iterPageSpans));
	}

 	return true;
}

bool WordPerfectCollector::_parseSourceDocument(WPXInputStream &input)
{
	// create a header for the preamble + add some default settings to it

 	WRITER_DEBUG_MSG(("WriterWordPerfect: Attempting to process state\n"));
	bool bRetVal = true;
	try {
		WPDocument::parse(&input, static_cast<WPXHLListenerImpl *>(this));
	}
	catch (FileException)
        {
                WRITER_DEBUG_MSG(("Caught a file exception..\n"));
                bRetVal = false;
        }

	return bRetVal;
}

void WordPerfectCollector::_writeDefaultStyles(DocumentHandler &xHandler)
{
	TagOpenElement stylesOpenElement("office:styles");
	stylesOpenElement.write(xHandler);

	TagOpenElement defaultParagraphStyleOpenElement("style:default-style");
	defaultParagraphStyleOpenElement.addAttribute("style:family", "paragraph");
	defaultParagraphStyleOpenElement.write(xHandler);

	TagOpenElement defaultParagraphStylePropertiesOpenElement("style:properties");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:family", "paragraph");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:tab-stop-distance", "0.5inch");
	defaultParagraphStylePropertiesOpenElement.write(xHandler);
	TagCloseElement defaultParagraphStylePropertiesCloseElement("style:properties");
	defaultParagraphStylePropertiesCloseElement.write(xHandler);

	TagCloseElement defaultParagraphStyleCloseElement("style:default-style");
	defaultParagraphStyleCloseElement.write(xHandler);
	
	TagOpenElement standardStyleOpenElement("style:style");
        standardStyleOpenElement.addAttribute("style:name", "Standard");
        standardStyleOpenElement.addAttribute("style:family", "paragraph");
        standardStyleOpenElement.addAttribute("style:class", "text");
        standardStyleOpenElement.write(xHandler);
        TagCloseElement standardStyleCloseElement("style:style");
        standardStyleCloseElement.write(xHandler);

        TagOpenElement textBodyStyleOpenElement("style:style");
        textBodyStyleOpenElement.addAttribute("style:name", "Text Body");
        textBodyStyleOpenElement.addAttribute("style:family", "paragraph");
        textBodyStyleOpenElement.addAttribute("style:parent-style-name", "Standard");
        textBodyStyleOpenElement.addAttribute("style:class", "text");
        textBodyStyleOpenElement.write(xHandler);
        TagCloseElement textBodyStyleCloseElement("style:style");
        textBodyStyleCloseElement.write(xHandler);

        TagOpenElement tableContentsStyleOpenElement("style:style");
        tableContentsStyleOpenElement.addAttribute("style:name", "Table Contents");
        tableContentsStyleOpenElement.addAttribute("style:family", "paragraph");
        tableContentsStyleOpenElement.addAttribute("style:parent-style-name", "Text Body");
        tableContentsStyleOpenElement.addAttribute("style:class", "extra");
        tableContentsStyleOpenElement.write(xHandler);
        TagCloseElement tableContentsStyleCloseElement("style:style");
        tableContentsStyleCloseElement.write(xHandler);

        TagOpenElement tableHeadingStyleOpenElement("style:style");
        tableHeadingStyleOpenElement.addAttribute("style:name", "Table Heading");
        tableHeadingStyleOpenElement.addAttribute("style:family", "paragraph");
        tableHeadingStyleOpenElement.addAttribute("style:parent-style-name", "Table Contents");
        tableHeadingStyleOpenElement.addAttribute("style:class", "extra");
        tableHeadingStyleOpenElement.write(xHandler);
        TagCloseElement tableHeadingStyleCloseElement("style:style");
        tableHeadingStyleCloseElement.write(xHandler);

	TagCloseElement stylesCloseElement("office:styles");
	stylesCloseElement.write(xHandler);

}

void WordPerfectCollector::_writeContentPreamble(DocumentHandler &xHandler)
{
	TagOpenElement documentContentOpenElement("office:document-content");
	documentContentOpenElement.addAttribute("xmlns:office", "http://openoffice.org/2000/office");
	documentContentOpenElement.addAttribute("xmlns:style", "http://openoffice.org/2000/style");
	documentContentOpenElement.addAttribute("xmlns:text", "http://openoffice.org/2000/text");
	documentContentOpenElement.addAttribute("xmlns:table", "http://openoffice.org/2000/table");
	documentContentOpenElement.addAttribute("xmlns:draw", "http://openoffice.org/2000/draw");
	documentContentOpenElement.addAttribute("xmlns:fo", "http://www.w3.org/1999/XSL/Format");
	documentContentOpenElement.addAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	documentContentOpenElement.addAttribute("xmlns:number", "http://openoffice.org/2000/datastyle");
	documentContentOpenElement.addAttribute("xmlns:svg", "http://www.w3.org/2000/svg");
	documentContentOpenElement.addAttribute("xmlns:chart", "http://openoffice.org/2000/chart");
	documentContentOpenElement.addAttribute("xmlns:dr3d", "http://openoffice.org/2000/dr3d");
	documentContentOpenElement.addAttribute("xmlns:math", "http://www.w3.org/1998/Math/MathML");
	documentContentOpenElement.addAttribute("xmlns:form", "http://openoffice.org/2000/form");
	documentContentOpenElement.addAttribute("xmlns:script", "http://openoffice.org/2000/script");
	documentContentOpenElement.addAttribute("office:class", "text");
	documentContentOpenElement.addAttribute("office:version", "1.0");
	documentContentOpenElement.write(xHandler);
}

void WordPerfectCollector::_writeMasterPages(DocumentHandler &xHandler)
{
        WPXPropertyList xBlankAttrList;

	xHandler.startElement("office:master-styles", xBlankAttrList);
	int pageNumber = 1;
	for (int i=0; i<mPageSpans.size(); i++)
	{
		bool bLastPage;
		(i == (mPageSpans.size() - 1)) ? bLastPage = true : bLastPage = false;
		mPageSpans[i]->writeMasterPages(pageNumber, i, bLastPage, xHandler);
		pageNumber += mPageSpans[i]->getSpan();
	}
	xHandler.endElement("office:master-styles");
}

void WordPerfectCollector::_writePageMasters(DocumentHandler &xHandler)
{
	int pageNumber = 1;
	for (int i=0; i<mPageSpans.size(); i++)
	{
		mPageSpans[i]->writePageMaster(i, xHandler);
	}
}

bool WordPerfectCollector::_writeTargetDocument(DocumentHandler &xHandler)
{
	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Printing out the header stuff..\n"));
	WPXPropertyList xBlankAttrList;

	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Start Document\n"));
	xHandler.startDocument();

	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: preamble\n"));
	_writeContentPreamble(xHandler);

 	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Writing out the styles..\n"));

	// write out the font styles
	xHandler.startElement("office:font-decls", xBlankAttrList);
	for (map<UTF8String, FontStyle *, ltstr>::iterator iterFont = mFontHash.begin(); iterFont != mFontHash.end(); iterFont++) {
		iterFont->second->write(xHandler);
	}
	TagOpenElement symbolFontOpen("style:font-decl");
	symbolFontOpen.addAttribute("style:name", "StarSymbol");
	symbolFontOpen.addAttribute("fo:font-family", "StarSymbol");
	symbolFontOpen.addAttribute("style:font-charset", "x-symbol");
	symbolFontOpen.write(xHandler);
	TagCloseElement symbolFontClose("style:font-decl");
	symbolFontClose.write(xHandler);

	xHandler.endElement("office:font-decls");

	// write default styles
	_writeDefaultStyles(xHandler);

	// write automatic styles: which encompasses quite a bit
	xHandler.startElement("office:automatic-styles", xBlankAttrList);
	for (map<UTF8String, ParagraphStyle *, ltstr>::iterator iterTextStyle = mTextStyleHash.begin(); 
             iterTextStyle != mTextStyleHash.end(); iterTextStyle++) 
        {
		// writing out the paragraph styles
		if (strcmp((iterTextStyle->second)->getName()(), "Standard")) 
                {
			// don't write standard paragraph "no styles" style
			(iterTextStyle->second)->write(xHandler);
		}
	}

	for (map<UTF8String, SpanStyle *, ltstr>::iterator iterSpanStyle = mSpanStyleHash.begin(); 
             iterSpanStyle != mSpanStyleHash.end(); iterSpanStyle++) 
        {
                (iterSpanStyle->second)->write(xHandler);
	}


 	// writing out the sections styles
	for (vector<SectionStyle *>::iterator iterSectionStyles = mSectionStyles.begin(); iterSectionStyles != mSectionStyles.end(); iterSectionStyles++) {
		(*iterSectionStyles)->write(xHandler);
	}

	// writing out the lists styles
	for (vector<ListStyle *>::iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); iterListStyles++) {
		(*iterListStyles)->write(xHandler);
	}

 	// writing out the table styles
	for (vector<TableStyle *>::iterator iterTableStyles = mTableStyles.begin(); iterTableStyles != mTableStyles.end(); iterTableStyles++) {
		(*iterTableStyles)->write(xHandler);
	}

	// writing out the page masters
	_writePageMasters(xHandler);

	xHandler.endElement("office:automatic-styles");

	_writeMasterPages(xHandler);

 	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Writing out the document..\n"));
 	// writing out the document
	xHandler.startElement("office:body", xBlankAttrList);

	for (vector<DocumentElement *>::iterator iterBodyElements = mBodyElements.begin(); iterBodyElements != mBodyElements.end(); iterBodyElements++) {
		(*iterBodyElements)->write(xHandler);
	}
 	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Finished writing all doc els..\n"));

	xHandler.endElement("office:body");
	xHandler.endElement("office:document-content");

	xHandler.endDocument();

	return true;
}


UTF8String getParagraphStyleKey(const WPXPropertyList & xPropList, const vector<WPXTabStop> & xTabStops)
{
        UTF8String sKey;
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
                UTF8String sProp;
                sProp.sprintf("[%s:%s]", i.key().c_str(), i()->getStr().getUTF8());
                sKey.append(sProp);
        }
        
        UTF8String sTabStops;
        sTabStops.sprintf("[num-tab-stops:%i]", xTabStops.size());
        for (vector<WPXTabStop>::const_iterator j = xTabStops.begin(); j != xTabStops.end(); j++)
        {
                UTF8String sTabStop;
                sTabStop.sprintf("[[position:%f][alignment:%i][leader-character:%i][leader-num-spaces:%i]]",
                                 (*j).m_position, (*j).m_alignment, (*j).m_leaderCharacter, (*j).m_leaderNumSpaces);
                sTabStops.append(sTabStop);
        }
        sKey.append(sTabStops);

        return sKey;
}

// _allocateFontName: add a (potentially mapped) font style to the hash if it's not already there, do nothing otherwise
void WordPerfectCollector::_allocateFontName(const UTF8String & sFontName)
{
	if (mFontHash.find(sFontName) == mFontHash.end())
	{
		FontStyle *pFontStyle = new FontStyle(sFontName.getUTF8(), sFontName.getUTF8());
		mFontHash[sFontName] = pFontStyle;
	}
}

void WordPerfectCollector::openPageSpan(const WPXPropertyList &propList)
{
	PageSpan *pPageSpan = new PageSpan(propList);
	mPageSpans.push_back(pPageSpan);
	mpCurrentPageSpan = pPageSpan;
}

void WordPerfectCollector::openHeader(const WPXPropertyList &propList)
{
	vector<DocumentElement *> * pHeaderFooterContentElements = new vector<DocumentElement *>;

	switch (propList["occurence"]->getInt())
		{
		case ALL:
		case ODD:
			WRITER_DEBUG_MSG(("WriterWordPerfect: Opening h_all or h_odd\n"));
			mpCurrentPageSpan->setHeaderContent(pHeaderFooterContentElements);
			break;
		case EVEN:
			WRITER_DEBUG_MSG(("WriterWordPerfect: Opening h_even\n"));
			mpCurrentPageSpan->setHeaderLeftContent(pHeaderFooterContentElements);
			break;
		}

	mpCurrentContentElements = pHeaderFooterContentElements;
}

void WordPerfectCollector::closeHeader()
{
	mpCurrentContentElements = &mBodyElements;
}

void WordPerfectCollector::openFooter(const WPXPropertyList &propList)
{
	vector<DocumentElement *> * pHeaderFooterContentElements = new vector<DocumentElement *>;

	switch (propList["occurence"]->getInt())
		{
		case ALL:
		case ODD:
			WRITER_DEBUG_MSG(("WriterWordPerfect: Opening f_all or f_odd\n"));
			mpCurrentPageSpan->setFooterContent(pHeaderFooterContentElements);
			break;
		case EVEN:
			WRITER_DEBUG_MSG(("WriterWordPerfect: Opening f_even\n"));
			mpCurrentPageSpan->setFooterLeftContent(pHeaderFooterContentElements);
			break;
		}

	mpCurrentContentElements = pHeaderFooterContentElements;
}

void WordPerfectCollector::closeFooter()
{
	mpCurrentContentElements = &mBodyElements;
}

void WordPerfectCollector::openSection(const WPXPropertyList &propList, const vector <WPXColumnDefinition> &columns)
{
	miCurrentNumColumns = propList["num-columns"]->getInt();
	mColumns = columns;

	if (miCurrentNumColumns > 1)
	{
		miNumSections++;
		mfSectionSpaceAfter = propList["margin-bottom"]->getFloat();
		UTF8String sSectionName;
		sSectionName.sprintf("Section%i", miNumSections);
		WRITER_DEBUG_MSG(("WriterWordPerfect:  New Section: %s\n", sSectionName.getUTF8()));
		
		SectionStyle *pSectionStyle = new SectionStyle(miCurrentNumColumns, mColumns, sSectionName.getUTF8());
		mSectionStyles.push_back(pSectionStyle);
		
		TagOpenElement *pSectionOpenElement = new TagOpenElement("text:section");
		pSectionOpenElement->addAttribute("text:style-name", pSectionStyle->getName());
		pSectionOpenElement->addAttribute("text:name", pSectionStyle->getName());
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pSectionOpenElement));
	}
	else
		mWriterDocumentState.mbInFakeSection = true;
}

void WordPerfectCollector::closeSection()
{
	if (!mWriterDocumentState.mbInFakeSection)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:section")));
	else
		mWriterDocumentState.mbInFakeSection = false;

	// open as many paragraphs as needed to simulate section space after
	// WLACH_REFACTORING: disable this for now..
	#if 0
	for (float f=0.0f; f<mfSectionSpaceAfter; f+=1.0f) {
		vector<WPXTabStop> dummyTabStops;
		openParagraph(WPX_PARAGRAPH_JUSTIFICATION_LEFT, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, dummyTabStops, false, false);
		closeParagraph();
	}
	#endif
	mfSectionSpaceAfter = 0.0f;
}

void WordPerfectCollector::openParagraph(const WPXPropertyList &propList, const vector<WPXTabStop> &tabStops)
{
	// FIXMENOW: What happens if we open a footnote inside a table? do we then inherit the footnote's style
	// from "Table Contents"
	
	WPXPropertyList *pPersistPropList = new WPXPropertyList(propList);
	ParagraphStyle *pStyle = NULL;

	if (mWriterDocumentState.mbFirstElement && mpCurrentContentElements == &mBodyElements)
	{
		WRITER_DEBUG_MSG(("WriterWordPerfect: If.. (mbFirstElement=%i)", mWriterDocumentState.mbFirstElement));
		// we don't have to go through the fuss of determining if the paragraph style is 
		// unique in this case, because if we are the first document element, then we
		// are singular. Neither do we have to determine what our parent style is-- we can't
		// be inside a table in this case (the table would be the first document element 
		//in that case)
		pPersistPropList->insert("style:parent-style-name", "Standard");
		UTF8String sName;
		sName.sprintf("FS"); 

		UTF8String sParagraphHashKey("P|FS");
		pPersistPropList->insert("style:master-page-name", "Page Style 1");
                pStyle = new ParagraphStyle(pPersistPropList, tabStops, sName);
		mTextStyleHash[sParagraphHashKey] = pStyle;
		mWriterDocumentState.mbFirstElement = false;
 	}
	else
	{
		if (mWriterDocumentState.mbTableCellOpened || miCurrentNumColumns > 1)
		{
			pPersistPropList->remove("margin-left");
			pPersistPropList->remove("margin-right");
			pPersistPropList->remove("text-indent");
		}

		if (mWriterDocumentState.mbTableCellOpened)
		{
			if (mWriterDocumentState.mbHeaderRow)
				pPersistPropList->insert("style:parent-style-name", WPXPropertyFactory::newStringProp("Table Heading"));
			else
				pPersistPropList->insert("style:parent-style-name", WPXPropertyFactory::newStringProp("Table Contents"));
		}
		else
			pPersistPropList->insert("style:parent-style-name", WPXPropertyFactory::newStringProp("Standard"));

                UTF8String sKey = getParagraphStyleKey(*pPersistPropList, tabStops);

		if (mTextStyleHash.find(sKey) == mTextStyleHash.end()) {
			UTF8String sName;
			sName.sprintf("S%i", mTextStyleHash.size()); 
			
			pStyle = new ParagraphStyle(pPersistPropList, tabStops, sName);
	
			mTextStyleHash[sKey] = pStyle;
		}
		else
		{
			pStyle = mTextStyleHash[sKey];
			delete pPersistPropList;
		}
	}
	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pParagraphOpenElement = new TagOpenElement("text:p");
	pParagraphOpenElement->addAttribute("text:style-name", pStyle->getName());
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pParagraphOpenElement));
}

void WordPerfectCollector::closeParagraph()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:p")));
}

void WordPerfectCollector::openSpan(const WPXPropertyList &propList)
{
	_allocateFontName(propList["font-name"]->getStr());
	UTF8String sSpanHashKey;
	sSpanHashKey.sprintf("S|%i|%s|%f|%s|%s", propList["text-attribute-bits"]->getInt(), propList["font-name"]->getStr().getUTF8(),
			     propList["font-size"]->getFloat(), propList["color"]->getStr().getUTF8(), 
                             propList["text-background-color"]->getStr().getUTF8());
	WRITER_DEBUG_MSG(("WriterWordPerfect: Span Hash Key: %s\n", sSpanHashKey.getUTF8()));

	// Get the style
	SpanStyle * pStyle = NULL;
	if (mSpanStyleHash.find(sSpanHashKey) == mSpanStyleHash.end())
        {
		// allocate a new paragraph style
		UTF8String sName;
		sName.sprintf("S%i", mSpanStyleHash.size());
		pStyle = new SpanStyle(propList["text-attribute-bits"]->getInt(), propList["font-name"]->getStr().getUTF8(),
                                       propList["font-size"]->getFloat(), propList["color"]->getStr().getUTF8(),
                                       propList["text-background-color"]->getStr().getUTF8(), sName.getUTF8());

		mSpanStyleHash[sSpanHashKey] = pStyle;
	}
	else 
        {
		pStyle = mSpanStyleHash.find(sSpanHashKey)->second; // yes, this could be optimized (see dup call above)
	}

	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pSpanOpenElement = new TagOpenElement("text:span");
	pSpanOpenElement->addAttribute("text:style-name", pStyle->getName());
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pSpanOpenElement));
}

void WordPerfectCollector::closeSpan()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:span")));
}

void WordPerfectCollector::defineOrderedListLevel(const WPXPropertyList &propList)
{
	WRITER_DEBUG_MSG(("Define ordered list level (listid: %i)\n", propList["id"]->getInt()));
 	OrderedListStyle *pOrderedListStyle = NULL;
	if (mpCurrentListStyle && mpCurrentListStyle->getListID() == propList["id"]->getInt())
		pOrderedListStyle = static_cast<OrderedListStyle *>(mpCurrentListStyle); // FIXME: using a dynamic cast here causes oo to crash?!
	// this rather appalling conditional makes sure we only start a new list (rather than continue an old
	// one) iff: (1) we have no prior list OR (2) the prior list is actually definitively different
	// from the list that is just being defined (listIDs differ) OR (3) we can tell that the user actually
	// is starting a new list at level 1 (and only level 1)
	if (pOrderedListStyle == NULL || pOrderedListStyle->getListID() != propList["id"]->getInt() ||
	    (propList["level"]->getInt()==1 && (propList["starting-number"]->getInt() != (miLastListNumber+1))))
	{
		WRITER_DEBUG_MSG(("Attempting to create a new ordered list style (listid: %i)\n"));
		UTF8String sName;
		sName.sprintf("OL%i", miNumListStyles);
		miNumListStyles++;
		pOrderedListStyle = new OrderedListStyle(sName.getUTF8(), propList["id"]->getInt());
		mListStyles.push_back(static_cast<ListStyle *>(pOrderedListStyle));
		mpCurrentListStyle = static_cast<ListStyle *>(pOrderedListStyle);
		mbListContinueNumbering = false;
		miLastListNumber = 0;
	}
	else
		mbListContinueNumbering = true;

	pOrderedListStyle->updateListLevel(miCurrentListLevel, (WPXNumberingType)propList["type"]->getInt(), 
					   propList["text-before-number"]->getStr(), propList["text-after-number"]->getStr(), propList["starting-number"]->getInt());
}

void WordPerfectCollector::defineUnorderedListLevel(const WPXPropertyList &propList)
{
	WRITER_DEBUG_MSG(("Define unordered list level (listid: %i)\n", propList["id"]->getInt()));

 	UnorderedListStyle *pUnorderedListStyle = NULL;
	if (mpCurrentListStyle && mpCurrentListStyle->getListID() == propList["id"]->getInt())
		pUnorderedListStyle = static_cast<UnorderedListStyle *>(mpCurrentListStyle); // FIXME: using a dynamic cast here causes oo to crash?!

	if (pUnorderedListStyle == NULL) {
		WRITER_DEBUG_MSG(("Attempting to create a new unordered list style (listid: %i)\n", propList["id"]->getInt()));
		UTF8String sName;
		sName.sprintf("UL%i", miNumListStyles);
		pUnorderedListStyle = new UnorderedListStyle(sName(), propList["id"]->getInt());
		mListStyles.push_back(static_cast<ListStyle *>(pUnorderedListStyle));
		mpCurrentListStyle = static_cast<ListStyle *>(pUnorderedListStyle);
	}
	pUnorderedListStyle->updateListLevel(miCurrentListLevel, propList["bullet"]->getStr());
}

void WordPerfectCollector::openOrderedListLevel(const WPXPropertyList &propList)
{
	WRITER_DEBUG_MSG(("Open ordered list level (listid: %i)\n", propList["id"]->getInt()));
	miCurrentListLevel++;
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:ordered-list");
	_openListLevel(pListLevelOpenElement);

	if (mbListContinueNumbering) {
		pListLevelOpenElement->addAttribute("text:continue-numbering", "true");
	}

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pListLevelOpenElement));
}

void WordPerfectCollector::openUnorderedListLevel(const WPXPropertyList &propList)
{
	WRITER_DEBUG_MSG(("Open unordered list level (listid: %i)\n", propList["id"]->getInt()));
	miCurrentListLevel++;
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:unordered-list");
	_openListLevel(pListLevelOpenElement);

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pListLevelOpenElement));
}

void WordPerfectCollector::_openListLevel(TagOpenElement *pListLevelOpenElement)
{
  	if (!mbListElementOpened && miCurrentListLevel > 1)
  	{
  		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:list-item")));
  	}
	else if (mbListElementParagraphOpened)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:p")));
		mbListElementParagraphOpened = false;
	}

	if (miCurrentListLevel==1) {
		pListLevelOpenElement->addAttribute("text:style-name", mpCurrentListStyle->getName());
	}

	mbListElementOpened = false;
}

void WordPerfectCollector::closeOrderedListLevel()
{
	WRITER_DEBUG_MSG(("Close ordered list level)\n"));
	_closeListLevel("ordered-list");
}

void WordPerfectCollector::closeUnorderedListLevel()
{
	WRITER_DEBUG_MSG(("Close unordered list level\n"));
	_closeListLevel("unordered-list");
}

void WordPerfectCollector::_closeListLevel(const char *szListType)
{
	if (mbListElementOpened)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:list-item")));

	miCurrentListLevel--;

	UTF8String sCloseElement;
	sCloseElement.sprintf("text:%s", szListType);
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement(sCloseElement.getUTF8())));

	if (miCurrentListLevel > 0)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:list-item")));
	mbListElementOpened = false;
}

void WordPerfectCollector::openListElement(const WPXPropertyList &propList, const vector<WPXTabStop> &tabStops)
{
	miLastListLevel = miCurrentListLevel;
	if (miCurrentListLevel == 1)
		miLastListNumber++;

	if (mbListElementOpened)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:list-item")));

	ParagraphStyle *pStyle = NULL;

	WPXPropertyList *pPersistPropList = new WPXPropertyList(propList);
	pPersistPropList->insert("list-style-name", WPXPropertyFactory::newStringProp(mpCurrentListStyle->getName()));
	pPersistPropList->insert("parent-style-name", WPXPropertyFactory::newStringProp("Standard"));

        UTF8String sKey = getParagraphStyleKey(*pPersistPropList, tabStops);

        if (mTextStyleHash.find(sKey) == mTextStyleHash.end()) {
                UTF8String sName;
                sName.sprintf("S%i", mTextStyleHash.size()); 
		
                pStyle = new ParagraphStyle(pPersistPropList, tabStops, sName);
                
                mTextStyleHash[sKey] = pStyle;
        }
        else
        {
                pStyle = mTextStyleHash[sKey];
                delete pPersistPropList;
        }

	TagOpenElement *pOpenListElement = new TagOpenElement("text:list-item");
	TagOpenElement *pOpenListElementParagraph = new TagOpenElement("text:p");

	pOpenListElementParagraph->addAttribute("text:style-name", pStyle->getName());

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pOpenListElement));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pOpenListElementParagraph));
		
	mbListElementOpened = true;
	mbListElementParagraphOpened = true;
	mbListContinueNumbering = false;
}

void WordPerfectCollector::closeListElement()
{
	WRITER_DEBUG_MSG(("close list element\n"));

	// this code is kind of tricky, because we don't actually close the list element (because this list element
	// could contain another list level in OOo's implementation of lists). that is done in the closeListLevel
	// code (or when we open another list element)

	if (mbListElementParagraphOpened)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:p")));
		mbListElementParagraphOpened = false;
	}
}

void WordPerfectCollector::openFootnote(const WPXPropertyList &propList)
{
	WRITER_DEBUG_MSG(("open footnote\n"));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:footnote")));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:footnote-citation")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new CharDataElement(propList["number"]->getStr().getUTF8())));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:footnote-citation")));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:footnote-body")));

}

void WordPerfectCollector::closeFootnote()
{
	WRITER_DEBUG_MSG(("close footnote\n"));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:footnote-body")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:footnote")));
}

void WordPerfectCollector::openEndnote(const WPXPropertyList &propList)
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:endnote")));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:endnote-citation")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new CharDataElement(propList["number"]->getStr().getUTF8())));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:endnote-citation")));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:endnote-body")));

}
void WordPerfectCollector::closeEndnote()
{
	WRITER_DEBUG_MSG(("close endnote\n"));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:endnote-body")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:endnote")));
}

void WordPerfectCollector::openTable(const WPXPropertyList &propList, const vector < WPXColumnDefinition > &columns)
{
	miNumTables++;

	UTF8String sTableName;
	sTableName.sprintf("Table%i", miNumTables);
	WRITER_DEBUG_MSG(("WriterWordPerfect:  New Table: %s\n", sTableName.getUTF8()));

	// FIXME: we base the table style off of the page's margin left, ignoring (potential) wordperfect margin
	// state which is transmitted inside the page. could this lead to unacceptable behaviour?
	TableStyle *pTableStyle = new TableStyle(mpCurrentPageSpan->getMarginLeft(), mpCurrentPageSpan->getMarginRight(), propList["margin-left"]->getFloat(), propList["margin-right"]->getFloat(), propList["alignment"]->getInt(), propList["left-offset"]->getFloat(), columns, sTableName.getUTF8());

	if (mWriterDocumentState.mbFirstElement && mpCurrentContentElements == &mBodyElements)
	{
		UTF8String sMasterPageName("Page Style 1");
		pTableStyle->setMasterPageName(sMasterPageName);
		mWriterDocumentState.mbFirstElement = false;
	}

	mTableStyles.push_back(pTableStyle);

	mpCurrentTableStyle = pTableStyle;

	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");

	pTableOpenElement->addAttribute("table:name", sTableName.getUTF8());
	pTableOpenElement->addAttribute("table:style-name", sTableName.getUTF8());
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pTableOpenElement));

	for (int i=0; i<pTableStyle->getNumColumns(); i++) {
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		UTF8String sColumnStyleName;
		sColumnStyleName.sprintf("%s.Column%i", sTableName.getUTF8(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.getUTF8());
		mpCurrentContentElements->push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpCurrentContentElements->push_back(pTableColumnCloseElement);
	}
}

void WordPerfectCollector::openTableRow(const WPXPropertyList &propList)
{
	if (propList["is-header-row"]->getInt())
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("table:table-header-rows")));
		mWriterDocumentState.mbHeaderRow = true;
	}

	UTF8String sTableRowStyleName;
	sTableRowStyleName.sprintf("%s.Row%i", mpCurrentTableStyle->getName()(), mpCurrentTableStyle->getNumTableRowStyles());
	TableRowStyle *pTableRowStyle = new TableRowStyle(propList["height"]->getFloat(), propList["is-minimum-height"]->getInt(), 
							  propList["is-header-row"]->getInt(), sTableRowStyleName());
	mpCurrentTableStyle->addTableRowStyle(pTableRowStyle);
	
	TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
	pTableRowOpenElement->addAttribute("table:style-name", sTableRowStyleName);
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pTableRowOpenElement));
}

void WordPerfectCollector::closeTableRow()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table-row")));
	if (mWriterDocumentState.mbHeaderRow)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table-header-rows")));
		mWriterDocumentState.mbHeaderRow = false;
	}
}

void WordPerfectCollector::openTableCell(const WPXPropertyList &propList)
{
	float fLeftBorderThickness = (propList["border-bits"]->getInt() & WPX_TABLE_CELL_LEFT_BORDER_OFF) ? 0.0f : 0.0007f;
	float fRightBorderThickness = (propList["border-bits"]->getInt() & WPX_TABLE_CELL_RIGHT_BORDER_OFF) ? 0.0f : 0.0007f;
	float fTopBorderThickness = (propList["border-bits"]->getInt() & WPX_TABLE_CELL_TOP_BORDER_OFF) ? 0.0f : 0.0007f;
	float fBottomBorderThickness = (propList["border-bits"]->getInt() & WPX_TABLE_CELL_BOTTOM_BORDER_OFF) ? 0.0f : 0.0007f;
	WRITER_DEBUG_MSG(("WriterWordPerfect: Borderbits=%d\n", propList["border-bits"]->getInt()));

	UTF8String sTableCellStyleName; 
	sTableCellStyleName.sprintf( "%s.Cell%i", mpCurrentTableStyle->getName()(), mpCurrentTableStyle->getNumTableCellStyles());
	TableCellStyle *pTableCellStyle = new TableCellStyle(fLeftBorderThickness, fRightBorderThickness, fTopBorderThickness, fBottomBorderThickness,
							     propList["color"]->getStr(), propList["border-color"]->getStr(), 
							     (WPXVerticalAlignment)propList["vertical-alignment"]->getInt(),
							     sTableCellStyleName.getUTF8());
	mpCurrentTableStyle->addTableCellStyle(pTableCellStyle);

	TagOpenElement *pTableCellOpenElement = new TagOpenElement("table:table-cell");
	pTableCellOpenElement->addAttribute("table:style-name", sTableCellStyleName);
	pTableCellOpenElement->addAttribute("table:number-columns-spanned", propList["col-span"]->getStr().getUTF8());
	pTableCellOpenElement->addAttribute("table:number-rows-spanned", propList["row-span"]->getStr().getUTF8());
	pTableCellOpenElement->addAttribute("table:value-type", "string");
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pTableCellOpenElement));

	mWriterDocumentState.mbTableCellOpened = true;
}

void WordPerfectCollector::closeTableCell()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table-cell")));
	mWriterDocumentState.mbTableCellOpened = false;
}

void WordPerfectCollector::insertCoveredTableCell(const WPXPropertyList &propList)
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("table:covered-table-cell")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:covered-table-cell")));
}

void WordPerfectCollector::closeTable()
{
	WRITER_DEBUG_MSG(("WriterWordPerfect: Closing Table\n"));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table")));
}


void WordPerfectCollector::insertTab()
{
	WRITER_DEBUG_MSG(("WriterWordPerfect: Insert Tab\n"));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:tab-stop")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:tab-stop")));
}

void WordPerfectCollector::insertLineBreak()
{
	WRITER_DEBUG_MSG(("WriterWordPerfect: Insert Line Break\n"));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:line-break")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:line-break")));
}

void WordPerfectCollector::insertText(const UTF8String &text)
{
	WRITER_DEBUG_MSG(("WriterWordPerfect: Insert Text\n"));

	DocumentElement *pText = new TextElement(text);
	mpCurrentContentElements->push_back(pText);
}
