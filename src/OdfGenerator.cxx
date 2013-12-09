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

#include <string>

#include <librevenge/librevenge.h>

#include "DocumentElement.hxx"
#include "InternalHandler.hxx"
#include "ListStyle.hxx"
#include "TableStyle.hxx"

#include "OdfGenerator.hxx"

OdfGenerator::OdfGenerator() :
	mpCurrentStorage(&mBodyStorage), mStorageStack(), mMetaDataStorage(), mBodyStorage(),
	mFontManager(), mSpanManager(), mParagraphManager(),
	mIdSpanMap(), mIdSpanNameMap(), mLastSpanName(""),
	mIdParagraphMap(), mIdParagraphNameMap(), mLastParagraphName(""),
	miNumListStyles(0), mListStyles(), mListStates(), mIdListStyleMap(), mIdListStorageMap(),
	mTableStyles(), mpCurrentTableStyle(0),
	miFrameNumber(0),  mFrameStyles(), mFrameAutomaticStyles(), mFrameNameIdMap(),
	mDocumentStreamHandlers(), mImageHandlers(), mObjectHandlers()
{
	mListStates.push(ListState());
}

OdfGenerator::~OdfGenerator()
{
	mParagraphManager.clean();
	mSpanManager.clean();
	mFontManager.clean();

	emptyStorage(&mMetaDataStorage);
	emptyStorage(&mBodyStorage);
	for (std::vector<ListStyle *>::iterator iterListStyles = mListStyles.begin();
	        iterListStyles != mListStyles.end(); ++iterListStyles)
		delete(*iterListStyles);
	for (std::vector<TableStyle *>::iterator iterTableStyles = mTableStyles.begin();
	        iterTableStyles != mTableStyles.end(); ++iterTableStyles)
		delete((*iterTableStyles));
	emptyStorage(&mFrameAutomaticStyles);
	emptyStorage(&mFrameStyles);
}

std::string OdfGenerator::getDocumentType(OdfStreamType streamType)
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

void OdfGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		// filter out librevenge elements
		if (strncmp(i.key(), "librevenge", 10) != 0 && strncmp(i.key(), "dcterms", 7) != 0)
		{
			mMetaDataStorage.push_back(new TagOpenElement(i.key()));
			librevenge::RVNGString sStringValue(i()->getStr(), true);
			mMetaDataStorage.push_back(new CharDataElement(sStringValue.cstr()));
			mMetaDataStorage.push_back(new TagCloseElement(i.key()));
		}
	}
}

void OdfGenerator::writeDocumentMetaData(OdfDocumentHandler *pHandler)
{
	if (mMetaDataStorage.empty()) return;
	TagOpenElement("office:meta").write(pHandler);
	sendStorage(&mMetaDataStorage, pHandler);
	pHandler->endElement("office:meta");
}


void OdfGenerator::initStateWith(OdfGenerator const &orig)
{
	mImageHandlers=orig.mImageHandlers;
	mObjectHandlers=orig.mObjectHandlers;
	mIdSpanMap=orig.mIdSpanMap;
	mIdParagraphMap=orig.mIdParagraphMap;
	mIdListStorageMap=orig.mIdListStorageMap;
}

////////////////////////////////////////////////////////////
// storage
////////////////////////////////////////////////////////////
void OdfGenerator::emptyStorage(Storage *storage)
{
	if (!storage)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::sendStorage: called without storage\n"));
		return;
	}
	for (size_t i=0; i<storage->size(); ++i)
	{
		if ((*storage)[i]) delete(*storage)[i];
	}
	storage->resize(0);
}

void OdfGenerator::sendStorage(Storage const *storage, OdfDocumentHandler *pHandler)
{
	if (!storage)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::sendStorage: called without storage\n"));
		return;
	}
	for (size_t i=0; i<storage->size(); ++i)
	{
		if ((*storage)[i])(*storage)[i]->write(pHandler);
	}
}

