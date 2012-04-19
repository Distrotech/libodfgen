/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
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

#include <libmwaw/TMWAWPropertyHandler.hxx>

class OdfDocumentHandler;

namespace MWAWObjectHandlerInternal
{
class Shape
{
public:
	Shape() : m_type(BAD), m_styleId(-1), m_w(0), m_h(0), m_rw(0), m_rh(0),
		m_x(), m_y(), m_angle()
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
	bool drawPolygon(OdfDocumentHandler *output) const;

	enum Type { LINE, RECTANGLE, CIRCLE, ARC, POLYGON, BAD } m_type;

	int m_styleId;
	double m_w,m_h, m_rw, m_rh;
	std::vector<double> m_x,m_y;
	std::vector<double> m_angle;
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

class MWAWObjectHandler : public TMWAWPropertyHandler
{
public:
	MWAWObjectHandler(OdfDocumentHandler *output) : TMWAWPropertyHandler(), m_document(), m_output(output) { }
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
	MWAWObjectHandlerInternal::Document m_document;
	OdfDocumentHandler *m_output;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
