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
#include <sstream>
#include <string>

#include <libodfgen/libodfgen.hxx>

#include "DocumentElement.hxx"
#include "TextRunStyle.hxx"
#include "FontStyle.hxx"
#include "ListStyle.hxx"
#include "PageSpan.hxx"
#include "SectionStyle.hxx"
#include "SheetStyle.hxx"
#include "FilterInternal.hxx"
#include "InternalHandler.hxx"

class OdsGeneratorPrivate
{
public:
	enum Command { C_Document=0, C_PageSpan, C_Header, C_Footer, C_Sheet, C_SheetRow, C_SheetCell, C_Chart,
	               C_Span, C_Paragraph, C_Section, C_OrderedList, C_UnorderedList, C_ListElement,
	               C_Footnote, C_Comment, C_TextBox, C_Frame, C_Table, C_TableRow, C_TableCell,
	               C_Graphics, C_GraphicPage, C_GraphicLayer
	             };
	// the state we use for writing the final document
	struct State
	{
		State() : mbStarted(false),
			mbInSheet(false), mbInSheetShapes(false), mbInSheetRow(false), mbInSheetCell(false), miLastSheetRow(0), miLastSheetColumn(0),
			mbInFootnote(false), mbInComment(false), mbInHeaderFooter(false), mbInFrame(false), mbFirstInFrame(false), mbInChart(false),
			mbInTable(false), mbInTableWithoutFrame(false),
			mbInGraphics(false),
			mbNewOdtGenerator(false), mbNewOdgGenerator(false)
		{
		}
		bool canOpenFrame() const
		{
			return mbStarted && mbInSheet && !mbInSheetCell && !mbInFootnote && !mbInComment
			       && !mbInHeaderFooter && !mbInFrame && !mbInChart && !mbInTableWithoutFrame;
		}
		bool mbStarted;

		bool mbInSheet;
		bool mbInSheetShapes;
		bool mbInSheetRow;
		bool mbInSheetCell;
		int miLastSheetRow;
		int miLastSheetColumn;
		bool mbInFootnote;
		bool mbInComment;
		bool mbInHeaderFooter;
		bool mbInFrame;
		bool mbFirstInFrame;
		bool mbInChart;
		bool mbInTable;
		bool mbInTableWithoutFrame;

		bool mbInGraphics;

		bool mbNewOdtGenerator;
		bool mbNewOdgGenerator;
	};
	// list state
	struct List
	{
	public:
		List() : mLevelMap(), mbSend(false) { }
		struct Level
		{
			Level(librevenge::RVNGPropertyList const &lvl=librevenge::RVNGPropertyList(), bool ordered=true) : mLevel(lvl), mbOrdered(ordered) { }
			librevenge::RVNGPropertyList mLevel;
			bool mbOrdered;
		};
		std::map<int, Level> mLevelMap;
		mutable bool mbSend;
	};
	// the odt state
	struct OdtGeneratorState
	{
		OdtGeneratorState() : mContentElements(), mInternalHandler(&mContentElements), mGenerator(&mInternalHandler,ODF_FLAT_XML)
		{
		}
		~OdtGeneratorState()
		{
			for (size_t i=0; i < mContentElements.size(); ++i)
			{
				if (mContentElements[i]) delete mContentElements[i];
			}
		}
		OdtGenerator &get()
		{
			return mGenerator;
		}
		std::vector<DocumentElement *> mContentElements;
		InternalHandler mInternalHandler;
		OdtGenerator mGenerator;
	};
	// the odg state
	struct OdgGeneratorState
	{
		OdgGeneratorState() : mContentElements(), mInternalHandler(&mContentElements), mGenerator(&mInternalHandler,ODF_FLAT_XML)
		{
		}
		~OdgGeneratorState()
		{
			for (size_t i=0; i < mContentElements.size(); ++i)
			{
				if (mContentElements[i]) delete mContentElements[i];
			}
		}
		OdgGenerator &get()
		{
			return mGenerator;
		}
		std::vector<DocumentElement *> mContentElements;
		InternalHandler mInternalHandler;
		OdgGenerator mGenerator;
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
		mDocumentStreamHandlers.push_back(pHandler);
		mDocumentStreamTypes.push_back(streamType);
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
		return mStateStack.top().mbInComment || mStateStack.top().mbInSheetCell || mStateStack.top().mbInHeaderFooter;
	}

	//
	// List gestion
	//

	// returns the list corresponding to an id
	List &getList(int id)
	{
		if (mListMap.find(id)!=mListMap.end())
			return mListMap.find(id)->second;
		mListMap[id]=List();
		return  mListMap.find(id)->second;
	}
	// store a level
	void storeLevel(int id, const librevenge::RVNGPropertyList &level, bool ordered)
	{
		if (!level["librevenge:level"]) return;
		List &list=getList(id);
		list.mLevelMap[level["librevenge:level"]->getInt()]=List::Level(level, ordered);
	}
	// send list level to odt auxiliary
	bool sendListLevelsToAuxiliary(int id)
	{
		if (!mAuxiliarOdtState) return true;
		List &list=getList(id);
		if (list.mbSend) return false;
		std::map<int, List::Level>::const_iterator it=list.mLevelMap.begin();
		while (it!=list.mLevelMap.end())
		{
			List::Level const &level=it++->second;
			if (level.mbOrdered)
				mAuxiliarOdtState->get().defineOrderedListLevel(level.mLevel);
			else
				mAuxiliarOdtState->get().defineUnorderedListLevel(level.mLevel);
		}
		list.mbSend=true;
		return true;
	}
	// update if possible a list property list
	void updateListLevelProperty(int id, bool ordered, librevenge::RVNGPropertyList &pList) const
	{
		if (!pList["librevenge:level"] || mListMap.find(id)==mListMap.end()) return;
		List const &list=mListMap.find(id)->second;
		int lvl=pList["librevenge:level"]->getInt();
		std::map<int, List::Level>::const_iterator it=list.mLevelMap.begin();
		while (it!=list.mLevelMap.end())
		{
			if (it->first!=lvl)
			{
				++it;
				continue;
			}
			List::Level const &level=it++->second;
			if (level.mbOrdered!=ordered) continue;
			librevenge::RVNGPropertyList::Iter i(level.mLevel);
			for (i.rewind(); i.next();)
			{
				if (pList[i.key()] || i.child()) continue;
				pList.insert(i.key(), i()->clone());
			}
		}
	}

