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
#include <set>
#include <stack>
#include <string>

#include <librevenge/librevenge.h>

#include "libodfgen/OdfDocumentHandler.hxx"

#include "FilterInternal.hxx"
#include "FontStyle.hxx"
#include "GraphicStyle.hxx"
#include "InternalHandler.hxx"
#include "PageSpan.hxx"
#include "TableStyle.hxx"
#include "TextRunStyle.hxx"

class DocumentElement;
class ListStyle;

class OdfGenerator
{
public:
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

	//! basic container used to store objects of not flat odf files
	struct ObjectContainer
	{
		//! constructor
		ObjectContainer(librevenge::RVNGString const &type, bool isDir)
			: mType(type), mIsDir(isDir), mStorage(), mInternalHandler(&mStorage)
		{
		}
		//! destructor
		~ObjectContainer();
		//! the file type
		librevenge::RVNGString mType;
		//! true if the file is not a plain file
		bool mIsDir;
		//! the contain storage
		libodfgen::DocumentElementVector mStorage;
		//! the handler
		InternalHandler mInternalHandler;
	private:
		ObjectContainer(ObjectContainer const &orig);
		ObjectContainer &operator=(ObjectContainer const &orig);
	};
	/** creates a new object */
	ObjectContainer &createObjectFile(librevenge::RVNGString const &objectName,
	                                  librevenge::RVNGString const &objectType,
	                                  bool isDir=false);
	/** returns the list created embedded object (needed to create chart) */
	librevenge::RVNGStringVector getObjectNames() const;
	/** retrieve an embedded object content via a document handler */
	bool getObjectContent(librevenge::RVNGString const &objectName, OdfDocumentHandler *pHandler);

	//! add a document handler
	void addDocumentHandler(OdfDocumentHandler *pHandler, const OdfStreamType streamType);
	//! calls writeTargetDocument on each document handler
	void writeTargetDocuments();
	//! appends local files in the manifest
	void appendFilesInManifest(OdfDocumentHandler *pHandler);
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

	//! append layer in master styles
	void appendLayersMasterStyles(OdfDocumentHandler *pHandler);


	//
	// storage
	//

	//! returns the current storage
	libodfgen::DocumentElementVector *getCurrentStorage()
	{
		return mpCurrentStorage;
	}
	//! push a storage
	void pushStorage(libodfgen::DocumentElementVector *newStorage);
	/** pop a storage */
	bool popStorage();
	//! returns the body storage
	libodfgen::DocumentElementVector &getBodyStorage()
	{
		return mBodyStorage;
	}
	//! returns the meta data storage
	libodfgen::DocumentElementVector &getMetaDataStorage()
	{
		return mMetaDataStorage;
	}
	//! write the storage data to a document handler
	static void sendStorage(libodfgen::DocumentElementVector const *storage, OdfDocumentHandler *pHandler);

	// page, header/footer, master page

	//! return the page span manager
	PageSpanManager &getPageSpanManager()
	{
		return mPageSpanManager;
	}

	//! starts a header/footer page.
	void startHeaderFooter(bool header, const librevenge::RVNGPropertyList &propList);
	//! ends a header/footer page
	void endHeaderFooter();
	//! returns if we are in a master page
	bool inHeaderFooter() const
	{
		return mbInHeaderFooter;
	}

	//! starts a master page.
	void startMasterPage(const librevenge::RVNGPropertyList &propList);
	//! ends a master page
	void endMasterPage();
	//! returns if we are in a master page
	bool inMasterPage() const
	{
		return mbInMasterPage;
	}

	//! returns if we must store the automatic style in the style or in the content xml zones
	bool useStyleAutomaticZone() const
	{
		return mbInHeaderFooter || mbInMasterPage;
	}

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

	//! return the font manager
	FontStyleManager &getFontManager()
	{
		return mFontManager;
	}

	//! define a character style
	void defineCharacterStyle(const librevenge::RVNGPropertyList &propList);
	//! open a span
	void openSpan(const librevenge::RVNGPropertyList &propList);
	//! close a span
	void closeSpan();

