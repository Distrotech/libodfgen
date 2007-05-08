#include "StdOutHandler.hxx"

StdOutHandler::StdOutHandler() :
	mbIsTagOpened(false)
{
}

void StdOutHandler::startElement(const char *psName, const WPXPropertyList &xPropList)
{
	if (mbIsTagOpened)
	{
		printf(">");
		mbIsTagOpened = false;
	}
	printf("<%s", psName);
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
                // filter out libwpd elements
                if (strncmp(i.key(), "libwpd", 6) != 0)
                        printf(" %s=\"%s\"", i.key(), i()->getStr().cstr());
        }
	mbIsTagOpened = true;
	msOpenedTagName.sprintf("%s", psName);
}

void StdOutHandler::endElement(const char *psName)
{
	if (mbIsTagOpened)
	{
		if( msOpenedTagName == psName )
		{
			printf("/>");
			mbIsTagOpened = false;
		}
		else // should not happen, but handle it
		{
			printf(">");
			printf("</%s>", psName);
			mbIsTagOpened = false;
		}
	}
	else
	{
		printf("</%s>", psName);
		mbIsTagOpened = false;
	}
}

void StdOutHandler::characters(const WPXString &sCharacters)
{
	if (mbIsTagOpened)
	{
		printf(">");
		mbIsTagOpened = false;
	}
        WPXString sEscapedCharacters(sCharacters, true);
	printf("%s", sEscapedCharacters.cstr());
}

void StdOutHandler::endDocument()
{
	if (mbIsTagOpened)
	{
		printf(">");
		mbIsTagOpened = false;
	}
}
