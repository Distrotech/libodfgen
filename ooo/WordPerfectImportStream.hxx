/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gsf-input-stdio.h: interface for use by the structured file layer to read raw data
 *
 * Copyright (C) 2002-2003 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GSF_INPUT_OO_H
#define GSF_INPUT_OO_H

#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_IO_XINPUTSTREAM_HPP_
#include <com/sun/star/io/XInputStream.hpp>
#endif

#include <gsf/gsf-input.h>

using com::sun::star::uno::Reference;
using com::sun::star::io::XInputStream;
using com::sun::star::uno::Sequence;

G_BEGIN_DECLS

#define GSF_INPUT_OO_TYPE        (gsf_input_oo_get_type ())
#define GSF_INPUT_OO(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GSF_INPUT_OO_TYPE, GsfInputOO))
#define GSF_IS_INPUT_OO(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSF_INPUT_OO_TYPE))

typedef struct _GsfInputOO GsfInputOO;

GType     gsf_input_oo_get_type (void);
GsfInputOO *gsf_input_oo_new (Reference < XInputStream > & xInputStream, GError **err);

G_END_DECLS

#endif /* GSF_INPUT_OO_H */

// extern "C" {
// long getBytes(void *ptr, size_t size, size_t num, WordPerfectStream *stream);
// int skipBytes(WordPerfectStream *stream, long num);
// }
