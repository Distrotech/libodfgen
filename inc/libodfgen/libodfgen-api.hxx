/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2013 Fridrich Strba <fridrich.strba@bluewin.ch>
 * Copyright (C) 2011 Eilidh McAdam <tibbylickle@gmail.com>
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

#ifndef INCLUDED_LIBODFGEN_LIBODFGEN_API_HXX
#define INCLUDED_LIBODFGEN_LIBODFGEN_API_HXX

#ifdef DLL_EXPORT
#ifdef LIBODFGEN_BUILD
#define ODFGENAPI __declspec(dllexport)
#else
#define ODFGENAPI __declspec(dllimport)
#endif
#else // !DLL_EXPORT
#ifdef LIBODFGEN_VISIBILITY
#define ODFGENAPI __attribute__((visibility("default")))
#else
#define ODFGENAPI
#endif
#endif

#endif // INCLUDED_LIBODFGEN_LIBODFGEN_API_HXX

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