	// reset all list send flag
	void resetSendFlagsList() const
	{
		std::map<int, List>::const_iterator it=mListMap.begin();
		while (it != mListMap.end())
			(it++)->second.mbSend=false;
	}

	//
	// auxilliar generator
	//
	bool createAuxiliarOdtGenerator()
	{
		if (mAuxiliarOdtState)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::createAuxiliarOdtGenerator: already created\n"));
			return false;
		}
		mAuxiliarOdtState.reset(new OdtGeneratorState);
		std::map<librevenge::RVNGString, OdfEmbeddedObject, ltstr >::const_iterator it=mObjectHandlers.begin();
		while (it!=mObjectHandlers.end())
		{
			mAuxiliarOdtState->get().registerEmbeddedObjectHandler(it->first,static_cast<OdfEmbeddedObject>(it->second));
			++it;
		}
		return true;;
	}
	bool sendAuxiliarOdtGenerator()
	{
		if (!mAuxiliarOdtState || mAuxiliarOdtState->mContentElements.empty())
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::sendAuxiliarOdtGenerator: data seems bad\n"));
			return false;
		}
		mpCurrentContentElements->push_back(new TagOpenElement("draw:object"));
		for (std::vector<DocumentElement *>::const_iterator iter = mAuxiliarOdtState->mContentElements.begin(); iter != mAuxiliarOdtState->mContentElements.end(); ++iter)
			mpCurrentContentElements->push_back(*iter);
		mAuxiliarOdtState->mContentElements.resize(0);
		mpCurrentContentElements->push_back(new TagCloseElement("draw:object"));
		return true;
	}
	void resetAuxiliarOdtGenerator()
	{
		mAuxiliarOdtState.reset();
	}

	bool createAuxiliarOdgGenerator()
	{
		if (mAuxiliarOdgState)
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::createAuxiliarOdgGenerator: already created\n"));
			return false;
		}
		mAuxiliarOdgState.reset(new OdgGeneratorState);
		return true;
	}
	bool sendAuxiliarOdgGenerator()
	{
		if (!mAuxiliarOdgState || mAuxiliarOdgState->mContentElements.empty())
		{
			ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::sendAuxiliarOdgGenerator: data seems bad\n"));
			return false;
		}
		mpCurrentContentElements->push_back(new TagOpenElement("draw:object"));
		for (std::vector<DocumentElement *>::const_iterator iter = mAuxiliarOdgState->mContentElements.begin(); iter != mAuxiliarOdgState->mContentElements.end(); ++iter)
			mpCurrentContentElements->push_back(*iter);
		mAuxiliarOdgState->mContentElements.resize(0);
		mpCurrentContentElements->push_back(new TagCloseElement("draw:object"));
		return true;
	}
	void resetAuxiliarOdgGenerator()
	{
		mAuxiliarOdgState.reset();
	}

	static std::string getDocumentType(OdfStreamType streamType);
	void _writeMasterPages(OdfDocumentHandler *pHandler);
	void _writePageLayouts(OdfDocumentHandler *pHandler);
	bool _writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, bool includePageLayout);

	OdfEmbeddedObject _findEmbeddedObjectHandler(const librevenge::RVNGString &mimeType);

	unsigned _getObjectId()
	{
		return miObjectNumber++;
	}

	std::vector<OdfDocumentHandler *>mDocumentStreamHandlers;
	std::vector<OdfStreamType> mDocumentStreamTypes;

	std::stack<Command> mCommandStack;
	std::stack<State> mStateStack;
	std::map<int, List> mListMap;
	// object state
	unsigned miObjectNumber;

	// paragraph styles
	ParagraphStyleManager mParagraphManager;
	// span styles
	SpanStyleManager mSpanManager;
	// font styles
	FontStyleManager mFontManager;
	// embedded object handlers
	std::map<librevenge::RVNGString, OdfEmbeddedObject, ltstr > mObjectHandlers;
	// auxiliar odt handler to create data
	shared_ptr<OdtGeneratorState> mAuxiliarOdtState;
	// auxiliar odg handler to create data
	shared_ptr<OdgGeneratorState> mAuxiliarOdgState;

	// table styles
	SheetManager  mSheetManager;

	// frame styles
	std::vector<DocumentElement *> mFrameStyles;

	std::vector<DocumentElement *> mFrameAutomaticStyles;

	std::map<librevenge::RVNGString, unsigned, ltstr> mFrameIdMap;

	// metadata
	std::vector<DocumentElement *> mMetaData;

	// content elements
	std::vector<DocumentElement *> mBodyElements;
	// the current set of elements that we're writing to
	std::vector<DocumentElement *> *mpCurrentContentElements;

	// page state
	std::vector<PageSpan *> mPageSpans;
	PageSpan *mpCurrentPageSpan;
	int miNumPageStyles;

