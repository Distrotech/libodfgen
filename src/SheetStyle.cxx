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

#include <math.h>
#ifdef _MSC_VER
# include <minmax.h>
#endif
#include <string.h>

#include <sstream>
#include <string>

#include "DocumentElement.hxx"
#include "FilterInternal.hxx"
#include "TextRunStyle.hxx"

#include "SheetStyle.hxx"

SheetNumberingStyle::SheetNumberingStyle(const librevenge::RVNGPropertyList &xPropList, const librevenge::RVNGString &psName)
	: Style(psName), mPropList(xPropList)
{
}

void SheetNumberingStyle::writeCondition(librevenge::RVNGPropertyList const &propList, OdfDocumentHandler *pHandler, SheetManager const &manager) const
{
	librevenge::RVNGString applyName("");
	librevenge::RVNGPropertyListVector const *formula=propList.child("librevenge:formula");
	librevenge::RVNGString formulaString("");
	if (!formula||!propList["librevenge:name"]||
	        (applyName=manager.getNumberingStyleName(propList["librevenge:name"]->getStr())).empty() ||
	        (formulaString=SheetManager::convertFormula(*formula)).empty())
	{
		ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeCondition: can not find condition data\n"));
		return;
	}
	TagOpenElement mapOpen("style:map");
	mapOpen.addAttribute("style:condition",formulaString);
	mapOpen.addAttribute("style:apply-style-name",applyName);
	mapOpen.write(pHandler);
	TagCloseElement("style:map").write(pHandler);
}

