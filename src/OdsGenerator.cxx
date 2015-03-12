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

#include <map>
#include <stack>
#include <sstream>
#include <string>

#include <libodfgen/libodfgen.hxx>

#include "DocumentElement.hxx"
#include "FilterInternal.hxx"

#include "InternalHandler.hxx"
#include "FontStyle.hxx"
#include "ListStyle.hxx"
#include "PageSpan.hxx"
#include "SheetStyle.hxx"
#include "TableStyle.hxx"

#include "OdcGenerator.hxx"
#include "OdfGenerator.hxx"

class OdsGeneratorPrivate : public OdfGenerator
{
public:
	enum Command { C_Document=0, C_PageSpan, C_Header, C_Footer, C_Sheet, C_SheetRow, C_SheetCell,
	               C_Chart, C_ChartDataLabel, C_ChartPlotArea, C_ChartSerie, C_ChartTextObject,
	               C_Span, C_Paragraph, C_Section, C_OrderedList, C_UnorderedList, C_ListElement,
	               C_Footnote, C_Comment, C_TextBox, C_Frame, C_Table, C_TableRow, C_TableCell,
	               C_Group
	             };
	// the state we use for writing the final document
	struct State
	{
		State() : mbStarted(false),
			mbInSheet(false), mbInSheetShapes(false), mbInSheetRow(false), mbFirstInSheetRow(false), mbInSheetCell(false), miLastSheetRow(0), miLastSheetColumn(0),
			mbInFootnote(false), mbInComment(false), mbInHeaderFooter(false), mbInFrame(false), mbFirstInFrame(false), mbInChart(false),
			mbInGroup(false), mbInTable(false), mbInTextBox(false),
			mbNewOdcGenerator(false), mbNewOdtGenerator(false)
		{
		}
		bool canOpenFrame() const
		{
			return mbStarted && mbInSheet && !mbInSheetCell && !mbInFootnote && !mbInComment
			       && !mbInHeaderFooter && !mbInFrame && !mbInChart;
		}
		bool mbStarted;

		bool mbInSheet;
		bool mbInSheetShapes;
		bool mbInSheetRow;
		bool mbFirstInSheetRow;
		bool mbInSheetCell;
		int miLastSheetRow;
		int miLastSheetColumn;
		bool mbInFootnote;
		bool mbInComment;
		bool mbInHeaderFooter;
		bool mbInFrame;
		bool mbFirstInFrame;
		bool mbInChart;
		bool mbInGroup;
		bool mbInTable;
		bool mbInTextBox;

		bool mbNewOdcGenerator;
		bool mbNewOdtGenerator;
	};
	// the odc state
	struct OdcGeneratorState
	{
		OdcGeneratorState(librevenge::RVNGString const &dir) : mDir(dir), mContentElements(), mInternalHandler(&mContentElements), mGenerator()
		{
			if (!mDir.empty()) return;
			mGenerator.addDocumentHandler(&mInternalHandler,ODF_FLAT_XML);
		}
		~OdcGeneratorState()
		{
		}
		OdcGenerator &get()
		{
			return mGenerator;
		}
		librevenge::RVNGString mDir;
		libodfgen::DocumentElementVector mContentElements;
		InternalHandler mInternalHandler;
		OdcGenerator mGenerator;
	};

	// the odt state
	struct OdtGeneratorState
	{
		OdtGeneratorState() : mContentElements(), mInternalHandler(&mContentElements), mGenerator()
		{
			mGenerator.addDocumentHandler(&mInternalHandler,ODF_FLAT_XML);
		}
		~OdtGeneratorState()
		{
		}
		OdtGenerator &get()
		{
			return mGenerator;
		}
		libodfgen::DocumentElementVector mContentElements;
		InternalHandler mInternalHandler;
		OdtGenerator mGenerator;
	};

