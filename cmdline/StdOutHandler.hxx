#ifndef _STDOUTHANDLER_H
#define _STDOUTHANDLER_H
#include "DocumentElement.hxx"

class StdOutHandler : public DocumentHandler
{
  public:
  	StdOutHandler();
        virtual void startDocument() {}
        virtual void endDocument();
        virtual void startElement(const char *psName, const WPXPropertyList &xPropList);
        virtual void endElement(const char *psName);
        virtual void characters(const WPXString &sCharacters);
  private:
	bool mbIsTagOpened;
	WPXString msOpenedTagName;
};
#endif
