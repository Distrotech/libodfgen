/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <math.h>

#include <algorithm>
#include <string>

#include <librevenge/librevenge.h>

// removeme for ODFGEN_DEBUG_MSG
#include "FilterInternal.hxx"

#include "GraphicFunctions.hxx"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace libodfgen
{
double getAngle(double bx, double by);
void getEllipticalArcBBox(double x0, double y0,
                          double rx, double ry, double phi, bool largeArc, bool sweep, double x, double y,
                          double &xmin, double &ymin, double &xmax, double &ymax);
double quadraticExtreme(double t, double a, double b, double c);
double quadraticDerivative(double a, double b, double c);
void getQuadraticBezierBBox(double x0, double y0, double x1, double y1, double x, double y,
                            double &xmin, double &ymin, double &xmax, double &ymax);
double cubicBase(double t, double a, double b, double c, double d);
void getCubicBezierBBox(double x0, double y0, double x1, double y1, double x2, double y2, double x, double y,
                        double &xmin, double &ymin, double &xmax, double &ymax);


double getAngle(double bx, double by)
{
	return fmod(2*M_PI + (by > 0.0 ? 1.0 : -1.0) * acos(bx / sqrt(bx * bx + by * by)), 2*M_PI);
}

void getEllipticalArcBBox(double x0, double y0,
                          double rx, double ry, double phi, bool largeArc, bool sweep, double x, double y,
                          double &xmin, double &ymin, double &xmax, double &ymax)
{
	phi *= M_PI/180;
	if (rx < 0.0)
		rx *= -1.0;
	if (ry < 0.0)
		ry *= -1.0;

	double const absError=1e-5;
	if ((rx>-absError && rx<absError) || (ry>-absError && ry<absError))
	{
		xmin = (x0 < x ? x0 : x);
		xmax = (x0 > x ? x0 : x);
		ymin = (y0 < y ? y0 : y);
		ymax = (y0 > y ? y0 : y);
		return;
	}

	// F.6.5.1
	const double x1prime = cos(phi)*(x0 - x)/2 + sin(phi)*(y0 - y)/2;
	const double y1prime = -sin(phi)*(x0 - x)/2 + cos(phi)*(y0 - y)/2;

	// F.6.5.2
	double radicant = (rx*rx*ry*ry - rx*rx*y1prime*y1prime - ry*ry*x1prime*x1prime)/(rx*rx*y1prime*y1prime + ry*ry*x1prime*x1prime);
	double cxprime = 0.0;
	double cyprime = 0.0;
	if (radicant < 0.0)
	{
		double ratio = rx/ry;
		radicant = y1prime*y1prime + x1prime*x1prime/(ratio*ratio);
		if (radicant < 0.0)
		{
			xmin = (x0 < x ? x0 : x);
			xmax = (x0 > x ? x0 : x);
			ymin = (y0 < y ? y0 : y);
			ymax = (y0 > y ? y0 : y);
			return;
		}
		ry=sqrt(radicant);
		rx=ratio*ry;
	}
	else
	{
		double factor = (largeArc==sweep ? -1.0 : 1.0)*sqrt(radicant);

		cxprime = factor*rx*y1prime/ry;
		cyprime = -factor*ry*x1prime/rx;
	}

	// F.6.5.3
	double cx = cxprime*cos(phi) - cyprime*sin(phi) + (x0 + x)/2;
	double cy = cxprime*sin(phi) + cyprime*cos(phi) + (y0 + y)/2;

	// now compute bounding box of the whole ellipse

	// Parametric equation of an ellipse:
	// x(theta) = cx + rx*cos(theta)*cos(phi) - ry*sin(theta)*sin(phi)
	// y(theta) = cy + rx*cos(theta)*sin(phi) + ry*sin(theta)*cos(phi)

	// Compute local extrems
	// 0 = -rx*sin(theta)*cos(phi) - ry*cos(theta)*sin(phi)
	// 0 = -rx*sin(theta)*sin(phi) - ry*cos(theta)*cos(phi)

	// Local extrems for X:
	// theta = -atan(ry*tan(phi)/rx)
	// and
	// theta = M_PI -atan(ry*tan(phi)/rx)

	// Local extrems for Y:
	// theta = atan(ry/(tan(phi)*rx))
	// and
	// theta = M_PI + atan(ry/(tan(phi)*rx))

	double txmin, txmax, tymin, tymax;

	// First handle special cases
	if ((phi > -absError&&phi < absError) || (phi > M_PI-absError && phi < M_PI+absError))
	{
		xmin = cx - rx;
		txmin = getAngle(-rx, 0);
		xmax = cx + rx;
		txmax = getAngle(rx, 0);
		ymin = cy - ry;
		tymin = getAngle(0, -ry);
		ymax = cy + ry;
		tymax = getAngle(0, ry);
	}
	else if ((phi > M_PI / 2.0-absError && phi < M_PI / 2.0+absError) ||
	         (phi > 3.0*M_PI/2.0-absError && phi < 3.0*M_PI/2.0+absError))
	{
		xmin = cx - ry;
		txmin = getAngle(-ry, 0);
		xmax = cx + ry;
		txmax = getAngle(ry, 0);
		ymin = cy - rx;
		tymin = getAngle(0, -rx);
		ymax = cy + rx;
		tymax = getAngle(0, rx);
	}
	else
	{
		txmin = -atan(ry*tan(phi)/rx);
		txmax = M_PI - atan(ry*tan(phi)/rx);
		xmin = cx + rx*cos(txmin)*cos(phi) - ry*sin(txmin)*sin(phi);
		xmax = cx + rx*cos(txmax)*cos(phi) - ry*sin(txmax)*sin(phi);
		double tmpY = cy + rx*cos(txmin)*sin(phi) + ry*sin(txmin)*cos(phi);
		txmin = getAngle(xmin - cx, tmpY - cy);
		tmpY = cy + rx*cos(txmax)*sin(phi) + ry*sin(txmax)*cos(phi);
		txmax = getAngle(xmax - cx, tmpY - cy);

		tymin = atan(ry/(tan(phi)*rx));
		tymax = atan(ry/(tan(phi)*rx))+M_PI;
		ymin = cy + rx*cos(tymin)*sin(phi) + ry*sin(tymin)*cos(phi);
		ymax = cy + rx*cos(tymax)*sin(phi) + ry*sin(tymax)*cos(phi);
		double tmpX = cx + rx*cos(tymin)*cos(phi) - ry*sin(tymin)*sin(phi);
		tymin = getAngle(tmpX - cx, ymin - cy);
		tmpX = cx + rx*cos(tymax)*cos(phi) - ry*sin(tymax)*sin(phi);
		tymax = getAngle(tmpX - cx, ymax - cy);
	}
	if (xmin > xmax)
	{
		std::swap(xmin,xmax);
		std::swap(txmin,txmax);
	}
	if (ymin > ymax)
	{
		std::swap(ymin,ymax);
		std::swap(tymin,tymax);
	}
	double angle1 = getAngle(x0 - cx, y0 - cy);
	double angle2 = getAngle(x - cx, y - cy);

	// for sweep == 0 it is normal to have delta theta < 0
	// but we don't care about the rotation direction for bounding box
	if (!sweep)
		std::swap(angle1, angle2);

	// We cannot check directly for whether an angle is included in
	// an interval of angles that cross the 360/0 degree boundary
	// So here we will have to check for their absence in the complementary
	// angle interval
	bool otherArc = false;
	if (angle1 > angle2)
	{
		std::swap(angle1, angle2);
		otherArc = true;
	}

	// Check txmin
	if ((!otherArc && (angle1 > txmin || angle2 < txmin)) || (otherArc && !(angle1 > txmin || angle2 < txmin)))
		xmin = x0 < x ? x0 : x;
	// Check txmax
	if ((!otherArc && (angle1 > txmax || angle2 < txmax)) || (otherArc && !(angle1 > txmax || angle2 < txmax)))
		xmax = x0 > x ? x0 : x;
	// Check tymin
	if ((!otherArc && (angle1 > tymin || angle2 < tymin)) || (otherArc && !(angle1 > tymin || angle2 < tymin)))
		ymin = y0 < y ? y0 : y;
	// Check tymax
	if ((!otherArc && (angle1 > tymax || angle2 < tymax)) || (otherArc && !(angle1 > tymax || angle2 < tymax)))
		ymax = y0 > y ? y0 : y;
}

double quadraticExtreme(double t, double a, double b, double c)
{
	return (1.0-t)*(1.0-t)*a + 2.0*(1.0-t)*t*b + t*t*c;
}

double quadraticDerivative(double a, double b, double c)
{
	double denominator = a - 2.0*b + c;
	if (fabs(denominator)>1e-10*(a-b))
		return (a - b)/denominator;
	return -1.0;
}

void getQuadraticBezierBBox(double x0, double y0, double x1, double y1, double x, double y,
                            double &xmin, double &ymin, double &xmax, double &ymax)
{
	xmin = x0 < x ? x0 : x;
	xmax = x0 > x ? x0 : x;
	ymin = y0 < y ? y0 : y;
	ymax = y0 > y ? y0 : y;

	double t = quadraticDerivative(x0, x1, x);
	if (t>=0 && t<=1)
	{
		double tmpx = quadraticExtreme(t, x0, x1, x);
		xmin = tmpx < xmin ? tmpx : xmin;
		xmax = tmpx > xmax ? tmpx : xmax;
	}

	t = quadraticDerivative(y0, y1, y);
	if (t>=0 && t<=1)
	{
		double tmpy = quadraticExtreme(t, y0, y1, y);
		ymin = tmpy < ymin ? tmpy : ymin;
		ymax = tmpy > ymax ? tmpy : ymax;
	}
}

double cubicBase(double t, double a, double b, double c, double d)
{
	return (1.0-t)*(1.0-t)*(1.0-t)*a + 3.0*(1.0-t)*(1.0-t)*t*b + 3.0*(1.0-t)*t*t*c + t*t*t*d;
}

void getCubicBezierBBox(double x0, double y0, double x1, double y1, double x2, double y2, double x, double y,
                        double &xmin, double &ymin, double &xmax, double &ymax)
{
	xmin = x0 < x ? x0 : x;
	xmax = x0 > x ? x0 : x;
	ymin = y0 < y ? y0 : y;
	ymax = y0 > y ? y0 : y;

	for (int i=0; i<=100; ++i)
	{
		double t=double(i)/100.;
		double tmpx = cubicBase(t, x0, x1, x2, x);
		xmin = tmpx < xmin ? tmpx : xmin;
		xmax = tmpx > xmax ? tmpx : xmax;
		double tmpy = cubicBase(t, y0, y1, y2, y);
		ymin = tmpy < ymin ? tmpy : ymin;
		ymax = tmpy > ymax ? tmpy : ymax;
	}
}

static double getInchValue(librevenge::RVNGProperty const *prop)
{
	double value=prop->getDouble();
	switch (prop->getUnit())
	{
	case librevenge::RVNG_INCH:
	case librevenge::RVNG_GENERIC: // assume inch
		return value;
	case librevenge::RVNG_POINT:
		return value/72.;
	case librevenge::RVNG_TWIP:
		return value/1440.;
	case librevenge::RVNG_PERCENT:
	case librevenge::RVNG_UNIT_ERROR:
	default:
	{
		static bool first=true;
		if (first)
		{
			ODFGEN_DEBUG_MSG(("GraphicFunctions::getInchValue: call with no double value\n"));
			first=false;
		}
		break;
	}
	}
	return value;
}
bool getPathBBox(const librevenge::RVNGPropertyListVector &path, double &px, double &py, double &qx, double &qy)
{
	// This must be a mistake and we do not want to crash lower
	if (!path.count() || !path[0]["librevenge:path-action"] || path[0]["librevenge:path-action"]->getStr() == "Z")
	{
		ODFGEN_DEBUG_MSG(("libodfgen:getPathBdBox: get a spurious path\n"));
		return false;
	}

	// try to find the bounding box
	// this is simple convex hull technique, the bounding box might not be
	// accurate but that should be enough for this purpose
	bool isFirstPoint = true;

	double lastX = 0.0;
	double lastY = 0.0;
	double lastPrevX = 0.0;
	double lastPrevY = 0.0;
	px = py = qx = qy = 0.0;

	for (unsigned k = 0; k < path.count(); ++k)
	{
		if (!path[k]["librevenge:path-action"])
			continue;
		std::string action=path[k]["librevenge:path-action"]->getStr().cstr();
		if (action.length()!=1 || action[0]=='Z') continue;

		bool coordOk=path[k]["svg:x"]&&path[k]["svg:y"];
		bool coord1Ok=coordOk && path[k]["svg:x1"]&&path[k]["svg:y1"];
		bool coord2Ok=coord1Ok && path[k]["svg:x2"]&&path[k]["svg:y2"];
		double x=lastX, y=lastY;
		if (isFirstPoint)
		{
			if (!coordOk)
			{
				ODFGEN_DEBUG_MSG(("OdgGeneratorPrivate::_drawPath: the first point has no coordinate\n"));
				continue;
			}
			qx = px = x = getInchValue(path[k]["svg:x"]);
			qy = py = y = getInchValue(path[k]["svg:y"]);
			lastPrevX = lastX = px;
			lastPrevY = lastY = py;
			isFirstPoint = false;
		}
		else
		{
			if (path[k]["svg:x"]) x=getInchValue(path[k]["svg:x"]);
			if (path[k]["svg:y"]) y=getInchValue(path[k]["svg:y"]);
			px = (px > x) ? x : px;
			py = (py > y) ? y : py;
			qx = (qx < x) ? x : qx;
			qy = (qy < y) ? y : qy;
		}

		double xmin=px, xmax=qx, ymin=py, ymax=qy;
		bool lastPrevSet=false;

		if (action[0] == 'C' && coord2Ok)
		{
			getCubicBezierBBox(lastX, lastY, getInchValue(path[k]["svg:x1"]), getInchValue(path[k]["svg:y1"]),
			                   getInchValue(path[k]["svg:x2"]), getInchValue(path[k]["svg:y2"]),
			                   x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-getInchValue(path[k]["svg:x2"]);
			lastPrevY=2*y-getInchValue(path[k]["svg:y2"]);
		}
		else if (action[0] == 'S' && coord1Ok)
		{
			getCubicBezierBBox(lastX, lastY, lastPrevX, lastPrevY,
			                   getInchValue(path[k]["svg:x1"]), getInchValue(path[k]["svg:y1"]),
			                   x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-getInchValue(path[k]["svg:x1"]);
			lastPrevY=2*y-getInchValue(path[k]["svg:y1"]);
		}
		else if (action[0] == 'Q' && coord1Ok)
		{
			getQuadraticBezierBBox(lastX, lastY, getInchValue(path[k]["svg:x1"]), getInchValue(path[k]["svg:y1"]),
			                       x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-getInchValue(path[k]["svg:x1"]);
			lastPrevY=2*y-getInchValue(path[k]["svg:y1"]);
		}
		else if (action[0] == 'T' && coordOk)
		{
			getQuadraticBezierBBox(lastX, lastY, lastPrevX, lastPrevY,
			                       x, y, xmin, ymin, xmax, ymax);
			lastPrevSet=true;
			lastPrevX=2*x-lastPrevX;
			lastPrevY=2*y-lastPrevY;
		}
		else if (action[0] == 'A' && coordOk && path[k]["svg:rx"] && path[k]["svg:ry"])
		{
			getEllipticalArcBBox(lastX, lastY, getInchValue(path[k]["svg:rx"]), getInchValue(path[k]["svg:ry"]),
			                     path[k]["librevenge:rotate"] ? path[k]["librevenge:rotate"]->getDouble() : 0.0,
			                     path[k]["librevenge:large-arc"] ? path[k]["librevenge:large-arc"]->getInt() : 1,
			                     path[k]["librevenge:sweep"] ? path[k]["librevenge:sweep"]->getInt() : 1,
			                     x, y, xmin, ymin, xmax, ymax);
		}
		else if (action[0] != 'M' && action[0] != 'L' && action[0] != 'H' && action[0] != 'V')
		{
			ODFGEN_DEBUG_MSG(("OdgGeneratorPrivate::_drawPath: problem reading a path\n"));
		}
		px = (px > xmin ? xmin : px);
		py = (py > ymin ? ymin : py);
		qx = (qx < xmax ? xmax : qx);
		qy = (qy < ymax ? ymax : qy);
		lastX = x;
		lastY = y;
		if (!lastPrevSet)
		{
			lastPrevX=lastX;
			lastPrevY=lastY;
		}
	}
	return true;
}

librevenge::RVNGString convertPath(const librevenge::RVNGPropertyListVector &path, double px, double py)
{
	librevenge::RVNGString sValue("");
	for (unsigned i = 0; i < path.count(); ++i)
	{
		if (!path[i]["librevenge:path-action"])
			continue;
		std::string action=path[i]["librevenge:path-action"]->getStr().cstr();
		if (action.length()!=1) continue;
		bool coordOk=path[i]["svg:x"]&&path[i]["svg:y"];
		bool coord1Ok=coordOk && path[i]["svg:x1"]&&path[i]["svg:y1"];
		bool coord2Ok=coord1Ok && path[i]["svg:x2"]&&path[i]["svg:y2"];
		librevenge::RVNGString sElement;
		// 2540 is 2.54*1000, 2.54 in = 1 inch
		if (path[i]["svg:x"] && action[0] == 'H')
		{
			sElement.sprintf("H%i", (int)((getInchValue(path[i]["svg:x"])-px)*2540));
			sValue.append(sElement);
		}
		else if (path[i]["svg:y"] && action[0] == 'V')
		{
			sElement.sprintf("V%i", (int)((getInchValue(path[i]["svg:y"])-py)*2540));
			sValue.append(sElement);
		}
		else if (coordOk && (action[0] == 'M' || action[0] == 'L' || action[0] == 'T'))
		{
			sElement.sprintf("%c%i %i", action[0], (int)((getInchValue(path[i]["svg:x"])-px)*2540),
			                 (int)((getInchValue(path[i]["svg:y"])-py)*2540));
			sValue.append(sElement);
		}
		else if (coord1Ok && (action[0] == 'Q' || action[0] == 'S'))
		{
			sElement.sprintf("%c%i %i %i %i", action[0], (int)((getInchValue(path[i]["svg:x1"])-px)*2540),
			                 (int)((getInchValue(path[i]["svg:y1"])-py)*2540), (int)((getInchValue(path[i]["svg:x"])-px)*2540),
			                 (int)((getInchValue(path[i]["svg:y"])-py)*2540));
			sValue.append(sElement);
		}
		else if (coord2Ok && action[0] == 'C')
		{
			sElement.sprintf("C%i %i %i %i %i %i", (int)((getInchValue(path[i]["svg:x1"])-px)*2540),
			                 (int)((getInchValue(path[i]["svg:y1"])-py)*2540), (int)((getInchValue(path[i]["svg:x2"])-px)*2540),
			                 (int)((getInchValue(path[i]["svg:y2"])-py)*2540), (int)((getInchValue(path[i]["svg:x"])-px)*2540),
			                 (int)((getInchValue(path[i]["svg:y"])-py)*2540));
			sValue.append(sElement);
		}
		else if (coordOk && path[i]["svg:rx"] && path[i]["svg:ry"] && action[0] == 'A')
		{
			sElement.sprintf("A%i %i %i %i %i %i %i", (int)((getInchValue(path[i]["svg:rx"]))*2540),
			                 (int)((getInchValue(path[i]["svg:ry"]))*2540), (path[i]["librevenge:rotate"] ? path[i]["librevenge:rotate"]->getInt() : 0),
			                 (path[i]["librevenge:large-arc"] ? path[i]["librevenge:large-arc"]->getInt() : 1),
			                 (path[i]["librevenge:sweep"] ? path[i]["librevenge:sweep"]->getInt() : 1),
			                 (int)((getInchValue(path[i]["svg:x"])-px)*2540), (int)((getInchValue(path[i]["svg:y"])-py)*2540));
			sValue.append(sElement);
		}
		else if (action[0] == 'Z')
			sValue.append(" Z");
	}
	return sValue;
}

}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