void SheetNumberingStyle::writeStyle(OdfDocumentHandler *pHandler, SheetManager const &manager) const
{
	if (!mPropList["librevenge:value-type"])
	{
		ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeStyle: can not find value type\n"));
		return;
	}
	std::string type(mPropList["librevenge:value-type"]->getStr().cstr());
	librevenge::RVNGString what("");
	if (type.substr(0,7)=="number:") type=type.substr(7);
	size_t len=type.length();
	if (len>5 && type.substr(len-5)=="-type") type=type.substr(0,len-5);
	if (type=="float" || type=="double") type="number";
	else if (type=="percent") type="percentage";
	else if (type=="bool") type="boolean";
	if (type=="number" || type=="fraction" || type=="percentage" || type=="scientific")
	{
		what.sprintf("number:%s-style", type=="percentage" ? "percentage" : "number");
		TagOpenElement styleOpen(what);
		styleOpen.addAttribute("style:name", getName());
		styleOpen.write(pHandler);
		librevenge::RVNGString subWhat;
		subWhat.sprintf("number:%s", type=="percentage" ? "number" : type=="scientific" ? "scientific-number" : type.c_str());
		TagOpenElement number(subWhat);
		if (mPropList["number:decimal-places"])
			number.addAttribute("number:decimal-places", mPropList["number:decimal-places"]->getStr());
		if (mPropList["number:min-integer-digits"])
			number.addAttribute("number:min-integer-digits", mPropList["number:min-integer-digits"]->getStr());
		else
			number.addAttribute("number:min-integer-digits", "1");
		if (mPropList["number:grouping"])
			number.addAttribute("number:grouping", mPropList["number:grouping"]->getStr());
		if (type=="scientific")
		{
			if (mPropList["number:min-exponent-digits"])
				number.addAttribute("number:min-exponent-digits", mPropList["number:min-exponent-digits"]->getStr());
			else
				number.addAttribute("number:min-exponent-digits", "2");
		}
		else if (type=="fraction")
		{
			if (mPropList["number:min-numerator-digits"])
				number.addAttribute("number:min-numerator-digits", mPropList["number:min-numerator-digits"]->getStr());
			if (mPropList["number:min-denominator-digits"])
				number.addAttribute("number:min-denominator-digits", mPropList["number:min-denominator-digits"]->getStr());
		}
		number.write(pHandler);
		TagCloseElement(subWhat).write(pHandler);
		if (type=="percentage")
		{
			TagOpenElement("number:text").write(pHandler);
			pHandler->characters("%");
			TagCloseElement("number:text").write(pHandler);
		}
	}
	else if (type=="boolean")
	{
		what="number:boolean-style";
		TagOpenElement styleOpen(what);
		styleOpen.addAttribute("style:name", getName());
		styleOpen.write(pHandler);
		TagOpenElement("number:boolean").write(pHandler);
		TagCloseElement("number:boolean").write(pHandler);
	}
	else if (type=="time" || type=="date")
	{
		what.sprintf("number:%s-style", type.c_str());
		TagOpenElement styleOpen(what);
		styleOpen.addAttribute("style:name", getName());
		if (mPropList["number:language"])
			styleOpen.addAttribute("number:language", mPropList["number:language"]->getStr());
		if (mPropList["number:country"])
			styleOpen.addAttribute("number:country", mPropList["number:country"]->getStr());
		if (type=="date" && mPropList["number:automatic-order"])
			styleOpen.addAttribute("number:automatic-order", mPropList["number:automatic-order"]->getStr());
		styleOpen.write(pHandler);
	}
	else if (type=="currency")
	{
		what.sprintf("number:currency-style", type.c_str());
		TagOpenElement styleOpen(what);
		styleOpen.addAttribute("style:name", getName());
		styleOpen.write(pHandler);
	}
	else
	{
		ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeStyle: unexpected value type %s\n", type.c_str()));
		return;
	}
	librevenge::RVNGPropertyListVector const *format=mPropList.child("librevenge:format");
	// now read the potential formats sub list
	for (unsigned long f=0; format && f < format->count(); ++f)
	{
		librevenge::RVNGPropertyList const &prop=(*format)[f];
		if (!prop["librevenge:value-type"])
		{
			ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeStyle: can not find format type[%d]\n", int(f)));
			continue;
		}
		std::string wh=prop["librevenge:value-type"]->getStr().cstr();
		if (wh.substr(0,7)=="number:") wh=wh.substr(7);

		if (wh=="number")
		{
			TagOpenElement formatOpen("number:number");
			if (prop["number:decimal-places"])
				formatOpen.addAttribute("number:decimal-places", prop["number:decimal-places"]->getStr());
			if (prop["number:min-integer-digits"])
				formatOpen.addAttribute("number:min-integer-digits", prop["number:min-integer-digits"]->getStr());
			else
				formatOpen.addAttribute("number:min-integer-digits", "1");
			if (prop["number:grouping"])
				formatOpen.addAttribute("number:grouping", prop["number:grouping"]->getStr());
			formatOpen.write(pHandler);
			TagCloseElement("number:number").write(pHandler);
		}
		else if (wh=="year" || wh=="month" || wh=="day" || wh=="day-of-week" || wh=="quarter" || wh=="week-of-year" ||
		         wh=="hours" || wh=="minutes" || wh=="seconds" || wh=="am-pm")
		{
			librevenge::RVNGString subWhat;
			subWhat.sprintf("number:%s", wh.c_str());
			TagOpenElement formatOpen(subWhat);
			if (prop["number:style"])
				formatOpen.addAttribute("number:style", prop["number:style"]->getStr());
			if (prop["number:textual"])
				formatOpen.addAttribute("number:textual", prop["number:textual"]->getStr());
			formatOpen.write(pHandler);
			TagCloseElement(subWhat).write(pHandler);
		}
		else if (wh=="text")
		{
			if (prop["librevenge:text"])
			{
				TagOpenElement("number:text").write(pHandler);
				pHandler->characters(prop["librevenge:text"]->getStr());
				TagCloseElement("number:text").write(pHandler);
			}
			else
			{
				ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeStyle: can not find text data\n"));
			}
		}
		else if (wh=="currency-symbol")
		{
			if (prop["librevenge:currency"])
			{
				TagOpenElement currency("number:currency-symbol");
				if (prop["number:language"])
					currency.addAttribute("number:language", prop["number:language"]->getStr());
				if (prop["number:country"])
					currency.addAttribute("number:country", prop["number:country"]->getStr());
				currency.write(pHandler);
				pHandler->characters(prop["librevenge:currency"]->getStr());
				TagCloseElement("number:currency-symbol").write(pHandler);
			}
			else
			{
				ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeStyle: can not find currency data\n"));
			}
		}
		else
		{
			ODFGEN_DEBUG_MSG(("SheetNumberingStyle::writeStyle: find unexpected format type:%s\n", wh.c_str()));
		}
	}

	librevenge::RVNGPropertyListVector const *conditions=mPropList.child("librevenge::conditions");
	for (unsigned long c=0; c < (conditions ? conditions->count() : 0); ++c)
		writeCondition((*conditions)[c], pHandler, manager);
	TagCloseElement(what).write(pHandler);
}