void OdfGenerator::pushStorage(Storage *newStorage)
{
	if (!newStorage)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::pushStorage: called without storage\n"));
		return;
	}
	mStorageStack.push(mpCurrentStorage);
	mpCurrentStorage=newStorage;
}

bool OdfGenerator::popStorage()
{
	if (mStorageStack.empty())
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::popStorage: the stack is empty\n"));
		return false;
	}
	mpCurrentStorage=mStorageStack.top();
	mStorageStack.pop();
	return false;
}

////////////////////////////////////////////////////////////
// document handler
////////////////////////////////////////////////////////////
void OdfGenerator::addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
{
	if (!pHandler)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::addDocumentHandler: called without handler\n"));
		return;
	}
	mDocumentStreamHandlers[streamType] = pHandler;
}

void  OdfGenerator::writeTargetDocuments()
{
	std::map<OdfStreamType, OdfDocumentHandler *>::const_iterator iter = mDocumentStreamHandlers.begin();
	for (; iter != mDocumentStreamHandlers.end(); ++iter)
		writeTargetDocument(iter->second, iter->first);
}

////////////////////////////////////////////////////////////
// embedded
////////////////////////////////////////////////////////////
OdfEmbeddedObject OdfGenerator::findEmbeddedObjectHandler(const librevenge::RVNGString &mimeType) const
{
	std::map<librevenge::RVNGString, OdfEmbeddedObject, ltstr>::const_iterator i = mObjectHandlers.find(mimeType);
	if (i != mObjectHandlers.end())
		return i->second;

	return 0;
}

OdfEmbeddedImage OdfGenerator::findEmbeddedImageHandler(const librevenge::RVNGString &mimeType) const
{
	std::map<librevenge::RVNGString, OdfEmbeddedImage, ltstr>::const_iterator i = mImageHandlers.find(mimeType);
	if (i != mImageHandlers.end())
		return i->second;

	return 0;
}

void OdfGenerator::registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler)
{
	mObjectHandlers[mimeType]=objectHandler;
}

void OdfGenerator::registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler)
{
	mImageHandlers[mimeType]=imageHandler;
}

////////////////////////////////////////////////////////////
// frame
////////////////////////////////////////////////////////////
void OdfGenerator::openFrame(const librevenge::RVNGPropertyList &propList)
{
	// First, let's create a Frame Style for this box
	TagOpenElement *frameStyleOpenElement = new TagOpenElement("style:style");
	librevenge::RVNGString frameStyleName;
	unsigned objectId = 0;
	if (propList["librevenge:frame-name"])
		objectId= getFrameId(propList["librevenge:frame-name"]->getStr());
	else
		objectId= getFrameId("");
	frameStyleName.sprintf("GraphicFrame_%i", objectId);
	frameStyleOpenElement->addAttribute("style:name", frameStyleName);
	frameStyleOpenElement->addAttribute("style:family", "graphic");

	mFrameStyles.push_back(frameStyleOpenElement);

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

	mFrameStyles.push_back(frameStylePropertiesOpenElement);

	mFrameStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mFrameStyles.push_back(new TagCloseElement("style:style"));

	// Now, let's create an automatic style for this frame
	TagOpenElement *frameAutomaticStyleElement = new TagOpenElement("style:style");
	librevenge::RVNGString frameAutomaticStyleName;
	frameAutomaticStyleName.sprintf("fr%i", objectId);
	frameAutomaticStyleElement->addAttribute("style:name", frameAutomaticStyleName);
	frameAutomaticStyleElement->addAttribute("style:family", "graphic");
	frameAutomaticStyleElement->addAttribute("style:parent-style-name", frameStyleName);

	mFrameAutomaticStyles.push_back(frameAutomaticStyleElement);

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

	mFrameAutomaticStyles.push_back(frameAutomaticStylePropertiesElement);

	mFrameAutomaticStyles.push_back(new TagCloseElement("style:graphic-properties"));

	mFrameAutomaticStyles.push_back(new TagCloseElement("style:style"));

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

	mpCurrentStorage->push_back(drawFrameOpenElement);
}

