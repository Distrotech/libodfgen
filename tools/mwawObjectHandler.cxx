/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* writerperfect
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

#include <libmwaw/libmwaw.hxx>

#if MWAW_GRAPHIC_EXPORT==1
#include "mwawObjectHandler.hxx"

#include <stdlib.h>

#include <sstream>
#include <string>

#include <libodfgen/libodfgen.hxx>

#include <libodfgen/FilterInternal.hxx>

namespace MWAWObjectHandlerInternal
{
//! Internal: creates the string "f pt"
static std::string getStringPt(double f)
{
	std::stringstream s;
	s << float(f) << "pt";
	return s.str();
}
static double getSizeInPt(WPXProperty const &prop)
{
	WPXString str = prop.getStr();
	if (!str.len()) return 0.0;

	// we have a string, so we can not use getDouble
	std::istringstream iss(str.cstr());
	double res = 0.0;
	iss >> res;

	// try to guess the type
	// point->pt, twip->*, inch -> in
	char c = str.len() ? str.cstr()[str.len()-1] : ' ';
	if (c == '*') res /= 1440.;
	else if (c == 't') res /= 72.;
	else if (c == 'n') ;
	else if (c == '%')
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::getSizeInPoint: called with a percent property\n"));
	}
	return res *= 72.;
}
static std::string getStringSizeInPt(WPXProperty const &prop)
{
	return getStringPt(getSizeInPt(prop));
}

static std::string getStyleName(int id)
{
	std::stringstream s;
	s.str("");
	s << "bd" << id+1;
	return s.str();
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
bool Shape::read(const char *psName, WPXPropertyList const &xPropList, int stylId)
{
	m_styleId = stylId;
	m_type = BAD;
	if (strcmp(psName,"drawLine")==0) m_type = LINE;
	else if (strcmp(psName,"drawRectangle")==0) m_type = RECTANGLE;
	else if (strcmp(psName,"drawCircle")==0) m_type = CIRCLE;
	else if (strcmp(psName,"drawArc")==0) m_type = ARC;
	else if (strcmp(psName,"drawPath")==0) m_type = PATH;
	else if (strcmp(psName,"drawPolyline")==0) m_type = POLYLINE;
	else if (strcmp(psName,"drawPolygon")==0) m_type = POLYGON;
	else return false;

	WPXPropertyList::Iter i(xPropList);
	for (i .rewind(); i.next(); )
	{
		if (strcmp(i.key(), "w") == 0) m_w = getSizeInPt(*i());
		else if (strcmp(i.key(), "h") == 0) m_h = getSizeInPt(*i());
		else if (strcmp(i.key(), "rw") == 0) m_rw = getSizeInPt(*i());
		else if (strcmp(i.key(), "rh") == 0) m_rh = getSizeInPt(*i());
		else if (strcmp(i.key(), "path") == 0)
		{
			if (i()->getStr().len())
				m_path = i()->getStr().cstr();
		}
		else
		{
			char const *key = i.key();
			int len = (int) strlen(key);
			bool readOk = len > 1, generic = false;
			std::vector<double> *which = 0L;
			if (readOk && strncmp(i.key(),"x",1)==0)
			{
				which = &m_x;
				key++;
			}
			else if (readOk && strncmp(i.key(),"y",1)==0)
			{
				which = &m_y;
				key++;
			}
			else if (readOk && strncmp(i.key(),"angle",5)==0)
			{
				which = &m_angle;
				key+=5;
				readOk = len>5;
				generic=true;
			}
			else readOk = false;

			long w;
			if (readOk)
			{
				char *res;
				w = strtol(key, &res, 10);
				readOk = (*res=='\0') && (w >= 0);
			}
			if (readOk)
			{
				if (int(which->size()) < w+1) which->resize(size_t(w)+1,0.0);
				double unit = generic ? 1./72.0 : 1.0;
				(*which)[size_t(w)] = getSizeInPt(*i()) * unit;
			}
			if (!readOk)
			{
				WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::read: find an unknown key '%s'\n",i.key()));
			}
		}
	}

	return true;
}

bool Shape::write(OdfDocumentHandler *output) const
{
	if (!output) return true;
	if (m_type == LINE) return drawLine(output);
	else if (m_type == RECTANGLE) return drawRectangle(output);
	else if (m_type == CIRCLE) return drawCircle(output);
	else if (m_type == ARC) return drawArc(output);
	else if (m_type == PATH) return drawPath(output);
	else if (m_type == POLYLINE) return drawPolygon(output, false);
	else if (m_type == POLYGON) return drawPolygon(output, true);

	return false;
}

bool Shape::drawLine(OdfDocumentHandler *output) const
{
	if (m_x.size() < 2 || m_y.size() < 2)
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawLine: PB\n"));
		return false;
	}

	WPXPropertyList list;
	list.insert("draw:text-style-name","P1");
	list.insert("draw:layer","layout");
	list.insert("draw:style-name",getStyleName(m_styleId).c_str());
	list.insert("svg:x1",getStringPt(m_x[0]).c_str());
	list.insert("svg:y1",getStringPt(m_y[0]).c_str());
	list.insert("svg:x2",getStringPt(m_x[1]).c_str());
	list.insert("svg:y2",getStringPt(m_y[1]).c_str());
	output->startElement("draw:line", list);
	output->endElement("draw:line");
	return true;
}

