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

#include <map>
#include <stack>
#include <string>

#include <librevenge/librevenge.h>
#include <libodfgen/libodfgen.hxx>

#include "DocumentElement.hxx"
#include "FilterInternal.hxx"
#include "InternalHandler.hxx"

#include "FontStyle.hxx"
#include "ListStyle.hxx"
#include "PageSpan.hxx"
#include "SectionStyle.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"

#include "OdfGenerator.hxx"

class OdtGeneratorPrivate : public OdfGenerator
{
public:
	OdtGeneratorPrivate();
	~OdtGeneratorPrivate();

	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType);

	void initPageManager();

	//
	// state gestion
	//

	// the state we use for writing the final document
	struct State
	{
		State() : mbFirstElement(true),	mbFirstParagraphInPageSpan(false),
			mbInFakeSection(false), mbListElementOpenedAtCurrentLevel(false),
			mbTableCellOpened(false),	mbInNote(false),
			mbInTextBox(false), mbInFrame(false)
		{
		}

		bool mbFirstElement;
		bool mbFirstParagraphInPageSpan;
		bool mbInFakeSection;
		bool mbListElementOpenedAtCurrentLevel;
		bool mbTableCellOpened;
		bool mbInNote;
		bool mbInTextBox;
		bool mbInFrame;
	};

	// returns the actual state
	State &getState()
	{
		if (mStateStack.empty())
		{
			ODFGEN_DEBUG_MSG(("OdtGeneratorPrivate::getState: no state\n"));
			mStateStack.push(State());
		}
		return mStateStack.top();
	}
	// push a state
	void pushState()
	{
		mStateStack.push(State());
	}
	// pop a state
	void popState()
	{
		if (!mStateStack.empty())
			mStateStack.pop();
		else
		{
			ODFGEN_DEBUG_MSG(("OdtGeneratorPrivate::popState: no state\n"));
		}
	}
	std::stack<State> mStateStack;

	// section styles manager
	SectionStyleManager mSectionManager;

	// the current page span
	PageSpan *mpCurrentPageSpan;

private:
	OdtGeneratorPrivate(const OdtGeneratorPrivate &);
	OdtGeneratorPrivate &operator=(const OdtGeneratorPrivate &);

};

OdtGeneratorPrivate::OdtGeneratorPrivate() :
	mStateStack(),
	mSectionManager(),
	mpCurrentPageSpan(0)
{
	initPageManager();
	pushState();
}

OdtGeneratorPrivate::~OdtGeneratorPrivate()
{
	// clean up the mess we made
	ODFGEN_DEBUG_MSG(("OdtGenerator: Cleaning up our mess..\n"));
}

void OdtGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	TagOpenElement("office:automatic-styles").write(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
	{
		mPageSpanManager.writePageStyles(pHandler, Style::Z_StyleAutomatic);
		mSectionManager.write(pHandler, Style::Z_StyleAutomatic);
		mSpanManager.write(pHandler, Style::Z_StyleAutomatic);
		mParagraphManager.write(pHandler, Style::Z_StyleAutomatic);
		mListManager.write(pHandler, Style::Z_StyleAutomatic);
		mGraphicManager.write(pHandler, Style::Z_StyleAutomatic);
		mTableManager.write(pHandler, Style::Z_StyleAutomatic);
	}
	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML))
	{
		mPageSpanManager.writePageStyles(pHandler, Style::Z_ContentAutomatic);
		mSectionManager.write(pHandler, Style::Z_ContentAutomatic);
		mSpanManager.write(pHandler, Style::Z_ContentAutomatic);
		mParagraphManager.write(pHandler, Style::Z_ContentAutomatic);
		mListManager.write(pHandler, Style::Z_ContentAutomatic);
		mGraphicManager.write(pHandler, Style::Z_ContentAutomatic);
		mTableManager.write(pHandler, Style::Z_ContentAutomatic);
	}

	pHandler->endElement("office:automatic-styles");
}

void OdtGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);

	mPageSpanManager.writePageStyles(pHandler, Style::Z_Style);

	// style:default-style

	// graphic
	TagOpenElement defaultGraphicStyleOpenElement("style:default-style");
	defaultGraphicStyleOpenElement.addAttribute("style:family", "graphic");
	defaultGraphicStyleOpenElement.write(pHandler);
	pHandler->endElement("style:default-style");

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
	mSpanManager.write(pHandler, Style::Z_Style);
	mParagraphManager.write(pHandler, Style::Z_Style);
	mListManager.write(pHandler, Style::Z_Style);

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
	mGraphicManager.write(pHandler, Style::Z_Style);
	pHandler->endElement("office:styles");
}

void OdtGeneratorPrivate::initPageManager()
{
	librevenge::RVNGPropertyList page;
	page.insert("fo:margin-bottom", "1in");
	page.insert("fo:margin-left", "1in");
	page.insert("fo:margin-right", "1in");
	page.insert("fo:margin-top", "1in");
	page.insert("fo:page-height", "11in");
	page.insert("fo:page-width", "8.5in");
	page.insert("style:print-orientation", "portrait");

	librevenge::RVNGPropertyList footnoteSep;
	footnoteSep.insert("style:adjustment","left");
	footnoteSep.insert("style:color","#000000");
	footnoteSep.insert("style:rel-width","25%");
	footnoteSep.insert("style:distance-after-sep","0.0398in");
	footnoteSep.insert("style:distance-before-sep","0.0398in");
	footnoteSep.insert("style:width","0.0071in");
	librevenge::RVNGPropertyListVector footnoteVector;
	footnoteVector.append(footnoteSep);
	page.insert("librevenge:footnote", footnoteVector);
	page.insert("librevenge:master-page-name", "Standard");
	mPageSpanManager.add(page);


	footnoteSep.remove("style:distance-after-sep");
	footnoteSep.remove("style:distance-before-sep");
	footnoteSep.remove("style:width");
	footnoteVector.clear();
	footnoteVector.append(footnoteSep);
	page.insert("librevenge:footnote", footnoteVector);
	page.insert("librevenge:master-page-name", "EndNote");
	mPageSpanManager.add(page);
}

bool OdtGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	if (streamType == ODF_MANIFEST_XML)
	{
		pHandler->startDocument();
		TagOpenElement manifestElement("manifest:manifest");
		manifestElement.addAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
		manifestElement.write(pHandler);

		TagOpenElement mainFile("manifest:file-entry");
		mainFile.addAttribute("manifest:media-type", "application/vnd.oasis.opendocument.text");
		mainFile.addAttribute("manifest:full-path", "/");
		mainFile.write(pHandler);
		TagCloseElement("manifest:file-entry").write(pHandler);
		appendFilesInManifest(pHandler);

		TagCloseElement("manifest:manifest").write(pHandler);
		pHandler->endDocument();
		return true;
	}

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
	{
		TagOpenElement("office:font-face-decls").write(pHandler);
		mFontManager.write(pHandler, Style::Z_Font);
		TagCloseElement("office:font-face-decls").write(pHandler);
	}

	ODFGEN_DEBUG_MSG(("OdtGenerator: Document Body: Writing out the styles..\n"));

	// write default styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
		_writeStyles(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML || streamType == ODF_CONTENT_XML)
		_writeAutomaticStyles(pHandler, streamType);

	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
	{
		TagOpenElement("office:master-styles").write(pHandler);
		mPageSpanManager.writeMasterPages(pHandler);
		pHandler->endElement("office:master-styles");
	}

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

librevenge::RVNGStringVector OdtGenerator::getObjectNames() const
{
	if (mpImpl)
		return mpImpl->getObjectNames();
	return librevenge::RVNGStringVector();
}

bool OdtGenerator::getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler)
{
	if (!mpImpl)
		return false;
	return mpImpl->getObjectContent(objectName, pHandler);
}

void OdtGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdtGenerator::defineEmbeddedFont(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineEmbeddedFont(propList);
}

