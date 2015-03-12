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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include <limits>
#include <string>

#include <librevenge/librevenge.h>

#include "DocumentElement.hxx"
#include "GraphicFunctions.hxx"
#include "InternalHandler.hxx"
#include "ListStyle.hxx"
#include "TableStyle.hxx"

#include "OdfGenerator.hxx"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace libodfgen;

OdfGenerator::OdfGenerator() :
	mpCurrentStorage(&mBodyStorage), mStorageStack(), mMetaDataStorage(), mBodyStorage(),
	mPageSpanManager(), mFontManager(), mGraphicManager(), mSpanManager(),
	mParagraphManager(), mListManager(), mTableManager(),
	mbInHeaderFooter(false), mbInMasterPage(false),
	mIdSpanMap(), mIdSpanNameMap(), mLastSpanName(""),
	mIdParagraphMap(), mIdParagraphNameMap(), mLastParagraphName(""),
	miFrameNumber(0),  mFrameNameIdMap(),
	mLayerNameStack(), mLayerNameSet(), mLayerNameMap(),
	mGraphicStyle(),
	mIdChartMap(), mIdChartNameMap(),
	mDocumentStreamHandlers(),
	miObjectNumber(1), mNameObjectMap(),
	mImageHandlers(), mObjectHandlers(),
	mCurrentParaIsHeading(false)
{
}

OdfGenerator::~OdfGenerator()
{
	mParagraphManager.clean();
	mSpanManager.clean();
	mFontManager.clean();
	mGraphicManager.clean();
	mTableManager.clean();
	std::map<librevenge::RVNGString, ObjectContainer *>::iterator it;
	for (it=mNameObjectMap.begin(); it!=mNameObjectMap.end(); ++it)
	{
		if (it->second)
			delete it->second;
	}
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
	case ODF_MANIFEST_XML:
	default:
		return "office:document";
	}
}

void OdfGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	std::string generator;

	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		// filter out librevenge elements
		if (strncmp(i.key(), "librevenge:", 11) && strncmp(i.key(), "dcterms:", 8))
		{
			if (strncmp(i.key(), "meta:generator", 14))
			{
				mMetaDataStorage.push_back(new TagOpenElement(i.key()));
				mMetaDataStorage.push_back(new CharDataElement(i()->getStr().cstr()));
				mMetaDataStorage.push_back(new TagCloseElement(i.key()));
			}
			else
			{
				generator = i()->getStr().cstr();
			}
		}
		else if (strncmp(i.key(), "librevenge:template", 19) == 0)
		{
			librevenge::RVNGString elementName = "meta:template";
			TagOpenElement *element = new TagOpenElement(elementName);
			element->addAttribute("xlink:type", "simple");
			element->addAttribute("xlink:actuate", "onRequest");
			element->addAttribute("xlink:title", i()->getStr().cstr());
			element->addAttribute("xlink:href", "");
			mMetaDataStorage.push_back(element);
			mMetaDataStorage.push_back(new TagCloseElement(elementName));
		}
		else if (strncmp(i.key(), "librevenge:", 11) == 0)
		{
			// convert to <meta:user-defined meta:name="some_metadata">
			librevenge::RVNGString elementName = "meta:user-defined";
			TagOpenElement *element = new TagOpenElement(elementName);
			std::string user_defined(i.key());
			size_t found = user_defined.find_last_of(":");
			if (found != std::string::npos)
				user_defined = user_defined.substr(found+1);
			element->addAttribute("meta:name", user_defined.c_str());
			mMetaDataStorage.push_back(element);
			mMetaDataStorage.push_back(new CharDataElement(i()->getStr().cstr()));
			mMetaDataStorage.push_back(new TagCloseElement(elementName));
		}
	}

#ifdef VERSION
	const std::string version(VERSION);
#else
	const std::string version("unknown");
#endif
	if (generator.empty())
	{
		generator = "libodfgen/" + version;
	}
	else
	{
		generator += " (";
		generator += "libodfgen/" + version;
		generator += ")";
	}

	mMetaDataStorage.push_back(new TagOpenElement("meta:generator"));
	mMetaDataStorage.push_back(new CharDataElement(generator.c_str()));
	mMetaDataStorage.push_back(new TagCloseElement("meta:generator"));
}

void OdfGenerator::writeDocumentMetaData(OdfDocumentHandler *pHandler)
{
	if (mMetaDataStorage.empty()) return;
	TagOpenElement("office:meta").write(pHandler);
	sendStorage(&mMetaDataStorage, pHandler);
	pHandler->endElement("office:meta");
}