void OdfGenerator::closeFrame()
{
	mpCurrentStorage->push_back(new TagCloseElement("draw:frame"));
}

unsigned OdfGenerator::getFrameId(librevenge::RVNGString val)
{
	bool hasLabel = val.cstr() && val.len();
	if (hasLabel && mFrameNameIdMap.find(val) != mFrameNameIdMap.end())
		return mFrameNameIdMap.find(val)->second;
	unsigned id=miFrameNumber++;
	if (hasLabel)
		mFrameNameIdMap[val]=id;
	return id;
}

////////////////////////////////////////////////////////////
// span/paragraph
////////////////////////////////////////////////////////////
void OdfGenerator::insertTab()
{
	mpCurrentStorage->push_back(new TagOpenElement("text:tab"));
	mpCurrentStorage->push_back(new TagCloseElement("text:tab"));
}

void OdfGenerator::insertSpace()
{
	mpCurrentStorage->push_back(new TagOpenElement("text:s"));
	mpCurrentStorage->push_back(new TagCloseElement("text:s"));
}

void OdfGenerator::insertLineBreak(bool forceParaClose)
{
	if (!forceParaClose)
	{
		mpCurrentStorage->push_back(new TagOpenElement("text:line-break"));
		mpCurrentStorage->push_back(new TagCloseElement("text:line-break"));
		return;
	}
	closeSpan();
	closeParagraph();

	TagOpenElement *pParagraphOpenElement = new TagOpenElement("text:p");
	if (!mLastParagraphName.empty())
		pParagraphOpenElement->addAttribute("text:style-name", mLastParagraphName.cstr());
	mpCurrentStorage->push_back(pParagraphOpenElement);

	TagOpenElement *pSpanOpenElement = new TagOpenElement("text:span");
	if (!mLastSpanName.empty())
		pSpanOpenElement->addAttribute("text:style-name", mLastSpanName.cstr());
	mpCurrentStorage->push_back(pSpanOpenElement);

}

void OdfGenerator::insertField(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:field-type"] || propList["librevenge:field-type"]->getStr().empty())
		return;

	const librevenge::RVNGString &type = propList["librevenge:field-type"]->getStr();

	TagOpenElement *openElement = new TagOpenElement(type);
	if (type == "text:page-number")
		openElement->addAttribute("text:select-page", "current");

	if (propList["style:num-format"])
		openElement->addAttribute("style:num-format", propList["style:num-format"]->getStr());

	mpCurrentStorage->push_back(openElement);
	mpCurrentStorage->push_back(new TagCloseElement(type));
}

void OdfGenerator::insertText(const librevenge::RVNGString &text)
{
	if (!text.empty())
		mpCurrentStorage->push_back(new TextElement(text));
}

void OdfGenerator::defineCharacterStyle(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:span-id"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::defineCharacterStyle: called without id\n"));
		return;
	}
	mIdSpanMap[propList["librevenge:span-id"]->getInt()]=propList;
}

void OdfGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGString sName("");
	librevenge::RVNGPropertyList pList(propList);
	if (pList["librevenge:span-id"])
	{
		int id=pList["librevenge:span-id"]->getInt();
		if (mIdSpanNameMap.find(id)!=mIdSpanNameMap.end())
			sName=mIdSpanNameMap.find(id)->second;
		else if (mIdSpanMap.find(id)!=mIdSpanMap.end())
			pList=mIdSpanMap.find(id)->second;
		else
		{
			ODFGEN_DEBUG_MSG(("OdfGenerator::openSpan: can not find the style %d\n", id));
			pList.clear();
		}
	}

	if (sName.empty())
	{
		if (pList["style:font-name"])
			mFontManager.findOrAdd(pList["style:font-name"]->getStr().cstr());
		sName = mSpanManager.findOrAdd(pList);
		if (pList["librevenge:span-id"])
			mIdSpanNameMap[pList["librevenge:span-id"]->getInt()]=sName;
	}
	TagOpenElement *pSpanOpenElement = new TagOpenElement("text:span");
	pSpanOpenElement->addAttribute("text:style-name", sName.cstr());
	mpCurrentStorage->push_back(pSpanOpenElement);
	mLastSpanName=sName;
}

