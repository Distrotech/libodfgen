#include "StdOutHandler.hxx"

void StdOutHandler::startElement(const char *psName, const WPXPropertyList &xPropList)
{
	printf("<%s", psName);
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
                // filter out libwpd elements
                if (strlen(i.key().c_str()) > 6 && strncmp(i.key().c_str(), "libwpd", 6) != 0)
                        printf(" %s=\"%s\"", i.key().c_str(), i()->getStr().cstr());
        }
	printf(">\n");
}

void StdOutHandler::endElement(const char *psName)
{
	printf("</%s>\n", psName);
}

void StdOutHandler::characters(const WPXString &sCharacters)
{
        WPXString sEscapedCharacters(sCharacters, true);
	printf("%s", sEscapedCharacters.cstr());
}
