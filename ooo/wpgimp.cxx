/* genericfilter: mostly generic code for registering the filter
 *
 * Copyright 2000 by Sun Microsystems, Inc.
 * Copyright (C) 2002-2004 William Lachance (william.lachance@sympatico.ca
 * Copyright (C) 2004 Net Integration Technologies (http://www.net-itech.com)
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
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */

#include <osl/mutex.hxx>
#include <osl/thread.h>
#include <cppuhelper/factory.hxx>

#ifndef _COM_SUN_STAR_LANG_XSINGLESERVICEFACTORY_HPP_
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#endif

#include "WPGImportFilter.hxx"

using namespace ::rtl;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;

extern "C"
{
//==================================================================================================
void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** ppEnv )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}
//==================================================================================================
sal_Bool SAL_CALL component_writeInfo(
	void * pServiceManager, void * pRegistryKey )
{
	if (pRegistryKey)
	{
		try
		{
            sal_Int32 nPos = 0;
            Reference< XRegistryKey > xNewKey(
				reinterpret_cast< XRegistryKey * >( pRegistryKey )->createKey( WPGImportFilter_getImplementationName() ) ); 
            xNewKey = xNewKey->createKey( OUString::createFromAscii( "/UNO/SERVICES" ) );
			
			const Sequence< OUString > & rSNL = WPGImportFilter_getSupportedServiceNames();
			const OUString * pArray = rSNL.getConstArray();
			for ( nPos = rSNL.getLength(); nPos--; )
				xNewKey->createKey( pArray[nPos] );

			return sal_True;
		}
		catch (InvalidRegistryException &)
		{
			OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
		}
	}
	return sal_False;
}
//==================================================================================================
void * SAL_CALL component_getFactory(
	const sal_Char * pImplName, void * pServiceManager, void * pRegistryKey )
{
	void * pRet = 0;
	
    OUString implName = OUString::createFromAscii( pImplName );
	if ( pServiceManager && implName.equals(WPGImportFilter_getImplementationName()) )
	{
		Reference< XSingleServiceFactory > xFactory( createSingleFactory(
			reinterpret_cast< XMultiServiceFactory * >( pServiceManager ),
			OUString::createFromAscii( pImplName ),
			WPGImportFilter_createInstance, WPGImportFilter_getSupportedServiceNames() ) );
		
		if (xFactory.is())
		{
			xFactory->acquire();
			pRet = xFactory.get();
		}
	}
	
	return pRet;
}
}
