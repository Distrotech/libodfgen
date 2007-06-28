/* WordPerfectCollector: Collects sections and runs of text from a
 * wordperfect file (and styles to go along with them) and writes them
 * to a Writer target document
 *
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
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
#include <string>

#include "WordPerfectCollector.hxx"
#include "DocumentElement.hxx"
#include "TextRunStyle.hxx"
#include "FontStyle.hxx"
#include "GraphicsStyle.hxx"
#include "ListStyle.hxx"
#include "PageSpan.hxx"
#include "SectionStyle.hxx"
#include "TableStyle.hxx"
#include "FilterInternal.hxx"
#include "WriterProperties.hxx"
#include "OdgExporter.hxx"
#include "InternalHandler.hxx"

_WriterDocumentState::_WriterDocumentState() :
	mbFirstElement(true),
	mbInFakeSection(false),
	mbListElementOpenedAtCurrentLevel(false),
	mbTableCellOpened(false),
	mbHeaderRow(false),
	mbInNote(false)
{
}

WordPerfectCollector::WordPerfectCollector(WPXInputStream *pInput, DocumentHandler *pHandler, const bool isFlatXML) :
	mpInput(pInput),
	mpHandler(pHandler),
	mbUsed(false),
	mfSectionSpaceAfter(0.0f),
	miNumListStyles(0),
	mpCurrentContentElements(&mBodyElements),
	mpCurrentPageSpan(NULL),
	miNumPageStyles(0),
	mpCurrentListStyle(NULL),
	miCurrentListLevel(0),
	miLastListLevel(0),
	miLastListNumber(0),
	mbListContinueNumbering(false),
	mbListElementOpened(false),
	mbListElementParagraphOpened(false),
	miObjectNumber(0),
	mbIsFlatXML(isFlatXML)
{
}

WordPerfectCollector::~WordPerfectCollector()
{
}

bool WordPerfectCollector::filter()
{
	// The contract for WordPerfectCollector is that it will only be used once after it is
	// instantiated
	if (mbUsed)
		return false;

	mbUsed = true;

	// parse & write
	// WLACH_REFACTORING: Remove these args..
 	if (!_parseSourceDocument(*mpInput))
		return false;
	if (!_writeTargetDocument(mpHandler))
		return false;

 	// clean up the mess we made
 	WRITER_DEBUG_MSG(("WriterWordPerfect: Cleaning up our mess..\n"));

	WRITER_DEBUG_MSG(("Destroying the body elements\n"));
	for (std::vector<DocumentElement *>::iterator iterBody = mBodyElements.begin(); iterBody != mBodyElements.end(); iterBody++) {
		delete (*iterBody);
		(*iterBody) = NULL;
	}

	WRITER_DEBUG_MSG(("Destroying the styles elements\n"));
	for (std::vector<DocumentElement *>::iterator iterStyles = mStylesElements.begin(); iterStyles != mStylesElements.end(); iterStyles++) {
 		delete (*iterStyles);
		(*iterStyles) = NULL; // we may pass over the same element again (in the case of headers/footers spanning multiple pages)
				      // so make sure we don't do a double del
	}

	WRITER_DEBUG_MSG(("Destroying the rest of the styles elements\n"));
	for (std::map<WPXString, ParagraphStyle *, ltstr>::iterator iterTextStyle = mTextStyleHash.begin(); iterTextStyle != mTextStyleHash.end(); iterTextStyle++) {
		delete (iterTextStyle->second);
	}
	for (std::map<WPXString, SpanStyle *, ltstr>::iterator iterSpanStyle = mSpanStyleHash.begin(); iterSpanStyle != mSpanStyleHash.end(); iterSpanStyle++) {
		delete(iterSpanStyle->second);
	}
	
	for (std::map<WPXString, FontStyle *, ltstr>::iterator iterFont = mFontHash.begin(); iterFont != mFontHash.end(); iterFont++) {
		delete(iterFont->second);
	}

	for (std::vector<ListStyle *>::iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); iterListStyles++) {
		delete(*iterListStyles);
	}
	for (std::vector<SectionStyle *>::iterator iterSectionStyles = mSectionStyles.begin(); iterSectionStyles != mSectionStyles.end(); iterSectionStyles++) {
		delete(*iterSectionStyles);
	}
	for (std::vector<TableStyle *>::iterator iterTableStyles = mTableStyles.begin(); iterTableStyles != mTableStyles.end(); iterTableStyles++) {
		delete((*iterTableStyles));
	}

	for (std::vector<PageSpan *>::iterator iterPageSpans = mPageSpans.begin(); iterPageSpans != mPageSpans.end(); iterPageSpans++) {
		delete(*iterPageSpans);
	}

 	return true;
}

bool WordPerfectCollector::_parseSourceDocument(WPXInputStream &input)
{
	WPDResult result = WPDocument::parse(&input, static_cast<WPXDocumentInterface *>(this));
	if (result != WPD_OK)
		return false;

	return true;
}

void WordPerfectCollector::_writeDefaultStyles(DocumentHandler *pHandler)
{
	TagOpenElement stylesOpenElement("office:styles");
	stylesOpenElement.write(pHandler);

	TagOpenElement defaultParagraphStyleOpenElement("style:default-style");
	defaultParagraphStyleOpenElement.addAttribute("style:family", "paragraph");
	defaultParagraphStyleOpenElement.write(pHandler);

	TagOpenElement defaultParagraphStylePropertiesOpenElement("style:paragraph-properties");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:tab-stop-distance", "0.5in");
	defaultParagraphStylePropertiesOpenElement.write(pHandler);
	TagCloseElement defaultParagraphStylePropertiesCloseElement("style:paragraph-properties");
	defaultParagraphStylePropertiesCloseElement.write(pHandler);

	pHandler->endElement("style:default-style");

	TagOpenElement standardStyleOpenElement("style:style");
	standardStyleOpenElement.addAttribute("style:name", "Standard");
	standardStyleOpenElement.addAttribute("style:family", "paragraph");
	standardStyleOpenElement.addAttribute("style:class", "text");
	standardStyleOpenElement.write(pHandler);
	
	pHandler->endElement("style:style");

	TagOpenElement textBodyStyleOpenElement("style:style");
	textBodyStyleOpenElement.addAttribute("style:name", "Text_Body");
	textBodyStyleOpenElement.addAttribute("style:display-name", "Text Body");
	textBodyStyleOpenElement.addAttribute("style:family", "paragraph");
	textBodyStyleOpenElement.addAttribute("style:parent-style-name", "Standard");
	textBodyStyleOpenElement.addAttribute("style:class", "text");
	textBodyStyleOpenElement.write(pHandler);
	
	pHandler->endElement("style:style");

	TagOpenElement tableContentsStyleOpenElement("style:style");
	tableContentsStyleOpenElement.addAttribute("style:name", "Table_Contents");
	tableContentsStyleOpenElement.addAttribute("style:display-name", "Table Contents");
	tableContentsStyleOpenElement.addAttribute("style:family", "paragraph");
	tableContentsStyleOpenElement.addAttribute("style:parent-style-name", "Text_Body");
	tableContentsStyleOpenElement.addAttribute("style:class", "extra");
	tableContentsStyleOpenElement.write(pHandler);
	
	pHandler->endElement("style:style");

	TagOpenElement tableHeadingStyleOpenElement("style:style");
	tableHeadingStyleOpenElement.addAttribute("style:name", "Table_Heading");
	tableHeadingStyleOpenElement.addAttribute("style:display-name", "Table Heading");
	tableHeadingStyleOpenElement.addAttribute("style:family", "paragraph");
	tableHeadingStyleOpenElement.addAttribute("style:parent-style-name", "Table_Contents");
	tableHeadingStyleOpenElement.addAttribute("style:class", "extra");
	tableHeadingStyleOpenElement.write(pHandler);
	
	pHandler->endElement("style:style");
#if 1
	TagOpenElement graphicFrameStyleOpenElement("style:style");
	graphicFrameStyleOpenElement.addAttribute("style:name", "OLE");
	graphicFrameStyleOpenElement.addAttribute("style:family", "graphic");
	graphicFrameStyleOpenElement.write(pHandler);
	
	TagOpenElement graphicFramePropertiesOpenElement("style:graphic-properties");
	graphicFramePropertiesOpenElement.addAttribute("text:anchor-type","page");
	graphicFramePropertiesOpenElement.addAttribute("svg:x", "0.000in");
	graphicFramePropertiesOpenElement.addAttribute("svg:y", "0.000in");
	graphicFramePropertiesOpenElement.addAttribute("svg:width", "1.000in");
	graphicFramePropertiesOpenElement.addAttribute("svg:height", "1.500in");
	graphicFramePropertiesOpenElement.write(pHandler);
	
	pHandler->endElement("style:graphic-properties");
	
	pHandler->endElement("style:style");
#endif	
	pHandler->endElement("office:styles");
}

// writes everything up to the automatic styles declarations..
void WordPerfectCollector::_writeBegin()
{
}

void WordPerfectCollector::_writeMasterPages(DocumentHandler *pHandler)
{
	WPXPropertyList xBlankAttrList;

	pHandler->startElement("office:master-styles", xBlankAttrList);
	int pageNumber = 1;
	for (unsigned int i=0; i<mPageSpans.size(); i++)
	{
		bool bLastPage;
		(i == (mPageSpans.size() - 1)) ? bLastPage = true : bLastPage = false;
		mPageSpans[i]->writeMasterPages(pageNumber, i, bLastPage, pHandler);
		pageNumber += mPageSpans[i]->getSpan();
	}
	pHandler->endElement("office:master-styles");
}

void WordPerfectCollector::_writePageLayouts(DocumentHandler *pHandler)
{
	for (unsigned int i=0; i<mPageSpans.size(); i++)
	{
		mPageSpans[i]->writePageLayout(i, pHandler);
	}
}

bool WordPerfectCollector::_writeTargetDocument(DocumentHandler *pHandler)
{	
	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Printing out the header stuff..\n"));
	WPXPropertyList xBlankAttrList;

	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Start Document\n"));
	mpHandler->startDocument();

	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: preamble\n"));
	WPXPropertyList docContentPropList;
	docContentPropList.insert("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	docContentPropList.insert("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	docContentPropList.insert("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	docContentPropList.insert("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	docContentPropList.insert("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	docContentPropList.insert("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	docContentPropList.insert("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	docContentPropList.insert("xmlns:xlink", "http://www.w3.org/1999/xlink");
	docContentPropList.insert("xmlns:number", "http://openoffice.org/2000/datastyle");
	docContentPropList.insert("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	docContentPropList.insert("xmlns:chart", "urn:oasis:names:tc:opendocument:xmlns:chart:1.0");
	docContentPropList.insert("xmlns:dr3d", "urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0");
	docContentPropList.insert("xmlns:math", "http://www.w3.org/1998/Math/MathML");
	docContentPropList.insert("xmlns:form", "urn:oasis:names:tc:opendocument:xmlns:form:1.0");
	docContentPropList.insert("xmlns:script", "urn:oasis:names:tc:opendocument:xmlns:script:1.0");
	docContentPropList.insert("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	docContentPropList.insert("office:version", "1.0");
	if (mbIsFlatXML)
	{
		docContentPropList.insert("office:mimetype", "application/x-vnd.oasis.openoffice.text");

		mpHandler->startElement("office:document", docContentPropList);
	}
	else
		mpHandler->startElement("office:document-content", docContentPropList);

	// write out the font styles
	mpHandler->startElement("office:font-face-decls", xBlankAttrList);
	for (std::map<WPXString, FontStyle *, ltstr>::iterator iterFont = mFontHash.begin(); iterFont != mFontHash.end(); iterFont++) {
		iterFont->second->write(mpHandler);
	}
	TagOpenElement symbolFontOpen("style:font-face");
	symbolFontOpen.addAttribute("style:name", "StarSymbol");
	symbolFontOpen.addAttribute("svg:font-family", "StarSymbol");
	symbolFontOpen.addAttribute("style:font-charset", "x-symbol");
	symbolFontOpen.write(mpHandler);
	mpHandler->endElement("style:font-face");

	mpHandler->endElement("office:font-face-decls");

 	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Writing out the styles..\n"));

	// write default styles
	_writeDefaultStyles(mpHandler);

	mpHandler->startElement("office:automatic-styles", xBlankAttrList);
#if 1
	// FIXME: hic sunt leones
	TagOpenElement frameStyleElement("style:style");
	frameStyleElement.addAttribute("style:name", "fr1");
	frameStyleElement.addAttribute("style:family", "graphic");
	frameStyleElement.addAttribute("style:parent-style-name", "OLE");
	frameStyleElement.write(mpHandler);
	
	TagOpenElement framePropertiesElement("style:graphic-properties");
	framePropertiesElement.addAttribute("style:horizontal-pos", "left");
	framePropertiesElement.addAttribute("style:horizontal-rel", "page");
	framePropertiesElement.addAttribute("draw:ole-draw-aspect", "1");
	framePropertiesElement.write(mpHandler);
	
	mpHandler->endElement("style:graphic-properties");
	mpHandler->endElement("style:style");
	// FIXME: hic sunt leones
#endif
	for (std::map<WPXString, ParagraphStyle *, ltstr>::iterator iterTextStyle = mTextStyleHash.begin(); 
	     iterTextStyle != mTextStyleHash.end(); iterTextStyle++) 
	{
		// writing out the paragraph styles
		if (strcmp((iterTextStyle->second)->getName().cstr(), "Standard")) 
		{
			// don't write standard paragraph "no styles" style
			(iterTextStyle->second)->write(pHandler);
		}
	}

	// span styles..
	for (std::map<WPXString, SpanStyle *, ltstr>::iterator iterSpanStyle = mSpanStyleHash.begin(); 
	     iterSpanStyle != mSpanStyleHash.end(); iterSpanStyle++) 
	{
		(iterSpanStyle->second)->write(pHandler);
	}

 	// writing out the sections styles
	for (std::vector<SectionStyle *>::iterator iterSectionStyles = mSectionStyles.begin(); iterSectionStyles != mSectionStyles.end(); iterSectionStyles++) {
		(*iterSectionStyles)->write(pHandler);
	}

	// writing out the lists styles
	for (std::vector<ListStyle *>::iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); iterListStyles++) {
		(*iterListStyles)->write(pHandler);
	}

 	// writing out the table styles
	for (std::vector<TableStyle *>::iterator iterTableStyles = mTableStyles.begin(); iterTableStyles != mTableStyles.end(); iterTableStyles++) {
		(*iterTableStyles)->write(pHandler);
	}

	// writing out the page masters
	_writePageLayouts(pHandler);


	pHandler->endElement("office:automatic-styles");

	_writeMasterPages(pHandler);

 	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Writing out the document..\n"));
 	// writing out the document
	pHandler->startElement("office:body", xBlankAttrList);
	pHandler->startElement("office:text", xBlankAttrList);
	
	for (std::vector<DocumentElement *>::iterator iterBodyElements = mBodyElements.begin(); iterBodyElements != mBodyElements.end(); iterBodyElements++) {
		(*iterBodyElements)->write(pHandler);
	}
 	WRITER_DEBUG_MSG(("WriterWordPerfect: Document Body: Finished writing all doc els..\n"));

	pHandler->endElement("office:text");
	pHandler->endElement("office:body");
	if (mbIsFlatXML)
		pHandler->endElement("office:document");
	else
		pHandler->endElement("office:document-content");

	pHandler->endDocument();

	return true;
}


WPXString propListToStyleKey(const WPXPropertyList & xPropList)
{
	WPXString sKey;
	WPXPropertyList::Iter i(xPropList);
	for (i.rewind(); i.next(); )
	{
		WPXString sProp;
		sProp.sprintf("[%s:%s]", i.key(), i()->getStr().cstr());
		sKey.append(sProp);
	}

	return sKey;
}

WPXString getParagraphStyleKey(const WPXPropertyList & xPropList, const WPXPropertyListVector & xTabStops)
{
	WPXString sKey = propListToStyleKey(xPropList);
	
	WPXString sTabStops;
	sTabStops.sprintf("[num-tab-stops:%i]", xTabStops.count());
	WPXPropertyListVector::Iter i(xTabStops);
	for (i.rewind(); i.next();)
	{
		sTabStops.append(propListToStyleKey(i()));
	}
	sKey.append(sTabStops);

	return sKey;
}

// _allocateFontName: add a (potentially mapped) font style to the hash if it's not already there, do nothing otherwise
void WordPerfectCollector::_allocateFontName(const WPXString & sFontName)
{
	if (mFontHash.find(sFontName) == mFontHash.end())
	{
		FontStyle *pFontStyle = new FontStyle(sFontName.cstr(), sFontName.cstr());
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
	std::vector<DocumentElement *> * pHeaderFooterContentElements = new std::vector<DocumentElement *>;

	if (propList["libwpd:occurence"]->getStr() == "even")
		mpCurrentPageSpan->setHeaderLeftContent(pHeaderFooterContentElements);
	else
		mpCurrentPageSpan->setHeaderContent(pHeaderFooterContentElements);

	mpCurrentContentElements = pHeaderFooterContentElements;
}

void WordPerfectCollector::closeHeader()
{
	mpCurrentContentElements = &mBodyElements;
}

void WordPerfectCollector::openFooter(const WPXPropertyList &propList)
{
	std::vector<DocumentElement *> * pHeaderFooterContentElements = new std::vector<DocumentElement *>;

	if (propList["libwpd:occurence"]->getStr() == "even")
		mpCurrentPageSpan->setFooterLeftContent(pHeaderFooterContentElements);
	else
		mpCurrentPageSpan->setFooterContent(pHeaderFooterContentElements);

	mpCurrentContentElements = pHeaderFooterContentElements;
}

void WordPerfectCollector::closeFooter()
{
	mpCurrentContentElements = &mBodyElements;
}

void WordPerfectCollector::openSection(const WPXPropertyList &propList, const WPXPropertyListVector &columns)
{
	int iNumColumns = columns.count();
	float fSectionMarginLeft = 0.0f;
	float fSectionMarginRight = 0.0f;
	if (propList["fo:margin-left"])
		fSectionMarginLeft = propList["fo:margin-left"]->getFloat();
	if (propList["fo:margin-right"])
		fSectionMarginRight = propList["fo:margin-right"]->getFloat();

	if (iNumColumns > 1 || fSectionMarginLeft != 0 || fSectionMarginRight != 0)
	{
		if (propList["fo:margin-bottom"])
			mfSectionSpaceAfter = propList["fo:margin-bottom"]->getFloat();
		else if (propList["libwpd:margin-bottom"])
			mfSectionSpaceAfter =  propList["libwpd:margin-bottom"]->getFloat();

		WPXString sSectionName;
		sSectionName.sprintf("Section%i", mSectionStyles.size());
		
		SectionStyle *pSectionStyle = new SectionStyle(propList, columns, sSectionName.cstr());
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

void WordPerfectCollector::openParagraph(const WPXPropertyList &propList, const WPXPropertyListVector &tabStops)
{
	// FIXMENOW: What happens if we open a footnote inside a table? do we then inherit the footnote's style
	// from "Table Contents"
	
	WPXPropertyList *pPersistPropList = new WPXPropertyList(propList);
	ParagraphStyle *pStyle = NULL;

	if (mWriterDocumentState.mbFirstElement && mpCurrentContentElements == &mBodyElements)
	{
		// we don't have to go through the fuss of determining if the paragraph style is 
		// unique in this case, because if we are the first document element, then we
		// are singular. Neither do we have to determine what our parent style is-- we can't
		// be inside a table in this case (the table would be the first document element 
		//in that case)
		pPersistPropList->insert("style:parent-style-name", "Standard");
		WPXString sName;
		sName.sprintf("FS");

		WPXString sParagraphHashKey("P|FS");
		pPersistPropList->insert("style:master-page-name", "Page_Style_1");
		pStyle = new ParagraphStyle(pPersistPropList, tabStops, sName);
		mTextStyleHash[sParagraphHashKey] = pStyle;
		mWriterDocumentState.mbFirstElement = false;
 	}
	else
	{
		if (mWriterDocumentState.mbTableCellOpened)
		{
			if (mWriterDocumentState.mbHeaderRow)
				pPersistPropList->insert("style:parent-style-name", "Table_Heading");
			else
				pPersistPropList->insert("style:parent-style-name", "Table_Contents");
		}
		else
			pPersistPropList->insert("style:parent-style-name", "Standard");

		WPXString sKey = getParagraphStyleKey(*pPersistPropList, tabStops);

		if (mTextStyleHash.find(sKey) == mTextStyleHash.end())
		{
			WPXString sName;
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
	if (propList["style:font-name"])
		_allocateFontName(propList["style:font-name"]->getStr());
	WPXString sSpanHashKey = propListToStyleKey(propList);
	WRITER_DEBUG_MSG(("WriterWordPerfect: Span Hash Key: %s\n", sSpanHashKey.cstr()));

	// Get the style
	WPXString sName;
	if (mSpanStyleHash.find(sSpanHashKey) == mSpanStyleHash.end())
	{
		// allocate a new paragraph style
		sName.sprintf("Span%i", mSpanStyleHash.size());
		SpanStyle *pStyle = new SpanStyle(sName.cstr(), propList);		

		mSpanStyleHash[sSpanHashKey] = pStyle;
	}
	else 
	{
		sName.sprintf("%s", mSpanStyleHash.find(sSpanHashKey)->second->getName().cstr());
	}

	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pSpanOpenElement = new TagOpenElement("text:span");
	pSpanOpenElement->addAttribute("text:style-name", sName.cstr());
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pSpanOpenElement));
}

void WordPerfectCollector::closeSpan()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:span")));
}

void WordPerfectCollector::defineOrderedListLevel(const WPXPropertyList &propList)
{
	int id = 0;
	if (propList["libwpd:id"])
		id = propList["libwpd:id"]->getInt();

 	OrderedListStyle *pOrderedListStyle = NULL;
	if (mpCurrentListStyle && mpCurrentListStyle->getListID() == id)
		pOrderedListStyle = static_cast<OrderedListStyle *>(mpCurrentListStyle); // FIXME: using a dynamic cast here causes oo to crash?!

	// this rather appalling conditional makes sure we only start a new list (rather than continue an old
	// one) if: (1) we have no prior list OR (2) the prior list is actually definitively different
	// from the list that is just being defined (listIDs differ) OR (3) we can tell that the user actually
	// is starting a new list at level 1 (and only level 1)
	if (pOrderedListStyle == NULL || pOrderedListStyle->getListID() != id  ||
	    (propList["libwpd:level"] && propList["libwpd:level"]->getInt()==1 && 
	     (propList["text:start-value"] && propList["text:start-value"]->getInt() != (miLastListNumber+1))))
	{
		WRITER_DEBUG_MSG(("Attempting to create a new ordered list style (listid: %i)\n", id));
		WPXString sName;
		sName.sprintf("OL%i", miNumListStyles);
		miNumListStyles++;
		pOrderedListStyle = new OrderedListStyle(sName.cstr(), propList["libwpd:id"]->getInt());
		mListStyles.push_back(static_cast<ListStyle *>(pOrderedListStyle));
		mpCurrentListStyle = static_cast<ListStyle *>(pOrderedListStyle);
		mbListContinueNumbering = false;
		miLastListNumber = 0;
	}
	else
		mbListContinueNumbering = true;

	// Iterate through ALL list styles with the same WordPerfect list id and define a level if it is not already defined
	// This solves certain problems with lists that start and finish without reaching certain levels and then begin again
	// and reach those levels. See gradguide0405_PC.wpd in the regression suite
	for (std::vector<ListStyle *>::iterator iterOrderedListStyles = mListStyles.begin(); iterOrderedListStyles != mListStyles.end(); iterOrderedListStyles++)
	{
		if ((* iterOrderedListStyles)->getListID() == propList["libwpd:id"]->getInt())
			(* iterOrderedListStyles)->updateListLevel((propList["libwpd:level"]->getInt() - 1), propList);
	}
}

void WordPerfectCollector::defineUnorderedListLevel(const WPXPropertyList &propList)
{
	int id = 0;
	if (propList["libwpd:id"])
		id = propList["libwpd:id"]->getInt();

 	UnorderedListStyle *pUnorderedListStyle = NULL;
	if (mpCurrentListStyle && mpCurrentListStyle->getListID() == id)
		pUnorderedListStyle = static_cast<UnorderedListStyle *>(mpCurrentListStyle); // FIXME: using a dynamic cast here causes oo to crash?!

	if (pUnorderedListStyle == NULL) {
		WRITER_DEBUG_MSG(("Attempting to create a new unordered list style (listid: %i)\n", id));
		WPXString sName;
		sName.sprintf("UL%i", miNumListStyles);
		pUnorderedListStyle = new UnorderedListStyle(sName.cstr(), id);
		mListStyles.push_back(static_cast<ListStyle *>(pUnorderedListStyle));
		mpCurrentListStyle = static_cast<ListStyle *>(pUnorderedListStyle);
	}

	// See comment in WordPerfectCollector::defineOrderedListLevel
	for (std::vector<ListStyle *>::iterator iterUnorderedListStyles = mListStyles.begin(); iterUnorderedListStyles != mListStyles.end(); iterUnorderedListStyles++)
	{
		if ((* iterUnorderedListStyles)->getListID() == propList["libwpd:id"]->getInt())
			(* iterUnorderedListStyles)->updateListLevel((propList["libwpd:level"]->getInt() - 1), propList);
	}
}

void WordPerfectCollector::openOrderedListLevel(const WPXPropertyList &propList)
{
	miCurrentListLevel++;
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
	_openListLevel(pListLevelOpenElement);

	if (mbListContinueNumbering) {
		pListLevelOpenElement->addAttribute("text:continue-numbering", "true");
	}

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pListLevelOpenElement));
}

void WordPerfectCollector::openUnorderedListLevel(const WPXPropertyList &propList)
{
	miCurrentListLevel++;
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
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
	_closeListLevel("list");
}

void WordPerfectCollector::closeUnorderedListLevel()
{
	_closeListLevel("list");
}

void WordPerfectCollector::_closeListLevel(const char *szListType)
{
	if (mbListElementOpened)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:list-item")));

	miCurrentListLevel--;

	WPXString sCloseElement;
	sCloseElement.sprintf("text:%s", szListType);
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement(sCloseElement.cstr())));

	if (miCurrentListLevel > 0)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:list-item")));
	mbListElementOpened = false;
}

void WordPerfectCollector::openListElement(const WPXPropertyList &propList, const WPXPropertyListVector &tabStops)
{
	miLastListLevel = miCurrentListLevel;
	if (miCurrentListLevel == 1)
		miLastListNumber++;

	if (mbListElementOpened)
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:list-item")));

	ParagraphStyle *pStyle = NULL;

	WPXPropertyList *pPersistPropList = new WPXPropertyList(propList);
	pPersistPropList->insert("style:list-style-name", mpCurrentListStyle->getName());
	pPersistPropList->insert("style:parent-style-name", "Standard");

	WPXString sKey = getParagraphStyleKey(*pPersistPropList, tabStops);

	if (mTextStyleHash.find(sKey) == mTextStyleHash.end()) 
	{
		WPXString sName;
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
	TagOpenElement *pOpenFootNote = new TagOpenElement("text:note");
	pOpenFootNote->addAttribute("text:note-class", "footnote");
	if (propList["libwpd:number"])
	{
		WPXString tmpString("ftn");
		tmpString.append(propList["libwpd:number"]->getStr());
		pOpenFootNote->addAttribute("text:id", tmpString);
	}
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pOpenFootNote));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:note-citation")));
	if (propList["libwpd:number"])
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new CharDataElement(propList["libwpd:number"]->getStr().cstr())));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:note-citation")));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:note-body")));
	
	mWriterDocumentState.mbInNote = true;
}

void WordPerfectCollector::closeFootnote()
{
	mWriterDocumentState.mbInNote = false;

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:note-body")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:note")));
}

void WordPerfectCollector::openEndnote(const WPXPropertyList &propList)
{
	TagOpenElement *pOpenEndNote = new TagOpenElement("text:note");
	pOpenEndNote->addAttribute("text:note-class", "endnote");
	if (propList["libwpd:number"])
	{
		WPXString tmpString("edn");
		tmpString.append(propList["libwpd:number"]->getStr());
		pOpenEndNote->addAttribute("text:id", tmpString);
	}
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pOpenEndNote));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:note-citation")));
	if (propList["libwpd:number"])
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new CharDataElement(propList["libwpd:number"]->getStr().cstr())));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:note-citation")));

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:note-body")));
	
	mWriterDocumentState.mbInNote = true;
}

void WordPerfectCollector::closeEndnote()
{
	mWriterDocumentState.mbInNote = false;

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:note-body")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:note")));
}

void WordPerfectCollector::openTable(const WPXPropertyList &propList, const WPXPropertyListVector &columns)
{
	if (!mWriterDocumentState.mbInNote)
	{
		WPXString sTableName;
		sTableName.sprintf("Table%i", mTableStyles.size());

		// FIXME: we base the table style off of the page's margin left, ignoring (potential) wordperfect margin
		// state which is transmitted inside the page. could this lead to unacceptable behaviour?
		// WLACH_REFACTORING: characterize this behaviour, probably should nip it at the bud within libwpd
		TableStyle *pTableStyle = new TableStyle(propList, columns, sTableName.cstr());

		if (mWriterDocumentState.mbFirstElement && mpCurrentContentElements == &mBodyElements)
		{
			WPXString sMasterPageName("Page_Style_1");
			pTableStyle->setMasterPageName(sMasterPageName);
			mWriterDocumentState.mbFirstElement = false;
		}

		mTableStyles.push_back(pTableStyle);

		mpCurrentTableStyle = pTableStyle;

		TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");

		pTableOpenElement->addAttribute("table:name", sTableName.cstr());
		pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pTableOpenElement));

		for (int i=0; i<pTableStyle->getNumColumns(); i++) 
		{
			TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
			WPXString sColumnStyleName;
			sColumnStyleName.sprintf("%s.Column%i", sTableName.cstr(), (i+1));
			pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
			mpCurrentContentElements->push_back(pTableColumnOpenElement);

			TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
			mpCurrentContentElements->push_back(pTableColumnCloseElement);
		}
	}
}

void WordPerfectCollector::openTableRow(const WPXPropertyList &propList)
{
	if (!mWriterDocumentState.mbInNote)
	{
		if (propList["libwpd:is-header-row"] && (propList["libwpd:is-header-row"]->getInt()))
		{
			mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("table:table-header-rows")));
			mWriterDocumentState.mbHeaderRow = true;
		}

		WPXString sTableRowStyleName;
		sTableRowStyleName.sprintf("%s.Row%i", mpCurrentTableStyle->getName().cstr(), mpCurrentTableStyle->getNumTableRowStyles());
		TableRowStyle *pTableRowStyle = new TableRowStyle(propList, sTableRowStyleName.cstr());
		mpCurrentTableStyle->addTableRowStyle(pTableRowStyle);

		TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
		pTableRowOpenElement->addAttribute("table:style-name", sTableRowStyleName);
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pTableRowOpenElement));
	}
}

void WordPerfectCollector::closeTableRow()
{
	if (!mWriterDocumentState.mbInNote)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table-row")));
		if (mWriterDocumentState.mbHeaderRow)
		{
			mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table-header-rows")));
			mWriterDocumentState.mbHeaderRow = false;
		}
	}
}

void WordPerfectCollector::openTableCell(const WPXPropertyList &propList)
{
	if (!mWriterDocumentState.mbInNote)
	{
		WPXString sTableCellStyleName;
		sTableCellStyleName.sprintf( "%s.Cell%i", mpCurrentTableStyle->getName().cstr(), mpCurrentTableStyle->getNumTableCellStyles());
		TableCellStyle *pTableCellStyle = new TableCellStyle(propList, sTableCellStyleName.cstr());
		mpCurrentTableStyle->addTableCellStyle(pTableCellStyle);

		TagOpenElement *pTableCellOpenElement = new TagOpenElement("table:table-cell");
		pTableCellOpenElement->addAttribute("table:style-name", sTableCellStyleName);
		if (propList["table:number-columns-spanned"])
			pTableCellOpenElement->addAttribute("table:number-columns-spanned", 
							    propList["table:number-columns-spanned"]->getStr().cstr());
		if (propList["table:number-rows-spanned"])
			pTableCellOpenElement->addAttribute("table:number-rows-spanned",
							    propList["table:number-rows-spanned"]->getStr().cstr());
		// pTableCellOpenElement->addAttribute("table:value-type", "string");
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(pTableCellOpenElement));

		mWriterDocumentState.mbTableCellOpened = true;
	}
}

void WordPerfectCollector::closeTableCell()
{
	if (!mWriterDocumentState.mbInNote)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table-cell")));
		mWriterDocumentState.mbTableCellOpened = false;
	}
}

void WordPerfectCollector::insertCoveredTableCell(const WPXPropertyList &propList)
{
	if (!mWriterDocumentState.mbInNote)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("table:covered-table-cell")));
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:covered-table-cell")));
	}
}

void WordPerfectCollector::closeTable()
{
	if (!mWriterDocumentState.mbInNote)
	{
		mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("table:table")));
	}
}


void WordPerfectCollector::insertTab()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:tab")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:tab")));
}

void WordPerfectCollector::insertLineBreak()
{
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:line-break")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:line-break")));
}

void WordPerfectCollector::insertText(const WPXString &text)
{
	DocumentElement *pText = new TextElement(text);
	mpCurrentContentElements->push_back(pText);
}

void WordPerfectCollector::insertGraphics(const WPXInputStream *graphicsData)
{
	if (!graphicsData)
		return;
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("text:p")));
	TagOpenElement *drawFrameOpenElement = new TagOpenElement("draw:frame");
#if 0
	drawFrameOpenElement->addAttribute("draw:style-name","fr1");
	WPXString objectName;
	objectName.sprintf("Object%i", miObjectNumber++);
	drawFrameOpenElement->addAttribute("draw:name", objectName);
	drawFrameOpenElement->addAttribute("text:anchor-type","page");
	drawFrameOpenElement->addAttribute("svg:x", "0.000in");
	drawFrameOpenElement->addAttribute("svg:y", "0.000in");
	drawFrameOpenElement->addAttribute("svg:width", "1.000in");
	drawFrameOpenElement->addAttribute("svg:height", "1.500in");
#endif
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(drawFrameOpenElement));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagOpenElement("draw:object")));
	
	InternalHandler tmpHandler(mpCurrentContentElements);
	OdgExporter exporter(&tmpHandler, true);
	libwpg::WPGraphics::parse(const_cast<WPXInputStream *>(graphicsData), &exporter);

	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("draw:object")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("draw:frame")));
	mpCurrentContentElements->push_back(static_cast<DocumentElement *>(new TagCloseElement("text:p")));
}
