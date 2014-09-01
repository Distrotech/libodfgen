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
#include <string>

#include <libodfgen/libodfgen.hxx>

#include "FilterInternal.hxx"
#include "InternalHandler.hxx"

#include "DocumentElement.hxx"
#include "FontStyle.hxx"
#include "ListStyle.hxx"
#include "SheetStyle.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"

#include "OdfGenerator.hxx"

#include "OdcGenerator.hxx"

// the state we use for writing the final document
struct ChartDocumentState
{
	ChartDocumentState();

	bool mbChartOpened;
	bool mbChartPlotAreaOpened;
	bool mbChartSerieOpened;
	bool mbChartTextObjectOpened;
	bool mbTableCellOpened;

	std::string mCharTextObjectType;
};

ChartDocumentState::ChartDocumentState() :
	mbChartOpened(false), mbChartPlotAreaOpened(false), mbChartSerieOpened(false), mbChartTextObjectOpened(false),
	mbTableCellOpened(false),
	mCharTextObjectType("")
{
}

class OdcGeneratorPrivate : public OdfGenerator
{
public:
	OdcGeneratorPrivate();
	~OdcGeneratorPrivate();

	bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType);
	void _writeStyles(OdfDocumentHandler *pHandler);
	void _writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType);

	bool canWriteText() const
	{
		return mChartDocumentStates.top().mbChartTextObjectOpened ||
		       mChartDocumentStates.top().mbTableCellOpened;
	}

	librevenge::RVNGString getChartStyleName(int id);
	void writeChartStyle(librevenge::RVNGPropertyList const &style, OdfDocumentHandler *pHandler);
	librevenge::RVNGString getAddressString(librevenge::RVNGPropertyListVector const *vector) const;
	std::stack<ChartDocumentState> mChartDocumentStates;

protected:
	// hash key -> name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mHashChartNameMap;
	// style name -> chart style
	std::map<librevenge::RVNGString, librevenge::RVNGPropertyList> mChartStyleHash;

private:
	OdcGeneratorPrivate(const OdcGeneratorPrivate &);
	OdcGeneratorPrivate &operator=(const OdcGeneratorPrivate &);

};

OdcGeneratorPrivate::OdcGeneratorPrivate() :
	mChartDocumentStates(), mHashChartNameMap(), mChartStyleHash()
{
	mChartDocumentStates.push(ChartDocumentState());
}

OdcGeneratorPrivate::~OdcGeneratorPrivate()
{
	// clean up the mess we made
	ODFGEN_DEBUG_MSG(("OdcGenerator: Cleaning up our mess..\n"));
}

librevenge::RVNGString OdcGeneratorPrivate::getChartStyleName(int id)
{
	if (mIdChartNameMap.find(id)!=mIdChartNameMap.end())
		return mIdChartNameMap.find(id)->second;

	librevenge::RVNGPropertyList pList;
	if (mIdChartMap.find(id)!=mIdChartMap.end())
		pList=mIdChartMap.find(id)->second;
	else
	{
		ODFGEN_DEBUG_MSG(("OdcGeneratorPrivate::getChartStyleName: can not find the style %d\n", id));
		pList.clear();
	}

	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mHashChartNameMap.find(hashKey);
	if (iter!=mHashChartNameMap.end())
	{
		mIdChartNameMap[id]=iter->second;
		return iter->second;
	}

	// ok create a new list
	librevenge::RVNGString sName("");
	sName.sprintf("Chart%i", mChartStyleHash.size());
	pList.insert("style:name", sName);
	mChartStyleHash[sName] =pList;
	mHashChartNameMap[hashKey] = sName;

	return sName;
}