	OdsGeneratorPrivate();
	~OdsGeneratorPrivate();
	void addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
	{
		if (!pHandler)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::addDocumentHandler: called without handler\n"));
			return;
		}
		mDocumentStreamHandlers[streamType] = pHandler;
	}
	//
	// command gestion
	//

	void open(Command const command)
	{
		mCommandStack.push(command);
	}
	bool close(Command command);

	//
	// state gestion
	//

	// returns the actual state
	State &getState()
	{
		if (mStateStack.empty())
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::getState: no state\n"));
			mStateStack.push(State());
		}
		return mStateStack.top();
	}
	// push a state
	void pushState(State const &state)
	{
		mStateStack.push(state);
	}
	// pop a state
	void popState()
	{
		if (!mStateStack.empty())
			mStateStack.pop();
		else
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::popState: no state\n"));
		}
	}
	// check if we are in a sheetcell or in a comment
	bool canWriteText() const
	{
		if (mStateStack.empty() || mStateStack.top().mbInFootnote)  return false;
		return mStateStack.top().mbInComment || mStateStack.top().mbInSheetCell ||
		       mStateStack.top().mbInHeaderFooter || mStateStack.top().mbInTextBox;
	}
	bool canAddNewShape(bool add=true)
	{
		if (mStateStack.empty())
		{
			if (add)
			{
				ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::canAddNewShape: can not find the state!!!\n"));
			}
			return false;
		}
		State &state=mStateStack.top();
		if (!state.mbStarted || !state.mbInSheet  || state.mbInChart || state.mbInComment || state.mbInSheetRow)
		{
			if (add)
			{
				ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::canAddNewShape: call outside a sheet shapes zone!!!\n"));
			}
			return false;
		}
		if (add && !state.mbInSheetShapes)
		{
			getCurrentStorage()->push_back(new TagOpenElement("table:shapes"));
			state.mbInSheetShapes=true;
		}
		return true;
	}

	//
	// auxilliar generator
	//
	bool createAuxiliarOdcGenerator()
	{
		if (mAuxiliarOdcState)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::createAuxiliarOdcGenerator: already created\n"));
			return false;
		}
		bool isFlat = mDocumentStreamHandlers.find(ODF_FLAT_XML)!=mDocumentStreamHandlers.end();
		librevenge::RVNGString dir("");
		if (!isFlat)
			dir.sprintf("Object %i/", miObjectNumber++);
		mAuxiliarOdcState.reset(new OdcGeneratorState(dir));
		if (!isFlat)
		{
			createObjectFile(dir, "application/vnd.oasis.opendocument.chart", true);
			librevenge::RVNGString file(dir);
			file.append("content.xml");
			mAuxiliarOdcState->mGenerator.addDocumentHandler
			(&createObjectFile(file, "text/xml").mInternalHandler, ODF_CONTENT_XML);
			file=dir;
			file.append("meta.xml");
			mAuxiliarOdcState->mGenerator.addDocumentHandler
			(&createObjectFile(file, "text/xml").mInternalHandler, ODF_META_XML);
			file=dir;
			file.append("styles.xml");
			mAuxiliarOdcState->mGenerator.addDocumentHandler
			(&createObjectFile(file, "text/xml").mInternalHandler, ODF_STYLES_XML);
		}
		mAuxiliarOdcState->mGenerator.initStateWith(*this);
		mAuxiliarOdcState->mGenerator.startDocument(librevenge::RVNGPropertyList());

		return true;
	}
	bool sendAuxiliarOdcGenerator()
	{
		if (!mAuxiliarOdcState)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::sendAuxiliarOdcGenerator: data seems bad\n"));
			return false;
		}
		mAuxiliarOdcState->mGenerator.endDocument();
		if (mAuxiliarOdcState->mDir.empty() && mAuxiliarOdcState->mContentElements.empty())
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::sendAuxiliarOdcGenerator: data seems bad\n"));
			return false;
		}
		TagOpenElement *object=new TagOpenElement("draw:object");
		if (!mAuxiliarOdcState->mDir.empty())
		{
			object->addAttribute("xlink:href",mAuxiliarOdcState->mDir.cstr());
			object->addAttribute("xlink:type","simple");
			object->addAttribute("xlink:show","embed");
			object->addAttribute("xlink:actuate","onLoad");
		}
		getCurrentStorage()->push_back(object);
		mAuxiliarOdcState->mContentElements.appendTo(*getCurrentStorage());
		mAuxiliarOdcState->mContentElements.resize(0);
		getCurrentStorage()->push_back(new TagCloseElement("draw:object"));
		return true;
	}
	void resetAuxiliarOdcGenerator()
	{
		mAuxiliarOdcState.reset();
	}
	bool checkOutsideOdc(char const *function) const
	{
		if (!mAuxiliarOdcState) return true;
		if (!function) return false;
		ODFGEN_DEBUG_MSG(("OdsGenerator::%s: call in chart\n", function));
		return false;
	}
	bool createAuxiliarOdtGenerator()
	{
		if (mAuxiliarOdtState)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::createAuxiliarOdtGenerator: already created\n"));
			return false;
		}
		mAuxiliarOdtState.reset(new OdtGeneratorState);
		mAuxiliarOdtState->mGenerator.initStateWith(*this);
		mAuxiliarOdtState->mGenerator.startDocument(librevenge::RVNGPropertyList());
		librevenge::RVNGPropertyList page;
		page.insert("librevenge:num-pages", 1);
		page.insert("fo:margin-left", 0.0, librevenge::RVNG_INCH);
		page.insert("fo:margin-right", 0.0, librevenge::RVNG_INCH);
		page.insert("fo:margin-top", 0.0, librevenge::RVNG_INCH);
		page.insert("fo:margin-bottom", 0.0, librevenge::RVNG_INCH);
		mAuxiliarOdtState->mGenerator.openPageSpan(page);

		return true;
	}
	bool sendAuxiliarOdtGenerator()
	{
		if (!mAuxiliarOdtState)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::sendAuxiliarOdtGenerator: data seems bad\n"));
			return false;
		}
		mAuxiliarOdtState->mGenerator.closePageSpan();
		mAuxiliarOdtState->mGenerator.endDocument();
		if (mAuxiliarOdtState->mContentElements.empty())
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::sendAuxiliarOdtGenerator: data seems bad\n"));
			return false;
		}
		getCurrentStorage()->push_back(new TagOpenElement("draw:object"));
		mAuxiliarOdtState->mContentElements.appendTo(*getCurrentStorage());
		mAuxiliarOdtState->mContentElements.resize(0);
		getCurrentStorage()->push_back(new TagCloseElement("draw:object"));
		return true;
	}
	void resetAuxiliarOdtGenerator()
	{
		mAuxiliarOdtState.reset();
	}
	bool checkOutsideOdt(char const *function) const
	{
		if (!mAuxiliarOdtState) return true;
		if (!function) return false;
		ODFGEN_DEBUG_MSG(("OdsGenerator::%s: call in text auxilliar\n", function));
		return false;
	}

	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void initPageManager();

	std::stack<Command> mCommandStack;
	std::stack<State> mStateStack;

	// auxiliar odc handler to create data
	shared_ptr<OdcGeneratorState> mAuxiliarOdcState;
	// auxiliar odt handler to create data
	shared_ptr<OdtGeneratorState> mAuxiliarOdtState;

	// table styles
	SheetManager  mSheetManager;

	// page manager
	PageSpan *mpCurrentPageSpan;

	//
private:
	OdsGeneratorPrivate(const OdsGeneratorPrivate &);
	OdsGeneratorPrivate &operator=(const OdsGeneratorPrivate &);

};

OdsGeneratorPrivate::OdsGeneratorPrivate() : OdfGenerator(),
	mCommandStack(),
	mStateStack(),
	mAuxiliarOdcState(), mAuxiliarOdtState(),
	mSheetManager(),
	mpCurrentPageSpan(0)
{
	mStateStack.push(State());
	initPageManager();
}

OdsGeneratorPrivate::~OdsGeneratorPrivate()
{
	// clean up the mess we made
	ODFGEN_DEBUG_MSG(("OdsGenerator: Cleaning up our mess..\n"));

	ODFGEN_DEBUG_MSG(("OdsGenerator: Destroying the body elements\n"));
	mSheetManager.clean();
}

