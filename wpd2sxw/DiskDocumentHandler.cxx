#include "DiskDocumentHandler.hxx"

DiskDocumentHandler::DiskDocumentHandler(GsfOutput *pOutput) :
        mpOutput(pOutput)
{
}

void DiskDocumentHandler::startElement(const UTF8String &sName, const WPXPropertyList &xPropList)
{
	gsf_output_printf(mpOutput, "<%s", sName.getUTF8());
        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
		gsf_output_printf(mpOutput, " %s=\"%s\"", i.key().c_str(), i()->getStr().getUTF8());
        }
	gsf_output_printf(mpOutput, ">");
}

void DiskDocumentHandler::endElement(const UTF8String &sName)
{
	gsf_output_printf(mpOutput, "</%s>", sName.getUTF8());
}

void DiskDocumentHandler::characters(const UTF8String &sCharacters)
{
	gsf_output_printf(mpOutput, "%s", sCharacters.getUTF8());
}
