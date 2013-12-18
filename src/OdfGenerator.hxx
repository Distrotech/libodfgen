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

#ifndef _ODFGENERATOR_HXX_
#define _ODFGENERATOR_HXX_

#include <map>
#include <stack>
#include <string>
#include <vector>

#include <librevenge/librevenge.h>

#include "libodfgen/OdfDocumentHandler.hxx"

#include "FilterInternal.hxx"
#include "FontStyle.hxx"
#include "GraphicStyle.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"

class DocumentElement;
class ListStyle;

class OdfGenerator
{
public:
	typedef std::vector<DocumentElement *> Storage;

	//! constructor
	OdfGenerator();
	//! destructor
	virtual ~OdfGenerator();
	//! retrieve data from another odfgenerator ( the list and the embedded handler)
	void initStateWith(OdfGenerator const &orig);

	//
	// general
	//

	//! returns the document type corresponding to stream type
	static std::string getDocumentType(OdfStreamType streamType);
	//! store the document meta data
	void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);
	//! write the document meta data
	void writeDocumentMetaData(OdfDocumentHandler *pHandler);

	//
	// document handler
	//

	//! add a document handler
	void addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	//! calls writeTargetDocument on each document handler
	void writeTargetDocuments();
	//! a virtual function used to write final data
	virtual bool writeTargetDocument(OdfDocumentHandler *pHandler, OdfStreamType streamType) = 0;

	//
	// embedded image/object handler
	//

	//! register an embedded object handler
	void registerEmbeddedObjectHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedObject objectHandler);
	//! register an embedded image handler
	void registerEmbeddedImageHandler(const librevenge::RVNGString &mimeType, OdfEmbeddedImage imageHandler);
	//! returns a embedded object handler if it exists
	OdfEmbeddedObject findEmbeddedObjectHandler(const librevenge::RVNGString &mimeType) const;
	//! returns a embedded image handler if it exists
	OdfEmbeddedImage findEmbeddedImageHandler(const librevenge::RVNGString &mimeType) const;

	//
	// storage
	//

	//! returns the current storage
	Storage *getCurrentStorage()
	{
		return mpCurrentStorage;
	}
	//! push a storage
	void pushStorage(Storage *newStorage);
	/** pop a storage */
	bool popStorage();
	//! returns the body storage
	Storage &getBodyStorage()
	{
		return mBodyStorage;
	}
	//! returns the meta data storage
	Storage &getMetaDataStorage()
	{
		return mMetaDataStorage;
	}
	//! delete the storage content ( but not the Storage pointer )
	static void emptyStorage(Storage *storage);
	//! write the storage data to a document handler
	static void sendStorage(Storage const *storage, OdfDocumentHandler *pHandler);

	//
	// basic to the font/paragraph/span manager
	//

	void insertTab();
	void insertSpace();
	void insertLineBreak(bool forceParaClose=false);
	void insertField(const librevenge::RVNGPropertyList &field);
	void insertText(const librevenge::RVNGString &text);

	//! open a link
	void openLink(const librevenge::RVNGPropertyList &propList);
	//! close a link
	void closeLink();

	//! define a character style
	void defineCharacterStyle(const librevenge::RVNGPropertyList &propList);
	//! open a span
	void openSpan(const librevenge::RVNGPropertyList &propList);
	//! close a span
	void closeSpan();

	//! define a paragraph style
	void defineParagraphStyle(const librevenge::RVNGPropertyList &propList);
	//! returns a paragraph name corresponding to proplist
	librevenge::RVNGString getParagraphName(const librevenge::RVNGPropertyList &propList)
	{
		return mParagraphManager.findOrAdd(propList);
	}
	//! open a paragraph
	void openParagraph(const librevenge::RVNGPropertyList &propList);
	//! close a paragraph
	void closeParagraph();

	//
	// list function
	//

	/// pop the list state (if possible)
	void popListState();
	/// push the list state by adding an empty value
	void pushListState();

	/// call to define a list level
	void defineListLevel(const librevenge::RVNGPropertyList &propList, bool ordered);
	/// call to open a list level
	void openListLevel(const librevenge::RVNGPropertyList &propList, bool ordered);
	/// call to close a list level
	void closeListLevel();
	/// call to open a list element
	void openListElement(const librevenge::RVNGPropertyList &propList);
	/// call to close a list element
	void closeListElement();

	//
	// table
	//

	/// call to open a table
	void openTable(const librevenge::RVNGPropertyList &propList);
	/// call to close a table
	void closeTable();
	/// call to open a table row
	bool openTableRow(const librevenge::RVNGPropertyList &propList);
	/// call to close a table row
	void closeTableRow();
	/// returns true if a table row is opened
	bool isInTableRow(bool &inHeaderRow) const;
	/// call to open a table cell
	bool openTableCell(const librevenge::RVNGPropertyList &propList);
	/// call to close a table cell
	void closeTableCell();
	void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList);

	//
	// frame/group
	//

	/// call to open a frame
	void openFrame(const librevenge::RVNGPropertyList &propList);
	/// call to close a frame
	void closeFrame();
	//! return a frame id corresponding to a name ( or a new frame id)
	unsigned getFrameId(librevenge::RVNGString name="");
	/// call to open a group
	void openGroup(const librevenge::RVNGPropertyList &propList);
	/// call to close a group
	void closeGroup();

	//
	// image
	//

	//! inserts a binary object
	void insertBinaryObject(const librevenge::RVNGPropertyList &propList);

	//
	// graphic
	//

	//! call to define a graphic style
	void defineGraphicStyle(const librevenge::RVNGPropertyList &propList);
	//! returns the last graphic style (REMOVEME)
	librevenge::RVNGPropertyList const &getGraphicStyle() const
	{
		return mGraphicStyle;
	}

	//! call to draw an ellipse
	void drawEllipse(const librevenge::RVNGPropertyList &propList);
	//! call to draw a path
	void drawPath(const librevenge::RVNGPropertyList &propList);
	//! call to draw a polygon or a polyline
	void drawPolySomething(const librevenge::RVNGPropertyList &vertices, bool isClosed);
	//! call to draw a rectangle
	void drawRectangle(const librevenge::RVNGPropertyList &propList);