void OdtGenerator::openPageSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mpCurrentPageSpan = mpImpl->getPageSpanManager().add(propList);

	mpImpl->getState().mbFirstParagraphInPageSpan = true;
}

void OdtGenerator::openHeader(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->inHeaderFooter() || !mpImpl->mpCurrentPageSpan)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::openHeader: can not open a header\n"));
		return;
	}
	mpImpl->startHeaderFooter(true, propList);
	if (!mpImpl->inHeaderFooter())
		return;

	libodfgen::DocumentElementVector *pHeaderFooterContentElements = new libodfgen::DocumentElementVector;
	if (propList["librevenge:occurrence"] && (propList["librevenge:occurrence"]->getStr() == "even" ||
	                                          propList["librevenge:occurrence"]->getStr() == "left"))
		mpImpl->mpCurrentPageSpan->setHeaderLeftContent(pHeaderFooterContentElements);
	else if (propList["librevenge:occurrence"] && propList["librevenge:occurrence"]->getStr() == "first")
		mpImpl->mpCurrentPageSpan->setHeaderFirstContent(pHeaderFooterContentElements);
	else if (propList["librevenge:occurrence"] && propList["librevenge:occurrence"]->getStr() == "last")
		mpImpl->mpCurrentPageSpan->setHeaderLastContent(pHeaderFooterContentElements);
	else
		mpImpl->mpCurrentPageSpan->setHeaderContent(pHeaderFooterContentElements);

	mpImpl->pushStorage(pHeaderFooterContentElements);
}

void OdtGenerator::closeHeader()
{
	if (!mpImpl->inHeaderFooter())
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::closeHeader: no header/footer is already opened\n"));
		return;
	}
	mpImpl->endHeaderFooter();
	mpImpl->popStorage();
}

void OdtGenerator::openFooter(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->inHeaderFooter() || !mpImpl->mpCurrentPageSpan)
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::openFooter: can not open a footer\n"));
		return;
	}
	mpImpl->startHeaderFooter(false, propList);
	if (!mpImpl->inHeaderFooter())
		return;

	libodfgen::DocumentElementVector *pHeaderFooterContentElements = new libodfgen::DocumentElementVector;
	if (propList["librevenge:occurrence"] && (propList["librevenge:occurrence"]->getStr() == "even" ||
	                                          propList["librevenge:occurrence"]->getStr() == "left"))
		mpImpl->mpCurrentPageSpan->setFooterLeftContent(pHeaderFooterContentElements);
	else if (propList["librevenge:occurrence"] && propList["librevenge:occurrence"]->getStr() == "first")
		mpImpl->mpCurrentPageSpan->setFooterFirstContent(pHeaderFooterContentElements);
	else if (propList["librevenge:occurrence"] && propList["librevenge:occurrence"]->getStr() == "last")
		mpImpl->mpCurrentPageSpan->setFooterLastContent(pHeaderFooterContentElements);
	else
		mpImpl->mpCurrentPageSpan->setFooterContent(pHeaderFooterContentElements);

	mpImpl->pushStorage(pHeaderFooterContentElements);
}

void OdtGenerator::closeFooter()
{
	if (!mpImpl->inHeaderFooter())
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::closeFooter: no header/footer is already opened\n"));
		return;
	}
	mpImpl->endHeaderFooter();
	mpImpl->popStorage();
}

void OdtGenerator::openSection(const librevenge::RVNGPropertyList &propList)
{
	double fSectionMarginLeft = 0.0;
	double fSectionMarginRight = 0.0;
	if (propList["fo:margin-left"])
		fSectionMarginLeft = propList["fo:margin-left"]->getDouble();
	if (propList["fo:margin-right"])
		fSectionMarginRight = propList["fo:margin-right"]->getDouble();

	const librevenge::RVNGPropertyListVector *columns = propList.child("style:columns");
	double const eps=1e-4;
	if ((columns && columns->count() > 1) ||
	        (fSectionMarginLeft<-eps || fSectionMarginLeft>eps) ||
	        (fSectionMarginRight<-eps || fSectionMarginRight>eps))
	{
		librevenge::RVNGString sSectionName=
		    mpImpl->mSectionManager.add(propList, mpImpl->useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_Unknown);
		TagOpenElement *pSectionOpenElement = new TagOpenElement("text:section");
		pSectionOpenElement->addAttribute("text:style-name", sSectionName);
		pSectionOpenElement->addAttribute("text:name", sSectionName);
		mpImpl->getCurrentStorage()->push_back(pSectionOpenElement);
	}
	else
		mpImpl->getState().mbInFakeSection = true;
}