bool OdsGeneratorPrivate::close(Command command)
{
	if (mCommandStack.empty() || mCommandStack.top()!=command)
	{
#ifdef DEBUG
		static char const *(wh[]) =
		{
			"Document", "PageSpan", "Header", "Footer", "Sheet", "SheetRow", "SheetCell",
			"Chart", "ChartDataLabel", "ChartPlotArea", "ChartSerie", "ChartTextObject",
			"Span", "Paragraph", "Section", "OrderedListLevel", "UnorderedListLevel", "ListElement",
			"Footnote", "Comment", "TextBox", "Frame", "Table", "TableRow", "TableCell",
			"Group"
		};
		ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::close: unexpected %s\n", wh[command]));
#endif
		return false;
	}
	mCommandStack.pop();
	return true;
}

void OdsGeneratorPrivate::initPageManager()
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

OdsGenerator::OdsGenerator() : mpImpl(new OdsGeneratorPrivate())
{
}

OdsGenerator::~OdsGenerator()
{
	if (mpImpl)
		delete mpImpl;
}

void OdsGenerator::addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
{
	if (mpImpl)
		mpImpl->addDocumentHandler(pHandler, streamType);
}

librevenge::RVNGStringVector OdsGenerator::getObjectNames() const
{
	if (mpImpl)
		return mpImpl->getObjectNames();
	return librevenge::RVNGStringVector();
}

bool OdsGenerator::getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler)
{
	if (!mpImpl)
		return false;
	return mpImpl->getObjectContent(objectName, pHandler);
}

void OdsGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	TagOpenElement("office:automatic-styles").write(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
	{
		mPageSpanManager.writePageStyles(pHandler, Style::Z_StyleAutomatic);
		mSpanManager.write(pHandler, Style::Z_StyleAutomatic);
		mParagraphManager.write(pHandler, Style::Z_StyleAutomatic);
		mListManager.write(pHandler, Style::Z_StyleAutomatic);
		mGraphicManager.write(pHandler, Style::Z_StyleAutomatic);
		mSheetManager.write(pHandler, Style::Z_StyleAutomatic);
	}

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML))
	{
		mPageSpanManager.writePageStyles(pHandler, Style::Z_ContentAutomatic);
		mSpanManager.write(pHandler, Style::Z_ContentAutomatic);
		mParagraphManager.write(pHandler, Style::Z_ContentAutomatic);
		mListManager.write(pHandler, Style::Z_ContentAutomatic);
		mGraphicManager.write(pHandler, Style::Z_ContentAutomatic);
		mSheetManager.write(pHandler, Style::Z_ContentAutomatic);
	}

	pHandler->endElement("office:automatic-styles");
}

void OdsGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);

	mPageSpanManager.writePageStyles(pHandler, Style::Z_Style);

	// style:default-style
	// paragraph
	TagOpenElement defaultParagraphStyleOpenElement("style:default-style");
	defaultParagraphStyleOpenElement.addAttribute("style:family", "paragraph");
	defaultParagraphStyleOpenElement.write(pHandler);
	TagOpenElement defaultParagraphStylePropertiesOpenElement("style:paragraph-properties");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:tab-stop-distance", "0.5in");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:text-autospace", "ideograph-alpha");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:punctuation-wrap", "hanging");
	defaultParagraphStylePropertiesOpenElement.addAttribute("style:writing-mode", "page");
	defaultParagraphStylePropertiesOpenElement.write(pHandler);
	pHandler->endElement("style:paragraph-properties");
	pHandler->endElement("style:default-style");

	// graphic
	TagOpenElement defaultGraphicStyleOpenElement("style:default-style");
	defaultGraphicStyleOpenElement.addAttribute("style:family", "graphic");
	defaultGraphicStyleOpenElement.write(pHandler);
	TagOpenElement defaultGraphicStylePropertiesOpenElement("style:graphic-properties");
	defaultGraphicStylePropertiesOpenElement.addAttribute("draw:fill","solid");
	defaultGraphicStylePropertiesOpenElement.addAttribute("draw:fill-color","#ffffff");
	defaultGraphicStylePropertiesOpenElement.addAttribute("draw:stroke","none");
	defaultGraphicStylePropertiesOpenElement.addAttribute("draw:shadow","hidden");
	defaultGraphicStylePropertiesOpenElement.write(pHandler);
	pHandler->endElement("style:graphic-properties");
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

	// paragraph Standard
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
	mGraphicManager.write(pHandler, Style::Z_Style);
	pHandler->endElement("office:styles");
}

