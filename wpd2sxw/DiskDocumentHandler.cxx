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
                if (strlen(i.key().c_str()) > 6 && strcmp(i.key().c_str(), "libwpd") != 0)
                        gsf_output_printf(mpOutput, " %s=\"%s\"", i.key().c_str(), i()->getStr().cstr());
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