void OdcGeneratorPrivate::writeChartStyle(librevenge::RVNGPropertyList const &style, OdfDocumentHandler *pHandler)
{
	if (!style["style:name"])
	{
		ODFGEN_DEBUG_MSG(("OdcGeneratorPrivate::writeChartStyle: can not find the style name\n"));
		return;
	}

	librevenge::RVNGPropertyList styleOpenList;
	styleOpenList.insert("style:name", style["style:name"]->clone());
	if (style["style:display-name"])
		styleOpenList.insert("style:display-name", style["style:display-name"]->clone());
	styleOpenList.insert("style:family", "chart");
	pHandler->startElement("style:style", styleOpenList);

	librevenge::RVNGPropertyList chartProp;
	librevenge::RVNGPropertyList::Iter i(style);
	for (i.rewind(); i.next();)
	{
		if (i.child()) continue;
		if (!strncmp(i.key(), "chart:", 6) || !strcmp(i.key(), "style:direction") ||
		        !strcmp(i.key(), "style:rotation-angle")  || !strcmp(i.key(), "text:line-break"))
			chartProp.insert(i.key(),i()->clone());
	}
	if (!chartProp.empty())
	{
		pHandler->startElement("style:chart-properties", chartProp);
		pHandler->endElement("style:chart-properties");
	}
	librevenge::RVNGPropertyList textProp;
	SpanStyleManager::addSpanProperties(style, textProp);
	if (!textProp.empty())
	{
		if (textProp["style:font-name"])
			mFontManager.findOrAdd(textProp["style:font-name"]->getStr().cstr());
		pHandler->startElement("style:text-properties", textProp);
		pHandler->endElement("style:text-properties");
	}
	librevenge::RVNGPropertyList graphProp;
	mGraphicManager.addGraphicProperties(style,graphProp);
	mGraphicManager.addFrameProperties(style,graphProp);
	if (!graphProp.empty())
	{
		pHandler->startElement("style:graphic-properties", graphProp);
		pHandler->endElement("style:graphic-properties");
	}
	pHandler->endElement("style:style");

}

librevenge::RVNGString OdcGeneratorPrivate::getAddressString(librevenge::RVNGPropertyListVector const *vector) const
{
	librevenge::RVNGString res("");
	if (!vector || vector->count()!=1)
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::getAddressString: can not find the address propertyList..\n"));
		return res;
	}
	if ((*vector)[0]["librevenge:row"])
		res=SheetManager::convertCellRange((*vector)[0]);
	else
		res=SheetManager::convertCellsRange((*vector)[0]);
	return res;
}

void OdcGeneratorPrivate::_writeAutomaticStyles(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	TagOpenElement("office:automatic-styles").write(pHandler);

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_STYLES_XML))
	{
		mSpanManager.write(pHandler, Style::Z_StyleAutomatic);
		mParagraphManager.write(pHandler, Style::Z_StyleAutomatic);
		mListManager.write(pHandler, Style::Z_StyleAutomatic);
		mGraphicManager.write(pHandler, Style::Z_StyleAutomatic);
		mTableManager.write(pHandler, Style::Z_StyleAutomatic);
	}

	if ((streamType == ODF_FLAT_XML) || (streamType == ODF_CONTENT_XML))
	{
		mSpanManager.write(pHandler, Style::Z_ContentAutomatic);
		mParagraphManager.write(pHandler, Style::Z_ContentAutomatic);
		mListManager.write(pHandler, Style::Z_ContentAutomatic);
		mGraphicManager.write(pHandler, Style::Z_ContentAutomatic);
		mTableManager.write(pHandler, Style::Z_ContentAutomatic);

		std::map<librevenge::RVNGString, librevenge::RVNGPropertyList>::const_iterator iterChartStyles;
		for (iterChartStyles=mChartStyleHash.begin(); iterChartStyles!=mChartStyleHash.end(); ++iterChartStyles)
			writeChartStyle(iterChartStyles->second,pHandler);
	}

	pHandler->endElement("office:automatic-styles");
}