bool Shape::drawRectangle(OdfDocumentHandler *output) const
{
	if (m_x.size() < 1 || m_y.size() < 1)
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawRectangle: PB\n"));
		return false;
	}

	WPXPropertyList list;
	list.insert("draw:text-style-name","P1");
	list.insert("draw:layer","layout");
	list.insert("draw:style-name",getStyleName(m_styleId).c_str());
	list.insert("svg:x",getStringPt(m_x[0]).c_str());
	list.insert("svg:y",getStringPt(m_y[0]).c_str());
	list.insert("svg:width",getStringPt(m_w).c_str());
	list.insert("svg:height",getStringPt(m_h).c_str());

	if (m_rw <= 0.0 || m_rh <= 0.0 || m_w < 2*m_rw || m_h < 2*m_rh)
	{
		if (m_rw > 0.0 || m_rh > 0.0)
		{
			WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawRectangle:: can only create a rectangle\n"));
		}
		output->startElement("draw:rect", list);
		output->endElement("draw:rect");
		return true;
	}

	// ok, we draw a rond rect
	std::stringstream s;
	float const unit = 1.0;//35.3;
	s.str("");

	double const minPt[] = { m_x[0] *unit, m_y[0] *unit };
	double const maxPt[] = { (m_w+m_x[0]) *unit, (m_h+m_y[0]) *unit };

	s << "0 0 " << int(maxPt[0]) << " " << int(maxPt[1]);
	list.insert("svg:viewBox", s.str().c_str());

	double const W[] = { m_rw *unit, m_rh *unit };
	double const xPt[] = { minPt[0]+W[0], maxPt[0]-W[0], maxPt[0],
	                       maxPt[0], maxPt[0], maxPt[0],
	                       maxPt[0]-W[0], minPt[0]+W[0], minPt[0],
	                       minPt[0], minPt[0], minPt[0],
	                       minPt[0]+W[0]
	                     };
	double const yPt[] = { minPt[1], minPt[1], minPt[1],
	                       minPt[1]+W[1], maxPt[1]-W[1], maxPt[1],
	                       maxPt[1], maxPt[1], maxPt[1],
	                       maxPt[1]-W[1], minPt[1]+W[1], minPt[1],
	                       minPt[1]
	                     };
	s.str("");
	for (int i = 0; i < 13; i++)
	{
		if (i) s << " ";

		if (i == 0) s << "M";
		else if ((i%3) == 2) s << "Q";
		else if ((i%3) == 0);
		else s << "L";
		s << int(xPt[i]) << " " << int(yPt[i]);
	}
	s << "Z";
	list.insert("svg:d", s.str().c_str());

	output->startElement("draw:path", list);
	output->endElement("draw:path");
	return true;
}

bool Shape::drawCircle(OdfDocumentHandler *output) const
{
	if (m_x.size() < 1 || m_y.size() < 1)
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawCircle: PB\n"));
		return false;
	}

	WPXPropertyList list;
	list.insert("draw:text-style-name","P1");
	list.insert("draw:layer","layout");
	list.insert("draw:style-name",getStyleName(m_styleId).c_str());
	list.insert("svg:x",getStringPt(m_x[0]).c_str());
	list.insert("svg:y",getStringPt(m_y[0]).c_str());
	list.insert("svg:width",getStringPt(m_w).c_str());
	list.insert("svg:height",getStringPt(m_h).c_str());
	if (m_w < m_h || m_w > m_h)
	{
		output->startElement("draw:ellipse", list);
		output->endElement("draw:ellipse");
	}
	else
	{
		output->startElement("draw:circle", list);
		output->endElement("draw:circle");
	}
	return true;
}