void OdfGenerator::closeSpan()
{
	mpCurrentStorage->push_back(new TagCloseElement("text:span"));
}

void OdfGenerator::openLink(const librevenge::RVNGPropertyList &propList)
{
	TagOpenElement *pLinkOpenElement = new TagOpenElement("text:a");
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (!i.child()) // write out simple properties only
			// The string we get here might be url decoded, so
			// sscape characters that might mess up the resulting xml
			pLinkOpenElement->addAttribute(i.key(), librevenge::RVNGString(i()->getStr(), true));
	}
	mpCurrentStorage->push_back(pLinkOpenElement);
}

void OdfGenerator::closeLink()
{
	mpCurrentStorage->push_back(new TagCloseElement("text:a"));
}

void OdfGenerator::defineParagraphStyle(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["librevenge:paragraph-id"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::defineParagraphStyle: called without id\n"));
		return;
	}
	mIdParagraphMap[propList["librevenge:paragraph-id"]->getInt()]=propList;
}

void OdfGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGPropertyList pList(propList);
	librevenge::RVNGString paragraphName("");
	if (pList["librevenge:paragraph-id"])
	{
		int id=pList["librevenge:paragraph-id"]->getInt();
		if (mIdParagraphNameMap.find(id)!=mIdParagraphNameMap.end())
			paragraphName=mIdParagraphNameMap.find(id)->second;
		else if (mIdParagraphMap.find(id)!=mIdParagraphMap.end())
			pList=mIdParagraphMap.find(id)->second;
		else
		{
			ODFGEN_DEBUG_MSG(("OdfGenerator::openParagraph: can not find the style %d\n", id));
			pList.clear();
		}
	}

	if (paragraphName.empty())
	{
		if (pList["style:font-name"])
			mFontManager.findOrAdd(pList["style:font-name"]->getStr().cstr());
		paragraphName = mParagraphManager.findOrAdd(pList);
		if (pList["librevenge:paragraph-id"])
			mIdParagraphNameMap[pList["librevenge:paragraph-id"]->getInt()]=paragraphName;
	}

	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pParagraphOpenElement = new TagOpenElement("text:p");
	pParagraphOpenElement->addAttribute("text:style-name", paragraphName);
	mpCurrentStorage->push_back(pParagraphOpenElement);
	mLastParagraphName=paragraphName;
}

void OdfGenerator::closeParagraph()
{
	mpCurrentStorage->push_back(new TagCloseElement("text:p"));
}

////////////////////////////////////////////////////////////
// list
////////////////////////////////////////////////////////////
OdfGenerator::ListState::ListState() :
	mpCurrentListStyle(0),
	miCurrentListLevel(0),
	miLastListLevel(0),
	miLastListNumber(0),
	mbListContinueNumbering(false),
	mbListElementParagraphOpened(false),
	mbListElementOpened()
{
}

OdfGenerator::ListState::ListState(const OdfGenerator::ListState &state) :
	mpCurrentListStyle(state.mpCurrentListStyle),
	miCurrentListLevel(state.miCurrentListLevel),
	miLastListLevel(state.miCurrentListLevel),
	miLastListNumber(state.miLastListNumber),
	mbListContinueNumbering(state.mbListContinueNumbering),
	mbListElementParagraphOpened(state.mbListElementParagraphOpened),
	mbListElementOpened(state.mbListElementOpened)
{
}

OdfGenerator::ListState &OdfGenerator::getListState()
{
	if (!mListStates.empty()) return mListStates.top();
	ODFGEN_DEBUG_MSG(("OdfGenerator::getListState: call with no state\n"));
	static OdfGenerator::ListState bad;
	return bad;
}