private:
	OdsGeneratorPrivate(const OdsGeneratorPrivate &);
	OdsGeneratorPrivate &operator=(const OdsGeneratorPrivate &);

};

std::string OdsGeneratorPrivate::getDocumentType(OdfStreamType streamType)
{
	switch (streamType)
	{
	case ODF_FLAT_XML:
		return "office:document";
	case ODF_CONTENT_XML:
		return "office:document-content";
	case ODF_STYLES_XML:
		return "office:document-styles";
	case ODF_SETTINGS_XML:
		return "office:document-settings";
	case ODF_META_XML:
		return "office:document-meta";
	default:
		return "office:document";
	}
}

OdsGeneratorPrivate::OdsGeneratorPrivate() :
	mDocumentStreamHandlers(), mDocumentStreamTypes(),
	mCommandStack(), mStateStack(), mListMap(), miObjectNumber(0),

	mParagraphManager(), mSpanManager(), mFontManager(),
	mObjectHandlers(), mAuxiliarOdtState(), mAuxiliarOdgState(),

	mSheetManager(),
	mFrameStyles(), mFrameAutomaticStyles(), mFrameIdMap(),
	mMetaData(),
	mBodyElements(), mpCurrentContentElements(&mBodyElements),
	mPageSpans(), mpCurrentPageSpan(0),	miNumPageStyles(0)
{
	mStateStack.push(State());
}

OdsGeneratorPrivate::~OdsGeneratorPrivate()
{
	// clean up the mess we made
	ODFGEN_DEBUG_MSG(("OdsGenerator: Cleaning up our mess..\n"));

	ODFGEN_DEBUG_MSG(("OdsGenerator: Destroying the body elements\n"));
	for (std::vector<DocumentElement *>::iterator iterBody = mBodyElements.begin(); iterBody != mBodyElements.end(); ++iterBody)
	{
		delete(*iterBody);
		(*iterBody) = 0;
	}

	mParagraphManager.clean();
	mSpanManager.clean();
	mFontManager.clean();
	mSheetManager.clean();

	for (std::vector<DocumentElement *>::iterator iterFrameStyles = mFrameStyles.begin();
	        iterFrameStyles != mFrameStyles.end(); ++iterFrameStyles)
	{
		delete(*iterFrameStyles);
	}
	for (std::vector<PageSpan *>::iterator iterPageSpans = mPageSpans.begin();
	        iterPageSpans != mPageSpans.end(); ++iterPageSpans)
	{
		delete(*iterPageSpans);
	}
	for (std::vector<DocumentElement *>::iterator iterFrameAutomaticStyles = mFrameAutomaticStyles.begin();
	        iterFrameAutomaticStyles != mFrameAutomaticStyles.end(); ++iterFrameAutomaticStyles)
	{
		delete(*iterFrameAutomaticStyles);
	}
	for (std::vector<DocumentElement *>::iterator iterMetaData = mMetaData.begin();
	        iterMetaData != mMetaData.end(); ++iterMetaData)
	{
		delete(*iterMetaData);
	}
}

bool OdsGeneratorPrivate::close(Command command)
{
	if (mCommandStack.empty() || mCommandStack.top()!=command)
	{
#ifdef DEBUG
		static char const *(wh[]) =
		{
			"Document", "PageSpan", "Header", "Footer", "Sheet", "SheetRow", "SheetCell", "Chart",
			"Span", "Paragraph", "Section", "OrderedListLevel", "UnorderedListLevel", "ListElement",
			"Comment", "TextBox", "Frame", "Table", "TableRow", "TableCell",
			"Graphic", "GraphicPage", "GraphicLayer"
		};
		ODFGEN_DEBUG_MSG(("OdsGeneratorPrivate::close: unexpected %s\n", wh[command]));
#endif
		return false;
	}
	mCommandStack.pop();
	return true;
}

OdfEmbeddedObject OdsGeneratorPrivate::_findEmbeddedObjectHandler(const librevenge::RVNGString &mimeType)
{
	std::map<librevenge::RVNGString, OdfEmbeddedObject, ltstr>::iterator i = mObjectHandlers.find(mimeType);
	if (i != mObjectHandlers.end())
		return i->second;

	return 0;
}

void OdsGeneratorPrivate::_writeMasterPages(OdfDocumentHandler *pHandler)
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

void OdsGeneratorPrivate::_writePageLayouts(OdfDocumentHandler *pHandler)
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

void OdsGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler, bool includePageLayout)
{
	TagOpenElement("office:automatic-styles").write(pHandler);
	for (std::vector<DocumentElement *>::const_iterator iterFrameAutomaticStyles = mFrameAutomaticStyles.begin();
	        iterFrameAutomaticStyles != mFrameAutomaticStyles.end(); ++iterFrameAutomaticStyles)
		(*iterFrameAutomaticStyles)->write(pHandler);
	if (includePageLayout)
		_writePageLayouts(pHandler);
	mFontManager.write(pHandler); // do nothing
	mParagraphManager.write(pHandler);
	mSpanManager.write(pHandler);
	mSheetManager.write(pHandler);

	pHandler->endElement("office:automatic-styles");
}

void OdsGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);

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

	for (std::vector<DocumentElement *>::const_iterator iter = mFrameStyles.begin();
	        iter != mFrameStyles.end(); ++iter)
		(*iter)->write(pHandler);

	pHandler->endElement("office:styles");
}

