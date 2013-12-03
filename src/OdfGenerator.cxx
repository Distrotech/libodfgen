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

#include "OdfGenerator.hxx"

OdfGenerator::OdfGenerator() :
	mpCurrentStorage(&mBodyStorage), mStorageStack(), mMetaDataStorage(), mBodyStorage(),
	mFontManager(), mSpanManager(), mParagraphManager(),
	mIdSpanMap(), mIdSpanNameMap(), mIdParagraphMap(), mIdParagraphNameMap(),
	mIdListStorageMap(),	miFrameNumber(0), mFrameNameIdMap(),
	mImageHandlers(), mObjectHandlers()
{
}

OdfGenerator::~OdfGenerator()
{
	mParagraphManager.clean();
	mSpanManager.clean();
	mFontManager.clean();


	emptyStorage(&mMetaDataStorage);
	emptyStorage(&mBodyStorage);
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

void OdfGenerator::initStateWith(OdfGenerator const &orig)
{
	mImageHandlers=orig.mImageHandlers;
	mObjectHandlers=orig.mObjectHandlers;
	mIdSpanMap=orig.mIdSpanMap;
	mIdParagraphMap=orig.mIdParagraphMap;
	mIdListStorageMap=orig.mIdListStorageMap;
	std::map<int, ListStorage>::iterator it=mIdListStorageMap.begin();
	for (; it != mIdListStorageMap.end() ; ++it)
		it->second.mbUsed=false;
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
	librevenge::RVNGString out;
	librevenge::RVNGString::Iter i(text);
	for (i.rewind(); i.next();)
	{
		if ((*i()) == '\n' || (*i()) == '\t')
		{
			if (out.len() != 0)
			{
				mpCurrentStorage->push_back(new TextElement(out));
				out.clear();
			}
			if ((*i()) == '\n')
			{
				mpCurrentStorage->push_back(new TagOpenElement("text:line-break"));
				mpCurrentStorage->push_back(new TagCloseElement("text:line-break"));
			}
			else if ((*i()) == '\t')
			{
				mpCurrentStorage->push_back(new TagOpenElement("text:tab"));
				mpCurrentStorage->push_back(new TagCloseElement("text:tab"));
			}
		}
		else
			out.append(i());
	}
	if (out.len() != 0)
		mpCurrentStorage->push_back(new TextElement(out));
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
OdfGenerator::ListStorage &OdfGenerator::getList(int id)
{
	if (mIdListStorageMap.find(id)!=mIdListStorageMap.end())
		return mIdListStorageMap.find(id)->second;
	mIdListStorageMap[id]=ListStorage();
	return  mIdListStorageMap.find(id)->second;
}

void OdfGenerator::storeLevel(int id, const librevenge::RVNGPropertyList &level, bool ordered)
{
	if (!level["librevenge:level"]) return;
	ListStorage &list=getList(id);
	list.mLevelMap[level["librevenge:level"]->getInt()]=ListStorage::Level(level, ordered);
}

void OdfGenerator::updateListLevelProperty(int id, bool ordered, librevenge::RVNGPropertyList &pList) const
{
	if (!pList["librevenge:level"] || mIdListStorageMap.find(id)==mIdListStorageMap.end()) return;
	ListStorage const &list=mIdListStorageMap.find(id)->second;
	int lvl=pList["librevenge:level"]->getInt();
	std::map<int, ListStorage::Level>::const_iterator it=list.mLevelMap.begin();
	while (it!=list.mLevelMap.end())
	{
		if (it->first!=lvl)
		{
			++it;
			continue;
		}
		ListStorage::Level const &level=it++->second;
		if (level.mbOrdered!=ordered) continue;
		librevenge::RVNGPropertyList::Iter i(level.mLevel);
		for (i.rewind(); i.next();)
		{
			if (pList[i.key()] || i.child()) continue;
			pList.insert(i.key(), i()->clone());
		}
	}
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