void OdtGenerator::closeSection()
{
	if (!mpImpl->getState().mbInFakeSection)
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:section"));
	else
		mpImpl->getState().mbInFakeSection = false;
}

void OdtGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	// FIXMENOW: What happens if we open a footnote inside a table? do we then inherit the footnote's style
	// from "Table Contents"

	librevenge::RVNGPropertyList finalPropList(propList);
	if (mpImpl->getState().mbFirstParagraphInPageSpan && mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
	{
		if (!mpImpl->mpCurrentPageSpan)
		{
			ODFGEN_DEBUG_MSG(("OdtGenerator::openParagraph: can not find the current page\n"));
		}
		else
		{
			finalPropList.insert("style:master-page-name", mpImpl->mpCurrentPageSpan->getMasterName());
			mpImpl->getState().mbFirstElement = false;
			mpImpl->getState().mbFirstParagraphInPageSpan = false;
		}
	}

	if (mpImpl->getState().mbTableCellOpened)
	{
		bool inHeader=false;
		if (mpImpl->isInTableRow(inHeader) && inHeader)
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

void OdtGenerator::openLink(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openLink(propList);
}

void OdtGenerator::closeLink()
{
	mpImpl->closeLink();
}

// -------------------------------
//      list
// -------------------------------
void OdtGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListLevel(propList, true);
}

void OdtGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListLevel(propList, false);
}

void OdtGenerator::closeOrderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdtGenerator::closeUnorderedListLevel()
{
	mpImpl->closeListLevel();
}

void OdtGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openListElement(propList);

	if (mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
		mpImpl->getState().mbFirstParagraphInPageSpan = false;
}

void OdtGenerator::closeListElement()
{
	mpImpl->closeListElement();
}

void OdtGenerator::openFootnote(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->pushListState();
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
		librevenge::RVNGString tmpString;
		tmpString.appendEscapedXML(propList["text:label"]->getStr());
		pOpenFootCitation->addAttribute("text:label", tmpString);
	}
	mpImpl->getCurrentStorage()->push_back(pOpenFootCitation);

	if (propList["text:label"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["text:label"]->getStr().cstr()));
	else if (propList["librevenge:number"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["librevenge:number"]->getStr().cstr()));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-citation"));

	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("text:note-body"));

	mpImpl->getState().mbInNote = true;
}

void OdtGenerator::closeFootnote()
{
	mpImpl->getState().mbInNote = false;
	mpImpl->popListState();

	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-body"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note"));
}

void OdtGenerator::openEndnote(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->pushListState();
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
		librevenge::RVNGString tmpString;
		tmpString.appendEscapedXML(propList["text:label"]->getStr());
		pOpenEndCitation->addAttribute("text:label", tmpString);
	}
	mpImpl->getCurrentStorage()->push_back(pOpenEndCitation);

	if (propList["text:label"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["text:label"]->getStr().cstr()));
	else if (propList["librevenge:number"])
		mpImpl->getCurrentStorage()->push_back(new CharDataElement(propList["librevenge:number"]->getStr().cstr()));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-citation"));

	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("text:note-body"));

	mpImpl->getState().mbInNote = true;
}

void OdtGenerator::closeEndnote()
{
	mpImpl->getState().mbInNote = false;
	mpImpl->popListState();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note-body"));
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("text:note"));
}

void OdtGenerator::openComment(const librevenge::RVNGPropertyList &)
{
	mpImpl->pushListState();
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("office:annotation"));

	mpImpl->getState().mbInNote = true;
}

