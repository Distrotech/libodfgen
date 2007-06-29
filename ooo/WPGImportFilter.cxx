/* WPGImportFilter: Sets up the filter, and calls OdgExporter
 * to do the actual filtering
 *
 * Copyright (C) 2000 by Sun Microsystems, Inc.
 * Copyright (C) 2002-2004 William Lachance (wlach@interlog.com)
 * Copyright (C) 2004 Net Integration Technologies (http://www.net-itech.com)
 * Copyright (C) 2004-2006 Fridrich Strba <fridrich.strba@bluewin.ch>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 *
 *  Contributor(s): Martin Gallwey (gallwey@sun.com)
 *
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef _OSL_DIAGNOSE_H_
#include <osl/diagnose.h>
#endif
#ifndef _RTL_TENCINFO_H_
#include <rtl/tencinfo.h>
#endif
#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_IO_XINPUTSTREAM_HPP_
#include <com/sun/star/io/XInputStream.hpp>
#endif
#ifndef _COM_SUN_STAR_XML_SAX_XATTRIBUTELIST_HPP_
#include <com/sun/star/xml/sax/XAttributeList.hpp>
#endif
#ifndef _COM_SUN_STAR_XML_SAX_XDOCUMENTHANDLER_HPP_
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#endif
#ifndef _COM_SUN_STAR_XML_SAX_INPUTSOURCE_HPP_
#include <com/sun/star/xml/sax/InputSource.hpp>
#endif
#ifndef _COM_SUN_STAR_XML_SAX_XPARSER_HPP_
#include <com/sun/star/xml/sax/XParser.hpp>
#endif

#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_IO_XINPUTSTREAM_HPP_
#include <com/sun/star/io/XInputStream.hpp>
#endif

#ifndef _COM_SUN_STAR_IO_XSEEKABLE_HPP_
#include <com/sun/star/io/XSeekable.hpp>
#endif

#include <com/sun/star/uno/Reference.h>


#include "OODocumentHandler.hxx"
#include "OdgExporter.hxx"
#include "WPGImportFilter.hxx"
#include "attrlist.hxx"
#include "xmlkywd.hxx"
#include <libwpg/WPGStreamImplementation.h>

#include <iostream>

using namespace ::com::sun::star::uno;
using com::sun::star::uno::Reference;
using com::sun::star::io::XInputStream;
using com::sun::star::io::XSeekable;
using com::sun::star::uno::Sequence;
using namespace ::rtl;
using rtl::OString;
using rtl::OUString;
using com::sun::star::uno::Sequence;
using com::sun::star::uno::Reference;
using com::sun::star::uno::Any;
using com::sun::star::uno::UNO_QUERY;
using com::sun::star::uno::XInterface;
using com::sun::star::uno::Exception;
using com::sun::star::uno::RuntimeException;
using com::sun::star::lang::XMultiServiceFactory;
using com::sun::star::beans::PropertyValue;
using com::sun::star::document::XFilter;
using com::sun::star::document::XExtendedFilterDetection;

using com::sun::star::io::XInputStream;
using com::sun::star::document::XImporter;
using com::sun::star::xml::sax::InputSource;
using com::sun::star::xml::sax::XAttributeList;
using com::sun::star::xml::sax::XDocumentHandler;
using com::sun::star::xml::sax::XParser;


sal_Bool SAL_CALL WPGImportFilter::filter( const Sequence< ::com::sun::star::beans::PropertyValue >& aDescriptor ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::filter" << std::endl;
#endif
	sal_Int32 nLength = aDescriptor.getLength();
	const PropertyValue * pValue = aDescriptor.getConstArray();
	OUString sURL;
	Reference < XInputStream > xInputStream;
	for ( sal_Int32 i = 0 ; i < nLength; i++)
	{
		if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "InputStream" ) ) )
		pValue[i].Value >>= xInputStream;
		else if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "URL" ) ) )
		pValue[i].Value >>= sURL;
		rtl_TextEncoding encoding = RTL_TEXTENCODING_INFO_ASCII;
	}
	if ( !xInputStream.is() )
	{
		OSL_ASSERT( 0 );
		return sal_False;
	}
	OString sFileName;
	sFileName = OUStringToOString(sURL, RTL_TEXTENCODING_INFO_ASCII);

	// An XML import service: what we push sax messages to..
	OUString sXMLImportService ( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.comp.Draw.XMLOasisImporter" ) );
	Reference < XDocumentHandler > xInternalHandler( mxMSF->createInstance( sXMLImportService ), UNO_QUERY );

	// The XImporter sets up an empty target document for XDocumentHandler to write to.. 
	Reference < XImporter > xImporter(xInternalHandler, UNO_QUERY);
	xImporter->setTargetDocument( mxDoc );

	// OO Graphics Handler: abstract class to handle document SAX messages, concrete implementation here
	// writes to in-memory target doc
	OODocumentHandler xHandler(xInternalHandler);

	Reference < XSeekable> xSeekable = Reference < XSeekable > (xInputStream, UNO_QUERY);
	long tmpNumBytesToRead = xSeekable->getLength();
	::com::sun::star::uno::Sequence< sal_Int8 > tmpData;
	xInputStream->readBytes(tmpData, tmpNumBytesToRead);

	::WPXInputStream* input = new libwpg::WPGMemoryStream((const char *)tmpData.getConstArray(), tmpNumBytesToRead);

	if (input->isOLEStream())
	{
		::WPXInputStream* olestream = input->getDocumentOLEStream();
		if (olestream)
		{
			delete input;
			input = olestream;
		}
	}
	OdgExporter exporter(&xHandler);
	bool tmpParseResult = libwpg::WPGraphics::parse(input, &exporter);
	if (input)
		delete input;
	xInputStream->closeInput();
	return tmpParseResult;
}

void SAL_CALL WPGImportFilter::cancel(  ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::cancel" << std::endl;
#endif
}

// XImporter
void SAL_CALL WPGImportFilter::setTargetDocument( const Reference< ::com::sun::star::lang::XComponent >& xDoc )
	throw (::com::sun::star::lang::IllegalArgumentException, RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::setTargetDocument" << std::endl;
#endif
	meType = FILTER_IMPORT;
	mxDoc = xDoc;
}

// XExtendedFilterDetection
OUString SAL_CALL WPGImportFilter::detect( com::sun::star::uno::Sequence< PropertyValue >& Descriptor )
	throw( com::sun::star::uno::RuntimeException )
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::detect" << std::endl;
#endif
	OUString sTypeName = OUString( RTL_CONSTASCII_USTRINGPARAM ( "" ) );
	sal_Int32 nLength = Descriptor.getLength();
	sal_Int32 location = nLength;
	OUString sURL;
	const PropertyValue * pValue = Descriptor.getConstArray();
	Reference < XInputStream > xInputStream;
	for ( sal_Int32 i = 0 ; i < nLength; i++)
	{
		if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "TypeName" ) ) )
			location=i;
		else if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "InputStream" ) ) )
			pValue[i].Value >>= xInputStream;
		else if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "URL" ) ) )
			pValue[i].Value >>= sURL;

		rtl_TextEncoding encoding = RTL_TEXTENCODING_INFO_ASCII;
	}

	Reference < XSeekable> xSeekable = Reference < XSeekable > (xInputStream, UNO_QUERY);
	long tmpNumBytesToRead = xSeekable->getLength();
	::com::sun::star::uno::Sequence< sal_Int8 > tmpData;
	xInputStream->readBytes(tmpData, tmpNumBytesToRead);

	::WPXInputStream* input = new libwpg::WPGMemoryStream((const char *)tmpData.getConstArray(), tmpNumBytesToRead);

	if (input->isOLEStream())
	{
		::WPXInputStream* olestream = input->getDocumentOLEStream();
		if (olestream)
		{
			delete input;
			input = olestream;
		}
	}

	if (libwpg::WPGraphics::isSupported(input))
		sTypeName = OUString( RTL_CONSTASCII_USTRINGPARAM ( "draw_WordPerfect_Graphics" ) );

	if (input)
		delete input;

	if (!sTypeName.equalsAscii(""))
	{
		if ( location == Descriptor.getLength() )
		{
			Descriptor.realloc(nLength+1);
			Descriptor[location].Name = ::rtl::OUString::createFromAscii( "TypeName" );
		}

		Descriptor[location].Value <<=sTypeName;
	}
	return sTypeName;		
}


// XInitialization
void SAL_CALL WPGImportFilter::initialize( const Sequence< Any >& aArguments ) 
	throw (Exception, RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::initialize" << std::endl;
#endif
	Sequence < PropertyValue > aAnySeq;
	sal_Int32 nLength = aArguments.getLength();
	if ( nLength && ( aArguments[0] >>= aAnySeq ) )
	{
		const PropertyValue * pValue = aAnySeq.getConstArray();
		nLength = aAnySeq.getLength();
		for ( sal_Int32 i = 0 ; i < nLength; i++)
		{
			if ( pValue[i].Name.equalsAsciiL ( RTL_CONSTASCII_STRINGPARAM ( "Type" ) ) )
			{
				pValue[i].Value >>= msFilterName;
				break;
			}
		}
	}
}
OUString WPGImportFilter_getImplementationName ()
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter_getImplementationName" << std::endl;
#endif
	return OUString ( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.comp.Draw.WPGImportFilter" ) );
}

#define SERVICE_NAME1 "com.sun.star.document.ImportFilter"
#define SERVICE_NAME2 "com.sun.star.document.ExtendedTypeDetection"
sal_Bool SAL_CALL WPGImportFilter_supportsService( const OUString& ServiceName ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter_supportsService" << std::endl;
#endif
	return (ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME1 ) ) ||
		ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME2 ) ) );
}
Sequence< OUString > SAL_CALL WPGImportFilter_getSupportedServiceNames(  ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter_getSupportedServiceNames" << std::endl;
#endif
	Sequence < OUString > aRet(2);
	OUString* pArray = aRet.getArray();
	pArray[0] =  OUString ( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME1 ) );
	pArray[1] =  OUString ( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME2 ) ); 
	return aRet;
}
#undef SERVICE_NAME2
#undef SERVICE_NAME1

Reference< XInterface > SAL_CALL WPGImportFilter_createInstance( const Reference< XMultiServiceFactory > & rSMgr)
	throw( Exception )
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter_createInstance" << std::endl;
#endif
	return (cppu::OWeakObject*) new WPGImportFilter( rSMgr );
}

// XServiceInfo
OUString SAL_CALL WPGImportFilter::getImplementationName(  ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::getImplementationName" << std::endl;
#endif
	return WPGImportFilter_getImplementationName();
}
sal_Bool SAL_CALL WPGImportFilter::supportsService( const OUString& rServiceName ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::supportsService" << std::endl;
#endif
	return WPGImportFilter_supportsService( rServiceName );
}
Sequence< OUString > SAL_CALL WPGImportFilter::getSupportedServiceNames(  ) 
	throw (RuntimeException)
{
#ifdef DEBUG
	std::cerr << "WPGImportFilter::getSupportedServiceNames" << std::endl;
#endif
	return WPGImportFilter_getSupportedServiceNames();
}