void OdfGenerator::popListState()
{
	if (mListStates.size()>1)
		mListStates.pop();
}

void OdfGenerator::pushListState()
{
	mListStates.push(ListState());
}

void OdfGenerator::defineListLevel(const librevenge::RVNGPropertyList &propList, bool ordered)
{
	int id = -1;
	if (propList["librevenge:list-id"])
		id = propList["librevenge:list-id"]->getInt();
	else if (propList["librevenge:id"]) // REMOVEME
		id = propList["librevenge:id"]->getInt();
	updateListStorage(propList, id, ordered);

	ListStyle *pListStyle = 0;
	ListState &state=getListState();
	// as all direct list have the same id:-1, we reused the last list
	// excepted at level 0 where we force the redefinition of a new list
	if ((id!=-1 || !state.mbListElementOpened.empty()) &&
	        state.mpCurrentListStyle && state.mpCurrentListStyle->getListID() == id)
		pListStyle = state.mpCurrentListStyle;

	// this rather appalling conditional makes sure we only start a
	// new list (rather than continue an old one) if: (1) we have no
	// prior list or the prior list has another listId OR (2) we can
	// tell that the user actually is starting a new list at level 1
	// (and only level 1)
	if (pListStyle == 0 ||
	        (ordered && propList["librevenge:level"] && propList["librevenge:level"]->getInt()==1 &&
	         (propList["text:start-value"] && propList["text:start-value"]->getInt() != int(state.miLastListNumber+1))))
	{
		// first retrieve the displayname
		librevenge::RVNGString displayName("");
		if (propList["style:display-name"])
			displayName=propList["style:display-name"]->getStr();
		else if (pListStyle)
			displayName=pListStyle->getDisplayName()

			            ODFGEN_DEBUG_MSG(("OdfGenerator: Attempting to create a new list style (listid: %i)\n", id));
		librevenge::RVNGString sName;
		if (ordered)
			sName.sprintf("OL%i", miNumListStyles);
		else
			sName.sprintf("UL%i", miNumListStyles);
		miNumListStyles++;

		pListStyle = new ListStyle(sName.cstr(), id);
		if (!displayName.empty())
			pListStyle->setDisplayName(displayName.cstr());
		storeListStyle(pListStyle);
		if (ordered)
		{
			state.mbListContinueNumbering = false;
			state.miLastListNumber = 0;
		}
	}
	else if (ordered)
		state.mbListContinueNumbering = true;

	if (!propList["librevenge:level"])
		return;
	// Iterate through ALL list styles with the same WordPerfect list id and define a level if it is not already defined
	// This solves certain problems with lists that start and finish without reaching certain levels and then begin again
	// and reach those levels. See gradguide0405_PC.wpd in the regression suite
	for (std::vector<ListStyle *>::iterator iterListStyles = mListStyles.begin(); iterListStyles != mListStyles.end(); ++iterListStyles)
	{
		if ((* iterListStyles) && (* iterListStyles)->getListID() == id)
			(* iterListStyles)->updateListLevel((propList["librevenge:level"]->getInt() - 1), propList, ordered);
	}
}

