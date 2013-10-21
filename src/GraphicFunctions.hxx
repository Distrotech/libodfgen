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

#ifndef GRAPHIC_FUNCTIONS_HXX_INCLUDED
#define GRAPHIC_FUNCTIONS_HXX_INCLUDED

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

}

#endif // GRAPHIC_FUNCTIONS_HXX_INCLUDED

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
