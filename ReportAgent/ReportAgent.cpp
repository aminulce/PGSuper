///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// ReportAgent.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ReportAgentps.mk in the project directory.

#include "stdafx.h"
#include "initguid.h"
#include "resource.h"
#include "ReportAgent_i.h"

#include "ReportAgent_i.c"
#include <WBFLCore_i.c>
#include <WBFLCogo_i.c>

#include <IReportManager.h>
#include <IFace\StatusCenter.h>

#include "ReporterImp.h"

#include "PGSuperCatCom.h"
#include <System\ComCatMgr.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ReportAgent, CReporterImp)
END_OBJECT_MAP()

class CReportAgentApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CReportAgentApp theApp;

BOOL CReportAgentApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	return CWinApp::InitInstance();
}

int CReportAgentApp::ExitInstance()
{
	_Module.Term();
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
   LONG cLock = _Module.GetLockCount();
   HRESULT hr = AfxDllCanUnloadNow();
   bool bCanUnload = ( hr == S_OK && cLock == 0 );
	return ( bCanUnload ) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr =  _Module.RegisterServer(TRUE);
   if ( FAILED(hr) )
      return hr;

   return sysComCatMgr::RegWithCategory(CLSID_ReportAgent,CATID_PGSuperAgent,true);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
   sysComCatMgr::RegWithCategory(CLSID_ReportAgent,CATID_PGSuperAgent,false);
	_Module.UnregisterServer();
	return S_OK;
}


