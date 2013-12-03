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
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <librevenge/librevenge.h>
#include <vector>
#include <map>
#include <stack>
#include <string>

#include <libodfgen/libodfgen.hxx>

#include "DocumentElement.hxx"
#include "TextRunStyle.hxx"
#include "FontStyle.hxx"
#include "ListStyle.hxx"
#include "OdfGenerator.hxx"
#include "PageSpan.hxx"
#include "SectionStyle.hxx"
#include "TableStyle.hxx"
#include "FilterInternal.hxx"
#include "InternalHandler.hxx"

// the state we use for writing the final document
struct WriterDocumentState
{
	WriterDocumentState();

	bool mbFirstElement;
	bool mbFirstParagraphInPageSpan;
	bool mbInFakeSection;
	bool mbListElementOpenedAtCurrentLevel;
	bool mbTableCellOpened;
	bool mbHeaderRow;
	bool mbInNote;
	bool mbInTextBox;
	bool mbInFrame;
};

// list state
struct WriterListState
{
	WriterListState();
	WriterListState(const WriterListState &state);

	ListStyle *mpCurrentListStyle;
	unsigned int miCurrentListLevel;
	unsigned int miLastListLevel;
	unsigned int miLastListNumber;
	bool mbListContinueNumbering;
	bool mbListElementParagraphOpened;
	std::stack<bool> mbListElementOpened;
	// a map id -> last list style defined with such id
	std::map<int, ListStyle *> mIdListStyleMap;
private:
	WriterListState &operator=(const WriterListState &state);
};

enum WriterListType { unordered, ordered };

WriterDocumentState::WriterDocumentState() :
	mbFirstElement(true),
	mbFirstParagraphInPageSpan(true),
	mbInFakeSection(false),
	mbListElementOpenedAtCurrentLevel(false),
	mbTableCellOpened(false),
	mbHeaderRow(false),
	mbInNote(false),
	mbInTextBox(false),
	mbInFrame(false)
{
}

WriterListState::WriterListState() :
	mpCurrentListStyle(0),
	miCurrentListLevel(0),
	miLastListLevel(0),
	miLastListNumber(0),
	mbListContinueNumbering(false),
	mbListElementParagraphOpened(false),
	mbListElementOpened(),
	mIdListStyleMap()
{
}

WriterListState::WriterListState(const WriterListState &state) :
	mpCurrentListStyle(state.mpCurrentListStyle),
	miCurrentListLevel(state.miCurrentListLevel),
	miLastListLevel(state.miCurrentListLevel),
	miLastListNumber(state.miLastListNumber),
	mbListContinueNumbering(state.mbListContinueNumbering),
	mbListElementParagraphOpened(state.mbListElementParagraphOpened),
	mbListElementOpened(state.mbListElementOpened),
	mIdListStyleMap(state.mIdListStyleMap)
{
}

class OdtGeneratorPrivate : public OdfGenerator
{
public:
	OdtGeneratorPrivate();
	~OdtGeneratorPrivate();

	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler);
	void _writeMasterPages(OdfDocumentHandler *pHandler);
	void _writePageLayouts(OdfDocumentHandler *pHandler);

	void _openListLevel(TagOpenElement *pListLevelOpenElement);
	void _closeListLevel();

	/** stores a list style: update mListStyles,
		mWriterListStates.top().mpCurrentListStyle and the different
		maps
	 */
	void _storeListStyle(ListStyle *listStyle);
	/** retrieves the list style corresponding to a given id. */
	void _retrieveListStyle(int id);


	std::stack<WriterDocumentState> mWriterDocumentStates;

	std::stack<WriterListState> mWriterListStates;

	// section styles
	std::vector<SectionStyle *> mSectionStyles;

	// table styles
	std::vector<TableStyle *> mTableStyles;

	// frame styles
	std::vector<DocumentElement *> mFrameStyles;

	std::vector<DocumentElement *> mFrameAutomaticStyles;

	std::map<librevenge::RVNGString, unsigned, ltstr> mFrameIdMap;

	// list styles
	unsigned int miNumListStyles;

	// page state
	std::vector<PageSpan *> mPageSpans;
	PageSpan *mpCurrentPageSpan;
	int miNumPageStyles;

	// list styles
	std::vector<ListStyle *> mListStyles;
	// a map id -> last list style defined with id
	std::map<int, ListStyle *> mIdListStyleMap;

	// table state
	TableStyle *mpCurrentTableStyle;

private:
	OdtGeneratorPrivate(const OdtGeneratorPrivate &);
	OdtGeneratorPrivate &operator=(const OdtGeneratorPrivate &);

};

OdtGeneratorPrivate::OdtGeneratorPrivate() :
	mWriterDocumentStates(),
	mWriterListStates(),
	mSectionStyles(), mTableStyles(),
	mFrameStyles(), mFrameAutomaticStyles(), mFrameIdMap(),
	miNumListStyles(0),
	mPageSpans(),
	mpCurrentPageSpan(0),
	miNumPageStyles(0),
	mListStyles(),
	mIdListStyleMap(),
	mpCurrentTableStyle(0)
{
	mWriterDocumentStates.push(WriterDocumentState());
	mWriterListStates.push(WriterListState());
}

OdtGeneratorPrivate::~OdtGeneratorPrivate()
{
	// clean up the mess we made
	ODFGEN_DEBUG_MSG(("OdtGenerator: Cleaning up our mess..\n"));

	ODFGEN_DEBUG_MSG(("OdtGenerator: Destroying the body elements\n"));
	for (std::vector<ListStyle *>::iterator iterListStyles = mListStyles.begin();
	        iterListStyles != mListStyles.end(); ++iterListStyles)
	{
		delete(*iterListStyles);
	}
	for (std::vector<SectionStyle *>::iterator iterSectionStyles = mSectionStyles.begin();
	        iterSectionStyles != mSectionStyles.end(); ++iterSectionStyles)
	{
		delete(*iterSectionStyles);
	}
	for (std::vector<TableStyle *>::iterator iterTableStyles = mTableStyles.begin();
	        iterTableStyles != mTableStyles.end(); ++iterTableStyles)
	{
		delete((*iterTableStyles));
	}

	for (std::vector<PageSpan *>::iterator iterPageSpans = mPageSpans.begin();
	        iterPageSpans != mPageSpans.end(); ++iterPageSpans)
	{
		delete(*iterPageSpans);
	}
	emptyStorage(&mFrameStyles);
	emptyStorage(&mFrameAutomaticStyles);
}

void OdtGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:automatic-styles").write(pHandler);
	sendStorage(&mFrameAutomaticStyles, pHandler);
	mFontManager.write(pHandler); // do nothing
	mSpanManager.write(pHandler);
	mParagraphManager.write(pHandler);

	_writePageLayouts(pHandler);
	// writing out the sections styles
	for (std::vector<SectionStyle *>::const_iterator iterSectionStyles = mSectionStyles.begin(); iterSectionStyles != mSectionStyles.end(); ++iterSectionStyles)
		(*iterSectionStyles)->write(pHandler);
	// writing out the lists styles
	for (std::vector<ListStyle *>::const_iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); ++iterListStyles)
		(*iterListStyles)->write(pHandler);
	// writing out the table styles
	for (std::vector<TableStyle *>::const_iterator iterTableStyles = mTableStyles.begin(); iterTableStyles != mTableStyles.end(); ++iterTableStyles)
		(*iterTableStyles)->write(pHandler);

	pHandler->endElement("office:automatic-styles");
}

void OdtGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);

	// style:default-style

	// paragraph
	TagOpenElement defaultParagraphStyleOpenElement("style:default-style");
	defaultParagraphStyleOpenElement.addAttribute("style:family", "paragraph");
	defaultParagraphStyleOpenElement.write(pHandler);
	TagOpenElement defaultParagraphStylePropertiesOpenElement("style:paragraph-properties");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:use-window-font-color", "true");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:line-break", "strict");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:tab-stop-distance", "0.5in");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:text-autospace", "ideograph-alpha");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:punctuation-wrap", "hanging");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:writing-mode", "page");
	defaultParagraphStylePropertiesOpenElement.write(pHandler);
	pHandler->endElement("style:paragraph-properties");
	pHandler->endElement("style:default-style");

	// table
	TagOpenElement defaultTableStyleOpenElement("style:default-style");
	defaultTableStyleOpenElement.addAttribute("style:family", "table");
	defaultTableStyleOpenElement.write(pHandler);
	pHandler->endElement("style:default-style");

	// table-row
	TagOpenElement defaultTableRowStyleOpenElement("style:default-style");
	defaultTableRowStyleOpenElement.addAttribute("style:family", "table-row");
	defaultTableRowStyleOpenElement.write(pHandler);
	TagOpenElement defaultTableRowPropertiesOpenElement("style:table-row-properties");
	defaultTableRowPropertiesOpenElement.addAttribute("fo:keep-together", "auto");
	defaultTableRowPropertiesOpenElement.write(pHandler);
	pHandler->endElement("style:table-row-properties");
	pHandler->endElement("style:default-style");

	// table-column
	TagOpenElement defaultTableColumnStyleOpenElement("style:default-style");
	defaultTableColumnStyleOpenElement.addAttribute("style:family", "table-column");
	defaultTableColumnStyleOpenElement.write(pHandler);
	pHandler->endElement("style:default-style");

	// table-cell
	TagOpenElement defaultTableCellStyleOpenElement("style:default-style");
	defaultTableCellStyleOpenElement.addAttribute("style:family", "table-cell");
	defaultTableCellStyleOpenElement.write(pHandler);
	pHandler->endElement("style:default-style");

	// basic style

	TagOpenElement standardStyleOpenElement("style:style");
	standardStyleOpenElement.addAttribute("style:name", "Standard");
	standardStyleOpenElement.addAttribute("style:family", "paragraph");
	standardStyleOpenElement.addAttribute("style:class", "text");
	standardStyleOpenElement.write(pHandler);
	pHandler->endElement("style:style");

	static char const *(s_paraStyle[4*10]) =
	{
		"Text_Body", "Text Body", "Standard", "text",
		"Table_Contents", "Table Contents", "Text_Body", "extra",
		"Table_Heading", "Table Heading", "Table_Contents", "extra",
		"List", "List", "Text_Body", "list",
		"Header", "Header", "Standard", "extra",
		"Footer", "Footer", "Standard", "extra",
		"Caption", "Caption", "Standard", "extra",
		"Footnote", "Footnote", "Standard", "extra",
		"Endnote", "Endnote", "Standard", "extra",
		"Index", "Index", "Standard", "extra"
	};
	for (int i=0; i<10; ++i)
	{
		TagOpenElement paraOpenElement("style:style");
		paraOpenElement.addAttribute("style:name", s_paraStyle[4*i]);
		paraOpenElement.addAttribute("style:display-name", s_paraStyle[4*i+1]);
		paraOpenElement.addAttribute("style:family", "paragraph");
		paraOpenElement.addAttribute("style:parent-style-name", s_paraStyle[4*i+2]);
		paraOpenElement.addAttribute("style:class", s_paraStyle[4*i+3]);
		paraOpenElement.write(pHandler);
		pHandler->endElement("style:style");
	}

	static char const *(s_textStyle[2*4])=
	{
		"Footnote_Symbol", "Footnote Symbol", "Endnote_Symbol", "Endnote Symbol",
		"Footnote_anchor", "Footnote anchor", "Endnote_anchor", "Endnote anchor"
	};
	for (int i=0; i<4; ++i)
	{
		TagOpenElement textOpenElement("style:style");
		textOpenElement.addAttribute("style:name", s_textStyle[2*i]);
		textOpenElement.addAttribute("style:display-name", s_textStyle[2*i]);
		textOpenElement.addAttribute("style:family", "text");
		textOpenElement.write(pHandler);
		TagOpenElement textPropertiesOpenElement("style:text-properties");
		textPropertiesOpenElement.addAttribute("style:text-position", "super 58%");
		textPropertiesOpenElement.write(pHandler);
		pHandler->endElement("style:text-properties");
		pHandler->endElement("style:style");
	}
	sendStorage(&mFrameStyles, pHandler);

	TagOpenElement lineOpenElement("text:linenumbering-configuration");
	lineOpenElement.addAttribute("text:number-lines", "false");
	lineOpenElement.addAttribute("text:number-position", "left");
	lineOpenElement.addAttribute("text:increment", "5");
	lineOpenElement.addAttribute("text:offset", "0.1965in");
	lineOpenElement.addAttribute("style:num-format", "1");
	lineOpenElement.write(pHandler);
	pHandler->endElement("text:linenumbering-configuration");

	static char const *(s_noteConfig[4*2])=
	{
		"footnote", "Footnote_Symbol", "Footnote_anchor", "1",
		"endnote", "Endnote_Symbol", "Endnote_anchor", "i"
	};
	for (int i=0; i<2; ++i)
	{
		TagOpenElement noteOpenElement("text:notes-configuration");
		noteOpenElement.addAttribute("text:note-class", s_noteConfig[4*i]);
		noteOpenElement.addAttribute("text:citation-style-name", s_noteConfig[4*i+1]);
		noteOpenElement.addAttribute("text:citation-body-style-name", s_noteConfig[4*i+2]);
		noteOpenElement.addAttribute("style:num-format", s_noteConfig[4*i+3]);
		noteOpenElement.addAttribute("text:start-value", "0");
		if (i==0)
		{
			noteOpenElement.addAttribute("text:footnotes-position", "page");
			noteOpenElement.addAttribute("text:start-numbering-at", "document");
		}
		else
			noteOpenElement.addAttribute("text:master-page-name", "Endnote");
		noteOpenElement.write(pHandler);
		pHandler->endElement("text:notes-configuration");
	}

	pHandler->endElement("office:styles");
}