SheetCellStyle::SheetCellStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName) :
	Style(psName),
	mPropList(xPropList)
{
}

void SheetCellStyle::writeStyle(OdfDocumentHandler *pHandler, SheetManager const &manager) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	if (mPropList["style:parent-style-name"])
		styleOpen.addAttribute("style:parent-style-name", mPropList["style:parent-style-name"]->getStr());
	styleOpen.addAttribute("style:family", "table-cell");
	if (mPropList["librevenge:numbering-name"])
	{
		librevenge::RVNGString numberingName=
		    manager.getNumberingStyleName(mPropList["librevenge:numbering-name"]->getStr());
		if (numberingName.empty())
		{
			ODFGEN_DEBUG_MSG(("SheetCellStyle::writeStyle can not find numbering style %s\n", mPropList["librevenge:numbering-name"]->getStr().cstr()));
		}
		else
			styleOpen.addAttribute("style:data-style-name", numberingName.cstr());
	}
	styleOpen.write(pHandler);

	librevenge::RVNGPropertyList textProp;
	SpanStyleManager::addSpanProperties(mPropList, textProp);
	if (!textProp.empty())
	{
		pHandler->startElement("style:text-properties", textProp);
		pHandler->endElement("style:text-properties");
	}
	// WLACH_REFACTORING: Only temporary.. a much better solution is to
	// generalize this sort of thing into the "Style" superclass
	librevenge::RVNGPropertyList stylePropList;
	librevenge::RVNGPropertyList::Iter i(mPropList);
	/* first set padding, so that mPropList can redefine, if
	   mPropList["fo:padding"] is defined */
	stylePropList.insert("fo:padding", "0.0382in");
	bool hasTextAlign=false;
	for (i.rewind(); i.next();)
	{
		int len = (int) strlen(i.key());
		if (len > 2 && !strncmp(i.key(), "fo", 2))
		{
			if (len==13 && !strcmp(i.key(), "fo:text-align"))
				hasTextAlign=true;
			else
				stylePropList.insert(i.key(), i()->clone());
		}
		else if (len > 22  && !strncmp(i.key(), "style:border-line-width", 23))
		{
			if (!strcmp(i.key(), "style:border-line-width") ||
			        !strcmp(i.key(), "style:border-line-width-left") ||
			        !strcmp(i.key(), "style:border-line-width-right") ||
			        !strcmp(i.key(), "style:border-line-width-top")||
			        !strcmp(i.key(), "style:border-line-width-bottom"))
				stylePropList.insert(i.key(), i()->clone());
		}
		else if (len == 23 && !strcmp(i.key(), "style:text-align-source"))
			stylePropList.insert(i.key(), i()->clone());
		else if (len == 18 && !strcmp(i.key(), "style:cell-protect"))
			stylePropList.insert(i.key(), i()->clone());
		else if (!strcmp(i.key(), "style:vertical-align"))
			stylePropList.insert(i.key(), i()->clone());
	}
	pHandler->startElement("style:table-cell-properties", stylePropList);
	pHandler->endElement("style:table-cell-properties");

	if (hasTextAlign)
	{
		librevenge::RVNGPropertyList paragPropList;
		paragPropList.insert("fo:margin-left", "0cm");
		paragPropList.insert("fo:text-align", mPropList["fo:text-align"]->clone());
		pHandler->startElement("style:paragraph-properties", paragPropList);
		pHandler->endElement("style:paragraph-properties");
	}
	pHandler->endElement("style:style");
}

