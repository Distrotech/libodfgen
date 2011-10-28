/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* FilterInternal: Debugging information
 *
 * Copyright (C) 2002-2003 William Lachance (wrlach@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * For further information visit http://libwpd.sourceforge.net
 *
 */
#ifndef __FILTERINTERNAL_HXX__
#define __FILTERINTERNAL_HXX__

#include <string.h> // for strcmp

#include <libwpd/libwpd.h>
#include <libwpd/WPXString.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// used by FontStyle.cxx
#define IMP_DEFAULT_FONT_PITCH "variable"

#if 0
const double fDefaultSideMargin = 1.0; // inches
const double fDefaultPageWidth = 8.5f; // inches (OOo required default: we will handle this later)
const double fDefaultPageHeight = 11.0; // inches
#endif

#ifdef DEBUG
#include <stdio.h>
#define WRITER_DEBUG_MSG(M) printf M
#else
#define WRITER_DEBUG_MSG(M)
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


class WPXPropertyList;
WPXString propListToStyleKey(const WPXPropertyList &xPropList);

struct ltstr
{
	bool operator()(const WPXString &s1, const WPXString &s2) const
	{
		return strcmp(s1.cstr(), s2.cstr()) < 0;
	}
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