bool Shape::drawArc(OdfDocumentHandler *output) const
{
	if (m_angle.size() < 2)
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawArc: angle are filled, call draw Circle\n"));
		return drawCircle(output);
	}
	if (m_x.size() < 1 || m_y.size() < 1)
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawArc: PB\n"));
		return false;
	}

	WPXPropertyList list;
	list.insert("draw:text-style-name","P1");
	list.insert("draw:layer","layout");
	list.insert("draw:style-name",getStyleName(m_styleId).c_str());
	list.insert("svg:x",getStringPt(m_x[0]).c_str());
	list.insert("svg:y",getStringPt(m_y[0]).c_str());
	list.insert("svg:width",getStringPt(m_w).c_str());
	list.insert("svg:height",getStringPt(m_h).c_str());
	list.insert("draw:kind","arc");

	std::stringstream s;
	// odg prefers angle between -360 and +360, ...
	int minAngl = int(m_angle[0]), maxAngl = int(m_angle[1]);
	if (minAngl >= 360 || maxAngl >= 360)
	{
		minAngl -= 360;
		maxAngl -= 360;
	}
	s.str("");
	s << minAngl;
	list.insert("draw:start-angle", s.str().c_str());
	s.str("");
	s << maxAngl;
	list.insert("draw:end-angle", s.str().c_str());

	if (m_w < m_h || m_w > m_h)
	{
		output->startElement("draw:ellipse", list);
		output->endElement("draw:ellipse");
	}
	else
	{
		output->startElement("draw:circle", list);
		output->endElement("draw:circle");
	}
	return true;
}

bool Shape::drawPolygon(OdfDocumentHandler *output, bool is2D) const
{
	if (m_x.size() < 1 || m_y.size() != m_x.size())
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawPolygon: PB\n"));
		return false;
	}
	std::stringstream s;
	WPXPropertyList list;
	list.insert("draw:text-style-name","P1");
	list.insert("draw:layer","layout");
	list.insert("draw:style-name","bd1");
	list.insert("svg:x","0pt");
	list.insert("svg:y","0pt");
	list.insert("svg:width",getStringPt(m_w).c_str());
	list.insert("svg:height",getStringPt(m_h).c_str());

	size_t numPt = m_x.size();

	float const unit = 1; //35.2777; // convert in centimeters
	s.str("");
	s << "0 0 " << int(m_w*unit) << " " << int(m_h*unit);
	list.insert("svg:viewBox", s.str().c_str());

	s.str("");
	for (size_t i = 0; i < numPt; i++)
	{
		if (i) s << " ";
		s << int(m_x[i]*unit) << "," << int(m_y[i]*unit);
	}
	list.insert("draw:points", s.str().c_str());
	if (!is2D)
	{
		output->startElement("draw:polyline", list);
		output->endElement("draw:polyline");
	}
	else
	{
		output->startElement("draw:polygon", list);
		output->endElement("draw:polygon");
	}
	return true;
}

bool Shape::drawPath(OdfDocumentHandler *output) const
{
	if (m_path.length()==0 || m_w <= 0 || m_h <= 0)
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Shape::drawPath: PB\n"));
		return false;
	}

	WPXPropertyList list;
	list.insert("draw:text-style-name","P1");
	list.insert("draw:layer","layout");
	list.insert("draw:style-name",getStyleName(m_styleId).c_str());
	list.insert("svg:x","0pt");
	list.insert("svg:y","0pt");
	list.insert("svg:width",getStringPt(m_w).c_str());
	list.insert("svg:height",getStringPt(m_h).c_str());
	std::stringstream s;
	s << "0 0 " << int(m_w) << " " << int(m_h);
	list.insert("svg:viewBox", s.str().c_str());
	list.insert("svg:d",m_path.c_str());

	output->startElement("draw:path", list);
	output->endElement("draw:path");
	return true;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