SheetRowStyle::SheetRowStyle(const librevenge::RVNGPropertyList &propList, const char *psName) :
	Style(psName),
	mPropList(propList)
{
}

void SheetRowStyle::writeStyle(OdfDocumentHandler *pHandler, SheetManager const &) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table-row");
	styleOpen.write(pHandler);

	TagOpenElement stylePropertiesOpen("style:table-row-properties");
	if (mPropList["style:min-row-height"])
		stylePropertiesOpen.addAttribute("style:min-row-height", mPropList["style:min-row-height"]->getStr());
	else if (mPropList["style:row-height"])
		stylePropertiesOpen.addAttribute("style:row-height", mPropList["style:row-height"]->getStr());
	stylePropertiesOpen.addAttribute("fo:keep-together", "auto");
	stylePropertiesOpen.write(pHandler);
	pHandler->endElement("style:table-row-properties");

	pHandler->endElement("style:style");
}


SheetStyle::SheetStyle(const librevenge::RVNGPropertyList &xPropList, const char *psName, Style::Zone zone) :
	Style(psName, zone), mPropList(xPropList), mColumns(0),
	mRowNameHash(), mRowStyleHash(), mCellNameHash(), mCellStyleHash()
{
	mColumns = mPropList.child("librevenge:columns");
}

SheetStyle::~SheetStyle()
{
}

void SheetStyle::writeStyle(OdfDocumentHandler *pHandler, SheetManager const &manager) const
{
	TagOpenElement styleOpen("style:style");
	styleOpen.addAttribute("style:name", getName());
	styleOpen.addAttribute("style:family", "table");
	if (mPropList["style:master-page-name"])
		styleOpen.addAttribute("style:master-page-name", mPropList["style:master-page-name"]->getStr());
	styleOpen.write(pHandler);

	TagOpenElement stylePropertiesOpen("style:table-properties");
	stylePropertiesOpen.addAttribute("table:display", "true");
	if (mPropList["table:align"])
		stylePropertiesOpen.addAttribute("table:align", mPropList["table:align"]->getStr());
	if (mPropList["fo:margin-left"])
		stylePropertiesOpen.addAttribute("fo:margin-left", mPropList["fo:margin-left"]->getStr());
	if (mPropList["fo:margin-right"])
		stylePropertiesOpen.addAttribute("fo:margin-right", mPropList["fo:margin-right"]->getStr());
	if (mPropList["style:width"])
		stylePropertiesOpen.addAttribute("style:width", mPropList["style:width"]->getStr());
	if (mPropList["fo:break-before"])
		stylePropertiesOpen.addAttribute("fo:break-before", mPropList["fo:break-before"]->getStr());
	if (mPropList["table:border-model"])
		stylePropertiesOpen.addAttribute("table:border-model", mPropList["table:border-model"]->getStr());
	stylePropertiesOpen.write(pHandler);

	pHandler->endElement("style:table-properties");

	pHandler->endElement("style:style");

	int col=1;
	if (mColumns)
	{
		librevenge::RVNGPropertyListVector::Iter j(*mColumns);
		for (j.rewind(); j.next(); ++col)
		{
			TagOpenElement columnStyleOpen("style:style");
			librevenge::RVNGString sColumnName;
			sColumnName.sprintf("%s_col%i", getName().cstr(), col);
			columnStyleOpen.addAttribute("style:name", sColumnName);
			columnStyleOpen.addAttribute("style:family", "table-column");
			columnStyleOpen.write(pHandler);

			pHandler->startElement("style:table-column-properties", j());
			pHandler->endElement("style:table-column-properties");

			pHandler->endElement("style:style");
		}
	}

	std::map<librevenge::RVNGString, shared_ptr<SheetRowStyle> >::const_iterator rIt;
	for (rIt=mRowStyleHash.begin(); rIt!=mRowStyleHash.end(); ++rIt)
	{
		if (!rIt->second) continue;
		rIt->second->writeStyle(pHandler, manager);
	}

	std::map<librevenge::RVNGString, shared_ptr<SheetCellStyle> >::const_iterator cIt;
	for (cIt=mCellStyleHash.begin(); cIt!=mCellStyleHash.end(); ++cIt)
	{
		if (!cIt->second) continue;
		cIt->second->writeStyle(pHandler, manager);
	}
}

