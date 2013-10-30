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

#include <algorithm>
#include <math.h>

#include "GraphicFunctions.hxx"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace libodfgen
{

double getAngle(double bx, double by)
{
	return fmod(2*M_PI + (by > 0.0 ? 1.0 : -1.0) * acos( bx / sqrt(bx * bx + by * by) ), 2*M_PI);
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

	if (rx == 0.0 || ry == 0.0)
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
	if (phi == 0 || phi == M_PI)
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
	else if (phi == M_PI / 2.0 || phi == 3.0*M_PI/2.0)
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
		txmax = M_PI - atan (ry*tan(phi)/rx);
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
	if (fabs(denominator) != 0.0)
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
	if(t>=0 && t<=1)
	{
		double tmpx = quadraticExtreme(t, x0, x1, x);
		xmin = tmpx < xmin ? tmpx : xmin;
		xmax = tmpx > xmax ? tmpx : xmax;
	}

	t = quadraticDerivative(y0, y1, y);
	if(t>=0 && t<=1)
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

	for (double t = 0.0; t <= 1.0; t+=0.01)
	{
		double tmpx = cubicBase(t, x0, x1, x2, x);
		xmin = tmpx < xmin ? tmpx : xmin;
		xmax = tmpx > xmax ? tmpx : xmax;
		double tmpy = cubicBase(t, y0, y1, y2, y);
		ymin = tmpy < ymin ? tmpy : ymin;
		ymax = tmpy > ymax ? tmpy : ymax;
	}
}

}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