void OdfGenerator::appendFilesInManifest(OdfDocumentHandler *pHandler)
{
	std::map<OdfStreamType, OdfDocumentHandler *>::const_iterator iter = mDocumentStreamHandlers.begin();
	for (; iter != mDocumentStreamHandlers.end(); ++iter)
	{
		std::string name("");
		switch (iter->first)
		{
		case ODF_CONTENT_XML:
			name="content.xml";
			break;
		case ODF_META_XML:
			name="meta.xml";
			break;
		case ODF_STYLES_XML:
			name="styles.xml";
			break;
		case ODF_SETTINGS_XML:
			name="settings.xml";
			break;
		case ODF_FLAT_XML:
		case ODF_MANIFEST_XML:
		default:
			break;
		}
		if (name.empty())
			continue;

		TagOpenElement file("manifest:file-entry");
		file.addAttribute("manifest:media-type","text/xml");
		file.addAttribute("manifest:full-path", name.c_str());
		file.write(pHandler);
		TagCloseElement("manifest:file-entry").write(pHandler);
	}
	std::map<librevenge::RVNGString, ObjectContainer *>::const_iterator oIt;
	for (oIt=mNameObjectMap.begin(); oIt!=mNameObjectMap.end(); ++oIt)
	{
		if (!oIt->second) continue;

		TagOpenElement file("manifest:file-entry");
		file.addAttribute("manifest:media-type",oIt->second->mType);
		file.addAttribute("manifest:full-path", oIt->first);
		file.write(pHandler);
		TagCloseElement("manifest:file-entry").write(pHandler);
	}

}

void OdfGenerator::initStateWith(OdfGenerator const &orig)
{
	mImageHandlers=orig.mImageHandlers;
	mObjectHandlers=orig.mObjectHandlers;
	mIdSpanMap=orig.mIdSpanMap;
	mIdParagraphMap=orig.mIdParagraphMap;
	mIdChartMap=orig.mIdChartMap;
}

////////////////////////////////////////////////////////////
// object
////////////////////////////////////////////////////////////
OdfGenerator::ObjectContainer::~ObjectContainer()
{
}

OdfGenerator::ObjectContainer &OdfGenerator::createObjectFile
(librevenge::RVNGString const &objectName, librevenge::RVNGString const &objectType, bool isDir)
{
	ObjectContainer *res=new ObjectContainer(objectType, isDir);
	mNameObjectMap[objectName]=res;
	return *res;
}

librevenge::RVNGStringVector OdfGenerator::getObjectNames() const
{
	librevenge::RVNGStringVector res;
	std::map<librevenge::RVNGString, ObjectContainer *>::const_iterator it;
	for (it=mNameObjectMap.begin(); it!=mNameObjectMap.end(); ++it)
	{
		if (!it->second || it->second->mIsDir) continue;
		res.append(it->first);
	}
	return res;
}

bool OdfGenerator::getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler)
{
	if (!pHandler) return false;
	std::map<librevenge::RVNGString, ObjectContainer *>::iterator it=mNameObjectMap.find(objectName);
	if (it==mNameObjectMap.end() || !it->second)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::getObjectContent: can not find object %s\n", objectName.cstr()));
		return false;
	}
	pHandler->startDocument();
	ObjectContainer &object=*(it->second);
	for (size_t i=0; i<object.mStorage.size(); ++i)
	{
		if (!object.mStorage[i]) continue;
		object.mStorage[i]->write(pHandler);
	}
	pHandler->endDocument();
	return true;
}

////////////////////////////////////////////////////////////
// storage
////////////////////////////////////////////////////////////
void OdfGenerator::sendStorage(libodfgen::DocumentElementVector const *storage, OdfDocumentHandler *pHandler)
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

void OdfGenerator::pushStorage(libodfgen::DocumentElementVector *newStorage)
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
	std::map<librevenge::RVNGString, OdfEmbeddedObject>::const_iterator i = mObjectHandlers.find(mimeType);
	if (i != mObjectHandlers.end())
		return i->second;

	return 0;
}

OdfEmbeddedImage OdfGenerator::findEmbeddedImageHandler(const librevenge::RVNGString &mimeType) const
{
	std::map<librevenge::RVNGString, OdfEmbeddedImage>::const_iterator i = mImageHandlers.find(mimeType);
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
// page function
////////////////////////////////////////////////////////////
void OdfGenerator::startHeaderFooter(bool , const librevenge::RVNGPropertyList &)
{
	if (mbInHeaderFooter)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::startHeaderFooter: a master page is already open\n"));
		return;
	}
	mbInHeaderFooter=true;
}

void OdfGenerator::endHeaderFooter()
{
	if (!mbInHeaderFooter)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::endHeaderFooter: can not find any open master page\n"));
		return;
	}
	mbInHeaderFooter=false;
}

