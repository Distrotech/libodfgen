#include "DiskDocumentHandler.hxx"

DiskDocumentHandler::DiskDocumentHandler(GsfOutput *pOutput) :
        mpOutput(pOutput),
	mbIsTagOpened(false)
{
}

void DiskDocumentHandler::startElement(const char *psName, const WPXPropertyList &xPropList)
{
	if (mbIsTagOpened)
	{
		gsf_output_puts(mpOutput, ">");
		mbIsTagOpened = false;
	}
	gsf_output_puts(mpOutput, "<");
	gsf_output_puts(mpOutput, psName);
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
                // filter out libwpd elements
                if (strncmp(i.key(), "libwpd", 6) != 0)
                        gsf_output_printf(mpOutput, " %s=\"%s\"", i.key(), i()->getStr().cstr());

        }
	mbIsTagOpened = true;
	msOpenedTagName.sprintf("%s", psName);
}

void DiskDocumentHandler::endElement(const char *psName)
{
	if (mbIsTagOpened)
	{
		if( msOpenedTagName == psName )
		{
			gsf_output_puts(mpOutput, "/>");
			mbIsTagOpened = false;
		}
		else // should not happen, but handle it
		{
			gsf_output_puts(mpOutput, ">");
			gsf_output_puts(mpOutput, "</");
			gsf_output_puts(mpOutput, psName);
			mbIsTagOpened = false;
		}
	}
	else
	{
		gsf_output_puts(mpOutput, "</");
		gsf_output_puts(mpOutput, psName);
		gsf_output_puts(mpOutput, ">");
		mbIsTagOpened = false;
	}
}

void DiskDocumentHandler::characters(const WPXString &sCharacters)
{
	if (mbIsTagOpened)
	{
		gsf_output_puts(mpOutput, ">");
		mbIsTagOpened = false;
	}
        WPXString sEscapedCharacters(sCharacters, true);
	if (sEscapedCharacters.len() > 0)
		gsf_output_puts(mpOutput, sEscapedCharacters.cstr());
}

void DiskDocumentHandler::endDocument()
{
	if (mbIsTagOpened)
	{
		gsf_output_puts(mpOutput, ">");
		mbIsTagOpened = false;
	}
}