void OdtGeneratorPrivate::_writeMasterPages(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:master-styles").write(pHandler);

	static char const *(s_default[2*2]) = { "Standard", "PM0", "Endnote", "PM1" };
	for (int i=0; i < 2; ++i)
	{
		TagOpenElement pageOpenElement("style:master-page");
		pageOpenElement.addAttribute("style:name", s_default[2*i]);
		pageOpenElement.addAttribute("style:page-layout-name", s_default[2*i+1]);
		pageOpenElement.write(pHandler);
		pHandler->endElement("style:master-page");
	}

	int pageNumber = 1;
	for (unsigned int i=0; i<mPageSpans.size(); ++i)
	{
		bool bLastPage;
		(i == (mPageSpans.size() - 1)) ? bLastPage = true : bLastPage = false;
		mPageSpans[i]->writeMasterPages(pageNumber, (int)i, bLastPage, pHandler);
		pageNumber += mPageSpans[i]->getSpan();
	}
	pHandler->endElement("office:master-styles");
}

void OdtGeneratorPrivate::_writePageLayouts(OdfDocumentHandler *pHandler)
{
	for (int i=0; i < 2; ++i)
	{
		TagOpenElement layout("style:page-layout");
		layout.addAttribute("style:name", i==0 ? "PM0" : "PM1");
		layout.write(pHandler);

		TagOpenElement layoutProperties("style:page-layout-properties");
		layoutProperties.addAttribute("fo:margin-bottom", "1in");
		layoutProperties.addAttribute("fo:margin-left", "1in");
		layoutProperties.addAttribute("fo:margin-right", "1in");
		layoutProperties.addAttribute("fo:margin-top", "1in");
		layoutProperties.addAttribute("fo:page-height", "11in");
		layoutProperties.addAttribute("fo:page-width", "8.5in");
		layoutProperties.addAttribute("style:print-orientation", "portrait");
		layoutProperties.write(pHandler);

		TagOpenElement footnoteSep("style:footnote-sep");
		footnoteSep.addAttribute("style:adjustment","left");
		footnoteSep.addAttribute("style:color","#000000");
		footnoteSep.addAttribute("style:rel-width","25%");
		if (i==0)
		{
			footnoteSep.addAttribute("style:distance-after-sep","0.0398in");
			footnoteSep.addAttribute("style:distance-before-sep","0.0398in");
			footnoteSep.addAttribute("style:width","0.0071in");
		}
		footnoteSep.write(pHandler);
		TagCloseElement("style:footnote-sep").write(pHandler);
		TagCloseElement("style:page-layout-properties").write(pHandler);

		TagCloseElement("style:page-layout").write(pHandler);
	}
	for (unsigned int i=0; i<mPageSpans.size(); ++i)
		mPageSpans[i]->writePageLayout((int)i, pHandler);
}