void OdtGenerator::closeComment()
{
	mpImpl->getState().mbInNote = false;
	mpImpl->popListState();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("office:annotation"));
}

void OdtGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->getState().mbInNote)
		return;

	librevenge::RVNGPropertyList pList(propList);
	if (mpImpl->getState().mbFirstElement && mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
	{
		if (!mpImpl->mpCurrentPageSpan)
		{
			ODFGEN_DEBUG_MSG(("OdtGenerator::openTable: can not find the current page\n"));
		}
		else
		{
			pList.insert("style:master-page-name", mpImpl->mpCurrentPageSpan->getMasterName());
			mpImpl->getState().mbFirstElement = false;
		}
	}
	mpImpl->openTable(pList);
}

void OdtGenerator::closeTable()
{
	if (mpImpl->getState().mbInNote)
		return;
	mpImpl->closeTable();
}

void OdtGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->getState().mbInNote)
		return;
	mpImpl->openTableRow(propList);
}

void OdtGenerator::closeTableRow()
{
	if (mpImpl->getState().mbInNote)
		return;
	mpImpl->closeTableRow();
}

void OdtGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->getState().mbInNote)
		return;

	mpImpl->getState().mbTableCellOpened = mpImpl->openTableCell(propList);
}

void OdtGenerator::closeTableCell()
{
	if (mpImpl->getState().mbInNote)
		return;
	mpImpl->closeTableCell();
	mpImpl->getState().mbTableCellOpened = false;
}

void OdtGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->getState().mbInNote)
		return;
	mpImpl->insertCoveredTableCell(propList);
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
	mpImpl->pushListState();
	librevenge::RVNGPropertyList pList(propList);
	if (!propList["text:anchor-type"])
		pList.insert("text:anchor-type","paragraph");
	mpImpl->openFrame(pList);
	mpImpl->getState().mbInFrame = true;
}

void OdtGenerator::closeFrame()
{
	mpImpl->popListState();
	mpImpl->closeFrame();
	mpImpl->getState().mbInFrame = false;
}

void OdtGenerator::insertBinaryObject(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInFrame) // Embedded objects without a frame simply don't make sense for us
		return;
	mpImpl->insertBinaryObject(propList);
}

void OdtGenerator::openGroup(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->openGroup(propList);
}

void OdtGenerator::closeGroup()
{
	mpImpl->closeGroup();
}

void OdtGenerator::defineGraphicStyle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineGraphicStyle(propList);
}

void OdtGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawRectangle(propList);
}

void OdtGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawEllipse(propList);
}


void OdtGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPolySomething(propList, true);
}


void OdtGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawPolySomething(propList, false);
}


void OdtGenerator::drawPath(const ::librevenge::RVNGPropertyList &propList)
{
	/** CHECKME: actually, the drawing is often bad, ie. some
	 unexpected scaling be applied, can we automatically replace the
	 shape by an embedded objet ? */
	mpImpl->drawPath(propList);
}

void OdtGenerator::drawConnector(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawConnector(propList);
}

void OdtGenerator::openTextBox(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInFrame) // Text box without a frame simply doesn't make sense for us
		return;
	mpImpl->pushListState();
	mpImpl->pushState();
	TagOpenElement *textBoxOpenElement = new TagOpenElement("draw:text-box");
	if (propList["librevenge:next-frame-name"])
	{
		librevenge::RVNGString frameName;
		unsigned id=mpImpl->getFrameId(propList["librevenge:next-frame-name"]->getStr());
		frameName.sprintf("Object%i", id);
		textBoxOpenElement->addAttribute("draw:chain-next-name", frameName);
	}
	mpImpl->getCurrentStorage()->push_back(textBoxOpenElement);
	mpImpl->getState().mbInTextBox = true;
	mpImpl->getState().mbFirstElement = false;
}

void OdtGenerator::closeTextBox()
{
	if (!mpImpl->getState().mbInTextBox)
		return;
	mpImpl->popListState();
	mpImpl->popState();

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

void OdtGenerator::startDocument(const librevenge::RVNGPropertyList &)
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