void OdfGenerator::startMasterPage(const librevenge::RVNGPropertyList &)
{
	if (mbInMasterPage)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::startMasterPage: a master page is already open\n"));
		return;
	}
	mbInMasterPage=true;
}

void OdfGenerator::endMasterPage()
{
	if (!mbInMasterPage)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::endMasterPage: can not find any open master page\n"));
		return;
	}
	mbInMasterPage=false;
}

////////////////////////////////////////////////////////////
// frame/group
////////////////////////////////////////////////////////////
void OdfGenerator::openFrame(const librevenge::RVNGPropertyList &propList)
{
	// First, let's create a basic Style for this box
	librevenge::RVNGPropertyList style;
	if (propList["style:horizontal-pos"])
		style.insert("style:horizontal-pos", propList["style:horizontal-pos"]->getStr());
	else
		style.insert("style:horizontal-rel", "left");
	if (propList["style:horizontal-rel"])
		style.insert("style:horizontal-rel", propList["style:horizontal-rel"]->getStr());
	else
		style.insert("style:horizontal-rel", "paragraph");
	if (propList["style:vertical-pos"])
		style.insert("style:vertical-pos", propList["style:vertical-pos"]->getStr());
	else
		style.insert("style:vertical-rel", "top");
	if (propList["style:vertical-rel"])
		style.insert("style:vertical-rel", propList["style:vertical-rel"]->getStr());
	else
		style.insert("style:vertical-rel", "page-content");
	librevenge::RVNGString frameStyleName=mGraphicManager.findOrAdd(style, Style::Z_Style);

	librevenge::RVNGPropertyList graphic;
	mGraphicManager.addGraphicProperties(propList, graphic);
	// we do not want to add the default solid stroke if there are some double/... borders
	if (!propList["draw:stroke"])
		graphic.remove("draw:stroke");
	mGraphicManager.addFrameProperties(propList, graphic);
	graphic.insert("style:parent-style-name", frameStyleName);
	graphic.insert("draw:ole-draw-aspect", "1");
	librevenge::RVNGString frameAutomaticStyleName=
	    mGraphicManager.findOrAdd(graphic, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic);

	// And write the frame itself
	unsigned objectId = 0;
	if (propList["librevenge:frame-name"])
		objectId= getFrameId(propList["librevenge:frame-name"]->getStr());
	else
		objectId= getFrameId("");
	TagOpenElement *drawFrameOpenElement = new TagOpenElement("draw:frame");
	drawFrameOpenElement->addAttribute("draw:style-name", frameAutomaticStyleName);
	librevenge::RVNGString objectName;
	objectName.sprintf("Object%i", objectId);
	drawFrameOpenElement->addAttribute("draw:name", objectName);
	if (propList["svg:x"])
		drawFrameOpenElement->addAttribute("svg:x", propList["svg:x"]->getStr());
	if (propList["svg:y"])
		drawFrameOpenElement->addAttribute("svg:y", propList["svg:y"]->getStr());
	addFrameProperties(propList, *drawFrameOpenElement);
	mpCurrentStorage->push_back(drawFrameOpenElement);
}

void OdfGenerator::closeFrame()
{
	mpCurrentStorage->push_back(new TagCloseElement("draw:frame"));
}

