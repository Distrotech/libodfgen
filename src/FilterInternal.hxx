/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2003 William Lachance (wrlach@gmail.com)
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

#ifndef __FILTERINTERNAL_HXX__
#define __FILTERINTERNAL_HXX__

#include <string.h> // for strcmp

#include <vector>

#include <librevenge/librevenge.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef DEBUG
#include <stdio.h>
#define ODFGEN_DEBUG_MSG(M) printf M
#else
#define ODFGEN_DEBUG_MSG(M)
#endif

#if defined(SHAREDPTR_TR1)
#include <tr1/memory>
using std::tr1::shared_ptr;
#elif defined(SHAREDPTR_STD)
#include <memory>
using std::shared_ptr;
#else
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#endif

#define ODFGEN_N_ELEMENTS(m) sizeof(m)/sizeof(m[0])

class DocumentElement;

namespace libodfgen
{

librevenge::RVNGString doubleToString(const double value);
bool getInchValue(librevenge::RVNGProperty const &prop, double &value);

//! small class used to store a list of DocumentElement
class DocumentElementVector : public std::vector<DocumentElement *>
{
public:
	//! constructor
	DocumentElementVector() : std::vector<DocumentElement *>() {}
	//! destructor
	~DocumentElementVector()
	{
		clear();
	}
	//! delete all document element
	void clear();
};

} // namespace libodfgen

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
