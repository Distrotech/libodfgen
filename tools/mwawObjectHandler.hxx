/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* writerperfect
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2007 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <libmwaw/libmwaw.hxx>

#if !defined(MWAW_OBJECT_HANDLER_H) && MWAW_GRAPHIC_EXPORT==1
#define MWAW_OBJECT_HANDLER_H

#include <string.h>

#include <string>
#include <vector>

#include <libwpd/libwpd.h>
#include <libwpd/WPXProperty.h>
#include <libwpd/WPXString.h>

class OdfDocumentHandler;

namespace MWAWObjectHandlerInternal
{
class Shape
{
public:
	Shape() : m_type(BAD), m_styleId(-1), m_w(0), m_h(0), m_rw(0), m_rh(0),
		m_x(), m_y(), m_angle(), m_path("")
	{
	}
	bool read(const char *psName, WPXPropertyList const &xPropList, int styleId);
	bool write(OdfDocumentHandler *output) const;
	bool ok() const
	{
		return m_type != BAD;
	}

protected:
	bool drawLine(OdfDocumentHandler *output) const;
	bool drawRectangle(OdfDocumentHandler *output) const;
	bool drawCircle(OdfDocumentHandler *output) const;
	bool drawArc(OdfDocumentHandler *output) const;
	bool drawPath(OdfDocumentHandler *output) const;
	bool drawPolygon(OdfDocumentHandler *output, bool is2D) const;

	enum Type { LINE, RECTANGLE, CIRCLE, ARC, PATH, POLYLINE, POLYGON, BAD } m_type;

	int m_styleId;
	double m_w,m_h, m_rw, m_rh;
	std::vector<double> m_x,m_y;
	std::vector<double> m_angle;
	std::string m_path;
};

class Document
{
public:
	Document() : styles(), shapes(), m_w(0.0), m_h(0.0) {}
	bool open(const char *psName, WPXPropertyList const &xPropList);
	bool close(const char *psName);

	void characters(WPXString const &sCharacters);

	void write(OdfDocumentHandler *output);

protected:
	static void writeStyle(OdfDocumentHandler *output,
	                       WPXPropertyList const &style, int styleId);

	std::vector<WPXPropertyList> styles;
	std::vector<Shape> shapes;

protected:
	double m_w, m_h;
};

}

class MWAWObjectHandler : public MWAWPropertyHandler
{
public:
	MWAWObjectHandler(OdfDocumentHandler *output) : MWAWPropertyHandler(), m_document(), m_output(output) { }
	~MWAWObjectHandler() {};

	void startDocument()
	{
		m_document= MWAWObjectHandlerInternal::Document();
	};
	void endDocument()
	{
		if (m_output) m_document.write(m_output);
	};

	void startElement(const char *psName, const WPXPropertyList &xPropList)
	{
		m_document.open(psName, xPropList);
	}
	void endElement(const char *psName)
	{
		m_document.close(psName);
	}
	void characters(const WPXString &sCharacters)
	{
		m_document.characters(sCharacters);
	}

private:
	MWAWObjectHandler(MWAWObjectHandler const &);
	MWAWObjectHandler operator=(MWAWObjectHandler const &);

	MWAWObjectHandlerInternal::Document m_document;
	OdfDocumentHandler *m_output;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