void OdfGenerator::addFrameProperties(const librevenge::RVNGPropertyList &propList, TagOpenElement &element) const
{
	static char const *(frameAttrib[])=
	{
		"draw:z-index",
		"fo:max-width", "fo:max-height",
		"style:rel-width", "style:rel-height", // checkme
		"text:anchor-page-number", "text:anchor-type"
	};
	for (int i=0; i<7; ++i)
	{
		if (propList[frameAttrib[i]])
			element.addAttribute(frameAttrib[i], propList[frameAttrib[i]]->getStr());
	}

	if (propList["svg:width"])
		element.addAttribute("svg:width", propList["svg:width"]->getStr());
	else if (propList["fo:min-width"]) // fixme: must be an attribute of draw:text-box
		element.addAttribute("fo:min-width", propList["fo:min-width"]->getStr());
	if (propList["svg:height"])
		element.addAttribute("svg:height", propList["svg:height"]->getStr());
	else if (propList["fo:min-height"]) // fixme: must be an attribute of draw:text-box
		element.addAttribute("fo:min-height", propList["fo:min-height"]->getStr());
	element.addAttribute("draw:layer", getLayerName(propList));
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

void OdfGenerator::openGroup(const librevenge::RVNGPropertyList &propList)
{
	TagOpenElement *groupElement=new TagOpenElement("draw:g");
	addFrameProperties(propList, *groupElement);
	mpCurrentStorage->push_back(groupElement);
}

void OdfGenerator::closeGroup()
{
	mpCurrentStorage->push_back(new TagCloseElement("draw:g"));
}

librevenge::RVNGString OdfGenerator::getLayerName(const librevenge::RVNGPropertyList &propList) const
{
	// layer does not seems to works in masterpage
	if (inMasterPage())
		return "layout";

	if (propList["draw:layer"] && !propList["draw:layer"]->getStr().empty())
	{
		librevenge::RVNGString layer;
		layer.appendEscapedXML(propList["draw:layer"]->getStr());
		if (mLayerNameMap.find(layer)!=mLayerNameMap.end())
			return mLayerNameMap.find(layer)->second;
		ODFGEN_DEBUG_MSG(("OdfGenerator::getLayerName: called with not existing layer, returns the current layer name\n"));
	}
	if (mLayerNameStack.empty())
		return "layout";
	return mLayerNameStack.top();
}

void OdfGenerator::openLayer(const librevenge::RVNGPropertyList &propList)
{
	if (inMasterPage())
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openLayer: can not create layer in master page\n"));
		mLayerNameStack.push("layout");
		return;
	}
	librevenge::RVNGString layerName("");
	if (propList["draw:layer"])
		layerName=propList["draw:layer"]->getStr();
	else if (propList["svg:id"])
		layerName=propList["svg:id"]->getStr();
	if (layerName.empty())
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openLayer: can not find the layer name\n"));
		mLayerNameStack.push("layout");
		return;
	}
	librevenge::RVNGString layer;
	layer.appendEscapedXML(layerName);
	if (mLayerNameSet.find(layer)!=mLayerNameSet.end())
	{
		// try to find a new name
		ODFGEN_DEBUG_MSG(("OdfGenerator::openLayer: called with an existing name, try to find a new name\n"));
		bool ok=false;
		for (int i=0; i<100; ++i)
		{
			librevenge::RVNGString suffix;
			suffix.sprintf("#%d", i);
			librevenge::RVNGString newName(layer);
			newName.append(suffix);
			if (mLayerNameSet.find(newName)!=mLayerNameSet.end())
				continue;
			mLayerNameMap[layer]=newName;
			layer=newName;
			ok=true;
			break;
		}
		if (!ok)
		{
			ODFGEN_DEBUG_MSG(("OdfGenerator::openLayer: can not find a new name, used old\n"));
		}
	}
	else
		mLayerNameMap[layer]=layer;
	mLayerNameSet.insert(layer);
	mLayerNameStack.push(layer);
}

void OdfGenerator::closeLayer()
{
	if (mLayerNameStack.empty())
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::closeLayer: open layer is not called\n"));
		return;
	}
	mLayerNameStack.pop();
}

void OdfGenerator::appendLayersMasterStyles(OdfDocumentHandler *pHandler)
{
	if (mLayerNameSet.empty()) return;

	TagOpenElement("draw:layer-set").write(pHandler);

	TagOpenElement layer("draw:layer");

	// add the default layers
	for (int i=0; i<5; ++i)
	{
		static char const *(defaults[])= {"layout", "background", "backgroundobjects", "controls", "measurelines"};
		if (mLayerNameSet.find(defaults[i])!=mLayerNameSet.end())
			continue;
		layer.addAttribute("draw:name", defaults[i]);
		layer.write(pHandler);
		TagCloseElement("draw:layer").write(pHandler);
	}
	for (std::set<librevenge::RVNGString>::const_iterator it=mLayerNameSet.begin(); it!=mLayerNameSet.end(); ++it)
	{
		layer.addAttribute("draw:name", *it);
		layer.write(pHandler);
		TagCloseElement("draw:layer").write(pHandler);
	}
	TagCloseElement("draw:layer-set").write(pHandler);
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
		sName = mSpanManager.findOrAdd(pList, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_Unknown);
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
	if (!propList["librevenge:type"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openLink: linked type is not defined, assume link\n"));
	}
	TagOpenElement *pLinkOpenElement = new TagOpenElement("text:a");
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (!i.child()) // write out simple properties only
			// The string we get here might be url decoded, so
			// sscape characters that might mess up the resulting xml
			pLinkOpenElement->addAttribute(i.key(), librevenge::RVNGString::escapeXML(i()->getStr()));
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
	bool isMasterPage=(propList["style:master-page-name"]!=0);

	if (propList["text:outline-level"])
		pList.insert("style:default-outline-level", propList["text:outline-level"]->clone());
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
		if (isMasterPage)
			pList.insert("style:master-page-name", propList["style:master-page-name"]->clone());
	}

	if (paragraphName.empty() || isMasterPage)
	{
		if (pList["style:font-name"])
			mFontManager.findOrAdd(pList["style:font-name"]->getStr().cstr());
		paragraphName = mParagraphManager.findOrAdd(pList, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_Unknown);
		if (pList["librevenge:paragraph-id"] && !isMasterPage)
			mIdParagraphNameMap[pList["librevenge:paragraph-id"]->getInt()]=paragraphName;
	}

	// create a document element corresponding to the paragraph, and append it to our list of document elements
	TagOpenElement *pParagraphOpenElement = 0;

	if (propList["text:outline-level"])
	{
		mCurrentParaIsHeading = true;
		pParagraphOpenElement = new TagOpenElement("text:h");
		pParagraphOpenElement->addAttribute("text:outline-level", propList["text:outline-level"]->getStr());
	}
	else
	{
		pParagraphOpenElement = new TagOpenElement("text:p");
	}
	pParagraphOpenElement->addAttribute("text:style-name", paragraphName);
	mpCurrentStorage->push_back(pParagraphOpenElement);
	mLastParagraphName=paragraphName;
}

