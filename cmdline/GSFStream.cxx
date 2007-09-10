/* libwpd
 * Copyright (C) 2004, 2005 William Lachance (wrlach@gmail.com)
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */
#include "GSFStream.hxx"

#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-msole.h>
#include <gsf/gsf-input.h>

GSFInputStream::GSFInputStream(GsfInput *input) :
	WPXInputStream(true),
	m_input(input),
	m_ole(0)
{
	g_object_ref(G_OBJECT(input));
}

GSFInputStream::~GSFInputStream()
{
	if (m_ole)
		g_object_unref(G_OBJECT(m_ole));

	g_object_unref(G_OBJECT(m_input));
}

const uint8_t * GSFInputStream::read(size_t numBytes, size_t &numBytesRead)
{
	const uint8_t *buf = gsf_input_read(m_input, numBytes, 0);

	if (!buf)
		numBytesRead = 0;
	else
		numBytesRead = numBytes;

	return buf;
}

int GSFInputStream::seek(long offset, WPX_SEEK_TYPE seekType) 
{
	GSeekType gsfSeekType;
	switch(seekType)
	{
	case WPX_SEEK_CUR:
		gsfSeekType = G_SEEK_CUR;
		break;
	case WPX_SEEK_SET:
	default:
		gsfSeekType = G_SEEK_SET;
		break;
	}

	return gsf_input_seek(m_input, offset, gsfSeekType);
}

bool GSFInputStream::isOLEStream()
{
	if (!m_ole)
		m_ole = GSF_INFILE(gsf_infile_msole_new (m_input, 0)); 

	if (m_ole)
		return true;

	return false;
}

WPXInputStream * GSFInputStream::getDocumentOLEStream(const char *name)
{
	WPXInputStream *documentStream = 0;

	if (!m_ole)
		m_ole = GSF_INFILE(gsf_infile_msole_new (m_input, 0)); 

	if (m_ole)
	{
		GsfInput *document = gsf_infile_child_by_name(m_ole, name);
		if (document) 
		{
			documentStream = new GSFInputStream(document);
			g_object_unref(G_OBJECT (document)); // the only reference should be encapsulated within the new stream
		}
	}

	return documentStream;	
}