void OdcGeneratorPrivate::_writeStyles(OdfDocumentHandler *pHandler)
{
	TagOpenElement("office:styles").write(pHandler);

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

	static char const *(s_paraStyle[4*4]) =
	{
		"Text_Body", "Text Body", "Standard", "text",
		"Table_Contents", "Table Contents", "Text_Body", "extra",
		"Table_Heading", "Table Heading", "Table_Contents", "extra",
		"List", "List", "Text_Body", "list"
	};
	for (int i=0; i<4; ++i)
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

	mSpanManager.write(pHandler, Style::Z_Style);
	mParagraphManager.write(pHandler, Style::Z_Style);
	mListManager.write(pHandler, Style::Z_Style);
	mGraphicManager.write(pHandler, Style::Z_Style);
	pHandler->endElement("office:styles");
}

bool OdcGeneratorPrivate::writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType)
{
	if (streamType == ODF_MANIFEST_XML)
	{
		pHandler->startDocument();
		TagOpenElement manifestElement("manifest:manifest");
		manifestElement.addAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
		manifestElement.addAttribute("manifest:version", "1.2", true);
		manifestElement.write(pHandler);

		TagOpenElement mainFile("manifest:file-entry");
		mainFile.addAttribute("manifest:media-type", "application/vnd.oasis.opendocument.chart");
		mainFile.addAttribute("manifest:full-path", "/");
		mainFile.write(pHandler);
		TagCloseElement("manifest:file-entry").write(pHandler);
		appendFilesInManifest(pHandler);

		TagCloseElement("manifest:manifest").write(pHandler);
		pHandler->endDocument();
		return true;
	}

	ODFGEN_DEBUG_MSG(("OdcGenerator: Document Body: Printing out the header stuff..\n"));

	ODFGEN_DEBUG_MSG(("OdcGenerator: Document Body: Start Document\n"));
	pHandler->startDocument();

	ODFGEN_DEBUG_MSG(("OdcGenerator: Document Body: preamble\n"));
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
	docContentPropList.insert("office:version", librevenge::RVNGPropertyFactory::newStringProp("1.2"));
	if (streamType == ODF_FLAT_XML)
		docContentPropList.insert("office:mimetype", "application/vnd.oasis.opendocument.chart");
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
	ODFGEN_DEBUG_MSG(("OdcGenerator: Document Body: Writing out the styles..\n"));

	// write default styles
	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML)
		_writeStyles(pHandler);

	if (streamType == ODF_FLAT_XML || streamType == ODF_STYLES_XML || streamType == ODF_CONTENT_XML)
		_writeAutomaticStyles(pHandler, streamType);

	if (streamType == ODF_FLAT_XML || streamType == ODF_CONTENT_XML)
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator: Document Body: Writing out the document..\n"));
		// writing out the document
		TagOpenElement("office:body").write(pHandler);
		TagOpenElement("office:chart").write(pHandler);
		sendStorage(&mBodyStorage, pHandler);
		ODFGEN_DEBUG_MSG(("OdcGenerator: Document Body: Finished writing all doc els..\n"));

		pHandler->endElement("office:chart");
		pHandler->endElement("office:body");
	}

	pHandler->endElement(documentType.c_str());

	pHandler->endDocument();

	return true;
}

OdcGenerator::OdcGenerator() : mpImpl(new OdcGeneratorPrivate)
{
}

OdcGenerator::~OdcGenerator()
{
	if (mpImpl)
		delete mpImpl;
}

void OdcGenerator::addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType)
{
	if (mpImpl)
		mpImpl->addDocumentHandler(pHandler, streamType);
}

librevenge::RVNGStringVector OdcGenerator::getObjectNames() const
{
	if (mpImpl)
		return mpImpl->getObjectNames();
	return librevenge::RVNGStringVector();
}

bool OdcGenerator::getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler)
{
	if (!mpImpl)
		return false;
	return mpImpl->getObjectContent(objectName, pHandler);
}

void OdcGenerator::setDocumentMetaData(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->setDocumentMetaData(propList);
}

