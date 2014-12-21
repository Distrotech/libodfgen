/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwpd.sourceforge.net
 */

#include "DocumentElement.hxx"

#include "FilterInternal.hxx"

#include <cstdarg>
#include <cstdio>

librevenge::RVNGString libodfgen::doubleToString(const double value)
{
	librevenge::RVNGProperty *prop = librevenge::RVNGPropertyFactory::newDoubleProp(value);
	librevenge::RVNGString retVal = prop->getStr();
	delete prop;
	return retVal;
}

bool libodfgen::getInchValue(librevenge::RVNGProperty const &prop, double &value)
{
	value=prop.getDouble();
	switch (prop.getUnit())
	{
	case librevenge::RVNG_GENERIC: // assume inch
	case librevenge::RVNG_INCH:
		return true;
	case librevenge::RVNG_POINT:
		value /= 72.;
		return true;
	case librevenge::RVNG_TWIP:
		value /= 1440.;
		return true;
	case librevenge::RVNG_PERCENT:
	case librevenge::RVNG_UNIT_ERROR:
	default:
	{
		static bool first=true;
		if (first)
		{
			ODFGEN_DEBUG_MSG(("libodfgen::getInchValue: call with no double value\n"));
			first=false;
		}
		break;
	}
	}
	return false;
}

namespace libodfgen
{
DocumentElementVector::~DocumentElementVector()
{
}

void DocumentElementVector::resize(size_t newSize)
{
	mpElements.resize(newSize);
}

void DocumentElementVector::push_back(shared_ptr<DocumentElement> elt)
{
	mpElements.push_back(elt);
}

void DocumentElementVector::push_back(DocumentElement *elt)
{
	mpElements.push_back(shared_ptr<DocumentElement>(elt));
}

void DocumentElementVector::appendTo(DocumentElementVector &res)
{
	for (size_t i=0; i<mpElements.size(); ++i)
		res.mpElements.push_back(mpElements[i]);
}

void debugPrint(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);
}

}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