bool OdsGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	if (streamType == ODF_MANIFEST_XML)
	{
		pHandler->startDocument();
		TagOpenElement manifestElement("manifest:manifest");
		manifestElement.addAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
		manifestElement.addAttribute("manifest:version", "1.2", true);
		manifestElement.write(pHandler);

		TagOpenElement mainFile("manifest:file-entry");
		mainFile.addAttribute("manifest:media-type", "application/vnd.oasis.opendocument.spreadsheet");
		mainFile.addAttribute("manifest:full-path", "/");
		mainFile.write(pHandler);
		TagCloseElement("manifest:file-entry").write(pHandler);
		appendFilesInManifest(pHandler);

		TagCloseElement("manifest:manifest").write(pHandler);
		pHandler->endDocument();
		return true;
	}

	pHandler->startDocument();

	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: preamble\n"));
	}

	std::string const documentType=OdfGenerator::getDocumentType(streamType);
	librevenge::RVNGPropertyList docContentPropList;
	docContentPropList.insert("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	docContentPropList.insert("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	docContentPropList.insert("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	docContentPropList.insert("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	docContentPropList.insert("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	docContentPropList.insert("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	docContentPropList.insert("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	docContentPropList.insert("xmlns:of", "urn:oasis:names:tc:opendocument:xmlns:of:1.2");

	docContentPropList.insert("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	docContentPropList.insert("xmlns:xlink", "http://www.w3.org/1999/xlink");
	docContentPropList.insert("xmlns:number", "urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0");
	docContentPropList.insert("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	docContentPropList.insert("xmlns:chart", "urn:oasis:names:tc:opendocument:xmlns:chart:1.0");
	docContentPropList.insert("xmlns:dr3d", "urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0");
	docContentPropList.insert("xmlns:math", "http://www.w3.org/1998/Math/MathML");
	docContentPropList.insert("xmlns:form", "urn:oasis:names:tc:opendocument:xmlns:form:1.0");
	docContentPropList.insert("xmlns:script", "urn:oasis:names:tc:opendocument:xmlns:script:1.0");
	docContentPropList.insert("xmlns:tableooo", "http://openoffice.org/2009/table");
	docContentPropList.insert("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	docContentPropList.insert("xmlns:calcext","urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0");
	docContentPropList.insert("office:version", librevenge::RVNGPropertyFactory::newStringProp("1.2"));
	if (streamType == ODF_FLAT_XML)
		docContentPropList.insert("office:mimetype", "application/vnd.oasis.opendocument.spreadsheet");
	pHandler->startElement(documentType.c_str(), docContentPropList);

	if (streamType == ODF_FLAT_XML || streamType == ODF_META_XML)
		writeDocumentMetaData(pHandler);

	// write out the font styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML || streamType == ODF_CONTENT_XML)
	{
		TagOpenElement("office:font-face-decls").write(pHandler);
		mFontManager.write(pHandler, Style::Z_Font);
		TagCloseElement("office:font-face-decls").write(pHandler);
	}

	// write default styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: Writing out the styles..\n"));
		_writeStyles(pHandler);
	}
	// writing automatic style
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML || streamType == ODF_CONTENT_XML)
		_writeAutomaticStyles(pHandler, streamType);

	// writing out the page masters
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
	{
		TagOpenElement("office:master-styles").write(pHandler);
		mPageSpanManager.writeMasterPages(pHandler);
		pHandler->endElement("office:master-styles");
	}
	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: Writing out the document..\n"));
		// writing out the document
		TagOpenElement("office:body").write(pHandler);
		TagOpenElement("office:spreadsheet").write(pHandler);
		sendStorage(&mBodyStorage, pHandler);
		pHandler->endElement("office:spreadsheet");
		pHandler->endElement("office:body");
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: Finished writing all doc els..\n"));
	}
	pHandler->endElement(documentType.c_str());

	pHandler->endDocument();

	return true;
}


void OdsGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdsGenerator::defineEmbeddedFont(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineEmbeddedFont(propList);
}

void OdsGenerator::openPageSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_PageSpan);
	if (!mpImpl->checkOutsideOdc("openPageSpan") || !mpImpl->checkOutsideOdt("openPageSpan"))
		return;
	mpImpl->mpCurrentPageSpan = mpImpl->getPageSpanManager().add(propList);
}

void OdsGenerator::closePageSpan()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_PageSpan)) return;
}

void OdsGenerator::defineSheetNumberingStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mSheetManager.addNumberingStyle(propList);
}

void OdsGenerator::openSheet(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Sheet);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInSheet=false;
	mpImpl->pushState(state);
	if (!mpImpl->checkOutsideOdc("openSheet") || !mpImpl->checkOutsideOdt("openSheet"))
		return;
	if (state.mbInSheet || state.mbInFrame || state.mbInFootnote || state.mbInComment || state.mbInHeaderFooter ||
	        mpImpl->mSheetManager.isSheetOpened())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheet can not be called!!!\n"));
		return;
	}

	librevenge::RVNGPropertyList finalPropList(propList);
	if (mpImpl->getCurrentStorage() == &mpImpl->getBodyStorage())
	{
		if (mpImpl->mpCurrentPageSpan)
			finalPropList.insert("style:master-page-name", mpImpl->mpCurrentPageSpan->getMasterName());
		else
		{
			ODFGEN_DEBUG_MSG(("OdsGenerator::openSheet: can not find the current page\n"));
		}
	}

	if (!mpImpl->mSheetManager.openSheet(finalPropList, Style::Z_ContentAutomatic)) return;
	mpImpl->getState().mbInSheet=true;

	SheetStyle *style=mpImpl->mSheetManager.actualSheet();
	if (!style) return;
	librevenge::RVNGString sTableName(style->getName());
	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");
	if (propList["librevenge:sheet-name"])
		pTableOpenElement->addAttribute("table:name", propList["librevenge:sheet-name"]->getStr());
	else
		pTableOpenElement->addAttribute("table:name", sTableName.cstr());
	pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
	mpImpl->getCurrentStorage()->push_back(pTableOpenElement);

	/* TODO: open a table:shapes element
	   a environment to store the table content which must be merged in closeSheet
	*/
	for (int i=0; i< style->getNumColumns(); ++i)
	{
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		librevenge::RVNGString sColumnStyleName;
		sColumnStyleName.sprintf("%s_col%i", sTableName.cstr(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
		mpImpl->getCurrentStorage()->push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpImpl->getCurrentStorage()->push_back(pTableColumnCloseElement);
	}
}

void OdsGenerator::closeSheet()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Sheet))
		return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState || !state.mbInSheet) return;
	if (state.mbInSheetShapes)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:shapes"));
		mpImpl->getState().mbInSheetShapes=false;
	}
	mpImpl->mSheetManager.closeSheet();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table"));
}

void OdsGenerator::openSheetRow(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_SheetRow);
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	SheetStyle *style=mpImpl->mSheetManager.actualSheet();

	if (!state.mbInSheet || state.mbInComment || !style)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheetRow can not be called!!!\n"));
		return;
	}
	if (state.mbInSheetShapes)
	{
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:shapes"));
		mpImpl->getState().mbInSheetShapes=false;
	}

	// check if we need to add some empty row
	int row = propList["librevenge:row"] ? propList["librevenge:row"]->getInt() : -1;
	if (row > state.miLastSheetRow)
	{
		librevenge::RVNGString sEmptyRowStyleName=style->addRow(librevenge::RVNGPropertyList());

		TagOpenElement *pEmptyRowOpenElement = new TagOpenElement("table:table-row");
		pEmptyRowOpenElement->addAttribute("table:style-name", sEmptyRowStyleName);
		librevenge::RVNGString numEmpty;
		numEmpty.sprintf("%d", row-state.miLastSheetRow);
		pEmptyRowOpenElement->addAttribute("table:number-rows-repeated", numEmpty);

		mpImpl->getCurrentStorage()->push_back(pEmptyRowOpenElement);
		mpImpl->getCurrentStorage()->push_back(new TagOpenElement("table:table-cell"));
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-cell"));
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-row"));
	}
	else
		row=state.miLastSheetRow;
	mpImpl->getState().miLastSheetRow=row+1;

	state.miLastSheetColumn=0;
	state.mbInSheetRow=state.mbFirstInSheetRow=true;
	mpImpl->pushState(state);

	librevenge::RVNGString sSheetRowStyleName=style->addRow(propList);
	TagOpenElement *pSheetRowOpenElement = new TagOpenElement("table:table-row");
	pSheetRowOpenElement->addAttribute("table:style-name", sSheetRowStyleName);
	mpImpl->getCurrentStorage()->push_back(pSheetRowOpenElement);
}