void OdfGenerator::closeParagraph()
{
	mpCurrentStorage->push_back(mCurrentParaIsHeading ? new TagCloseElement("text:h") : new TagCloseElement("text:p"));
	mCurrentParaIsHeading = false;
}

////////////////////////////////////////////////////////////
// list
////////////////////////////////////////////////////////////
void OdfGenerator::popListState()
{
	mListManager.popState();
}

void OdfGenerator::pushListState()
{
	mListManager.pushState();
}

void OdfGenerator::openListLevel(const librevenge::RVNGPropertyList &propList, bool ordered)
{
	ListManager::State &state=mListManager.getState();
	if (state.mbListElementParagraphOpened)
	{
		closeParagraph();
		state.mbListElementParagraphOpened = false;
	}
	librevenge::RVNGPropertyList pList(propList);
	if (!pList["librevenge:level"])
		pList.insert("librevenge:level", int(state.mbListElementOpened.size())+1);
	mListManager.defineLevel(pList, ordered, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_Unknown);

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
	ListManager::State &state=mListManager.getState();
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
	ListManager::State &state=mListManager.getState();
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
	librevenge::RVNGString paragName =mParagraphManager.findOrAdd(finalPropList, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_Unknown);

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
	if (mListManager.getState().mbListElementParagraphOpened)
	{
		closeParagraph();
		mListManager.getState().mbListElementParagraphOpened = false;
	}
}


////////////////////////////////////////////////////////////
// table
////////////////////////////////////////////////////////////
void OdfGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	mTableManager.openTable(propList, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic);

	Table *table=mTableManager.getActualTable();
	if (!table)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openTable: can not retrieve the table data\n"));
		return;
	}
	librevenge::RVNGString tableName=table->getName();

	TagOpenElement *pTableOpenElement = new TagOpenElement("table:table");
	pTableOpenElement->addAttribute("table:name", tableName.cstr());
	pTableOpenElement->addAttribute("table:style-name", tableName.cstr());
	mpCurrentStorage->push_back(pTableOpenElement);

	for (int i=0; i<table->getNumColumns(); ++i)
	{
		TagOpenElement *pTableColumnOpenElement = new TagOpenElement("table:table-column");
		librevenge::RVNGString sColumnStyleName;
		sColumnStyleName.sprintf("%s.Column%i", tableName.cstr(), (i+1));
		pTableColumnOpenElement->addAttribute("table:style-name", sColumnStyleName.cstr());
		mpCurrentStorage->push_back(pTableColumnOpenElement);

		TagCloseElement *pTableColumnCloseElement = new TagCloseElement("table:table-column");
		mpCurrentStorage->push_back(pTableColumnCloseElement);
	}
}

void OdfGenerator::closeTable()
{
	if (!mTableManager.getActualTable())
		return;
	mTableManager.closeTable();
	mpCurrentStorage->push_back(new TagCloseElement("table:table"));
}

bool OdfGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	Table *table=mTableManager.getActualTable();
	if (!table)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openTableRow called with no table\n"));
		return false;
	}
	librevenge::RVNGString rowName=table->openRow(propList);
	if (rowName.empty())
		return false;
	bool inHeader=false;
	if (table->isRowOpened(inHeader) && inHeader)
		mpCurrentStorage->push_back(new TagOpenElement("table:table-header-rows"));

	TagOpenElement *pTableRowOpenElement = new TagOpenElement("table:table-row");
	pTableRowOpenElement->addAttribute("table:style-name", rowName);
	mpCurrentStorage->push_back(pTableRowOpenElement);
	return true;
}