bool OdtGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: Printing out the header stuff..\n"));

	ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: Start Document\n"));
	pHandler->startDocument();

	ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: preamble\n"));
	std::string const documentType=getDocumentType(streamType);
	librevenge::RVNGPropertyList docContentPropList;
	docContentPropList.insert("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	docContentPropList.insert("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	docContentPropList.insert("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	docContentPropList.insert("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	docContentPropList.insert("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	docContentPropList.insert("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	docContentPropList.insert("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	docContentPropList.insert("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	docContentPropList.insert("xmlns:xlink", "http://www.w3.org/1999/xlink");
	docContentPropList.insert("xmlns:number", "urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0");
	docContentPropList.insert("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	docContentPropList.insert("xmlns:chart", "urn:oasis:names:tc:opendocument:xmlns:chart:1.0");
	docContentPropList.insert("xmlns:dr3d", "urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0");
	docContentPropList.insert("xmlns:math", "http://www.w3.org/1998/Math/MathML");
	docContentPropList.insert("xmlns:form", "urn:oasis:names:tc:opendocument:xmlns:form:1.0");
	docContentPropList.insert("xmlns:script", "urn:oasis:names:tc:opendocument:xmlns:script:1.0");
	docContentPropList.insert("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	docContentPropList.insert("office:version", librevenge::RVNGPropertyFactory::newStringProp("1.1"));
	if (streamType == ODF_FLAT_XML)
		docContentPropList.insert("office:mimetype", "application/vnd.oasis.opendocument.text");
	pHandler->startElement(documentType.c_str(), docContentPropList);

	// write out the metadata
	if (streamType == ODF_FLAT_XML || streamType == ODF_META_XML)
		writeDocumentMetaData(pHandler);

	// write out the font styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML || streamType == ODF_CONTENT_XML)
		mFontManager.writeFontsDeclaration(pHandler);

	ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: Writing out the styles..\n"));

	// write default styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
		_writeStyles(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML || streamType == ODF_CONTENT_XML)
		_writeAutomaticStyles(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
		_writeMasterPages(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: Writing out the document..\n"));
		// writing out the document
		TagOpenElement("office:body").write(pHandler);
		TagOpenElement("office:text").write(pHandler);
		sendStorage(&mBodyStorage, pHandler);
		ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: Finished writing all doc els..\n"));

		pHandler->endElement("office:text");
		pHandler->endElement("office:body");
	}

	pHandler->endElement(documentType.c_str());

	pHandler->endDocument();

	return true;
}

OdtGenerator::OdtGenerator() : mpImpl(new OdtGeneratorPrivate)
{
}

OdtGenerator::~OdtGenerator()
{
	if (mpImpl)
		delete mpImpl;
}

void OdtGenerator::addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
{
	if (mpImpl)
		mpImpl->addDocumentHandler(pHandler, streamType);
}


void OdtGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdtGenerator::openPageSpan(const librevenge::RVNGPropertyList &propList)
{
	PageSpan *pPageSpan = new PageSpan(propList);
	mpImpl->mPageSpans.push_back(pPageSpan);
	mpImpl->mpCurrentPageSpan = pPageSpan;
	mpImpl->miNumPageStyles++;

	mpImpl->mWriterDocumentStates.top().mbFirstParagraphInPageSpan = true;
}

void OdtGenerator::openHeader(const librevenge::RVNGPropertyList &propList)
{
	std::vector<DocumentElement *> *pHeaderFooterContentElements = new std::vector<DocumentElement *>;

	if (propList["librevenge:occurence"] && propList["librevenge:occurence"]->getStr() == "even")
		mpImpl->mpCurrentPageSpan->setHeaderLeftContent(pHeaderFooterContentElements);
	else
		mpImpl->mpCurrentPageSpan->setHeaderContent(pHeaderFooterContentElements);

	mpImpl->pushStorage(pHeaderFooterContentElements);
}

void OdtGenerator::closeHeader()
{
	mpImpl->popStorage();
}

void OdtGenerator::openFooter(const librevenge::RVNGPropertyList &propList)
{
	std::vector<DocumentElement *> *pHeaderFooterContentElements = new std::vector<DocumentElement *>;

	if (propList["librevenge:occurence"] && propList["librevenge:occurence"]->getStr() == "even")
		mpImpl->mpCurrentPageSpan->setFooterLeftContent(pHeaderFooterContentElements);
	else
		mpImpl->mpCurrentPageSpan->setFooterContent(pHeaderFooterContentElements);

	mpImpl->pushStorage(pHeaderFooterContentElements);
}

void OdtGenerator::closeFooter()
{
	mpImpl->popStorage();
}

void OdtGenerator::openSection(const librevenge::RVNGPropertyList &propList)
{
	size_t iNumColumns = 0;
	const librevenge::RVNGPropertyListVector *columns = propList.child("style:columns");
	if (columns)
		iNumColumns = columns->count();
	double fSectionMarginLeft = 0.0;
	double fSectionMarginRight = 0.0;
	if (propList["fo:margin-left"])
		fSectionMarginLeft = propList["fo:margin-left"]->getDouble();
	if (propList["fo:margin-right"])
		fSectionMarginRight = propList["fo:margin-right"]->getDouble();

	if (iNumColumns > 1 || fSectionMarginLeft != 0 || fSectionMarginRight != 0)
	{
		librevenge::RVNGString sSectionName;
		sSectionName.sprintf("Section%i", mpImpl->mSectionStyles.size());

		SectionStyle *pSectionStyle = new SectionStyle(propList, *columns, sSectionName.cstr());
		mpImpl->mSectionStyles.push_back(pSectionStyle);

		TagOpenElement *pSectionOpenElement = new TagOpenElement("text:section");
		pSectionOpenElement->addAttribute("text:style-name", pSectionStyle->getName());
		pSectionOpenElement->addAttribute("text:name", pSectionStyle->getName());
		mpImpl->getCurrentStorage()->push_back(pSectionOpenElement);
	}
	else
		mpImpl->mWriterDocumentStates.top().mbInFakeSection = true;
}

void OdtGenerator::closeSection()
{
	if (!mpImpl->mWriterDocumentStates.top().mbInFakeSection)
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:section"));
	else
		mpImpl->mWriterDocumentStates.top().mbInFakeSection = false;
}

void OdtGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	// FIXMENOW: What happens if we open a footnote inside a table? do we then inherit the footnote's style
	// from "Table Contents"

	librevenge::RVNGPropertyList finalPropList(propList);
	if (mpImpl->mWriterDocumentStates.top().mbFirstParagraphInPageSpan && mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
	{
		librevenge::RVNGString sPageStyleName;
		sPageStyleName.sprintf("Page_Style_%i", mpImpl->miNumPageStyles);
		finalPropList.insert("style:master-page-name", sPageStyleName);
		mpImpl->mWriterDocumentStates.top().mbFirstElement = false;
		mpImpl->mWriterDocumentStates.top().mbFirstParagraphInPageSpan = false;
	}

	if (mpImpl->mWriterDocumentStates.top().mbTableCellOpened)
	{
		if (mpImpl->mWriterDocumentStates.top().mbHeaderRow)
			finalPropList.insert("style:parent-style-name", "Table_Heading");
		else
			finalPropList.insert("style:parent-style-name", "Table_Contents");
	}
	else
		finalPropList.insert("style:parent-style-name", "Standard");
	mpImpl->openParagraph(finalPropList);
}

void OdtGenerator::closeParagraph()
{
	mpImpl->closeParagraph();
}

void OdtGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openSpan(propList);
}

void OdtGenerator::closeSpan()
{
	mpImpl->closeSpan();
}

void OdtGeneratorPrivate::_storeListStyle(ListStyle *listStyle)
{
	if (!listStyle || listStyle == mWriterListStates.top().mpCurrentListStyle)
	{
		return;
	}
	mListStyles.push_back(listStyle);
	mWriterListStates.top().mpCurrentListStyle = listStyle;
	mWriterListStates.top().mIdListStyleMap[listStyle->getListID()]=listStyle;
	mIdListStyleMap[listStyle->getListID()]=listStyle;
}

void OdtGeneratorPrivate::_retrieveListStyle(int id)
{
	// first look if the current style is ok
	if (mWriterListStates.top().mpCurrentListStyle &&
	        id == mWriterListStates.top().mpCurrentListStyle->getListID())
	{
		return;
	}

	// use the current map
	if (mWriterListStates.top().mIdListStyleMap.find(id) !=
	        mWriterListStates.top().mIdListStyleMap.end())
	{
		mWriterListStates.top().mpCurrentListStyle =
		    mWriterListStates.top().mIdListStyleMap.find(id)->second;
		return;
	}

	// use the global map
	if (mIdListStyleMap.find(id) != mIdListStyleMap.end())
	{
		mWriterListStates.top().mpCurrentListStyle =
		    mIdListStyleMap.find(id)->second;
		return;
	}

	ODFGEN_DEBUG_MSG(("OdtGenerator: impossible to find a list with id=%d\n",id));
}

void OdtGenerator::defineOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	int id = 0;
	if (propList["librevenge:id"])
		id = propList["librevenge:id"]->getInt();

	ListStyle *pListStyle = 0;
	if (mpImpl->mWriterListStates.top().mpCurrentListStyle && mpImpl->mWriterListStates.top().mpCurrentListStyle->getListID() == id)
		pListStyle = mpImpl->mWriterListStates.top().mpCurrentListStyle;

	// this rather appalling conditional makes sure we only start a
	// new list (rather than continue an old one) if: (1) we have no
	// prior list or the prior list has another listId OR (2) we can
	// tell that the user actually is starting a new list at level 1
	// (and only level 1)
	if (pListStyle == 0 ||
	        (propList["librevenge:level"] && propList["librevenge:level"]->getInt()==1 &&
	         (propList["text:start-value"] && propList["text:start-value"]->getInt() != int(mpImpl->mWriterListStates.top().miLastListNumber+1))))
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator: Attempting to create a new ordered list style (listid: %i)\n", id));
		librevenge::RVNGString sName;
		sName.sprintf("OL%i", mpImpl->miNumListStyles);
		mpImpl->miNumListStyles++;
		pListStyle = new ListStyle(sName.cstr(), id);
		mpImpl->_storeListStyle(pListStyle);
		mpImpl->mWriterListStates.top().mbListContinueNumbering = false;
		mpImpl->mWriterListStates.top().miLastListNumber = 0;
	}
	else
		mpImpl->mWriterListStates.top().mbListContinueNumbering = true;

	// Iterate through ALL list styles with the same WordPerfect list id and define a level if it is not already defined
	// This solves certain problems with lists that start and finish without reaching certain levels and then begin again
	// and reach those levels. See gradguide0405_PC.wpd in the regression suite
	for (std::vector<ListStyle *>::iterator iterListStyles = mpImpl->mListStyles.begin(); iterListStyles != mpImpl->mListStyles.end(); ++iterListStyles)
	{
		if ((* iterListStyles) && (* iterListStyles)->getListID() == id && propList["librevenge:level"])
			(* iterListStyles)->updateListLevel((propList["librevenge:level"]->getInt() - 1), propList, true);
	}
}

void OdtGenerator::defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	int id = 0;
	if (propList["librevenge:id"])
		id = propList["librevenge:id"]->getInt();

	ListStyle *pListStyle = 0;
	if (mpImpl->mWriterListStates.top().mpCurrentListStyle && mpImpl->mWriterListStates.top().mpCurrentListStyle->getListID() == id)
		pListStyle = mpImpl->mWriterListStates.top().mpCurrentListStyle;

	if (pListStyle == 0)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator: Attempting to create a new unordered list style (listid: %i)\n", id));
		librevenge::RVNGString sName;
		sName.sprintf("UL%i", mpImpl->miNumListStyles);
		mpImpl->miNumListStyles++;
		pListStyle = new ListStyle(sName.cstr(), id);
		mpImpl->_storeListStyle(pListStyle);
	}

	// See comment in OdtGenerator::defineOrderedListLevel
	for (std::vector<ListStyle *>::iterator iterListStyles = mpImpl->mListStyles.begin(); iterListStyles != mpImpl->mListStyles.end(); ++iterListStyles)
	{
		if ((* iterListStyles) && (* iterListStyles)->getListID() == id && propList["librevenge:level"])
			(* iterListStyles)->updateListLevel((propList["librevenge:level"]->getInt() - 1), propList, false);
	}
}

void OdtGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mWriterListStates.top().mbListElementParagraphOpened)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:p"));
		mpImpl->mWriterListStates.top().mbListElementParagraphOpened = false;
	}
	if (mpImpl->mWriterListStates.top().mbListElementOpened.empty() && propList["librevenge:id"])
	{
		// first item of a list, be sure to use the list with given id
		mpImpl->_retrieveListStyle(propList["librevenge:id"]->getInt());
	}
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
	mpImpl->_openListLevel(pListLevelOpenElement);

	if (mpImpl->mWriterListStates.top().mbListContinueNumbering)
	{
		pListLevelOpenElement->addAttribute("text:continue-numbering", "true");
	}

	mpImpl->getCurrentStorage()->push_back(pListLevelOpenElement);
}

void OdtGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mWriterListStates.top().mbListElementParagraphOpened)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:p"));
		mpImpl->mWriterListStates.top().mbListElementParagraphOpened = false;
	}
	if (mpImpl->mWriterListStates.top().mbListElementOpened.empty() && propList["librevenge:id"])
	{
		// first item of a list, be sure to use the list with given id
		mpImpl->_retrieveListStyle(propList["librevenge:id"]->getInt());
	}
	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
	mpImpl->_openListLevel(pListLevelOpenElement);

	mpImpl->getCurrentStorage()->push_back(pListLevelOpenElement);
}

void OdtGeneratorPrivate::_openListLevel(TagOpenElement *pListLevelOpenElement)
{
	if (!mWriterListStates.top().mbListElementOpened.empty() &&
	        !mWriterListStates.top().mbListElementOpened.top())
	{
		getCurrentStorage()->push_back(new TagOpenElement("text:list-item"));
		mWriterListStates.top().mbListElementOpened.top() = true;
	}

	mWriterListStates.top().mbListElementOpened.push(false);
	if (mWriterListStates.top().mbListElementOpened.size() == 1)
	{
		// add a sanity check ( to avoid a crash if mpCurrentListStyle is NULL)
		if (mWriterListStates.top().mpCurrentListStyle)
		{
			pListLevelOpenElement->addAttribute("text:style-name", mWriterListStates.top().mpCurrentListStyle->getName());
		}
	}
}

void OdtGenerator::closeOrderedListLevel()
{
	mpImpl->_closeListLevel();
}

void OdtGenerator::closeUnorderedListLevel()
{
	mpImpl->_closeListLevel();
}

void OdtGeneratorPrivate::_closeListLevel()
{
	if (mWriterListStates.top().mbListElementOpened.empty())
	{
		// this implies that openListLevel was not called, so it is better to stop here
		ODFGEN_DEBUG_MSG(("OdtGenerator: Attempting to close an unexisting level\n"));
		return;
	}
	if (mWriterListStates.top().mbListElementOpened.top())
	{
		getCurrentStorage()->push_back(new TagCloseElement("text:list-item"));
		mWriterListStates.top().mbListElementOpened.top() = false;
	}

	getCurrentStorage()->push_back(new TagCloseElement("text:list"));
	mWriterListStates.top().mbListElementOpened.pop();
}

void OdtGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mWriterListStates.top().miLastListLevel = mpImpl->mWriterListStates.top().miCurrentListLevel;
	if (mpImpl->mWriterListStates.top().miCurrentListLevel == 1)
		mpImpl->mWriterListStates.top().miLastListNumber++;

	if (mpImpl->mWriterListStates.top().mbListElementOpened.top())
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:list-item"));
		mpImpl->mWriterListStates.top().mbListElementOpened.top() = false;
	}

	librevenge::RVNGPropertyList finalPropList(propList);
#if 0
	// this property is ignored in TextRunStyle.c++
	if (mpImpl->mWriterListStates.top().mpCurrentListStyle)
		finalPropList.insert("style:list-style-name", mpImpl->mWriterListStates.top().mpCurrentListStyle->getName());
#endif
	finalPropList.insert("style:parent-style-name", "Standard");
	librevenge::RVNGString paragName = mpImpl->getParagraphName(finalPropList);

	TagOpenElement *pOpenListItem = new TagOpenElement("text:list-item");
	if (propList["text:start-value"] && propList["text:start-value"]->getInt() > 0)
		pOpenListItem->addAttribute("text:start-value", propList["text:start-value"]->getStr());
	mpImpl->getCurrentStorage()->push_back(pOpenListItem);

	TagOpenElement *pOpenListElementParagraph = new TagOpenElement("text:p");
	pOpenListElementParagraph->addAttribute("text:style-name", paragName);
	mpImpl->getCurrentStorage()->push_back(pOpenListElementParagraph);

	if (mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
		mpImpl->mWriterDocumentStates.top().mbFirstParagraphInPageSpan = false;

	mpImpl->mWriterListStates.top().mbListElementOpened.top() = true;
	mpImpl->mWriterListStates.top().mbListElementParagraphOpened = true;
	mpImpl->mWriterListStates.top().mbListContinueNumbering = false;
}

void OdtGenerator::closeListElement()
{
	// this code is kind of tricky, because we don't actually close the list element (because this list element
	// could contain another list level in OOo's implementation of lists). that is done in the closeListLevel
	// code (or when we open another list element)

	if (mpImpl->mWriterListStates.top().mbListElementParagraphOpened)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:p"));
		mpImpl->mWriterListStates.top().mbListElementParagraphOpened = false;
	}
}