protected:

	//
	// list
	//

	// list state
	struct ListState
	{
		ListState();
		ListState(const ListState &state);

		ListStyle *mpCurrentListStyle;
		unsigned int miCurrentListLevel;
		unsigned int miLastListLevel;
		unsigned int miLastListNumber;
		bool mbListContinueNumbering;
		bool mbListElementParagraphOpened;
		std::stack<bool> mbListElementOpened;
	private:
		ListState &operator=(const ListState &state);
	};

	/// access to the current list state
	ListState &getListState();

	/** stores a list style: update mListStyles,
		mWriterListStates.top().mpCurrentListStyle and the different
		maps
	 */
	void storeListStyle(ListStyle *listStyle);
	/** retrieves the list style corresponding to a given id. */
	void retrieveListStyle(int id);

	//
	// frame/graphic
	//

	//! returns the list of properties which must be add to a frame, shape, ...
	void addFrameProperties(const librevenge::RVNGPropertyList &propList, TagOpenElement &element) const;
	//! call to draw a path
	void drawPath(const librevenge::RVNGPropertyListVector &path, const librevenge::RVNGPropertyList &propList);
	//! returns the current graphic style name ( MODIFYME)
	librevenge::RVNGString getCurrentGraphicStyleName();

	// the current set of elements that we're writing to
	Storage *mpCurrentStorage;
	// the stack of all storage
	std::stack<Storage *> mStorageStack;
	// the meta data elements
	Storage mMetaDataStorage;
	// content elements
	Storage mBodyStorage;

	// font manager
	FontStyleManager mFontManager;
	// graphic manager
	GraphicStyleManager mGraphicManager;
	// span manager
	SpanStyleManager mSpanManager;
	// paragraph manager
	ParagraphStyleManager mParagraphManager;
	// table manager
	TableManager mTableManager;

	// id to span map
	std::map<int, librevenge::RVNGPropertyList> mIdSpanMap;
	// id to span name map
	std::map<int, librevenge::RVNGString> mIdSpanNameMap;
	// the last span name
	librevenge::RVNGString mLastSpanName;

	// id to paragraph map
	std::map<int, librevenge::RVNGPropertyList> mIdParagraphMap;
	// id to paragraph name map
	std::map<int, librevenge::RVNGString> mIdParagraphNameMap;
	// the last span name
	librevenge::RVNGString mLastParagraphName;

	// list styles
	unsigned int miNumListStyles;
	// list styles
	std::vector<ListStyle *> mListStyles;
	// list states
	std::stack<ListState> mListStates;
	// a map id -> last list style defined with id
	std::map<int, ListStyle *> mIdListStyleMap;

	// the number of created frame
	unsigned miFrameNumber;
	// the list of frame seens
	std::map<librevenge::RVNGString, unsigned, ltstr > mFrameNameIdMap;

	// the last graphic style
	librevenge::RVNGPropertyList mGraphicStyle;

	// the document handlers
	std::map<OdfStreamType, OdfDocumentHandler *> mDocumentStreamHandlers;

	// embedded image handlers
	std::map<librevenge::RVNGString, OdfEmbeddedImage, ltstr > mImageHandlers;
	// embedded object handlers
	std::map<librevenge::RVNGString, OdfEmbeddedObject, ltstr > mObjectHandlers;

private:
	// copy constructor (unimplemented)
	OdfGenerator(OdfGenerator const &);
	// copy operator (unimplemented)
	OdfGenerator &operator=(OdfGenerator const &);
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