bool OdsGeneratorPrivate::_writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	pHandler->startDocument();

	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: preamble\n"));
	}

	std::string const documentType=getDocumentType(streamType);
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
	{
		// write out the metadata
		TagOpenElement("office:meta").write(pHandler);
		for (std::vector<DocumentElement *>::const_iterator iterMetaData = mMetaData.begin(); iterMetaData != mMetaData.end(); ++iterMetaData)
			(*iterMetaData)->write(pHandler);
		pHandler->endElement("office:meta");
	}

	// write out the font styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
		mFontManager.writeFontsDeclaration(pHandler);

	// write default styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: Writing out the styles..\n"));
		_writeStyles(pHandler);
	}
	if (streamType == ODF_STYLES_XML)
	{
		TagOpenElement("office:automatic-styles").write(pHandler);
		_writePageLayouts(pHandler);
		pHandler->endElement("office:automatic-styles");
	}
	// writing automatic style
	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
		_writeAutomaticStyles(pHandler, streamType == ODF_FLAT_XML);

	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
		// writing out the page masters
		_writeMasterPages(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator: Document Body: Writing out the document..\n"));
		// writing out the document
		TagOpenElement("office:body").write(pHandler);
		TagOpenElement("office:spreadsheet").write(pHandler);

		for (std::vector<DocumentElement *>::const_iterator iterBodyElements = mBodyElements.begin(); iterBodyElements != mBodyElements.end(); ++iterBodyElements)
			(*iterBodyElements)->write(pHandler);

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
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		// filter out librevenge elements
		if (strncmp(i.key(), "librevenge", 10) != 0 && strncmp(i.key(), "dcterms", 7) != 0)
		{
			mpImpl->mMetaData.push_back(new TagOpenElement(i.key()));
			librevenge::RVNGString sStringValue(i()->getStr(), true);
			mpImpl->mMetaData.push_back(new CharDataElement(sStringValue.cstr()));
			mpImpl->mMetaData.push_back(new TagCloseElement(i.key()));
		}
	}

}

void OdsGenerator::openPageSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_PageSpan);

	PageSpan *pPageSpan = new PageSpan(propList);
	mpImpl->mPageSpans.push_back(pPageSpan);
	mpImpl->mpCurrentPageSpan = pPageSpan;
	mpImpl->miNumPageStyles++;
}

void OdsGenerator::closePageSpan()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_PageSpan)) return;
}

void OdsGenerator::defineSheetNumberingStyle(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInSheet || !mpImpl->mSheetManager.actualSheet())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::defineSheetNumberingStyle can not be called outside a sheet!!!\n"));
		return;
	}
	mpImpl->mSheetManager.actualSheet()->addNumberingStyle(propList);
}

void OdsGenerator::openSheet(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Sheet);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInSheet=false;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheet call in OLE!!!\n"));
		return;
	}
	if (state.mbInSheet || state.mbInFrame || state.mbInFootnote || state.mbInComment || state.mbInHeaderFooter ||
	        mpImpl->mSheetManager.isSheetOpened())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheet can not be called!!!\n"));
		return;
	}

	librevenge::RVNGPropertyList finalPropList(propList);
	librevenge::RVNGString sPageStyleName;
	sPageStyleName.sprintf("Page_Style_%i", mpImpl->miNumPageStyles);
	finalPropList.insert("style:master-page-name", sPageStyleName);
	if (!mpImpl->mSheetManager.openSheet(finalPropList)) return;
	mpImpl->getState().mbInSheet=true;

	SheetStyle *style=mpImpl->mSheetManager.actualSheet();
	if (!style) return;
	librevenge::RVNGString sTableName(style->getName());

	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");
	pTableOpenElement->addAttribute("table:name", sTableName.cstr());
	pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
	mpImpl->mpCurrentContentElements->push_back(pTableOpenElement);

	/* TODO: open a table:shapes element
	   a environment to store the table content which must be merged in closeSheet
	*/
	for (int i=0; i< style->getNumColumns(); ++i)
	{
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		librevenge::RVNGString sColumnStyleName;
		sColumnStyleName.sprintf("%s_col%i", sTableName.cstr(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
		mpImpl->mpCurrentContentElements->push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpImpl->mpCurrentContentElements->push_back(pTableColumnCloseElement);
	}
}

void OdsGenerator::closeSheet()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Sheet))
		return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState || !state.mbInSheet) return;
	if (state.mbInSheetShapes)
	{
		mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:shapes"));
		mpImpl->getState().mbInSheetShapes=false;
	}
	mpImpl->mSheetManager.closeSheet();
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:table"));
}

void OdsGenerator::openSheetRow(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_SheetRow);
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	SheetStyle *style=mpImpl->mSheetManager.actualSheet();

	if (!state.mbInSheet || state.mbInComment || !style)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheetRow can not be called!!!\n"));
		return;
	}
	if (state.mbInSheetShapes)
	{
		mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:shapes"));
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

		mpImpl->mpCurrentContentElements->push_back(pEmptyRowOpenElement);
		mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("table:table-cell"));
		mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:table-cell"));
		mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:table-row"));
	}
	else
		row=state.miLastSheetRow;
	mpImpl->getState().miLastSheetRow=row+1;

	state.miLastSheetColumn=0;
	state.mbInSheetRow=true;
	mpImpl->pushState(state);

	librevenge::RVNGString sSheetRowStyleName=style->addRow(propList);
	TagOpenElement *pSheetRowOpenElement = new TagOpenElement("table:table-row");
	pSheetRowOpenElement->addAttribute("table:style-name", sSheetRowStyleName);
	mpImpl->mpCurrentContentElements->push_back(pSheetRowOpenElement);
}

void OdsGenerator::closeSheetRow()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_SheetRow) || mpImpl->mAuxiliarOdtState ||
	        mpImpl->mAuxiliarOdgState) return;
	if (!mpImpl->getState().mbInSheetRow) return;

	mpImpl->popState();
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:table-row"));
}

