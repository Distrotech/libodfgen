#ifndef _COM_SUN_STAR_XML_SAX_XDOCUMENTHANDLER_HPP_
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#endif

#include "DocumentHandler.hxx"

using com::sun::star::uno::Reference;
using com::sun::star::xml::sax::XDocumentHandler;

class OODocumentHandler : public DocumentHandler
{ 
public:
        OODocumentHandler(Reference < XDocumentHandler > &xHandler);
        virtual void startDocument();
        virtual void endDocument();
        virtual void startElement(const char *psName, const WPXPropertyList &xPropList);
        virtual void endElement(const char *psName);
        virtual void characters(const WPXString &sCharacters);

private:
        Reference < XDocumentHandler > mxHandler;
};