void OdfGenerator::openListLevel(const librevenge::RVNGPropertyList &propList, bool ordered)
{
	ListState &state=getListState();
	if (state.mbListElementParagraphOpened)
	{
		closeParagraph();
		state.mbListElementParagraphOpened = false;
	}
	librevenge::RVNGPropertyList pList(propList);
	if (!pList["librevenge:level"])
		pList.insert("librevenge:level", int(state.mbListElementOpened.size())+1);

	int id=-1;
	if (propList["librevenge:list-id"])
		id = propList["librevenge:list-id"]->getInt();
	else if (propList["librevenge:id"]) // REMOVEME
		id = propList["librevenge:id"]->getInt();

	if (id==-1)
		defineListLevel(pList, ordered);
	else if (state.mbListElementOpened.empty())
	{
		// first item of a list, be sure to use the list with given id
		retrieveListStyle(id);
	}
	// check if the list level is defined
	if (state.mpCurrentListStyle &&
	        !state.mpCurrentListStyle->isListLevelDefined(pList["librevenge:level"]->getInt()-1))
	{
		int level=pList["librevenge:level"]->getInt();
		ListStorage &list=getListStorage(id);
		if (list.mLevelMap.find(level) != list.mLevelMap.end())
			defineListLevel(list.mLevelMap.find(level)->second.mLevel, ordered);
	}

	TagOpenElement *pListLevelOpenElement = new TagOpenElement("text:list");
	if (!state.mbListElementOpened.empty() && !state.mbListElementOpened.top())
	{
		mpCurrentStorage->push_back(new TagOpenElement("text:list-item"));
		state.mbListElementOpened.top() = true;
	}

	state.mbListElementOpened.push(false);
	if (state.mbListElementOpened.size() == 1)
	{
		// add a sanity check ( to avoid a crash if mpCurrentListStyle is NULL)
		if (state.mpCurrentListStyle)
			pListLevelOpenElement->addAttribute("text:style-name", state.mpCurrentListStyle->getName());
	}

	if (ordered && state.mbListContinueNumbering)
		pListLevelOpenElement->addAttribute("text:continue-numbering", "true");
	mpCurrentStorage->push_back(pListLevelOpenElement);
}

void OdfGenerator::closeListLevel()
{
	ListState &state=getListState();
	if (state.mbListElementOpened.empty())
	{
		// this implies that openListLevel was not called, so it is better to stop here
		ODFGEN_DEBUG_MSG(("OdfGenerator: Attempting to close an unexisting level\n"));
		return;
	}
	if (state.mbListElementOpened.top())
	{
		mpCurrentStorage->push_back(new TagCloseElement("text:list-item"));
		state.mbListElementOpened.top() = false;
	}

	mpCurrentStorage->push_back(new TagCloseElement("text:list"));
	state.mbListElementOpened.pop();
}

void OdfGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	ListState &state=getListState();
	state.miLastListLevel = state.miCurrentListLevel;
	if (state.miCurrentListLevel == 1)
		state.miLastListNumber++;

	if (state.mbListElementOpened.top())
	{
		mpCurrentStorage->push_back(new TagCloseElement("text:list-item"));
		state.mbListElementOpened.top() = false;
	}

	librevenge::RVNGPropertyList finalPropList(propList);
#if 0
	// this property is ignored in TextRunStyle.c++
	if (state.mpCurrentListStyle)
		finalPropList.insert("style:list-style-name", state.mpCurrentListStyle->getName());
#endif
	finalPropList.insert("style:parent-style-name", "Standard");
	librevenge::RVNGString paragName = getParagraphName(finalPropList);

	TagOpenElement *pOpenListItem = new TagOpenElement("text:list-item");
	if (propList["text:start-value"] && propList["text:start-value"]->getInt() > 0)
		pOpenListItem->addAttribute("text:start-value", propList["text:start-value"]->getStr());
	mpCurrentStorage->push_back(pOpenListItem);

	TagOpenElement *pOpenListElementParagraph = new TagOpenElement("text:p");
	pOpenListElementParagraph->addAttribute("text:style-name", paragName);
	mpCurrentStorage->push_back(pOpenListElementParagraph);

	state.mbListElementOpened.top() = true;
	state.mbListElementParagraphOpened = true;
	state.mbListContinueNumbering = false;
}

void OdfGenerator::closeListElement()
{
	// this code is kind of tricky, because we don't actually close the list element (because this list element
	// could contain another list level in OOo's implementation of lists). that is done in the closeListLevel
	// code (or when we open another list element)
	if (getListState().mbListElementParagraphOpened)
	{
		closeParagraph();
		getListState().mbListElementParagraphOpened = false;
	}
}