bool Document::open(const char *psName, WPXPropertyList const &xPropList)
{
	if (strncmp(psName,"libmwaw:", 8) == 0)
		psName += 8;
	else
	{
		WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Document::open Unknown tag '%s'..\n", psName));
		return false;
	}
	if (strcmp(psName, "document") == 0)
	{
		m_w = m_h = 0.;
		WPXPropertyList::Iter i(xPropList);
		for (i .rewind(); i.next(); )
		{
			if (strcmp(i.key(), "w") == 0) m_w = getSizeInPt(*i());
			else if (strcmp(i.key(), "h") == 0) m_h = getSizeInPt(*i());
			else
			{
				WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Document::open: find an unknown key '%s'\n",i.key()));
			}
		}
		return true;
	}
	else if (strcmp(psName, "graphicStyle") == 0)
	{
		styles.push_back(xPropList);
		return true;
	}
	else
	{
		int id = int(styles.size());
		Shape shape;
		if (shape.read(psName, xPropList, id ? id-1 : 0))
		{
			if (id == 0)
			{
				WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Document::open shape created without style..\n"));
				styles.push_back(WPXPropertyList());
			}
			shapes.push_back(shape);
			return true;
		}
	}
	WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Document::open Unknown tag '%s'..\n", psName));
	return false;
}

bool Document::close(const char *)
{
	return true;
}

void Document::characters(WPXString const &)
{
	WRITER_DEBUG_MSG(("Document::characters must NOT be called..\n"));
}