void OdcGenerator::defineParagraphStyle(librevenge::RVNGPropertyList const &propList)
{
	mpImpl->defineParagraphStyle(propList);
}

void OdcGenerator::openParagraph(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->openParagraph(propList);
}

void OdcGenerator::closeParagraph()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeParagraph();
}

void OdcGenerator::defineCharacterStyle(librevenge::RVNGPropertyList const &propList)
{
	mpImpl->defineCharacterStyle(propList);
}

void OdcGenerator::openSpan(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText() || mpImpl->mChartDocumentStates.top().mbChartTextObjectOpened) return;
	mpImpl->openSpan(propList);
}

void OdcGenerator::closeSpan()
{
	if (!mpImpl->canWriteText() || mpImpl->mChartDocumentStates.top().mbChartTextObjectOpened) return;
	mpImpl->closeSpan();
}

void OdcGenerator::openLink(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->openLink(propList);
}

void OdcGenerator::closeLink()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeLink();
}

// -------------------------------
//      list
// -------------------------------
void OdcGenerator::openOrderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->openListLevel(propList, true);
}

void OdcGenerator::openUnorderedListLevel(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->openListLevel(propList, false);
}

void OdcGenerator::closeOrderedListLevel()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeListLevel();
}

void OdcGenerator::closeUnorderedListLevel()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeListLevel();
}

void OdcGenerator::openListElement(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->openListElement(propList);
}

void OdcGenerator::closeListElement()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeListElement();
}

// -------------------------------
//      chart
// -------------------------------

void OdcGenerator::defineChartStyle(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->defineChartStyle(propList);
}

void OdcGenerator::openChart(const librevenge::RVNGPropertyList &propList)
{
	if (mpImpl->mChartDocumentStates.top().mbChartOpened)
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::openChart: a chart is already opened\n"));
		return;
	}
	mpImpl->mChartDocumentStates.push(ChartDocumentState());
	mpImpl->mChartDocumentStates.top().mbChartOpened=true;

	TagOpenElement *openElement = new TagOpenElement("chart:chart");
	for (int i=0; i<8; ++i)
	{
		static char const *(wh[8]) =
		{
			"chart:class", "chart:column-mapping", "chart:row-mapping",
			"svg:height", "svg:width", "xlink:href", "xlink:type", "xml:id"
		};
		if (propList[wh[i]])
			openElement->addAttribute(wh[i], propList[wh[i]]->getStr());
	}
	if (!propList["xlink:href"])
	{
		openElement->addAttribute("xlink:href","..");
		openElement->addAttribute("xlink:type", "simple");
	}

	if (propList["librevenge:chart-id"])
		openElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(propList["librevenge:chart-id"]->getInt()));
	mpImpl->getCurrentStorage()->push_back(openElement);
}

void OdcGenerator::closeChart()
{
	if (!mpImpl->mChartDocumentStates.top().mbChartOpened)
		return;
	mpImpl->mChartDocumentStates.pop();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:chart"));
}

