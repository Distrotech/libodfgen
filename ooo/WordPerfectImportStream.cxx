/* WordPerfectImportStream: Wrapper class for OO stream -- lets it
 * be called from libwpd1.
 *
 * Copyright (C) 2002 William Lachance (wlach@interlog.com)
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
 *
 *  Contributor(s): 
 *
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */
#include "WordPerfectImportStream.hxx"
//#include "gsf-config.h"
#include <gsf/gsf-input-impl.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-utils.h>
#include <string.h>
#include <stdio.h>

#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_IO_XINPUTSTREAM_HPP_
#include <com/sun/star/io/XInputStream.hpp>
#endif

#ifndef _COM_SUN_STAR_IO_XSEEKABLE_HPP_
#include <com/sun/star/io/XSeekable.hpp>
#endif

#include <com/sun/star/uno/Reference.h>

using namespace ::com::sun::star::uno;
using com::sun::star::uno::Reference;
using com::sun::star::io::XInputStream;
using com::sun::star::io::XSeekable;
using com::sun::star::uno::Sequence;

struct _GsfInputOO {
	GsfInput input;

	Reference < XInputStream > xInputStream;
	guint8   *buf;
	size_t   buf_size;
};

typedef struct {
	GsfInputClass input_class;
} GsfInputOOClass;

/**
 * gsf_input_oo_new :
 * @filename : in utf8.
 * @err	     : optionally NULL.
 *
 * Returns a new file or NULL.
 **/
GsfInputOO *
gsf_input_oo_new (Reference < XInputStream > & xInputStream, GError **err)
{
	GsfInputOO * input = (GsfInputOO *)g_object_new (GSF_INPUT_OO_TYPE, NULL);

	input->xInputStream = xInputStream;
	input->buf  = NULL;
	input->buf_size = 0;

	Reference < XSeekable> xSeekable = Reference < XSeekable > (xInputStream, UNO_QUERY);
	gsf_input_set_size (GSF_INPUT (input), xSeekable->getLength());

	return input;
}

static void
gsf_input_oo_finalize (GObject *obj)
{
	GObjectClass *parent_class;
	GsfInputOO *input = GSF_INPUT_OO(obj);

	/* FIXMEFIXME: deref the stream (xInputStream)*/
	input->buf  = NULL;
	input->buf_size = 0;

	parent_class = (GObjectClass *)g_type_class_peek (GSF_INPUT_TYPE);
	if (parent_class && parent_class->finalize)
		parent_class->finalize (obj);
}

static GsfInput *
gsf_input_oo_dup (GsfInput *src_input, GError **err)
{
	GsfInputOO const *src = GSF_INPUT_OO(src_input);
	return GSF_INPUT (gsf_input_oo_new (((GsfInputOO *)src_input)->xInputStream, err));
}

static guint8 const *
gsf_input_oo_read (GsfInput *input, size_t num_bytes,
		      guint8 *buffer)
{

	GsfInputOO *oo = GSF_INPUT_OO (input);
	size_t nread = 0, total_read = 0;

	g_return_val_if_fail (oo != NULL, NULL);

	if (buffer == NULL) {
		if (oo->buf_size < num_bytes) {
			oo->buf_size = num_bytes;
			if (oo->buf != NULL)
				g_free (oo->buf);
			oo->buf = g_new (guint8, oo->buf_size);
		}
		buffer = oo->buf;
	}
	
	Sequence< sal_Int8 > *paData = new Sequence< sal_Int8 >;
	nread = oo->xInputStream->readBytes((*paData), (sal_Int32)(num_bytes));
	buffer = (guint8 *)memcpy(buffer, paData->getConstArray(), (size_t)(nread));
	delete(paData);

	return buffer;
}

static gboolean
gsf_input_oo_seek (GsfInput *input, gsf_off_t offset, GSeekType whence)
{

	GsfInputOO const *oo = GSF_INPUT_OO (input);
	gsf_off_t loffset;	

// 	int oo_whence = SEEK_SET;

	Reference < XSeekable> xSeekable = Reference < XSeekable >(oo->xInputStream, UNO_QUERY);

	loffset = offset;
	if ((gsf_off_t) loffset != offset) { /* Check for overflow */
		g_warning ("offset too large for fseek");
		return TRUE;
	}
	
	switch (whence) {
	case G_SEEK_CUR : loffset = xSeekable->getPosition() + loffset; break;
	case G_SEEK_END : loffset = xSeekable->getLength() - loffset; break;
	case G_SEEK_SET : break;
	default:
		return FALSE;
	}

	xSeekable->seek(loffset); // FIXME: catch exception!
	
	return FALSE;
}

static void
gsf_input_oo_init (GObject *obj)
{
	GsfInputOO *oo = GSF_INPUT_OO (obj);

	oo->buf  = NULL;
	oo->buf_size = 0;
}

static void
gsf_input_oo_class_init (GObjectClass *gobject_class)
{
	GsfInputClass *input_class = GSF_INPUT_CLASS (gobject_class);

	gobject_class->finalize = gsf_input_oo_finalize;
	input_class->Dup	= gsf_input_oo_dup;
	input_class->Read	= gsf_input_oo_read;
	input_class->Seek	= gsf_input_oo_seek;

}

GSF_CLASS (GsfInputOO, gsf_input_oo,
	   gsf_input_oo_class_init, gsf_input_oo_init, GSF_INPUT_TYPE)