void OdsGenerator::closeSheetRow()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_SheetRow) || mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	if (!state.mbInSheetRow) return;
	if (state.mbFirstInSheetRow)
	{
		TagOpenElement *pSheetCellOpenElement = new TagOpenElement("table:table-cell");
		pSheetCellOpenElement->addAttribute("table:number-columns-repeated","1");
		mpImpl->getCurrentStorage()->push_back(pSheetCellOpenElement);
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-cell"));
	}
	mpImpl->popState();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-row"));
}

void OdsGenerator::openSheetCell(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_SheetCell);
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	SheetStyle *style=mpImpl->mSheetManager.actualSheet();

	if (!state.mbInSheetRow || state.mbInComment || !style)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheetCell can not be called!!!\n"));
		return;
	}
	mpImpl->getState().mbFirstInSheetRow=false;
	// check if we need to add empty column
	int col = propList["librevenge:column"] ? propList["librevenge:column"]->getInt() : -1;
	if (col > state.miLastSheetColumn)
	{
		TagOpenElement *pEmptyElement = new TagOpenElement("table:table-cell");
		librevenge::RVNGString numEmpty;
		numEmpty.sprintf("%d", col-state.miLastSheetColumn);
		pEmptyElement->addAttribute("table:number-columns-repeated", numEmpty);
		mpImpl->getCurrentStorage()->push_back(pEmptyElement);
		mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-cell"));
	}
	else
		col=state.miLastSheetColumn;
	mpImpl->getState().miLastSheetColumn=col+1;

	state.mbInSheetCell=true;
	mpImpl->pushState(state);

	if (propList["style:font-name"])
		mpImpl->getFontManager().findOrAdd(propList["style:font-name"]->getStr().cstr());
	librevenge::RVNGString sSheetCellStyleName=style->addCell(propList);

	TagOpenElement *pSheetCellOpenElement = new TagOpenElement("table:table-cell");
	pSheetCellOpenElement->addAttribute("table:style-name", sSheetCellStyleName);

	if (propList["librevenge:value-type"])
	{
		std::string valueType(propList["librevenge:value-type"]->getStr().cstr());
		if (valueType=="double" || valueType=="scientific") valueType="float";
		else if (valueType=="percent") valueType="percentage";
		if (valueType=="float" || valueType=="percentage" || valueType=="currency")
		{
			pSheetCellOpenElement->addAttribute("calcext:value-type", valueType.c_str());
			pSheetCellOpenElement->addAttribute("office:value-type", valueType.c_str());
			if (propList["librevenge:value"])
			{
				if (!strncmp(propList["librevenge:value"]->getStr().cstr(),"nan",3) ||
				        !strncmp(propList["librevenge:value"]->getStr().cstr(),"NAN",3))
				{
					pSheetCellOpenElement->addAttribute("office:string-value", "");
					pSheetCellOpenElement->addAttribute("office:value-type", "string");
					pSheetCellOpenElement->addAttribute("calcext:value-type", "error");
				}
				else
				{
					pSheetCellOpenElement->addAttribute("office:value-type", valueType.c_str());
					pSheetCellOpenElement->addAttribute("office:value", propList["librevenge:value"]->getStr().cstr());
				}
			}
		}
		else if (valueType=="string" || valueType=="text")
		{
			pSheetCellOpenElement->addAttribute("office:value-type", "string");
			pSheetCellOpenElement->addAttribute("calcext:value-type", "string");
		}
		else if (valueType=="bool" || valueType=="boolean")
		{
			pSheetCellOpenElement->addAttribute("office:value-type", "boolean");
			pSheetCellOpenElement->addAttribute("calcext:value-type", "boolean");
			if (propList["librevenge:value"])
				pSheetCellOpenElement->addAttribute("office:boolean-value", propList["librevenge:value"]->getStr().cstr());
		}
		else if (valueType=="date")
		{
			pSheetCellOpenElement->addAttribute("office:value-type", "date");
			pSheetCellOpenElement->addAttribute("calcext:value-type", "date");
			if (propList["librevenge:day"] && propList["librevenge:month"] && propList["librevenge:year"])
			{
				librevenge::RVNGString date;
				if (propList["librevenge:hours"])
				{
					int minute=propList["librevenge:minutes"] ? propList["librevenge:minutes"]->getInt() : 0;
					int second=propList["librevenge:seconds"] ? propList["librevenge:seconds"]->getInt() : 0;
					date.sprintf("%04d-%02d-%02dT%02d:%02d:%02d", propList["librevenge:year"]->getInt(),
					             propList["librevenge:month"]->getInt(),  propList["librevenge:day"]->getInt(),
					             propList["librevenge:hours"]->getInt(), minute, second);
				}
				else
					date.sprintf("%04d-%02d-%02d", propList["librevenge:year"]->getInt(),
					             propList["librevenge:month"]->getInt(),  propList["librevenge:day"]->getInt());
				pSheetCellOpenElement->addAttribute("office:date-value", date);
			}
		}
		else if (valueType=="time")
		{
			pSheetCellOpenElement->addAttribute("office:value-type", "time");
			pSheetCellOpenElement->addAttribute("calcext:value-type", "time");
			if (propList["librevenge:hours"])
			{
				int minute=propList["librevenge:minutes"] ? propList["librevenge:minutes"]->getInt() : 0;
				int second=propList["librevenge:seconds"] ? propList["librevenge:seconds"]->getInt() : 0;
				librevenge::RVNGString time;
				time.sprintf("PT%02dH%02dM%02dS", propList["librevenge:hours"]->getInt(), minute, second);
				pSheetCellOpenElement->addAttribute("office:time-value", time);
			}
		}
		else
		{
			ODFGEN_DEBUG_MSG(("OdsGenerator::openSheetCell: unexpected value type: %s\n", valueType.c_str()));
		}
	}
	librevenge::RVNGPropertyListVector const *formula=propList.child("librevenge:formula");
	if (formula)
	{
		librevenge::RVNGString finalFormula=SheetManager::convertFormula(*formula);
		if (!finalFormula.empty())
			pSheetCellOpenElement->addAttribute("table:formula", finalFormula);
	}
	mpImpl->getCurrentStorage()->push_back(pSheetCellOpenElement);
}