void OdsGenerator::openSheetCell(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_SheetCell);
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	SheetStyle *style=mpImpl->mSheetManager.actualSheet();

	if (!state.mbInSheetRow || state.mbInComment || !style)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSheetCell can not be called!!!\n"));
		return;
	}
	// check if we need to add empty column
	int col = propList["librevenge:column"] ? propList["librevenge:column"]->getInt() : -1;
	if (col > state.miLastSheetColumn)
	{
		TagOpenElement *pEmptyElement = new TagOpenElement("table:table-cell");
		librevenge::RVNGString numEmpty;
		numEmpty.sprintf("%d", col-state.miLastSheetColumn);
		pEmptyElement->addAttribute("table:number-columns-repeated", numEmpty);
		mpImpl->mpCurrentContentElements->push_back(pEmptyElement);
		mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:table-cell"));
	}
	else
		col=state.miLastSheetColumn;
	mpImpl->getState().miLastSheetColumn=col+1;

	state.mbInSheetCell=true;
	mpImpl->pushState(state);

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
				if (strncmp(propList["librevenge:value"]->getStr().cstr(),"nan",3)==0 ||
				        strncmp(propList["librevenge:value"]->getStr().cstr(),"NAN",3)==0)
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
	mpImpl->mpCurrentContentElements->push_back(pSheetCellOpenElement);
}

void OdsGenerator::closeSheetCell()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_SheetCell) || mpImpl->mAuxiliarOdtState ||
	        mpImpl->mAuxiliarOdgState) return;
	if (!mpImpl->getState().mbInSheetCell) return;

	mpImpl->popState();
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("table:table-cell"));
}

void OdsGenerator::openChart(const librevenge::RVNGPropertyList &/*propList*/)
{
	mpImpl->open(OdsGeneratorPrivate::C_Chart);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openChart call in OLE!!!\n"));
		return;
	}
	if (!state.mbFirstInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openChart must be called in a frame!!!\n"));
		return;
	}
	// TODO
	mpImpl->getState().mbInChart=true;
	ODFGEN_DEBUG_MSG(("OdsGenerator::openChart not implemented\n"));
}

void OdsGenerator::closeChart()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Chart)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState || !state.mbInChart) return;
	// TODO
	ODFGEN_DEBUG_MSG(("OdsGenerator::closeChart not implemented\n"));
}

void OdsGenerator::insertChartSerie(const librevenge::RVNGPropertyList &/*commands*/)
{
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;
	if (!mpImpl->getState().mbInChart)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::insertChartSerie call outside chart\n"));
		return;
	}
	// TODO
	ODFGEN_DEBUG_MSG(("OdsGenerator::insertChartSerie not implemented\n"));
}

void OdsGenerator::openHeader(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Header);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInHeaderFooter=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;

	if (!mpImpl->mpCurrentPageSpan)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openHeader oops can not find the page span\n"));
		return;
	}
	std::vector<DocumentElement *> *pHeaderFooterContentElements = new std::vector<DocumentElement *>;
	if (propList["librevenge:occurence"] && propList["librevenge:occurence"]->getStr() == "even")
		mpImpl->mpCurrentPageSpan->setHeaderLeftContent(pHeaderFooterContentElements);
	else
		mpImpl->mpCurrentPageSpan->setHeaderContent(pHeaderFooterContentElements);
	mpImpl->mpCurrentContentElements = pHeaderFooterContentElements;
}

void OdsGenerator::closeHeader()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Header)) return;
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;

	if (!mpImpl->mpCurrentPageSpan) return;
	mpImpl->mpCurrentContentElements = &(mpImpl->mBodyElements);
}

void OdsGenerator::openFooter(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Footer);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInHeaderFooter=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;

	if (!mpImpl->mpCurrentPageSpan)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openFooter oops can not find the page span\n"));
		return;
	}
	std::vector<DocumentElement *> *pHeaderFooterContentElements = new std::vector<DocumentElement *>;
	if (propList["librevenge:occurence"] && propList["librevenge:occurence"]->getStr() == "even")
		mpImpl->mpCurrentPageSpan->setFooterLeftContent(pHeaderFooterContentElements);
	else
		mpImpl->mpCurrentPageSpan->setFooterContent(pHeaderFooterContentElements);
	mpImpl->mpCurrentContentElements = pHeaderFooterContentElements;
}

void OdsGenerator::closeFooter()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Footer)) return;
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState) return;

	if (!mpImpl->mpCurrentPageSpan) return;
	mpImpl->mpCurrentContentElements = &(mpImpl->mBodyElements);
}

void OdsGenerator::openSection(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Section);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openSection(propList);
	if (mpImpl->mAuxiliarOdgState) return;
	ODFGEN_DEBUG_MSG(("OdsGenerator::openSection ignored\n"));
}

void OdsGenerator::closeSection()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Section)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeSection();
	if (mpImpl->mAuxiliarOdgState) return;
}

void OdsGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Paragraph);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openParagraph(propList);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().openParagraph(propList);

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
	librevenge::RVNGString sName = mpImpl->mParagraphManager.findOrAdd(finalPropList);
	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pParagraphOpenElement = new TagOpenElement("text:p");
	pParagraphOpenElement->addAttribute("text:style-name", sName);
	mpImpl->mpCurrentContentElements->push_back(pParagraphOpenElement);
}

void OdsGenerator::closeParagraph()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Paragraph)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeParagraph();
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().closeParagraph();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("text:p"));
}

void OdsGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Span);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openSpan(propList);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().openSpan(propList);

	if (!mpImpl->canWriteText())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openSpan: not in text part\n"));
		return;
	}
	if (propList["style:font-name"])
		mpImpl->mFontManager.findOrAdd(propList["style:font-name"]->getStr().cstr());

	// Get the style
	librevenge::RVNGString sName = mpImpl->mSpanManager.findOrAdd(propList);
	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pSpanOpenElement = new TagOpenElement("text:span");
	pSpanOpenElement->addAttribute("text:style-name", sName.cstr());
	mpImpl->mpCurrentContentElements->push_back(pSpanOpenElement);
}

void OdsGenerator::closeSpan()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Span)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeSpan();
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().closeSpan();
	if (!mpImpl->canWriteText()) return;
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("text:span"));
}


void OdsGenerator::defineOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:level"])
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator:defineOrderedListLevel: can not find list level\n"));
		return;
	}
	int id = propList["librevenge:id"] ? propList["librevenge:id"]->getInt() : 0;
	mpImpl->storeLevel(id, propList, true);
	if (!mpImpl->mAuxiliarOdtState) return;
	if (!mpImpl->sendListLevelsToAuxiliary(id))
		mpImpl->mAuxiliarOdtState->get().defineOrderedListLevel(propList);
}

void OdsGenerator::defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:level"])
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator:defineUnorderedListLevel: can not find list level\n"));
		return;
	}
	int id = propList["librevenge:id"] ? propList["librevenge:id"]->getInt() : 0;
	mpImpl->storeLevel(id, propList, false);
	if (!mpImpl->mAuxiliarOdtState)
		return;
	if (!mpImpl->sendListLevelsToAuxiliary(id))
		mpImpl->mAuxiliarOdtState->get().defineUnorderedListLevel(propList);
}

void OdsGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_OrderedList);
	if (mpImpl->mAuxiliarOdtState)
	{
		int id = propList["librevenge:id"] ? propList["librevenge:id"]->getInt() : 0;
		mpImpl->sendListLevelsToAuxiliary(id);
		return mpImpl->mAuxiliarOdtState->get().openOrderedListLevel(propList);
	}
	if (mpImpl->mAuxiliarOdgState)
	{
		librevenge::RVNGPropertyList pList(propList);
		if (pList["librevenge:id"])
			mpImpl->updateListLevelProperty(propList["librevenge:id"]->getInt(), true, pList);
		return mpImpl->mAuxiliarOdgState->get().openOrderedListLevel(pList);
	}
	ODFGEN_DEBUG_MSG(("OdsGenerator::openOrderedListLevel: ignored\n"));
}

void OdsGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_UnorderedList);
	if (mpImpl->mAuxiliarOdtState)
	{
		int id = propList["librevenge:id"] ? propList["librevenge:id"]->getInt() : 0;
		mpImpl->sendListLevelsToAuxiliary(id);
		return mpImpl->mAuxiliarOdtState->get().openUnorderedListLevel(propList);
	}
	if (mpImpl->mAuxiliarOdgState)
	{
		librevenge::RVNGPropertyList pList(propList);
		if (pList["librevenge:id"])
			mpImpl->updateListLevelProperty(propList["librevenge:id"]->getInt(), false, pList);
		return mpImpl->mAuxiliarOdgState->get().openUnorderedListLevel(pList);
	}
	ODFGEN_DEBUG_MSG(("OdsGenerator::openUnorderedListLevel: ignored\n"));
}

void OdsGenerator::closeOrderedListLevel()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_OrderedList)) return;
	if (mpImpl->mAuxiliarOdtState)
		mpImpl->mAuxiliarOdtState->get().closeOrderedListLevel();
	if (mpImpl->mAuxiliarOdgState)
		mpImpl->mAuxiliarOdgState->get().closeOrderedListLevel();
}

void OdsGenerator::closeUnorderedListLevel()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_UnorderedList)) return;
	if (mpImpl->mAuxiliarOdtState)
		mpImpl->mAuxiliarOdtState->get().closeUnorderedListLevel();
	if (mpImpl->mAuxiliarOdgState)
		mpImpl->mAuxiliarOdgState->get().closeOrderedListLevel();
}

void OdsGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_ListElement);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openListElement(propList);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().openListElement(propList);
	if (!mpImpl->canWriteText())
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openListElement call outside a text zone\n"));
		return;
	}
	librevenge::RVNGPropertyList finalPropList(propList);
	finalPropList.insert("style:parent-style-name", "Standard");
	librevenge::RVNGString paragName = mpImpl->mParagraphManager.findOrAdd(finalPropList);

	TagOpenElement *pOpenListElementParagraph = new TagOpenElement("text:p");
	pOpenListElementParagraph->addAttribute("text:style-name", paragName);
	mpImpl->mpCurrentContentElements->push_back(pOpenListElementParagraph);
}

void OdsGenerator::closeListElement()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_ListElement)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeListElement();
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().closeListElement();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("text:p"));
}

void OdsGenerator::openFootnote(const librevenge::RVNGPropertyList &)
{
	mpImpl->open(OdsGeneratorPrivate::C_Footnote);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInFootnote=true;
	mpImpl->pushState(state);
	ODFGEN_DEBUG_MSG(("OdsGenerator::openFootnote ignored\n"));
}

void OdsGenerator::closeFootnote()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Footnote)) return;
	mpImpl->popState();
}

void OdsGenerator::openComment(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Comment);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->getState().mbInComment=false;
	mpImpl->pushState(state);

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openComment(propList);
	if (mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openComment call in graphics!!!\n"));
		return;
	}
	if (!state.mbInSheetCell)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openComment call outside a sheet cell!!!\n"));
		return;
	}

	mpImpl->getState().mbInComment=true;
	mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("office:annotation"));
}

