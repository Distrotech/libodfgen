#ifndef _STDOUTHANDLER_H
#define _STDOUTHANDLER_H
#include "DocumentElement.hxx"
#include <gsf/gsf-output.h>

class StdOutHandler : public DocumentHandler
{
  public:
        virtual void startDocument() {}
        virtual void endDocument() {}
        virtual void startElement(const UTF8String &sName, const WPXPropertyList &xPropList);
        virtual void endElement(const UTF8String &sName);
        virtual void characters(const UTF8String &sCharacters);
};
#endif