void OdfGenerator::closeTableRow()
{
	Table *table=mTableManager.getActualTable();
	if (!table) return;
	bool inHeader=false;
	if (!table->isRowOpened(inHeader) || !table->closeRow()) return;
	mpCurrentStorage->push_back(new TagCloseElement("table:table-row"));
	if (inHeader)
		mpCurrentStorage->push_back(new TagCloseElement("table:table-header-rows"));
}

bool OdfGenerator::isInTableRow(bool &inHeaderRow) const
{
	Table const *table=mTableManager.getActualTable();
	if (!table) return false;
	return table->isRowOpened(inHeaderRow);
}

bool OdfGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	Table *table=mTableManager.getActualTable();
	if (!table)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::openTableCell called with no table\n"));
		return false;
	}

	librevenge::RVNGString cellName = table->openCell(propList);
	if (cellName.empty())
		return false;

	TagOpenElement *pTableCellOpenElement = new TagOpenElement("table:table-cell");
	pTableCellOpenElement->addAttribute("table:style-name", cellName);
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
	if (!mTableManager.getActualTable() || !mTableManager.getActualTable()->closeCell())
		return;

	mpCurrentStorage->push_back(new TagCloseElement("table:table-cell"));
}

void OdfGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &propList)
{
	if (!mTableManager.getActualTable() ||
	        !mTableManager.getActualTable()->insertCoveredCell(propList))
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
		try
		{
			librevenge::RVNGBinaryData data(propList["office:binary-data"]->getStr());
			if (tmpObjectHandler)
			{
				DocumentElementVector tmpContentElements;
				InternalHandler tmpHandler(&tmpContentElements);


				if (tmpObjectHandler(data, &tmpHandler, ODF_FLAT_XML) && !tmpContentElements.empty())
				{
					mpCurrentStorage->push_back(new TagOpenElement("draw:object"));
					tmpContentElements.appendTo(*mpCurrentStorage);
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
		catch (...)
		{
			ODFGEN_DEBUG_MSG(("OdfGenerator::insertBinaryObject: ARGHH, catch an exception when decoding data!!!\n"));
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
		try
		{
			mpCurrentStorage->push_back(new CharDataElement(propList["office:binary-data"]->getStr().cstr()));
		}
		catch (...)
		{
			ODFGEN_DEBUG_MSG(("OdfGenerator::insertBinaryObject: ARGHH, catch an exception when decoding picture!!!\n"));
		}
		mpCurrentStorage->push_back(new TagCloseElement("office:binary-data"));

		if (propList["librevenge:mime-type"]->getStr() == "object/ole")
			mpCurrentStorage->push_back(new TagCloseElement("draw:object-ole"));
		else
			mpCurrentStorage->push_back(new TagCloseElement("draw:image"));
	}
}

////////////////////////////////////////////////////////////
// graphic
////////////////////////////////////////////////////////////
void OdfGenerator::defineGraphicStyle(const librevenge::RVNGPropertyList &propList)
{
	mGraphicStyle=propList;
}

librevenge::RVNGString OdfGenerator::getCurrentGraphicStyleName()
{
	librevenge::RVNGPropertyList styleList;
	mGraphicManager.addGraphicProperties(mGraphicStyle,styleList);
	return mGraphicManager.findOrAdd(styleList, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic);
}

librevenge::RVNGString OdfGenerator::getCurrentGraphicStyleName(const librevenge::RVNGPropertyList &shapeList)
{
	librevenge::RVNGPropertyList styleList;
	mGraphicManager.addGraphicProperties(shapeList,styleList);
	mGraphicManager.addGraphicProperties(mGraphicStyle,styleList);
	return mGraphicManager.findOrAdd(styleList, useStyleAutomaticZone() ? Style::Z_StyleAutomatic : Style::Z_ContentAutomatic);
}

void OdfGenerator::drawEllipse(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["svg:rx"] || !propList["svg:ry"] || !propList["svg:cx"] || !propList["svg:cy"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::drawEllipse: position undefined\n"));
		return;
	}
	double rx=0, ry=0, cx=0, cy=0;
	if (!getInchValue(*propList["svg:rx"], rx) || !getInchValue(*propList["svg:ry"], ry) ||
	        !getInchValue(*propList["svg:cx"], cx) || !getInchValue(*propList["svg:cy"], cy))
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::drawEllipse: can not read position\n"));
		return;
	}
	librevenge::RVNGString sValue=getCurrentGraphicStyleName(propList);
	TagOpenElement *pDrawEllipseElement = new TagOpenElement("draw:ellipse");
	pDrawEllipseElement->addAttribute("draw:style-name", sValue);
	addFrameProperties(propList, *pDrawEllipseElement);
	sValue = doubleToString(2 * rx);
	sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:width", sValue);
	sValue = doubleToString(2 * ry);
	sValue.append("in");
	pDrawEllipseElement->addAttribute("svg:height", sValue);
	if (propList["librevenge:rotate"] &&
	        (propList["librevenge:rotate"]->getDouble() < 0 || propList["librevenge:rotate"]->getDouble() > 0))
	{
		double rotation = propList["librevenge:rotate"]->getDouble();
		while (rotation < -180)
			rotation += 360;
		while (rotation > 180)
			rotation -= 360;
		double radrotation = rotation*M_PI/180.0;
		double deltax = sqrt(pow(rx, 2.0) + pow(ry, 2.0))*cos(atan(ry/rx)-radrotation) - rx;
		double deltay = sqrt(pow(rx, 2.0) + pow(ry, 2.0))*sin(atan(ry/rx)- radrotation) - ry;
		sValue = "rotate(";
		sValue.append(doubleToString(radrotation));
		sValue.append(") ");
		sValue.append("translate(");
		sValue.append(doubleToString(cx - rx - deltax));
		sValue.append("in, ");
		sValue.append(doubleToString(cy - ry - deltay));
		sValue.append("in)");
		pDrawEllipseElement->addAttribute("draw:transform", sValue);
	}
	else
	{
		sValue = doubleToString(cx-rx);
		sValue.append("in");
		pDrawEllipseElement->addAttribute("svg:x", sValue);
		sValue = doubleToString(cy-ry);
		sValue.append("in");
		pDrawEllipseElement->addAttribute("svg:y", sValue);
	}
	mpCurrentStorage->push_back(pDrawEllipseElement);
	mpCurrentStorage->push_back(new TagCloseElement("draw:ellipse"));
}

void OdfGenerator::drawPath(const librevenge::RVNGPropertyList &propList)
{
	const librevenge::RVNGPropertyListVector *path = propList.child("svg:d");
	if (!path) return;
	drawPath(*path, propList);
}

void OdfGenerator::drawPath(const librevenge::RVNGPropertyListVector &path, const librevenge::RVNGPropertyList &propList)
{
	if (!path.count())
		return;

	double px = 0.0, py = 0.0, qx = 0.0, qy = 0.0;
	if (!libodfgen::getPathBBox(path, px, py, qx, qy))
		return;

	librevenge::RVNGString sValue=getCurrentGraphicStyleName(propList);
	TagOpenElement *pDrawPathElement = new TagOpenElement("draw:path");
	pDrawPathElement->addAttribute("draw:style-name", sValue);
	addFrameProperties(propList, *pDrawPathElement);
	sValue = doubleToString(px);
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:x", sValue);
	sValue = doubleToString(py);
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:y", sValue);
	sValue = doubleToString((qx - px));
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:width", sValue);
	sValue = doubleToString((qy - py));
	sValue.append("in");
	pDrawPathElement->addAttribute("svg:height", sValue);
	sValue.sprintf("%i %i %i %i", 0, 0, (unsigned)(2540*(qx - px)), (unsigned)(2540*(qy - py)));
	pDrawPathElement->addAttribute("svg:viewBox", sValue);

	pDrawPathElement->addAttribute("svg:d", libodfgen::convertPath(path, px, py));
	mpCurrentStorage->push_back(pDrawPathElement);
	mpCurrentStorage->push_back(new TagCloseElement("draw:path"));
}

void OdfGenerator::drawPolySomething(const librevenge::RVNGPropertyList &propList, bool isClosed)
{
	const ::librevenge::RVNGPropertyListVector *vertices = propList.child("svg:points");
	if (!vertices || vertices->count() < 2)
		return;

	if (vertices->count() == 2)
	{
		if (!(*vertices)[0]["svg:x"]||!(*vertices)[0]["svg:y"]||!(*vertices)[1]["svg:x"]||!(*vertices)[1]["svg:y"])
		{
			ODFGEN_DEBUG_MSG(("OdfGenerator::drawPolySomething: some vertices are not defined\n"));
			return;
		}
		librevenge::RVNGString sValue=getCurrentGraphicStyleName(propList);
		TagOpenElement *pDrawLineElement = new TagOpenElement("draw:line");
		addFrameProperties(propList, *pDrawLineElement);
		pDrawLineElement->addAttribute("draw:style-name", sValue);
		pDrawLineElement->addAttribute("svg:x1", (*vertices)[0]["svg:x"]->getStr());
		pDrawLineElement->addAttribute("svg:y1", (*vertices)[0]["svg:y"]->getStr());
		pDrawLineElement->addAttribute("svg:x2", (*vertices)[1]["svg:x"]->getStr());
		pDrawLineElement->addAttribute("svg:y2", (*vertices)[1]["svg:y"]->getStr());
		mpCurrentStorage->push_back(pDrawLineElement);
		mpCurrentStorage->push_back(new TagCloseElement("draw:line"));
	}
	else
	{
		librevenge::RVNGPropertyListVector path;
		librevenge::RVNGPropertyList element;

		for (unsigned long ii = 0; ii < vertices->count(); ++ii)
		{
			element = (*vertices)[ii];
			if (ii == 0)
				element.insert("librevenge:path-action", "M");
			else
				element.insert("librevenge:path-action", "L");
			path.append(element);
			element.clear();
		}
		if (isClosed)
		{
			element.insert("librevenge:path-action", "Z");
			path.append(element);
		}
		drawPath(path,propList);
	}
}

void OdfGenerator::drawRectangle(const librevenge::RVNGPropertyList &propList)
{
	if (!propList["svg:x"] || !propList["svg:y"] ||
	        !propList["svg:width"] || !propList["svg:height"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::drawRectangle: position undefined\n"));
		return;
	}
	librevenge::RVNGString sValue=getCurrentGraphicStyleName(propList);
	librevenge::RVNGPropertyList frame(propList);
	frame.remove("svg:height");
	frame.remove("svg:width");
	TagOpenElement *pDrawRectElement = new TagOpenElement("draw:rect");
	addFrameProperties(frame, *pDrawRectElement);
	pDrawRectElement->addAttribute("draw:style-name", sValue);
	pDrawRectElement->addAttribute("svg:x", propList["svg:x"]->getStr());
	pDrawRectElement->addAttribute("svg:y", propList["svg:y"]->getStr());
	pDrawRectElement->addAttribute("svg:width", propList["svg:width"]->getStr());
	pDrawRectElement->addAttribute("svg:height", propList["svg:height"]->getStr());
	// FIXME: what to do when rx != ry ?
	if (propList["svg:rx"])
		pDrawRectElement->addAttribute("draw:corner-radius", propList["svg:rx"]->getStr());
	else
		pDrawRectElement->addAttribute("draw:corner-radius", "0.0000in");
	mpCurrentStorage->push_back(pDrawRectElement);
	mpCurrentStorage->push_back(new TagCloseElement("draw:rect"));
}

void OdfGenerator::drawConnector(const librevenge::RVNGPropertyList &propList)
{
	const librevenge::RVNGPropertyListVector *path = propList.child("svg:d");
	if (!path) return;
	drawPath(*path, propList);
}

////////////////////////////////////////////////////////////
// chart
////////////////////////////////////////////////////////////
void OdfGenerator::defineChartStyle(const librevenge::RVNGPropertyList &propList)
{
	int chartId=-1;
	if (propList["librevenge:chart-id"])
		chartId=propList["librevenge:chart-id"]->getInt();
	else
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::defineChartStyle: called without id\n"));
	}
	mIdChartMap[chartId]=propList;
	mIdChartNameMap.erase(chartId);
}

////////////////////////////////////////////////////////////
// font
////////////////////////////////////////////////////////////

void OdfGenerator::defineEmbeddedFont(const librevenge::RVNGPropertyList &propList)
{
	// AbiWord rejects such ODF files (rightfully, as office:binary-data is not allowed in
	// svg:font-face-uri). So only do this for Flat ODF, until we have a way to insert binary
	// files into the output archive (which is not possible without an API change).
	if (!((mDocumentStreamHandlers.size() == 1) && (mDocumentStreamHandlers.begin()->first == ODF_FLAT_XML)))
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::defineEmbeddedFont: embedded fonts are only handled for Flat ODF currently\n"));
		return;
	}
	if (!propList["office:binary-data"] || !propList["librevenge:mime-type"] || !propList["librevenge:name"])
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::defineEmbeddedFont: can not find data, mimetype, or name\n"));
		return;
	}

	try
	{
		const librevenge::RVNGString name(propList["librevenge:name"]->getStr());
		const librevenge::RVNGString mimeType(propList["librevenge:mime-type"]->getStr());
		const librevenge::RVNGBinaryData data(propList["office:binary-data"]->getStr());

		OdfEmbeddedImage imageHandler = findEmbeddedImageHandler(mimeType);
		if (imageHandler)
		{
			librevenge::RVNGBinaryData output;
			if (imageHandler(data, output))
				mFontManager.setEmbedded(name, "application/x-font-ttf", output);
		}
		else
		{
			mFontManager.setEmbedded(name, mimeType, data);
		}
	}
	catch (...)
	{
		ODFGEN_DEBUG_MSG(("OdfGenerator::defineEmbeddedFont: ARGHH, catch an exception when decoding font!!!\n"));
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
