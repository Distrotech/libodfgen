#ifndef _DISKDOCUMENTHANDLER_H
#define _DISKDOCUMENTHANDLER_H
#include "DocumentElement.hxx"
#include <gsf/gsf-output.h>

class DiskDocumentHandler : public DocumentHandler
{
  public:
        DiskDocumentHandler(GsfOutput *pOutput);
        virtual void startDocument() {}
        virtual void endDocument() {}
        virtual void startElement(const UTF8String &sName, const WPXPropertyList &xPropList);
        virtual void endElement(const UTF8String &sName);
        virtual void characters(const UTF8String &sCharacters);

  private:
        GsfOutput *mpOutput;
};
#endif