void OdsGenerator::closeComment()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Comment)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeComment();

	if (!state.mbInComment) return;
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("office:annotation"));
}

void OdsGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Table);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	bool needFrame = !state.mbInFrame && !mpImpl->mAuxiliarOdtState && !mpImpl->mAuxiliarOdgState;
	if (needFrame)
	{
		if (!state.canOpenFrame() && !mpImpl->canWriteText())
		{
			mpImpl->pushState(state);
			ODFGEN_DEBUG_MSG(("OdsGenerator::openTable can not open a frame!!!\n"));
		}
		else
		{
			librevenge::RVNGPropertyList fPropList;
			fPropList.insert("text:anchor-type","as-char");
			fPropList.insert("style:vertical-rel", "baseline");
			fPropList.insert("style:vertical-pos", "middle");
			fPropList.insert("style:wrap","none");
			openFrame(fPropList);
			state=mpImpl->getState();
			state.mbInTableWithoutFrame=true;
		}
	}
	state.mbInTable=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTable(propList);
	if (mpImpl->mAuxiliarOdgState)
	{
		/*CHECKME: maybe we can create a auxiliar odt generator, use it
		  to create an OLE and then send the OLE to the
		  OdgGenerator */
		ODFGEN_DEBUG_MSG(("OdsGenerator::openTable call in graphics!!!\n"));
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

	if (state.mbInTable && mpImpl->mAuxiliarOdtState)
	{
		mpImpl->mAuxiliarOdtState->get().closeTable();
		if (state.mbNewOdtGenerator)
		{
			mpImpl->sendAuxiliarOdtGenerator();
			mpImpl->resetAuxiliarOdtGenerator();
		}
	}
	if (state.mbInTableWithoutFrame)
		closeFrame();
}

void OdsGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_TableRow);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTableRow(propList);
}

void OdsGenerator::closeTableRow()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_TableRow)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeTableRow();
}

void OdsGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_TableCell);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTableCell(propList);
}

void OdsGenerator::closeTableCell()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_TableCell)) return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeTableCell();
}

void OdsGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertCoveredTableCell(propList);
}


void OdsGenerator::insertTab()
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertTab();
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().insertTab();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("text:tab"));
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("text:tab"));
}

void OdsGenerator::insertSpace()
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertSpace();
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().insertSpace();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("text:s"));
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("text:s"));
}

void OdsGenerator::insertLineBreak()
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertLineBreak();
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().insertLineBreak();
	if (!mpImpl->canWriteText())
		return;
	mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("text:line-break"));
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("text:line-break"));
}

void OdsGenerator::insertField(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:field-type"] || propList["librevenge:field-type"]->getStr().empty())
		return;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertField(propList);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().insertField(propList);
	ODFGEN_DEBUG_MSG(("OdsGenerator::insertField ignored\n"));
}

void OdsGenerator::insertText(const librevenge::RVNGString &text)
{
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertText(text);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().insertText(text);
	if (mpImpl->canWriteText())
	{
		DocumentElement *pText = new TextElement(text);
		mpImpl->mpCurrentContentElements->push_back(pText);
	}
	else
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::insertText ignored\n"));
	}
}

void OdsGenerator::openFrame(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Frame);
	OdsGeneratorPrivate::State state=mpImpl->getState();
	state.mbInFrame=state.mbFirstInFrame=true;
	mpImpl->pushState(state);
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openFrame(propList);
	if (mpImpl->mAuxiliarOdgState)
	{
		// CHECKME: maybe we can use startEmbeddedGraphic
		ODFGEN_DEBUG_MSG(("OdsGenerator::openFrame call in a graphics!!!\n"));
		return;
	}
	if (!state.mbInSheet || state.mbInComment)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::openFrame call outside a sheet!!!\n"));
		return;
	}
	// First, let's create a Frame Style for this box
	TagOpenElement *frameStyleOpenElement = new TagOpenElement("style:style");
	librevenge::RVNGString frameStyleName;
	unsigned objectId = mpImpl->_getObjectId();
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

	if (!state.mbInSheetRow && !state.mbInSheetShapes)
	{
		mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("table:shapes"));
		mpImpl->getState().mbInSheetShapes=true;
	}
	mpImpl->mpCurrentContentElements->push_back(drawFrameOpenElement);
}

void OdsGenerator::closeFrame()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_Frame)) return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().closeFrame();
	if (mpImpl->mAuxiliarOdgState)
		return;
	if (!state.mbInFrame) return;
	mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("draw:frame"));
}