void OdtGenerator::openFootnote(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mWriterListStates.push(WriterListState());
	TagOpenElement *pOpenFootNote = new TagOpenElement("text:note");
	pOpenFootNote->addAttribute("text:note-class", "footnote");
	if (propList["librevenge:number"])
	{
		librevenge::RVNGString tmpString("ftn");
		tmpString.append(propList["librevenge:number"]->getStr());
		pOpenFootNote->addAttribute("text:id", tmpString);
	}
	mpImpl->getCurrentStorage()->push_back(pOpenFootNote);

	TagOpenElement *pOpenFootCitation = new TagOpenElement("text:note-citation");
	if (propList["text:label"])
	{
		librevenge::RVNGString tmpString(propList["text:label"]->getStr(),true);
		pOpenFootCitation->addAttribute("text:label", tmpString);
	}
	mpImpl->getCurrentStorage()->push_back(pOpenFootCitation);

	if (propList["text:label"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["text:label"]->getStr().cstr()));
	else if (propList["librevenge:number"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["librevenge:number"]->getStr().cstr()));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-citation"));

	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("text:note-body"));

	mpImpl->mWriterDocumentStates.top().mbInNote = true;
}

void OdtGenerator::closeFootnote()
{
	mpImpl->mWriterDocumentStates.top().mbInNote = false;
	if (mpImpl->mWriterListStates.size() > 1)
		mpImpl->mWriterListStates.pop();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-body"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note"));
}

void OdtGenerator::openEndnote(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mWriterListStates.push(WriterListState());
	TagOpenElement *pOpenEndNote = new TagOpenElement("text:note");
	pOpenEndNote->addAttribute("text:note-class", "endnote");
	if (propList["librevenge:number"])
	{
		librevenge::RVNGString tmpString("edn");
		tmpString.append(propList["librevenge:number"]->getStr());
		pOpenEndNote->addAttribute("text:id", tmpString);
	}
	mpImpl->getCurrentStorage()->push_back(pOpenEndNote);

	TagOpenElement *pOpenEndCitation = new TagOpenElement("text:note-citation");
	if (propList["text:label"])
	{
		librevenge::RVNGString tmpString(propList["text:label"]->getStr(),true);
		pOpenEndCitation->addAttribute("text:label", tmpString);
	}
	mpImpl->getCurrentStorage()->push_back(pOpenEndCitation);

	if (propList["text:label"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["text:label"]->getStr().cstr()));
	else if (propList["librevenge:number"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["librevenge:number"]->getStr().cstr()));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-citation"));

	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("text:note-body"));

	mpImpl->mWriterDocumentStates.top().mbInNote = true;
}

void OdtGenerator::closeEndnote()
{
	mpImpl->mWriterDocumentStates.top().mbInNote = false;
	if (mpImpl->mWriterListStates.size() > 1)
		mpImpl->mWriterListStates.pop();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-body"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note"));
}

void OdtGenerator::openComment(const librevenge::RVNGPropertyList &)
{
	mpImpl->mWriterListStates.push(WriterListState());
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("office:annotation"));

	mpImpl->mWriterDocumentStates.top().mbInNote = true;
}

void OdtGenerator::closeComment()
{
	mpImpl->mWriterDocumentStates.top().mbInNote = false;
	if (mpImpl->mWriterListStates.size() > 1)
		mpImpl->mWriterListStates.pop();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("office:annotation"));
}

void OdtGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->mWriterDocumentStates.top().mbInNote)
	{
		librevenge::RVNGString sTableName;
		sTableName.sprintf("Table%i", mpImpl->mTableStyles.size());

		// FIXME: we base the table style off of the page's margin left, ignoring (potential) wordperfect margin
		// state which is transmitted inside the page. could this lead to unacceptable behaviour?
		const librevenge::RVNGPropertyListVector *columns = propList.child("librevenge:table-columns");
		TableStyle *pTableStyle = new TableStyle(propList, (columns ? *columns : librevenge::RVNGPropertyListVector()), sTableName.cstr());

		if (mpImpl->mWriterDocumentStates.top().mbFirstElement && mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
		{
			librevenge::RVNGString sMasterPageName("Page_Style_1");
			pTableStyle->setMasterPageName(sMasterPageName);
			mpImpl->mWriterDocumentStates.top().mbFirstElement = false;
		}

		mpImpl->mTableStyles.push_back(pTableStyle);

		mpImpl->mpCurrentTableStyle = pTableStyle;

		TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");

		pTableOpenElement->addAttribute("table:name", sTableName.cstr());
		pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
		mpImpl->getCurrentStorage()->push_back(pTableOpenElement);

		for (int i=0; i<pTableStyle->getNumColumns(); ++i)
		{
			TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
			librevenge::RVNGString sColumnStyleName;
			sColumnStyleName.sprintf("%s.Column%i", sTableName.cstr(), (i+1));
			pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
			mpImpl->getCurrentStorage()->push_back(pTableColumnOpenElement);

			TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
			mpImpl->getCurrentStorage()->push_back(pTableColumnCloseElement);
		}
	}
}

void OdtGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mWriterDocumentStates.top().mbInNote)
		return;
	if (!mpImpl->mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::openTableRow called with no table\n"));
		return;
	}
	if (propList["librevenge:is-header-row"] && (propList["librevenge:is-header-row"]->getInt()))
	{
		mpImpl->getCurrentStorage()->push_back(new TagOpenElement("table:table-header-rows"));
		mpImpl->mWriterDocumentStates.top().mbHeaderRow = true;
	}

	librevenge::RVNGString sTableRowStyleName;
	sTableRowStyleName.sprintf("%s.Row%i", mpImpl->mpCurrentTableStyle->getName().cstr(), mpImpl->mpCurrentTableStyle->getNumTableRowStyles());
	TableRowStyle *pTableRowStyle = new TableRowStyle(propList, sTableRowStyleName.cstr());
	mpImpl->mpCurrentTableStyle->addTableRowStyle(pTableRowStyle);

	TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
	pTableRowOpenElement->addAttribute("table:style-name", sTableRowStyleName);
	mpImpl->getCurrentStorage()->push_back(pTableRowOpenElement);
}

void OdtGenerator::closeTableRow()
{
	if (!mpImpl->mWriterDocumentStates.top().mbInNote && mpImpl->mpCurrentTableStyle)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-row"));
		if (mpImpl->mWriterDocumentStates.top().mbHeaderRow)
		{
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-header-rows"));
			mpImpl->mWriterDocumentStates.top().mbHeaderRow = false;
		}
	}
}

void OdtGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mWriterDocumentStates.top().mbInNote)
		return;
	if (!mpImpl->mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::openTableCell called with no table\n"));
		return;
	}

	librevenge::RVNGString sTableCellStyleName;
	sTableCellStyleName.sprintf("%s.Cell%i", mpImpl->mpCurrentTableStyle->getName().cstr(), mpImpl->mpCurrentTableStyle->getNumTableCellStyles());
	TableCellStyle *pTableCellStyle = new TableCellStyle(propList, sTableCellStyleName.cstr());
	mpImpl->mpCurrentTableStyle->addTableCellStyle(pTableCellStyle);

	TagOpenElement *pTableCellOpenElement = new TagOpenElement("table:table-cell");
	pTableCellOpenElement->addAttribute("table:style-name", sTableCellStyleName);
	if (propList["table:number-columns-spanned"])
		pTableCellOpenElement->addAttribute("table:number-columns-spanned",
		                                    propList["table:number-columns-spanned"]->getStr().cstr());
	if (propList["table:number-rows-spanned"])
		pTableCellOpenElement->addAttribute("table:number-rows-spanned",
		                                    propList["table:number-rows-spanned"]->getStr().cstr());
	// pTableCellOpenElement->addAttribute("table:value-type", "string");
	mpImpl->getCurrentStorage()->push_back(pTableCellOpenElement);

	mpImpl->mWriterDocumentStates.top().mbTableCellOpened = true;
}

void OdtGenerator::closeTableCell()
{
	if (!mpImpl->mWriterDocumentStates.top().mbInNote && mpImpl->mpCurrentTableStyle)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-cell"));
		mpImpl->mWriterDocumentStates.top().mbTableCellOpened = false;
	}
}

void OdtGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &)
{
	if (!mpImpl->mWriterDocumentStates.top().mbInNote && mpImpl->mpCurrentTableStyle)
	{
		mpImpl->getCurrentStorage()->push_back(new TagOpenElement("table:covered-table-cell"));
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:covered-table-cell"));
	}
}

void OdtGenerator::closeTable()
{
	if (!mpImpl->mWriterDocumentStates.top().mbInNote)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table"));
	}
}


void OdtGenerator::insertTab()
{
	mpImpl->insertTab();
}

void OdtGenerator::insertSpace()
{
	mpImpl->insertSpace();
}

void OdtGenerator::insertLineBreak()
{
	mpImpl->insertLineBreak();
}

void OdtGenerator::insertField(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->insertField(propList);
}

void OdtGenerator::insertText(const librevenge::RVNGString &text)
{
	mpImpl->insertText(text);
}

void OdtGenerator::openFrame(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mWriterListStates.push(WriterListState());

	// First, let's create a Frame Style for this box
	TagOpenElement *frameStyleOpenElement = new TagOpenElement("style:style");
	librevenge::RVNGString frameStyleName;
	unsigned objectId = 0;
	if (propList["librevenge:frame-name"])
		objectId= mpImpl->getFrameId(propList["librevenge:frame-name"]->getStr());
	else
		objectId= mpImpl->getFrameId("");
	frameStyleName.sprintf("GraphicFrame_%i", objectId);
	frameStyleOpenElement->addAttribute("style:name", frameStyleName);
	frameStyleOpenElement->addAttribute("style:family", "graphic");

	mpImpl->mFrameStyles.push_back(frameStyleOpenElement);

	TagOpenElement *frameStylePropertiesOpenElement = new TagOpenElement("style:graphic-properties");

	if (propList["text:anchor-type"])
		frameStylePropertiesOpenElement->addAttribute("text:anchor-type", propList["text:anchor-type"]->getStr());
	else
		frameStylePropertiesOpenElement->addAttribute("text:anchor-type","paragraph");

	if (propList["text:anchor-page-number"])
		frameStylePropertiesOpenElement->addAttribute("text:anchor-page-number", propList["text:anchor-page-number"]->getStr());

	if (propList["svg:x"])
		frameStylePropertiesOpenElement->addAttribute("svg:x", propList["svg:x"]->getStr());

	if (propList["svg:y"])
		frameStylePropertiesOpenElement->addAttribute("svg:y", propList["svg:y"]->getStr());

	if (propList["svg:width"])
		frameStylePropertiesOpenElement->addAttribute("svg:width", propList["svg:width"]->getStr());
	else if (propList["fo:min-width"])
		frameStylePropertiesOpenElement->addAttribute("fo:min-width", propList["fo:min-width"]->getStr());

	if (propList["svg:height"])
		frameStylePropertiesOpenElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	else if (propList["fo:min-height"])
		frameStylePropertiesOpenElement->addAttribute("fo:min-height", propList["fo:min-height"]->getStr());

	if (propList["style:rel-width"])
		frameStylePropertiesOpenElement->addAttribute("style:rel-width", propList["style:rel-width"]->getStr());

	if (propList["style:rel-height"])
		frameStylePropertiesOpenElement->addAttribute("style:rel-height", propList["style:rel-height"]->getStr());

	if (propList["fo:max-width"])
		frameStylePropertiesOpenElement->addAttribute("fo:max-width", propList["fo:max-width"]->getStr());

	if (propList["fo:max-height"])
		frameStylePropertiesOpenElement->addAttribute("fo:max-height", propList["fo:max-height"]->getStr());

	if (propList["style:wrap"])
		frameStylePropertiesOpenElement->addAttribute("style:wrap", propList["style:wrap"]->getStr());

	if (propList["style:run-through"])
		frameStylePropertiesOpenElement->addAttribute("style:run-through", propList["style:run-through"]->getStr());

	mpImpl->mFrameStyles.push_back(frameStylePropertiesOpenElement);

	mpImpl->mFrameStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mpImpl->mFrameStyles.push_back(new TagCloseElement("style:style"));

	// Now, let's create an automatic style for this frame
	TagOpenElement *frameAutomaticStyleElement = new TagOpenElement("style:style");
	librevenge::RVNGString frameAutomaticStyleName;
	frameAutomaticStyleName.sprintf("fr%i", objectId);
	frameAutomaticStyleElement->addAttribute("style:name", frameAutomaticStyleName);
	frameAutomaticStyleElement->addAttribute("style:family", "graphic");
	frameAutomaticStyleElement->addAttribute("style:parent-style-name", frameStyleName);

	mpImpl->mFrameAutomaticStyles.push_back(frameAutomaticStyleElement);

	TagOpenElement *frameAutomaticStylePropertiesElement = new TagOpenElement("style:graphic-properties");
	if (propList["style:horizontal-pos"])
		frameAutomaticStylePropertiesElement->addAttribute("style:horizontal-pos", propList["style:horizontal-pos"]->getStr());
	else
		frameAutomaticStylePropertiesElement->addAttribute("style:horizontal-pos", "left");

	if (propList["style:horizontal-rel"])
		frameAutomaticStylePropertiesElement->addAttribute("style:horizontal-rel", propList["style:horizontal-rel"]->getStr());
	else
		frameAutomaticStylePropertiesElement->addAttribute("style:horizontal-rel", "paragraph");

	if (propList["style:vertical-pos"])
		frameAutomaticStylePropertiesElement->addAttribute("style:vertical-pos", propList["style:vertical-pos"]->getStr());
	else
		frameAutomaticStylePropertiesElement->addAttribute("style:vertical-pos", "top");

	if (propList["style:vertical-rel"])
		frameAutomaticStylePropertiesElement->addAttribute("style:vertical-rel", propList["style:vertical-rel"]->getStr());
	else
		frameAutomaticStylePropertiesElement->addAttribute("style:vertical-rel", "page-content");

	if (propList["fo:max-width"])
		frameAutomaticStylePropertiesElement->addAttribute("fo:max-width", propList["fo:max-width"]->getStr());

	if (propList["fo:max-height"])
		frameAutomaticStylePropertiesElement->addAttribute("fo:max-height", propList["fo:max-height"]->getStr());

	// check if the frame has border, shadow, background attributes
	static char const *(bordersString[])=
	{
		"fo:border","fo:border-top","fo:border-left","fo:border-bottom","fo:border-right",
		"style:border-line-width","style:border-line-width-top","style:border-line-width-left",
		"style:border-line-width-bottom","style:border-line-width-right",
		"style:shadow"
	};
	for (int b = 0; b < 11; b++)
	{
		if (propList[bordersString[b]])
			frameAutomaticStylePropertiesElement->addAttribute(bordersString[b], propList[bordersString[b]]->getStr());
	}
	if (propList["fo:background-color"])
		frameAutomaticStylePropertiesElement->addAttribute("fo:background-color", propList["fo:background-color"]->getStr());
	if (propList["style:background-transparency"])
		frameAutomaticStylePropertiesElement->addAttribute("style:background-transparency", propList["style:background-transparency"]->getStr());

	if (propList["fo:clip"])
		frameAutomaticStylePropertiesElement->addAttribute("fo:clip", propList["fo:clip"]->getStr());

	frameAutomaticStylePropertiesElement->addAttribute("draw:ole-draw-aspect", "1");

	mpImpl->mFrameAutomaticStyles.push_back(frameAutomaticStylePropertiesElement);

	mpImpl->mFrameAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mpImpl->mFrameAutomaticStyles.push_back(new TagCloseElement("style:style"));

	// And write the frame itself
	TagOpenElement *drawFrameOpenElement = new TagOpenElement("draw:frame");

	drawFrameOpenElement->addAttribute("draw:style-name", frameAutomaticStyleName);
	librevenge::RVNGString objectName;
	objectName.sprintf("Object%i", objectId);
	drawFrameOpenElement->addAttribute("draw:name", objectName);
	if (propList["text:anchor-type"])
		drawFrameOpenElement->addAttribute("text:anchor-type", propList["text:anchor-type"]->getStr());
	else
		drawFrameOpenElement->addAttribute("text:anchor-type","paragraph");

	if (propList["text:anchor-page-number"])
		drawFrameOpenElement->addAttribute("text:anchor-page-number", propList["text:anchor-page-number"]->getStr());

	if (propList["svg:x"])
		drawFrameOpenElement->addAttribute("svg:x", propList["svg:x"]->getStr());

	if (propList["svg:y"])
		drawFrameOpenElement->addAttribute("svg:y", propList["svg:y"]->getStr());

	if (propList["svg:width"])
		drawFrameOpenElement->addAttribute("svg:width", propList["svg:width"]->getStr());
	else if (propList["fo:min-width"])
		drawFrameOpenElement->addAttribute("fo:min-width", propList["fo:min-width"]->getStr());

	if (propList["svg:height"])
		drawFrameOpenElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	else if (propList["fo:min-height"])
		drawFrameOpenElement->addAttribute("fo:min-height", propList["fo:min-height"]->getStr());

	if (propList["style:rel-width"])
		drawFrameOpenElement->addAttribute("style:rel-width", propList["style:rel-width"]->getStr());

	if (propList["style:rel-height"])
		drawFrameOpenElement->addAttribute("style:rel-height", propList["style:rel-height"]->getStr());

	if (propList["draw:z-index"])
		drawFrameOpenElement->addAttribute("draw:z-index", propList["draw:z-index"]->getStr());

	mpImpl->getCurrentStorage()->push_back(drawFrameOpenElement);

	mpImpl->mWriterDocumentStates.top().mbInFrame = true;
}

