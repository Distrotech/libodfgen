#include "DiskDocumentHandler.hxx"

DiskDocumentHandler::DiskDocumentHandler(GsfOutput *pOutput) :
        mpOutput(pOutput)
{
}

void DiskDocumentHandler::startElement(const char *psName, const WPXPropertyList &xPropList)
{
	gsf_output_printf(mpOutput, "<%s", psName);
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
                // filter out libwpd elements
                if (strlen(i.key()) > 6 && strcmp(i.key(), "libwpd") != 0)
                        gsf_output_printf(mpOutput, " %s=\"%s\"", i.key(), i()->getStr().cstr());

        }
	gsf_output_printf(mpOutput, ">");
}

void DiskDocumentHandler::endElement(const char *psName)
{
	gsf_output_printf(mpOutput, "</%s>", psName);
}

void DiskDocumentHandler::characters(const WPXString &sCharacters)
{
        WPXString sEscapedCharacters(sCharacters, true);
	gsf_output_printf(mpOutput, "%s", sEscapedCharacters.cstr());
}
