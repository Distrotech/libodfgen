#include "StdOutHandler.hxx"

void StdOutHandler::startElement(const UTF8String &sName, const WPXPropertyList &xPropList)
{
	printf("<%s", sName.getUTF8());
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
		printf(" %s=\"%s\"", i.key().c_str(), i()->getStr().getUTF8());
        }
	printf(">\n");
}

void StdOutHandler::endElement(const UTF8String &sName)
{
	printf("</%s>\n", sName.getUTF8());
}

void StdOutHandler::characters(const UTF8String &sCharacters)
{
	printf("%s", sCharacters.getUTF8());
}