void OdcGenerator::openChartTextObject(const librevenge::RVNGPropertyList &propList)
{
	ChartDocumentState state=mpImpl->mChartDocumentStates.top();
	std::string type("");
	if (propList["librevenge:zone-type"]) type=propList["librevenge:zone-type"]->getStr().cstr();
	if (type!="footer" && type!="legend" && type!="subtitle" && type!="title")
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::openChartTextObject: unknown zone type\n"));
		return;
	}
	if (!state.mbChartOpened || state.mbChartTextObjectOpened ||
	        (type!="label" && state.mbChartPlotAreaOpened) ||
	        (type=="label" && !state.mbChartSerieOpened))
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::openChartTextObject: can not open a text zone\n"));
		return;
	}

	std::string what="chart:"+type;
	state.mbChartTextObjectOpened=true;
	state.mCharTextObjectType=what;
	mpImpl->mChartDocumentStates.push(state);

	TagOpenElement *openElement = new TagOpenElement(what.c_str());
	for (int i=0; i<2; ++i)
	{
		static char const *(wh[2]) =
		{
			"svg:x", "svg:y"
		};
		if (propList[wh[i]])
			openElement->addAttribute(wh[i], propList[wh[i]]->getStr());
	}
	if (propList["librevenge:chart-id"])
		openElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(propList["librevenge:chart-id"]->getInt()));

	if (type=="legend")
	{
		for (int i=0; i<4; ++i)
		{
			static char const *(wh[4]) =
			{
				"chart:legend-align", "chart:legend-position",
				"style:legend-expansion",  "style:legend-expansion-aspect-ratio"
			};
			if (propList[wh[i]])
				openElement->addAttribute(wh[i], propList[wh[i]]->getStr());
		}
	}
	else if (type!="label")
	{
		if (propList.child("table:cell-range"))
		{
			librevenge::RVNGString range=
			    mpImpl->getAddressString(propList.child("table:cell-range"));
			if (!range.empty())
				openElement->addAttribute("table:cell-range", range);
		}
	}
	mpImpl->getCurrentStorage()->push_back(openElement);
}

void OdcGenerator::closeChartTextObject()
{
	if (!mpImpl->mChartDocumentStates.top().mbChartTextObjectOpened)
		return;
	std::string wh = mpImpl->mChartDocumentStates.top().mCharTextObjectType;
	mpImpl->mChartDocumentStates.pop();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement(wh.c_str()));
}

void OdcGenerator::openChartPlotArea(const librevenge::RVNGPropertyList &propList)
{
	ChartDocumentState state=mpImpl->mChartDocumentStates.top();
	if (!state.mbChartOpened || state.mbChartTextObjectOpened || state.mbChartPlotAreaOpened)
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::openChartPlotArea: can not open the plot area\n"));
		return;
	}
	state.mbChartPlotAreaOpened=true;
	mpImpl->mChartDocumentStates.push(state);

	TagOpenElement *openElement = new TagOpenElement("chart:plot-area");
	for (int i=0; i<17; ++i)
	{
		static char const *(wh[17]) =
		{
			"chart:data-source-has-labels",
			"dr3d:ambient-color", "dr3d:distance", "dr3d:focal-length", "dr3d:lighting-mode",
			"dr3d:projection", "dr3d:shade-mode", "dr3d:shadow-slant", "dr3d:transform",
			"dr3d:vpn", "dr3d:vrp",
			"dr3d:vup", "svg:height", "svg:width", "svg:x", "svg:y",
			"xml:id"
		};
		if (propList[wh[i]])
			openElement->addAttribute(wh[i], propList[wh[i]]->getStr());
	}
	if (propList.child("table:cell-range-address"))
	{
		librevenge::RVNGString range=
		    mpImpl->getAddressString(propList.child("table:cell-range-address"));
		if (!range.empty())
			openElement->addAttribute("table:cell-range-address", range);
	}
	if (propList["librevenge:chart-id"])
		openElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(propList["librevenge:chart-id"]->getInt()));
	mpImpl->getCurrentStorage()->push_back(openElement);

	const librevenge::RVNGPropertyListVector *childs=propList.child("librevenge:childs");
	for (unsigned long c=0; c<(childs ? childs->count() : 0); ++c)
	{
		const librevenge::RVNGPropertyList &child=(*childs)[c];
		std::string type("");
		if (child["librevenge:type"])
			type=child["librevenge:type"]->getStr().cstr();
		if (type=="stock-gain-marker" || type=="stock-loss-marker" || type=="stock-range-line")
		{
			std::string what="chart:"+type;
			TagOpenElement *childElement = new TagOpenElement(what.c_str());
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:categories"));
		}
		else if (type=="floor" || type=="wall")
		{
			std::string what="chart:"+type;
			TagOpenElement *childElement = new TagOpenElement(what.c_str());
			if (child["svg:width"])
				childElement->addAttribute("svg:width",child["svg:width"]->getStr());
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement(what.c_str()));
		}
		else
		{
			ODFGEN_DEBUG_MSG(("OdcGenerator::openChartPlotArea: can not find type of child %d\n", int(c)));
		}
	}
}