void OdsGenerator::insertBinaryObject(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["office:binary-data"] || !propList["librevenge:mimetype"])
		return;
	// Embedded objects without a frame simply don't make sense for us
	if (!mpImpl->getState().mbFirstInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::insertBinaryObject: called outsided a frame\n"));
		return;
	}
	mpImpl->getState().mbFirstInFrame=false;
	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().insertBinaryObject(propList);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().drawGraphicObject(propList);
	OdfEmbeddedObject tmpObjectHandler = mpImpl->_findEmbeddedObjectHandler(propList["librevenge:mimetype"]->getStr());

	if (tmpObjectHandler)
	{
		librevenge::RVNGBinaryData data(propList["office:binary-data"]->getStr());
		std::vector<DocumentElement *> tmpContentElements;
		InternalHandler tmpHandler(&tmpContentElements);

		if (tmpObjectHandler(data, &tmpHandler, ODF_FLAT_XML) && !tmpContentElements.empty())
		{
			mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("draw:object"));
			for (std::vector<DocumentElement *>::const_iterator iter = tmpContentElements.begin(); iter != tmpContentElements.end(); ++iter)
				mpImpl->mpCurrentContentElements->push_back(*iter);
			mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("draw:object"));
		}
	}
	else
		// assuming we have a binary image or a object_ole that we can just insert as it is
	{
		if (propList["librevenge:mimetype"]->getStr() == "object/ole")
			mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("draw:object-ole"));
		else
			mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("draw:image"));

		mpImpl->mpCurrentContentElements->push_back(new TagOpenElement("office:binary-data"));

		mpImpl->mpCurrentContentElements->push_back(new CharDataElement(propList["office:binary-data"]->getStr().cstr()));

		mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("office:binary-data"));

		if (propList["librevenge:mimetype"]->getStr() == "object/ole")
			mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("draw:object-ole"));
		else
			mpImpl->mpCurrentContentElements->push_back(new TagCloseElement("draw:image"));
	}
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

	if (mpImpl->mAuxiliarOdtState)
		return mpImpl->mAuxiliarOdtState->get().openTextBox(propList);
	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().startTextObject(propList);
	if (!mpImpl->createAuxiliarOdtGenerator())
		return;

	mpImpl->getState().mbNewOdtGenerator=true;
	mpImpl->mAuxiliarOdtState->get().openTextBox(propList);
}

void OdsGenerator::closeTextBox()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_TextBox))
		return;
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();

	if (mpImpl->mAuxiliarOdgState)
		return mpImpl->mAuxiliarOdgState->get().endTextObject();
	if (!mpImpl->mAuxiliarOdtState)
		return;
	mpImpl->mAuxiliarOdtState->get().closeTextBox();
	if (!state.mbNewOdtGenerator)
		return;

	mpImpl->sendAuxiliarOdtGenerator();
	mpImpl->resetAuxiliarOdtGenerator();
}

void OdsGenerator::startDocument()
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
	if (mpImpl->mAuxiliarOdtState || mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::endDocument: auxiliar odt generator is open\n"));
		return;
	}
	mpImpl->getState().mbStarted=false;

	if (!mpImpl->close(OdsGeneratorPrivate::C_Document)) return;
	// Write out the collected document
	for (size_t i=0; i<mpImpl->mDocumentStreamHandlers.size(); ++i)
		mpImpl->_writeTargetDocument(mpImpl->mDocumentStreamHandlers[i], mpImpl->mDocumentStreamTypes[i]);
}

void OdsGenerator::startGraphic(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_Graphics);
	// Graphic without a frame simply doesn't make sense for us
	OdsGeneratorPrivate::State state=mpImpl->getState();
	if (!state.mbInFrame || !state.mbFirstInFrame)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::startGraphic: called without frame\n"));
		return;
	}
	// update state
	mpImpl->getState().mbFirstInFrame=false;
	state.mbInGraphics=true;
	mpImpl->pushState(state);

	if (mpImpl->mAuxiliarOdtState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::startGraphic: call inside a text OLE\n"));
		return;
	}
	if (mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::startGraphic: call inside a graphic OLE\n"));
		return;
	}
	if (!mpImpl->createAuxiliarOdgGenerator())
		return;
	mpImpl->getState().mbNewOdgGenerator=true;
	mpImpl->mAuxiliarOdgState->get().startDocument(propList);
}

void OdsGenerator::endGraphic()
{
	OdsGeneratorPrivate::State state=mpImpl->getState();
	mpImpl->popState();
	if (!mpImpl->close(OdsGeneratorPrivate::C_Graphics))
		return;
	if (!state.mbInGraphics || !state.mbNewOdgGenerator || !mpImpl->mAuxiliarOdgState)
		return;
	mpImpl->mAuxiliarOdgState->get().endDocument();
	mpImpl->sendAuxiliarOdgGenerator();
	mpImpl->resetAuxiliarOdgGenerator();
}

void OdsGenerator::startGraphicPage(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_GraphicPage);
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::startGraphicPage: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().startPage(propList);
}

void OdsGenerator::endGraphicPage()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_GraphicPage))
		return;
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
		return;
	mpImpl->mAuxiliarOdgState->get().endPage();
}

void OdsGenerator::startGraphicLayer(const ::librevenge::RVNGPropertyList &propList)
{
	mpImpl->open(OdsGeneratorPrivate::C_GraphicLayer);
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::startGraphicLayer: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().startLayer(propList);
}

void OdsGenerator::endGraphicLayer()
{
	if (!mpImpl->close(OdsGeneratorPrivate::C_GraphicLayer))
		return;
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
		return;
	mpImpl->mAuxiliarOdgState->get().endLayer();
}

void OdsGenerator::setGraphicStyle(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::setGraphicStyles: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().setStyle(propList);
}

void OdsGenerator::drawRectangle(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::drawRectangle: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().drawRectangle(propList);
}

void OdsGenerator::drawEllipse(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::drawEllipse: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().drawEllipse(propList);
}


void OdsGenerator::drawPolygon(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::drawPolygon: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().drawPolygon(propList);
}


void OdsGenerator::drawPolyline(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::drawPolyline: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().drawPolyline(propList);
}


void OdsGenerator::drawPath(const ::librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->getState().mbInGraphics || !mpImpl->mAuxiliarOdgState)
	{
		ODFGEN_DEBUG_MSG(("OdsGenerator::drawPath: graphics not started\n"));
		return;
	}
	mpImpl->mAuxiliarOdgState->get().drawPath(propList);
}

void OdsGenerator::registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler)
{
	mpImpl->mObjectHandlers[mimeType] = objectHandler;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