librevenge::RVNGString SheetStyle::addRow(const librevenge::RVNGPropertyList &propList)
{
	// first remove unused data
	librevenge::RVNGPropertyList pList;
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (!strncmp(i.key(), "librevenge:", 11))
			continue;
		if (i.child())
			continue;
		pList.insert(i.key(),i()->clone());
	}
	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mRowNameHash.find(hashKey);
	if (iter!=mRowNameHash.end()) return iter->second;

	librevenge::RVNGString name;
	name.sprintf("%s_row%i", getName().cstr(), (int) mRowStyleHash.size());
	mRowNameHash[hashKey]=name;
	mRowStyleHash[name]=shared_ptr<SheetRowStyle>(new SheetRowStyle(propList, name.cstr()));
	return name;
}

librevenge::RVNGString SheetStyle::addCell(const librevenge::RVNGPropertyList &propList)
{
	// first remove unused data
	librevenge::RVNGPropertyList pList;
	librevenge::RVNGPropertyList::Iter i(propList);
	for (i.rewind(); i.next();)
	{
		if (!strncmp(i.key(), "librevenge:", 11) &&
		        strncmp(i.key(), "librevenge:numbering-name", 24))
			continue;
		if (i.child())
			continue;
		pList.insert(i.key(),i()->clone());
	}
	librevenge::RVNGString hashKey = pList.getPropString();
	std::map<librevenge::RVNGString, librevenge::RVNGString>::const_iterator iter =
	    mCellNameHash.find(hashKey);
	if (iter!=mCellNameHash.end()) return iter->second;

	librevenge::RVNGString name;
	name.sprintf("%s_cell%i", getName().cstr(), (int) mCellStyleHash.size());
	mCellNameHash[hashKey]=name;
	mCellStyleHash[name]=shared_ptr<SheetCellStyle>(new SheetCellStyle(propList, name.cstr()));
	return name;
}

SheetManager::SheetManager() : mbSheetOpened(false), mSheetStyles(), mNumberingHash()
{
}

SheetManager::~SheetManager()
{
}

void SheetManager::clean()
{
	mSheetStyles.clear();
}

bool SheetManager::openSheet(const librevenge::RVNGPropertyList &xPropList, Style::Zone zone)
{
	if (mbSheetOpened)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::oops: a sheet is already open\n"));
		return false;
	}
	mbSheetOpened=true;
	if (zone==Style::Z_Unknown)
		zone=Style::Z_ContentAutomatic;

	librevenge::RVNGString sTableName;
	if (zone==Style::Z_StyleAutomatic)
		sTableName.sprintf("Sheet_M%i", (int) mSheetStyles.size());
	else
		sTableName.sprintf("Sheet%i", (int) mSheetStyles.size());
	shared_ptr<SheetStyle> sheet(new SheetStyle(xPropList, sTableName.cstr(), zone));
	mSheetStyles.push_back(sheet);
	return true;
}

bool SheetManager::closeSheet()
{
	if (!mbSheetOpened)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::oops: no sheet are opened\n"));
		return false;
	}
	mbSheetOpened=false;
	return true;
}