void Document::write(OdfDocumentHandler *output)
{
	if (!output) return;
	WPXPropertyList list;
	std::stringstream s;

	list.clear();
	list.insert("office:mimetype","application/vnd.oasis.opendocument.graphics");
	list.insert("office:version", "1.0");

	list.insert("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	list.insert("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	list.insert("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	list.insert("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	list.insert("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	list.insert("xmlns:ooo", "http://openoffice.org/2004/office");
	list.insert("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	list.insert("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	list.insert("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");

	output->startElement("office:document", list);

	// SETTINGS
	{
		list.clear();
		output->startElement("office:settings", list);
		list.clear();
		list.insert("config:name","ooo:view-settings");
		output->startElement("config:config-item-set", list);

		// - Top
		list.clear();
		list.insert("config:name","VisibleAreaTop");
		list.insert("config:type","int");
		output->startElement("config:config-item", list);
		output->characters("0");

		output->endElement("config:config-item");
		// - Left
		list.clear();
		list.insert("config:name","VisibleAreaLeft");
		list.insert("config:type","int");
		output->startElement("config:config-item", list);
		output->characters("0");
		output->endElement("config:config-item");
		// - Width
		list.clear();
		list.insert("config:name","VisibleAreaWidth");
		list.insert("config:type","int");
		s.str("");
		s << int(m_w*35.2777);  // *2540/72. why ?
		output->startElement("config:config-item", list);
		output->characters(s.str().c_str());
		output->endElement("config:config-item");
		// - Height
		list.clear();
		list.insert("config:name","VisibleAreaHeight");
		list.insert("config:type","int");
		s.str("");
		s << int(m_h*35.2777);  // *2540/72. why ?
		output->startElement("config:config-item", list);
		output->characters(s.str().c_str());
		output->endElement("config:config-item");

		output->endElement("config:config-item-set");
		output->endElement("office:settings");
	}

	// STYLES
	{
		list.clear();
		output->startElement("office:styles", list);
		// - a gradient
		list.clear();
		list.insert("draw:angle","0");
		list.insert("draw:border","0%");
		list.insert("draw:end-color","#000000");
		list.insert("draw:end-intensity","100%");
		list.insert("draw:name","Gradient_1");
		list.insert("draw:start-color","#000000");
		list.insert("draw:start-intensity","100%");
		list.insert("draw:style","linear");
		output->startElement("draw:gradient", list);
		output->endElement("draw:gradient");
		// - an arrow
		list.clear();
		list.insert("draw:name","Arrow");
		list.insert("svg:viewBox","0 0 20 30");
		list.insert("svg:d","m10 0-10 30h20z");
		output->startElement("draw:marker", list);
		output->endElement("draw:marker");

		output->endElement("office:styles");
	}

	// AUTOMATIC STYLES
	{
		list.clear();
		output->startElement("office:automatic-styles", list);

		// - PM0
		{
			list.clear();
			list.insert("style:name","PM0");
			output->startElement("style:page-layout", list);
			list.clear();
			list.insert("fo:margin-bottom","0in");
			list.insert("fo:margin-left","0in");
			list.insert("fo:margin-right","0in");
			list.insert("fo:margin-top","0in");
			list.insert("fo:page-height",getStringPt(m_h).c_str());
			list.insert("fo:page-width",getStringPt(m_w).c_str());
			list.insert("style:print-orientation","portrait");
			output->startElement("style:page-layout-properties", list);
			output->endElement("style:page-layout-properties");
			output->endElement("style:page-layout");
		}

		// - dp1
		{
			list.clear();
			list.insert("style:family","drawing-page");
			list.insert("style:name","dp1");
			output->startElement("style:style", list);
			list.clear();
			list.insert("draw:fill","none");
			output->startElement("style:drawing-page-properties", list);
			output->endElement("style:drawing-page-properties");
			output->endElement("style:style");
		}

		// -- the styles
		for (size_t i = 0; i < styles.size() ; i++)
			writeStyle(output, styles[i], int(i));

		output->endElement("office:automatic-styles");
	}

	// MASTER STYLES
	{
		list.clear();
		output->startElement("office:master-styles", list);
		list.clear();
		list.insert("draw:style-name","dp1");
		list.insert("style:name","Default");
		list.insert("style:page-layout-name","PM0");
		output->startElement("style:master-page", list);
		output->endElement("style:master-page");
		output->endElement("office:master-styles");
	}

	// BODY
	{
		list.clear();
		output->startElement("office:body", list);
		output->startElement("office:drawing", list);
		{
			list.clear();
			list.insert("draw:master-page-name","Default");
			list.insert("draw:name","page1");
			list.insert("draw:style-name","dp1");

			output->startElement("draw:page", list);

			for (size_t i = 0; i < shapes.size() ; i++)
				shapes[i].write(output);

			output->endElement("draw:page");
		}
		output->endElement("office:drawing");
		output->endElement("office:body");
	}
	output->endElement("office:document");
}

void Document::writeStyle
(OdfDocumentHandler *output, WPXPropertyList const &style, int styleId)
{
	if (!output) return;

	WPXPropertyList list;
	list.clear();
	list.insert("style:family","graphic");
	list.insert("style:name",getStyleName(styleId).c_str());
	list.insert("style:parent-style-name","standard");
	output->startElement("style:style", list);
	{
		list.clear();

		WPXPropertyList::Iter i(style);
		for (i .rewind(); i.next(); )
		{
			if (strcmp(i.key(), "lineColor") == 0)
				list.insert("svg:stroke-color", i()->getStr().cstr());
			else if (strcmp(i.key(), "lineWidth") == 0)
				list.insert("svg:stroke-width", getStringSizeInPt(*i()).c_str());
			else if (strcmp(i.key(), "lineFill") == 0)
				list.insert("draw:stroke", i()->getStr().cstr());
			else if (strcmp(i.key(), "surfaceColor") == 0)
				list.insert("draw:fill-color", i()->getStr().cstr());
			else if (strcmp(i.key(), "surfaceFill") == 0)
				list.insert("draw:fill", i()->getStr().cstr());
			else if (strcmp(i.key(), "startArrow") == 0)
			{
				if (strcmp(i()->getStr().cstr(), "true") == 0)
				{
					list.insert("draw:marker-start", "Arrow");
					list.insert("draw:marker-start-center", "false");
				}
			}
			else if (strcmp(i.key(), "startArrowWidth") == 0)
				list.insert("draw:marker-start-width",  getStringSizeInPt(*i()).c_str());
			else if (strcmp(i.key(), "endArrow") == 0)
			{
				if (strcmp(i()->getStr().cstr(), "true") == 0)
				{
					list.insert("draw:marker-end", "Arrow");
					list.insert("draw:marker-end-center", "false");
				}
			}
			else if (strcmp(i.key(), "endArrowWidth") == 0)
				list.insert("draw:marker-end-width",  getStringSizeInPt(*i()).c_str());
			else
			{
				WRITER_DEBUG_MSG(("MWAWObjectHandlerInternal::Document::writeStyle: find an unknown key '%s'\n",i.key()));
			}
		}

		output->startElement("style:graphic-properties", list);
		output->endElement("style:graphic-properties");
	}
	output->endElement("style:style");
}
}
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