void OdcGenerator::closeChartPlotArea()
{
	if (!mpImpl->mChartDocumentStates.top().mbChartPlotAreaOpened)
		return;
	mpImpl->mChartDocumentStates.pop();
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:plot-area"));
}

void OdcGenerator::insertChartAxis(const librevenge::RVNGPropertyList &propList)
{
	ChartDocumentState state=mpImpl->mChartDocumentStates.top();
	if (!state.mbChartPlotAreaOpened)
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::insertChartAxis: can not insert axis outside the plot area\n"));
		return;
	}
	TagOpenElement *openElement = new TagOpenElement("chart:axis");
	for (int i=0; i<2; ++i)
	{
		static char const *(wh[2]) =
		{
			"chart:dimension", "chart:name"
		};
		if (propList[wh[i]])
			openElement->addAttribute(wh[i], propList[wh[i]]->getStr());
	}
	if (propList["librevenge:chart-id"])
		openElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(propList["librevenge:chart-id"]->getInt()));
	mpImpl->getCurrentStorage()->push_back(openElement);
	const librevenge::RVNGPropertyListVector *childs=propList.child("librevenge:childs");
	for (unsigned long c=0; c<(childs ? childs->count() : 0); ++c)
	{
		const librevenge::RVNGPropertyList &child=(*childs)[c];
		std::string type("");
		if (child["librevenge:type"])
			type=child["librevenge:type"]->getStr().cstr();
		if (type=="categories")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:categories");
			if (child.child("table:cell-range"))
			{
				librevenge::RVNGString range=
				    mpImpl->getAddressString(child.child("table:cell-range"));
				if (!range.empty())
					childElement->addAttribute("table:cell-range", range);
			}
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:categories"));
		}
		else if (type=="grid")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:grid");
			if (child["chart:class"])
				childElement->addAttribute("chart:class",child["chart:class"]->getStr());
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:grid"));
		}
		else if (type=="title")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:title");
			for (int i=0; i<2; ++i)
			{
				static char const *(wh[2]) =
				{
					"svg:x", "svg:y"
				};
				if (child[wh[i]])
					childElement->addAttribute(wh[i], child[wh[i]]->getStr());
			}
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:title"));
		}
		else
		{
			ODFGEN_DEBUG_MSG(("OdcGenerator::insertChartAxis: can not find type of child %d\n", int(c)));
		}
	}
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:axis"));
}

