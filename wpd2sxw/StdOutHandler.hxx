#ifndef _STDOUTHANDLER_H
#define _STDOUTHANDLER_H
#include "DocumentElement.hxx"
#include <gsf/gsf-output.h>

class StdOutHandler : public DocumentHandler
{
  public:
        virtual void startDocument() {}
        virtual void endDocument() {}
        virtual void startElement(const char *psName, const WPXPropertyList &xPropList);
        virtual void endElement(const char *psName);
        virtual void characters(const UTF8String &sCharacters);
};
#endif