librevenge::RVNGString SheetManager::getNumberingStyleName(librevenge::RVNGString const &localName) const
{
	std::map<librevenge::RVNGString, shared_ptr<SheetNumberingStyle> >::const_iterator it=
	    mNumberingHash.find(localName);
	if (it==mNumberingHash.end() || !it->second)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::getNumberingStyleName: can not find %s\n", localName.cstr()));
		return librevenge::RVNGString("");
	}
	return it->second->getName();
}

void SheetManager::addNumberingStyle(const librevenge::RVNGPropertyList &xPropList)
{
	if (!xPropList["librevenge:name"] || xPropList["librevenge:name"]->getStr().len()==0)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::addNumberingStyle: can not find the style name\n"));
		return;
	}
	librevenge::RVNGString name(xPropList["librevenge:name"]->getStr());
	librevenge::RVNGString finalName;
	if (mNumberingHash.find(name)!=mNumberingHash.end() && mNumberingHash.find(name)->second)
		finalName=mNumberingHash.find(name)->second->getName();
	else
		finalName.sprintf("Numbering_num%i", (int) mNumberingHash.size());

	shared_ptr<SheetNumberingStyle> style(new SheetNumberingStyle(xPropList, finalName));
	mNumberingHash[name]=style;
}

librevenge::RVNGString SheetManager::convertFormula(const librevenge::RVNGPropertyListVector &formula)
{
	librevenge::RVNGString res("");
	std::stringstream s;
	s << "of:=";
	for (unsigned long i=0; i<formula.count(); ++i)
	{
		librevenge::RVNGPropertyList const &list=formula[i];
		if (!list["librevenge:type"])
		{
			ODFGEN_DEBUG_MSG(("SheetManager::convertFormula can not find %s formula type !!!\n", s.str().c_str()));
			return res;
		}
		std::string type(list["librevenge:type"]->getStr().cstr());
		if (type=="librevenge-operator")
		{
			if (!list["librevenge:operator"])
			{
				ODFGEN_DEBUG_MSG(("SheetManager::convertFormula can not find operator for formula[%s]!!!\n", s.str().c_str()));
				return res;
			}
			std::string oper(list["librevenge:operator"]->getStr().cstr());
			bool find=false;
			for (int w=0; w<15; ++w)
			{
				static char const *(s_operators[15])=
				{
					"(", ")", "+", "-", "*", "/", "=", "<>",  ";", "<", ">", "<=", ">=", "^", "&"
				};
				if (oper!=s_operators[w]) continue;
				s << oper;
				find=true;
				break;
			}
			if (!find)
			{
				ODFGEN_DEBUG_MSG(("SheetManager::convertFormula find unknown operator for formula[%s]!!!\n", s.str().c_str()));
				return res;
			}
		}
		else if (type=="librevenge-function")
		{
			if (!list["librevenge:function"])
			{
				ODFGEN_DEBUG_MSG(("SheetManager::convertFormula can not find value for formula[%s]!!!\n", s.str().c_str()));
				return res;
			}
			s << list["librevenge:function"]->getStr().cstr();
		}
		else if (type=="librevenge-number")
		{
			if (!list["librevenge:number"])
			{
				ODFGEN_DEBUG_MSG(("SheetManager::convertFormula can not find value for formula[%s]!!!\n", s.str().c_str()));
				return res;
			}
			s << list["librevenge:number"]->getStr().cstr();
		}
		else if (type=="librevenge-text")
		{
			if (!list["librevenge:text"])
			{
				ODFGEN_DEBUG_MSG(("SheetManager::convertFormula can not find text for formula[%s]!!!\n", s.str().c_str()));
				return res;
			}
			librevenge::RVNGString escaped;
			escaped.appendEscapedXML(list["librevenge:text"]->getStr());
			s << "\"" << escaped.cstr() << "\"";
		}
		else if (type=="librevenge-cell")
		{
			librevenge::RVNGString range=convertCellRange(list);
			if (range.empty()) return res;
			s << "[" << range.cstr() << "]";
		}
		else if (type=="librevenge-cells")
		{
			librevenge::RVNGString ranges=convertCellsRange(list);
			if (ranges.empty()) return res;
			s << "[" << ranges.cstr() << "]";
		}
		else
		{
			ODFGEN_DEBUG_MSG(("SheetManager::convertFormula find unknown type %s!!!\n", type.c_str()));
			return res;
		}
	}
	return librevenge::RVNGString::escapeXML(s.str().c_str());
}