void OdsGenerator::closeSheetCell()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_SheetCell) || mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;
	if (!mpImpl->getState().mbInSheetCell) return;

	mpImpl->popState();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("table:table-cell"));
}

void OdsGenerator::defineChartStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineChartStyle(propList);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().defineChartStyle(propList);
}

void OdsGenerator::openChart(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Chart);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->pushState(state);
	if (!mpImpl->checkOutsideOdc("openChart") || !mpImpl->checkOutsideOdt("openChart"))
		return;
	if (!state.mbFirstInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openChart must be called in a frame!!!\n"));
		return;
	}
	if (mpImpl->createAuxiliarOdcGenerator())
	{
		mpImpl->getState().mbInChart=true;
		mpImpl->getState().mbNewOdcGenerator=true;
		return mpImpl->mAuxiliarOdcState->get().openChart(propList);
	}
}

void OdsGenerator::closeChart()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Chart)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (!mpImpl->mAuxiliarOdcState || !state.mbInChart) return;
	if (state.mbNewOdcGenerator)
	{
		mpImpl->mAuxiliarOdcState->get().closeChart();
		mpImpl->sendAuxiliarOdcGenerator();
		mpImpl->resetAuxiliarOdcGenerator();
	}
}

void OdsGenerator::openChartPlotArea(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_ChartPlotArea);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->pushState(state);
	if (!mpImpl->mAuxiliarOdcState || !state.mbInChart)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openChartPlotArea called outside chart!!!\n"));
		return;
	}
	mpImpl->mAuxiliarOdcState->get().openChartPlotArea(propList);
}

void OdsGenerator::closeChartPlotArea()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_ChartPlotArea)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (!mpImpl->mAuxiliarOdcState || !state.mbInChart) return;
	mpImpl->mAuxiliarOdcState->get().closeChartPlotArea();
}

void OdsGenerator::openChartTextObject(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_ChartTextObject);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->pushState(state);
	if (!mpImpl->mAuxiliarOdcState || !state.mbInChart)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openChartTextObject called outside chart!!!\n"));
		return;
	}
	mpImpl->mAuxiliarOdcState->get().openChartTextObject(propList);
}

void OdsGenerator::closeChartTextObject()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_ChartTextObject)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (!mpImpl->mAuxiliarOdcState || !state.mbInChart) return;
	mpImpl->mAuxiliarOdcState->get().closeChartTextObject();
}

void OdsGenerator::insertChartAxis(const librevenge::RVNGPropertyList &axis)
{
	if (mpImpl->mAuxiliarOdtState) return;
	if (!mpImpl->mAuxiliarOdcState || !mpImpl->getState().mbInChart)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::insertChartAxis call outside chart\n"));
		return;
	}
	mpImpl->mAuxiliarOdcState->get().insertChartAxis(axis);
}

void OdsGenerator::openChartSerie(const librevenge::RVNGPropertyList &serie)
{
	mpImpl->open(OdsGeneratorPrivate::C_ChartSerie);
	if (!mpImpl->mAuxiliarOdcState || !mpImpl->getState().mbInChart)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openChartSerie call outside chart\n"));
		return;
	}
	mpImpl->mAuxiliarOdcState->get().openChartSerie(serie);
}

void OdsGenerator::closeChartSerie()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_ChartSerie)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	if (!mpImpl->mAuxiliarOdcState || !state.mbInChart) return;
	mpImpl->mAuxiliarOdcState->get().closeChartSerie();
}

void OdsGenerator::openHeader(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Header);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInHeaderFooter=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;

	if (mpImpl->inHeaderFooter() || !mpImpl->mpCurrentPageSpan)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openHeader: can not open a header\n"));
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

void OdsGenerator::closeHeader()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Header)) return;
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;
	if (!mpImpl->inHeaderFooter())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::closeHeader: no header/footer is already opened\n"));
		return;
	}
	mpImpl->endHeaderFooter();
	mpImpl->popStorage();
}

void OdsGenerator::openFooter(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Footer);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInHeaderFooter=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;

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

void OdsGenerator::closeFooter()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Footer)) return;
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdcState || mpImpl->mAuxiliarOdtState) return;

	if (!mpImpl->inHeaderFooter())
	{
		ODFGEN_DEBUG_MSG(("OdtGenerator::closeFooter: no header/footer is already opened\n"));
		return;
	}
	mpImpl->endHeaderFooter();
	mpImpl->popStorage();
}

void OdsGenerator::openSection(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Section);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openSection(propList);
	ODFGEN_DEBUG_MSG(("OdsGenerator::openSection ignored\n"));
}

void OdsGenerator::closeSection()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Section)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeSection();
}

void OdsGenerator::defineParagraphStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineParagraphStyle(propList);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().defineParagraphStyle(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().defineParagraphStyle(propList);
}

void OdsGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Paragraph);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().openParagraph(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openParagraph(propList);

	if (!mpImpl->canWriteText())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openParagraph: calls outside a text zone\n"));
		return;
	}

	librevenge::RVNGPropertyList finalPropList(propList);
	if (mpImpl->getState().mbInSheetCell)
		finalPropList.insert("style:parent-style-name", "Table_Contents");
	else
		finalPropList.insert("style:parent-style-name", "Standard");
	mpImpl->openParagraph(finalPropList);
}

void OdsGenerator::closeParagraph()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Paragraph)) return;
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().closeParagraph();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeParagraph();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->closeParagraph();
}