void OdtGenerator::closeFrame()
{
	if (mpImpl->mWriterListStates.size() > 1)
		mpImpl->mWriterListStates.pop();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:frame"));

	mpImpl->mWriterDocumentStates.top().mbInFrame = false;
}

void OdtGenerator::insertBinaryObject(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->mWriterDocumentStates.top().mbInFrame) // Embedded objects without a frame simply don't make sense for us
		return;
	mpImpl->insertBinaryObject(propList);
}

void OdtGenerator::openTextBox(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->mWriterDocumentStates.top().mbInFrame) // Text box without a frame simply doesn't make sense for us
		return;
	mpImpl->mWriterListStates.push(WriterListState());
	mpImpl->mWriterDocumentStates.push(WriterDocumentState());
	TagOpenElement *textBoxOpenElement = new TagOpenElement("draw:text-box");
	if (propList["librevenge:next-frame-name"])
	{
		librevenge::RVNGString frameName;
		unsigned id=mpImpl->getFrameId(propList["librevenge:next-frame-name"]->getStr());
		frameName.sprintf("Object%i", id);
		textBoxOpenElement->addAttribute("draw:chain-next-name", frameName);
	}
	mpImpl->getCurrentStorage()->push_back(textBoxOpenElement);
	mpImpl->mWriterDocumentStates.top().mbInTextBox = true;
	mpImpl->mWriterDocumentStates.top().mbFirstElement = false;
}

void OdtGenerator::closeTextBox()
{
	if (!mpImpl->mWriterDocumentStates.top().mbInTextBox)
		return;
	if (mpImpl->mWriterListStates.size() > 1)
		mpImpl->mWriterListStates.pop();
	if (mpImpl->mWriterDocumentStates.size() > 1)
		mpImpl->mWriterDocumentStates.pop();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
}

void OdtGenerator::defineSectionStyle(librevenge::RVNGPropertyList const &)
{
}

void OdtGenerator::insertEquation(librevenge::RVNGPropertyList const &)
{
}

void OdtGenerator::endDocument()
{
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}

void OdtGenerator::startDocument()
{
}

void OdtGenerator::closePageSpan()
{
}

void OdtGenerator::definePageStyle(librevenge::RVNGPropertyList const &)
{
}

void OdtGenerator::defineParagraphStyle(librevenge::RVNGPropertyList const &propList)
{
	mpImpl->defineParagraphStyle(propList);
}

void OdtGenerator::defineCharacterStyle(librevenge::RVNGPropertyList const &propList)
{
	mpImpl->defineCharacterStyle(propList);
}

void OdtGenerator::initStateWith(OdfGenerator const &orig)
{
	mpImpl->initStateWith(orig);
}

void OdtGenerator::registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler)
{
	mpImpl->registerEmbeddedObjectHandler(mimeType, objectHandler);
}

void OdtGenerator::registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler)
{
	mpImpl->registerEmbeddedImageHandler(mimeType, imageHandler);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