librevenge::RVNGString SheetManager::convertCellRange(const librevenge::RVNGPropertyList &list)
{
	std::stringstream s;
	librevenge::RVNGString res("");
	if (!list["librevenge:row"]||!list["librevenge:column"])
	{
		ODFGEN_DEBUG_MSG(("SheetManager::convertCellRange can not find cordinate!!!\n"));
		return res;
	}
	int column=list["librevenge:column"]->getInt();
	int row=list["librevenge:row"]->getInt();
	if (column<0 || row<0)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::convertCellRange: find bad coordinate!!!\n"));
		return res;
	}
	if (list["librevenge:sheet"]) s << list["librevenge:sheet"]->getStr().cstr();
	s << ".";
	if (list["librevenge:column-absolute"] && list["librevenge:column-absolute"]->getInt()) s << "$";
	if (column>=26) s << char('A'+(column/26-1));
	s << char('A'+(column%26));
	if (list["librevenge:row-absolute"] && list["librevenge:row-absolute"]->getInt()) s << "$";
	s << row+1;
	return s.str().c_str();
}

librevenge::RVNGString SheetManager::convertCellsRange(const librevenge::RVNGPropertyList &list)
{
	std::stringstream s;
	librevenge::RVNGString res("");
	if (!list["librevenge:start-row"]||!list["librevenge:start-column"])
	{
		ODFGEN_DEBUG_MSG(("SheetManager::convertCellsRange can not find cordinate!!!\n"));
		return res;
	}
	int column=list["librevenge:start-column"]->getInt();
	int row=list["librevenge:start-row"]->getInt();
	if (column<0 || row<0)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::convertCellsRange: find bad coordinate1!!!\n"));
		return res;
	}
	if (list["librevenge:sheet-name"]) s << list["librevenge:sheet-name"]->getStr().cstr();
	s << ".";
	if (list["librevenge:start-column-absolute"] && list["librevenge:start-column-absolute"]->getInt()) s << "$";
	if (column>=26) s << char('A'+(column/26-1));
	s << char('A'+(column%26));
	if (list["librevenge:start-row-absolute"] && list["librevenge:start-row-absolute"]->getInt()) s << "$";
	s << row+1 << ":";
	if (list["librevenge:end-column"])
		column=list["librevenge:end-column"]->getInt();
	if (list["librevenge:end-row"])
		row=list["librevenge:end-row"]->getInt();
	if (column<0 || row<0)
	{
		ODFGEN_DEBUG_MSG(("SheetManager::convertCellsRange: find bad coordinate2!!!\n"));
		return res;
	}
	if (list["librevenge:end-column-absolute"] && list["librevenge:end-column-absolute"]->getInt()) s << "$";
	if (column>=26) s << char('A'+(column/26-1));
	s << char('A'+(column%26));
	if (list["librevenge:end-row-absolute"] && list["librevenge:end-row-absolute"]->getInt()) s << "$";
	s << row+1;
	return s.str().c_str();
}

void SheetManager::write(OdfDocumentHandler *pHandler, Style::Zone zone) const
{
	if (zone==Style::Z_ContentAutomatic)
	{
		std::map<librevenge::RVNGString, shared_ptr<SheetNumberingStyle> >::const_iterator nIt;
		for (nIt=mNumberingHash.begin(); nIt!=mNumberingHash.end(); ++nIt)
		{
			if (!nIt->second) continue;
			nIt->second->writeStyle(pHandler, *this);
		}
	}

	for (size_t i=0; i < mSheetStyles.size(); ++i)
	{
		if (mSheetStyles[i] && mSheetStyles[i]->getZone()==zone)
			mSheetStyles[i]->writeStyle(pHandler, *this);
	}
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