void OdsGenerator::defineCharacterStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineCharacterStyle(propList);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().defineCharacterStyle(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().defineCharacterStyle(propList);
}

void OdsGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Span);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().openSpan(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openSpan(propList);

	if (!mpImpl->canWriteText())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSpan: not in text part\n"));
		return;
	}
	mpImpl->openSpan(propList);
}

void OdsGenerator::closeSpan()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Span)) return;
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().closeSpan();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeSpan();
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeSpan();
}

void OdsGenerator::openLink(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().openLink(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openLink(propList);
	mpImpl->openLink(propList);
}

void OdsGenerator::closeLink()
{
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().closeLink();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeLink();
	mpImpl->closeLink();
}


void OdsGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_OrderedList);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().openOrderedListLevel(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openOrderedListLevel(propList);
	if (mpImpl->canWriteText() && !mpImpl->getState().mbInSheetCell)
		return mpImpl->openListLevel(propList,true);
	ODFGEN_DEBUG_MSG(("OdsGenerator::openOrderedListLevel: call outside a text zone\n"));
}

void OdsGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_UnorderedList);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().openUnorderedListLevel(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openUnorderedListLevel(propList);
	if (mpImpl->canWriteText() && !mpImpl->getState().mbInSheetCell)
		return mpImpl->openListLevel(propList,false);
	ODFGEN_DEBUG_MSG(("OdsGenerator::openUnorderedListLevel: call outside a text zone\n"));
}

void OdsGenerator::closeOrderedListLevel()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_OrderedList)) return;
	if (mpImpl->mAuxiliarOdcState)
		mpImpl->mAuxiliarOdcState->get().closeOrderedListLevel();
	if (mpImpl->mAuxiliarOdtState)
		mpImpl->mAuxiliarOdtState->get().closeOrderedListLevel();
	if (mpImpl->canWriteText() && !mpImpl->getState().mbInSheetCell)
		return mpImpl->closeListLevel();
}

void OdsGenerator::closeUnorderedListLevel()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_UnorderedList)) return;
	if (mpImpl->mAuxiliarOdcState)
		mpImpl->mAuxiliarOdcState->get().closeUnorderedListLevel();
	if (mpImpl->mAuxiliarOdtState)
		mpImpl->mAuxiliarOdtState->get().closeUnorderedListLevel();
	if (mpImpl->canWriteText() && !mpImpl->getState().mbInSheetCell)
		return mpImpl->closeListLevel();
}

void OdsGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_ListElement);
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().openListElement(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openListElement(propList);
	if (mpImpl->canWriteText())
	{
		if (mpImpl->getState().mbInSheetCell)
			return mpImpl->openParagraph(propList);
		return mpImpl->openListElement(propList);
	}

	ODFGEN_DEBUG_MSG(("OdsGenerator::openListElement call outside a text zone\n"));
	return;
}

void OdsGenerator::closeListElement()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_ListElement)) return;
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().closeListElement();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeListElement();
	if (mpImpl->canWriteText())
	{
		if (mpImpl->getState().mbInSheetCell)
			return mpImpl->closeParagraph();
		return mpImpl->closeListElement();
	}
}

void OdsGenerator::openFootnote(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Footnote);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInFootnote=true;
	mpImpl->pushState(state);

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openFootnote(propList);
	ODFGEN_DEBUG_MSG(("OdsGenerator::openFootnote ignored\n"));
}

void OdsGenerator::closeFootnote()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Footnote)) return;
	mpImpl->popState();

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeFootnote();
}

void OdsGenerator::openComment(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Comment);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->getState().mbInComment=false;
	mpImpl->pushState(state);

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openComment(propList);
	if (!mpImpl->checkOutsideOdc("openComment"))
		return;
	if (!state.mbInSheetCell)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openComment call outside a sheet cell!!!\n"));
		return;
	}

	mpImpl->getState().mbInComment=true;
	mpImpl->pushListState();
	mpImpl->getCurrentStorage()->push_back(new TagOpenElement("office:annotation"));
}

void OdsGenerator::closeComment()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Comment)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeComment();

	if (mpImpl->mAuxiliarOdcState || !state.mbInComment) return;
	mpImpl->popListState();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("office:annotation"));
}

void OdsGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->open(OdsGeneratorPrivate::C_Table);
	state.mbInTable=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTable(propList);
	if (!mpImpl->checkOutsideOdc("openTable"))
		return;
	if (!state.mbInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openTable a table must be in a frame!!!\n"));
		return;
	}
	if (mpImpl->createAuxiliarOdtGenerator())
	{
		mpImpl->getState().mbNewOdtGenerator=true;
		return mpImpl->mAuxiliarOdtState->get().openTable(propList);
	}
}

void OdsGenerator::closeTable()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Table)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdcState || !state.mbInTable || !mpImpl->mAuxiliarOdtState) return;
	mpImpl->mAuxiliarOdtState->get().closeTable();
	if (state.mbNewOdtGenerator)
	{
		mpImpl->sendAuxiliarOdtGenerator();
		mpImpl->resetAuxiliarOdtGenerator();
	}
}

void OdsGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_TableRow);
	if (mpImpl->mAuxiliarOdcState) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTableRow(propList);
}

void OdsGenerator::closeTableRow()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_TableRow)) return;
	if (mpImpl->mAuxiliarOdcState) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeTableRow();
}

void OdsGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_TableCell);
	if (mpImpl->mAuxiliarOdcState) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTableCell(propList);
}

void OdsGenerator::closeTableCell()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_TableCell)) return;
	if (mpImpl->mAuxiliarOdcState) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeTableCell();
}

void OdsGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdcState) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertCoveredTableCell(propList);
}


void OdsGenerator::insertTab()
{
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().insertTab();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertTab();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->insertTab();
}

void OdsGenerator::insertSpace()
{
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().insertSpace();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertSpace();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->insertSpace();
}

void OdsGenerator::insertLineBreak()
{
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().insertLineBreak();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertLineBreak();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->insertLineBreak(mpImpl->getState().mbInSheetCell);
}