void OdcGenerator::openChartSerie(const librevenge::RVNGPropertyList &propList)
{
	ChartDocumentState &state=mpImpl->mChartDocumentStates.top();
	if (!state.mbChartPlotAreaOpened || state.mbChartSerieOpened)
	{
		ODFGEN_DEBUG_MSG(("OdcGenerator::openChartSerie: can not insert serie outside the plot area\n"));
		return;
	}
	state.mbChartSerieOpened=true;
	TagOpenElement *openElement = new TagOpenElement("chart:series");
	for (int i=0; i<3; ++i)
	{
		static char const *(wh[3]) =
		{
			"chart:attached-axis", "chart:class", "xml:id"
		};
		if (propList[wh[i]])
			openElement->addAttribute(wh[i], propList[wh[i]]->getStr());
	}
	if (propList["librevenge:chart-id"])
		openElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(propList["librevenge:chart-id"]->getInt()));
	for (int i=0; i<2; ++i)
	{
		static char const *(wh[2]) =
		{
			"chart:label-cell-address", "chart:values-cell-range-address"
		};
		if (propList.child(wh[i]))
		{
			librevenge::RVNGString range=
			    mpImpl->getAddressString(propList.child(wh[i]));
			if (!range.empty())
				openElement->addAttribute(wh[i], range);
		}
	}
	mpImpl->getCurrentStorage()->push_back(openElement);
	const librevenge::RVNGPropertyListVector *childs=propList.child("librevenge:childs");
	for (unsigned long c=0; c<(childs ? childs->count() : 0); ++c)
	{
		const librevenge::RVNGPropertyList &child=(*childs)[c];
		std::string type("");
		if (child["librevenge:type"])
			type=child["librevenge:type"]->getStr().cstr();
		if (type=="data-point")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:data-point");
			for (int i=0; i<2; ++i)
			{
				static char const *(wh[2]) =
				{
					"chart:repeated", "xml:id"
				};
				if (child[wh[i]])
					childElement->addAttribute(wh[i], child[wh[i]]->getStr());
			}
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:data-point"));
		}
		else if (type=="domain")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:domain");
			if (child.child("table:cell-range"))
			{
				librevenge::RVNGString range=
				    mpImpl->getAddressString(child.child("table:cell-range"));
				if (!range.empty())
					childElement->addAttribute("table:cell-range", range);
			}
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:domain"));
		}
		else if (type=="error-indicator")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:error-indicator");
			if (child["chart:dimension"])
				childElement->addAttribute("chart:dimension", child["chart:dimension"]->getStr());
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:error-indicator"));
		}
		else if (type=="mean-value")
		{
			TagOpenElement *childElement = new TagOpenElement("chart:mean-value");
			if (child["librevenge:chart-id"])
				childElement->addAttribute("chart:style-name",mpImpl->getChartStyleName(child["librevenge:chart-id"]->getInt()));
			mpImpl->getCurrentStorage()->push_back(childElement);
			mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:mean-value"));
		}
		else
		{
			ODFGEN_DEBUG_MSG(("OdcGenerator::openChartSerie: can not find type of child %d\n", int(c)));
		}
	}
}

void OdcGenerator::closeChartSerie()
{
	ChartDocumentState &state=mpImpl->mChartDocumentStates.top();
	if (!state.mbChartSerieOpened)
		return;
	state.mbChartSerieOpened=false;
	mpImpl->getCurrentStorage()->push_back(new TagCloseElement("chart:series"));
}

// -------------------------------
//      table
// -------------------------------
void OdcGenerator::openTable(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openTable(propList);
}

void OdcGenerator::closeTable()
{
	mpImpl->closeTable();
}

void OdcGenerator::openTableRow(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->openTableRow(propList);
}

void OdcGenerator::closeTableRow()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->closeTableRow();
}

void OdcGenerator::openTableCell(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->mChartDocumentStates.top().mbTableCellOpened = mpImpl->openTableCell(propList);
}

void OdcGenerator::closeTableCell()
{
	mpImpl->closeTableCell();
	mpImpl->mChartDocumentStates.top().mbTableCellOpened = false;
}

void OdcGenerator::insertCoveredTableCell(const librevenge::RVNGPropertyList &propList)
{
	mpImpl->insertCoveredTableCell(propList);
}

void OdcGenerator::insertTab()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->insertTab();
}

void OdcGenerator::insertSpace()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->insertSpace();
}

void OdcGenerator::insertLineBreak()
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->insertLineBreak();
}

void OdcGenerator::insertField(const librevenge::RVNGPropertyList &propList)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->insertField(propList);
}

void OdcGenerator::insertText(const librevenge::RVNGString &text)
{
	if (!mpImpl->canWriteText()) return;
	mpImpl->insertText(text);
}

void OdcGenerator::endDocument()
{
	// Write out the collected document
	mpImpl->writeTargetDocuments();
}

void OdcGenerator::startDocument(const librevenge::RVNGPropertyList &)
{
}

void OdcGenerator::initStateWith(OdfGenerator const &orig)
{
	mpImpl->initStateWith(orig);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