void OdfGenerator::storeListStyle(ListStyle *listStyle)
{
	if (!listStyle || listStyle == getListState().mpCurrentListStyle)
		return;
	mListStyles.push_back(listStyle);
	getListState().mpCurrentListStyle = listStyle;
	mIdListStyleMap[listStyle->getListID()]=listStyle;
}

void OdfGenerator::retrieveListStyle(int id)
{
	// first look if the current style is ok
	if (getListState().mpCurrentListStyle &&
	        id == getListState().mpCurrentListStyle->getListID())
		return;

	// use the global map
	if (mIdListStyleMap.find(id) != mIdListStyleMap.end())
	{
		getListState().mpCurrentListStyle = mIdListStyleMap.find(id)->second;
		return;
	}

	ODFGEN_DEBUG_MSG(("OdfGenerator: impossible to find a list with id=%d\n",id));
}

OdfGenerator::ListStorage &OdfGenerator::getListStorage(int id)
{
	if (mIdListStorageMap.find(id)!=mIdListStorageMap.end())
		return mIdListStorageMap.find(id)->second;
	mIdListStorageMap[id]=ListStorage();
	return  mIdListStorageMap.find(id)->second;
}

void OdfGenerator::updateListStorage(const librevenge::RVNGPropertyList &level, int id, bool ordered)
{
	if (!level["librevenge:level"]) return;
	ListStorage &list=getListStorage(id);
	list.mLevelMap[level["librevenge:level"]->getInt()]=ListStorage::Level(level, ordered);
}

////////////////////////////////////////////////////////////
// table
////////////////////////////////////////////////////////////
void OdfGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	librevenge::RVNGString sTableName;
	sTableName.sprintf("Table%i", mTableStyles.size());

	// FIXME: we base the table style off of the page's margin left, ignoring (potential) wordperfect margin
	// state which is transmitted inside the page. could this lead to unacceptable behaviour?
	TableStyle *pTableStyle = new TableStyle(propList, sTableName.cstr());

	mTableStyles.push_back(pTableStyle);

	mpCurrentTableStyle = pTableStyle;

	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");

	pTableOpenElement->addAttribute("table:name", sTableName.cstr());
	pTableOpenElement->addAttribute("table:style-name", sTableName.cstr());
	mpCurrentStorage->push_back(pTableOpenElement);

	for (int i=0; i<pTableStyle->getNumColumns(); ++i)
	{
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		librevenge::RVNGString sColumnStyleName;
		sColumnStyleName.sprintf("%s.Column%i", sTableName.cstr(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
		mpCurrentStorage->push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpCurrentStorage->push_back(pTableColumnCloseElement);
	}
}

void OdfGenerator::closeTable()
{
	if (!mpCurrentTableStyle)
		return;
	mpCurrentStorage->push_back(new TagCloseElement("table:table"));
}

bool OdfGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	if (!mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openTableRow called with no table\n"));
		return false;
	}
	if (propList["librevenge:is-header-row"] && (propList["librevenge:is-header-row"]->getInt()))
		mpCurrentStorage->push_back(new TagOpenElement("table:table-header-rows"));

	librevenge::RVNGString sTableRowStyleName;
	sTableRowStyleName.sprintf("%s.Row%i", mpCurrentTableStyle->getName().cstr(), mpCurrentTableStyle->getNumTableRowStyles());
	TableRowStyle *pTableRowStyle = new TableRowStyle(propList, sTableRowStyleName.cstr());
	mpCurrentTableStyle->addTableRowStyle(pTableRowStyle);

	TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
	pTableRowOpenElement->addAttribute("table:style-name", sTableRowStyleName);
	mpCurrentStorage->push_back(pTableRowOpenElement);
	return true;
}

void OdfGenerator::closeTableRow(bool isHeaderRow)
{
	if (!mpCurrentTableStyle)
		return;

	mpCurrentStorage->push_back(new TagCloseElement("table:table-row"));
	if (isHeaderRow)
		mpCurrentStorage->push_back(new TagCloseElement("table:table-header-rows"));
}