void OdsGenerator::insertField(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:field-type"] || propList["librevenge:field-type"]->getStr().empty())
		return;
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().insertField(propList);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertField(propList);
	mpImpl->insertField(propList);
}

void OdsGenerator::insertText(const librevenge::RVNGString &text)
{
	if (mpImpl->mAuxiliarOdcState)
		return mpImpl->mAuxiliarOdcState->get().insertText(text);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertText(text);
	if (mpImpl->canWriteText())
		return mpImpl->insertText(text);
	ODFGEN_DEBUG_MSG(("OdsGenerator::insertText ignored\n"));
}

void OdsGenerator::openFrame(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Frame);
	OdsGeneratorPrivate::State &prevState=mpImpl->getState();
	OdsGeneratorPrivate::State state=prevState;
	state.mbInFrame=state.mbFirstInFrame=true;
	mpImpl->pushState(state);
	mpImpl->pushListState();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openFrame(propList);
	if (!mpImpl->checkOutsideOdc("openFrame"))
		return;
	if (!state.mbInSheet || state.mbInComment)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openFrame call outside a sheet!!!\n"));
		return;
	}

	if (!state.mbInSheetRow && !state.mbInSheetShapes)
	{
		mpImpl->getCurrentStorage()->push_back(new TagOpenElement("table:shapes"));
		mpImpl->getState().mbInSheetShapes=prevState.mbInSheetShapes=true;
	}

	librevenge::RVNGPropertyList pList(propList);
	if (!propList["text:anchor-type"])
		pList.insert("text:anchor-type","paragraph");
	mpImpl->openFrame(pList);
}

void OdsGenerator::closeFrame()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Frame)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popListState();
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeFrame();
	if (mpImpl->mAuxiliarOdcState || !state.mbInFrame) return;
	mpImpl->closeFrame();
}

void OdsGenerator::insertBinaryObject(const librevenge::RVNGPropertyList &propList)
{
	// Embedded objects without a frame simply don't make sense for us
	if (!mpImpl->getState().mbFirstInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::insertBinaryObject: called outsided a frame\n"));
		return;
	}
	mpImpl->getState().mbFirstInFrame=false;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertBinaryObject(propList);
	if (!mpImpl->checkOutsideOdc("insertBinaryObject"))
		return;
	mpImpl->insertBinaryObject(propList);
}

void OdsGenerator::openTextBox(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_TextBox);

	OdsGeneratorPrivate::State state=mpImpl->getState();
	// Text box without a frame simply doesn't make sense for us
	if (!state.mbInFrame || !state.mbFirstInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openTextBox: called without frame\n"));
		return;
	}
	mpImpl->getState().mbFirstInFrame=false;
	mpImpl->pushState(state);
	mpImpl->pushListState();

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTextBox(propList);
	if (!mpImpl->checkOutsideOdc("openTextBox"))
		return;

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
}

void OdsGenerator::closeTextBox()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_TextBox))
		return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popListState();
	mpImpl->popState();

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeTextBox();
	if (mpImpl->mAuxiliarOdcState || !state.mbInTextBox)
		return;
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("draw:text-box"));
}

void OdsGenerator::startDocument(const librevenge::RVNGPropertyList &)
{
	if (mpImpl->getState().mbStarted)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::startDocument: document already started\n"));
		return;
	}
	mpImpl->getState().mbStarted=true;
	mpImpl->open(OdsGeneratorPrivate::C_Document);
}

void OdsGenerator::endDocument()
{
	if (!mpImpl->getState().mbStarted)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::endDocument: document not started\n"));
		return;
	}
	if (mpImpl->mAuxiliarOdcState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::endDocument: auxiliar odc generator is open\n"));
		return;
	}
	if (mpImpl->mAuxiliarOdtState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::endDocument: auxiliar odt generator is open\n"));
		return;
	}
	mpImpl->getState().mbStarted=false;

	if (!mpImpl->close(OdsGeneratorPrivate::C_Document)) return;
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}


void OdsGenerator::openGroup(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Group);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openGroup(propList);
	if (!mpImpl->checkOutsideOdc("openGroup") || !mpImpl->canAddNewShape())
		return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInGroup=true;
	mpImpl->pushState(state);
	mpImpl->openGroup(propList);
}

void OdsGenerator::closeGroup()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Group))
		return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeGroup();
	if (mpImpl->mAuxiliarOdcState || !mpImpl->getState().mbInGroup)
		return;
	mpImpl->popState();
	mpImpl->closeGroup();
}

void OdsGenerator::defineGraphicStyle(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().defineGraphicStyle(propList);
	mpImpl->defineGraphicStyle(propList);
}

void OdsGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().drawRectangle(propList);
	if (!mpImpl->checkOutsideOdc("drawRectangle") || !mpImpl->canAddNewShape())
		return;
	mpImpl->drawRectangle(propList);
}

void OdsGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().drawEllipse(propList);
	if (!mpImpl->checkOutsideOdc("drawEllipse") || !mpImpl->canAddNewShape())
		return;
	mpImpl->drawEllipse(propList);
}


void OdsGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().drawPolygon(propList);
	if (!mpImpl->checkOutsideOdc("drawPolygon") || !mpImpl->canAddNewShape())
		return;
	mpImpl->drawPolySomething(propList, true);
}


void OdsGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().drawPolyline(propList);
	if (!mpImpl->checkOutsideOdc("drawPolyline") || !mpImpl->canAddNewShape())
		return;
	mpImpl->drawPolySomething(propList, false);
}


void OdsGenerator::drawPath(const ::librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().drawPath(propList);
	if (!mpImpl->checkOutsideOdc("drawPath") || !mpImpl->canAddNewShape())
		return;
	mpImpl->drawPath(propList);
}

void OdsGenerator::drawConnector(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->drawConnector(propList);
}

void OdsGenerator::initStateWith(OdfGenerator const &orig)
{
	mpImpl->initStateWith(orig);
}

void OdsGenerator::registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler)
{
	mpImpl->registerEmbeddedImageHandler(mimeType, imageHandler);
}

void OdsGenerator::registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler)
{
	mpImpl->registerEmbeddedObjectHandler(mimeType, objectHandler);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
