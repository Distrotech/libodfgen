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

#include "OdfGenerator.hxx"

OdfGenerator::OdfGenerator() :
	mpCurrentStorage(&mBodyStorage), mStorageStack(), mMetaDataStorage(), mBodyStorage(),
	mFontManager(), mSpanManager(), mParagraphManager(),
	mIdSpanMap(), mIdSpanNameMap(), mIdParagraphMap(), mIdParagraphNameMap(),
	miNumListStyles(0), mListStyles(), mListStates(), mIdListStyleMap(), mIdListStorageMap(),
	miFrameNumber(0), mFrameNameIdMap(),
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

void OdfGenerator::insertLineBreak()
{
	mpCurrentStorage->push_back(new TagOpenElement("text:line-break"));
	mpCurrentStorage->push_back(new TagCloseElement("text:line-break"));
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
}

void OdfGenerator::closeSpan()
{
	mpCurrentStorage->push_back(new TagCloseElement("text:span"));
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
	if (propList["librevenge:id"])
		id = propList["librevenge:id"]->getInt();
	updateListStorage(propList, id, ordered);

	ListStyle *pListStyle = 0;
	ListState &state=getListState();
	if (state.mpCurrentListStyle && state.mpCurrentListStyle->getListID() == id)
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
		ODFGEN_DEBUG_MSG(("OdfGenerator: Attempting to create a new list style (listid: %i)\n", id));
		librevenge::RVNGString sName;
		if (ordered)
			sName.sprintf("OL%i", miNumListStyles);
		else
			sName.sprintf("UL%i", miNumListStyles);
		miNumListStyles++;
		pListStyle = new ListStyle(sName.cstr(), id);
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
	if (!propList["librevenge:id"])
		// suppose that propList define the style
		defineListLevel(propList, ordered);
	else if (state.mbListElementOpened.empty())
	{
		// first item of a list, be sure to use the list with given id
		retrieveListStyle(propList["librevenge:id"]->getInt());
	}
	// check if the list level is defined
	if (propList["librevenge:level"] && state.mpCurrentListStyle &&
	        !state.mpCurrentListStyle->isListLevelDefined(propList["librevenge:level"]->getInt()-1))
	{
		int id=propList["librevenge:id"] ? propList["librevenge:id"]->getInt() : -1;
		int level=propList["librevenge:level"]->getInt();
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
// list
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