bool OdfGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (!mpCurrentTableStyle)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openTableCell called with no table\n"));
		return false;
	}

	librevenge::RVNGString sTableCellStyleName;
	sTableCellStyleName.sprintf("%s.Cell%i", mpCurrentTableStyle->getName().cstr(), mpCurrentTableStyle->getNumTableCellStyles());
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
	mpCurrentStorage->push_back(pTableCellOpenElement);
	return true;
}

void OdfGenerator::closeTableCell()
{
	if (!mpCurrentTableStyle)
		return;

	mpCurrentStorage->push_back(new TagCloseElement("table:table-cell"));
}

void OdfGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &)
{
	if (!mpCurrentTableStyle)
		return;

	mpCurrentStorage->push_back(new TagOpenElement("table:covered-table-cell"));
	mpCurrentStorage->push_back(new TagCloseElement("table:covered-table-cell"));
}

////////////////////////////////////////////////////////////
// image/embedded
////////////////////////////////////////////////////////////
void OdfGenerator::insertBinaryObject(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["office:binary-data"] || !propList["librevenge:mime-type"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::addDocumentHandler: can not find data or mimetype\n"));
		return;
	}

	OdfEmbeddedObject tmpObjectHandler = findEmbeddedObjectHandler(propList["librevenge:mime-type"]->getStr());
	OdfEmbeddedImage tmpImageHandler = findEmbeddedImageHandler(propList["librevenge:mime-type"]->getStr());

	if (tmpObjectHandler || tmpImageHandler)
	{
		librevenge::RVNGBinaryData data(propList["office:binary-data"]->getStr());
		if (tmpObjectHandler)
		{
			std::vector<DocumentElement *> tmpContentElements;
			InternalHandler tmpHandler(&tmpContentElements);


			if (tmpObjectHandler(data, &tmpHandler, ODF_FLAT_XML) && !tmpContentElements.empty())
			{
				mpCurrentStorage->push_back(new TagOpenElement("draw:object"));
				for (std::vector<DocumentElement *>::const_iterator iter = tmpContentElements.begin(); iter != tmpContentElements.end(); ++iter)
					mpCurrentStorage->push_back(*iter);
				mpCurrentStorage->push_back(new TagCloseElement("draw:object"));
			}
		}
		if (tmpImageHandler)
		{
			librevenge::RVNGBinaryData output;
			if (tmpImageHandler(data, output))
			{
				mpCurrentStorage->push_back(new TagOpenElement("draw:image"));

				mpCurrentStorage->push_back(new TagOpenElement("office:binary-data"));

				librevenge::RVNGString binaryBase64Data = output.getBase64Data();

				mpCurrentStorage->push_back(new CharDataElement(binaryBase64Data.cstr()));

				mpCurrentStorage->push_back(new TagCloseElement("office:binary-data"));

				mpCurrentStorage->push_back(new TagCloseElement("draw:image"));
			}
		}
	}
	else
		// assuming we have a binary image or a object_ole that we can just insert as it is
	{
		if (propList["librevenge:mime-type"]->getStr() == "object/ole")
			mpCurrentStorage->push_back(new TagOpenElement("draw:object-ole"));
		else
			mpCurrentStorage->push_back(new TagOpenElement("draw:image"));

		mpCurrentStorage->push_back(new TagOpenElement("office:binary-data"));

		mpCurrentStorage->push_back(new CharDataElement(propList["office:binary-data"]->getStr().cstr()));

		mpCurrentStorage->push_back(new TagCloseElement("office:binary-data"));

		if (propList["librevenge:mime-type"]->getStr() == "object/ole")
			mpCurrentStorage->push_back(new TagCloseElement("draw:object-ole"));
		else
			mpCurrentStorage->push_back(new TagCloseElement("draw:image"));
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
