/* writerperfect:
 *
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef _DISKDOCUMENTHANDLER_H
#define _DISKDOCUMENTHANDLER_H
#include "DocumentElement.hxx"
#include <gsf/gsf-output.h>

class DiskDocumentHandler : public DocumentHandler
{
  public:
        DiskDocumentHandler(GsfOutput *pOutput);
        virtual void startDocument() {}
        virtual void endDocument();
        virtual void startElement(const char *psName, const WPXPropertyList &xPropList);
        virtual void endElement(const char *psName);
        virtual void characters(const WPXString &sCharacters);

  private:
        GsfOutput *mpOutput;
	bool mbIsTagOpened;
	WPXString msOpenedTagName;
};
#endif
