/* 
 * Copyright (C) 2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004 Net Integration Technologies (http://www.net-itech.com)
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

#ifndef _COM_SUN_STAR_XML_SAX_XDOCUMENTHANDLER_HPP_
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#endif

#include "DocumentHandler.hxx"

using com::sun::star::uno::Reference;
using com::sun::star::xml::sax::XDocumentHandler;

class OODocumentHandler : public DocumentHandler
{ 
public:
        OODocumentHandler(Reference < XDocumentHandler > &xHandler);
        virtual void startDocument();
        virtual void endDocument();
        virtual void startElement(const char *psName, const WPXPropertyList &xPropList);
        virtual void endElement(const char *psName);
        virtual void characters(const WPXString &sCharacters);

private:
        Reference < XDocumentHandler > mxHandler;
};