	//! define a paragraph style
	void defineParagraphStyle(const librevenge::RVNGPropertyList &propList);
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
	// frame/group/layer
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
	/// call to open layer.
	void openLayer(const librevenge::RVNGPropertyList &propList);
	/// call to close a layer
	void closeLayer();
	/// return the layer name: either propList[draw:layer] if it exists or the current layer name
	librevenge::RVNGString getLayerName(const librevenge::RVNGPropertyList &propList) const;

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
	//! call to draw a connector
	void drawConnector(const ::librevenge::RVNGPropertyList &propList);

	//
	// chart
	//

	//! define a chart style
	void defineChartStyle(const librevenge::RVNGPropertyList &propList);

	//
	// font
	//

	//! add embedded font
	void defineEmbeddedFont(const librevenge::RVNGPropertyList &propList);

protected:

	//
	// frame/graphic
	//

	//! returns the list of properties which must be add to a frame, shape, ...
	void addFrameProperties(const librevenge::RVNGPropertyList &propList, TagOpenElement &element) const;
	//! call to draw a path
	void drawPath(const librevenge::RVNGPropertyListVector &path, const librevenge::RVNGPropertyList &propList);
	//! returns the current graphic style name ( MODIFYME)
	librevenge::RVNGString getCurrentGraphicStyleName(const librevenge::RVNGPropertyList &shapeList);
	//! returns the current graphic style name ( MODIFYME)
	librevenge::RVNGString getCurrentGraphicStyleName();

	// the current set of elements that we're writing to
	libodfgen::DocumentElementVector *mpCurrentStorage;
	// the stack of all storage
	std::stack<libodfgen::DocumentElementVector *> mStorageStack;
	// the meta data elements
	libodfgen::DocumentElementVector mMetaDataStorage;
	// content elements
	libodfgen::DocumentElementVector mBodyStorage;

	// page span manager
	PageSpanManager mPageSpanManager;
	// font manager
	FontStyleManager mFontManager;
	// graphic manager
	GraphicStyleManager mGraphicManager;
	// span manager
	SpanStyleManager mSpanManager;
	// paragraph manager
	ParagraphStyleManager mParagraphManager;
	// list manager
	ListManager mListManager;
	// table manager
	TableManager mTableManager;

	// a flag to know if we are in a header/footer zone
	bool mbInHeaderFooter;

	// a flag to know if we are in a master page
	bool mbInMasterPage;

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
	// the last paragraph name
	librevenge::RVNGString mLastParagraphName;

	// the number of created frame
	unsigned miFrameNumber;
	// the list of frame seens
	std::map<librevenge::RVNGString, unsigned > mFrameNameIdMap;

	// the layer name stack
	std::stack<librevenge::RVNGString> mLayerNameStack;
	// the list of layer (final name)
	std::set<librevenge::RVNGString> mLayerNameSet;
	// the layer original name to final name
	std::map<librevenge::RVNGString, librevenge::RVNGString> mLayerNameMap;

	// the last graphic style
	librevenge::RVNGPropertyList mGraphicStyle;

	// id to chart map
	std::map<int, librevenge::RVNGPropertyList> mIdChartMap;
	// id to chart name map
	std::map<int, librevenge::RVNGString> mIdChartNameMap;

	//
	// handler and/or object creation
	//

	// the document handlers
	std::map<OdfStreamType, OdfDocumentHandler *> mDocumentStreamHandlers;

	// the number of created object
	int miObjectNumber;
	// name to object map
	std::map<librevenge::RVNGString, ObjectContainer *> mNameObjectMap;

	// embedded image handlers
	std::map<librevenge::RVNGString, OdfEmbeddedImage > mImageHandlers;
	// embedded object handlers
	std::map<librevenge::RVNGString, OdfEmbeddedObject > mObjectHandlers;

	bool mCurrentParaIsHeading;

private:
	// copy constructor (unimplemented)
	OdfGenerator(OdfGenerator const &);
	// copy operator (unimplemented)
	OdfGenerator &operator=(OdfGenerator const &);
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
