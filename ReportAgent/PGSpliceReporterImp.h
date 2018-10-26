///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2014  Washington State Department of Transportation
//                        Bridge and Structures Office
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

// PGSpliceReporterImp.h : Declaration of the CPGSpliceReporterImp
#pragma once

#include "resource.h"       // main symbols

#include <EAF\EAFInterfaceCache.h>

#include <boost\shared_ptr.hpp>

#include <map>

#include <ReporterBase.h>

class rptReport;

/////////////////////////////////////////////////////////////////////////////
// CPGSpliceReporterImp
class ATL_NO_VTABLE CPGSpliceReporterImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPGSpliceReporterImp, &CLSID_PGSpliceReportAgent>,
   public CReporterBase,
	public IConnectionPointContainerImpl<CPGSpliceReporterImp>,
   public IAgentEx
{
public:
	CPGSpliceReporterImp()
	{
      m_pBroker = 0;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_PGSPLICE_REPORTER)

BEGIN_COM_MAP(CPGSpliceReporterImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CPGSpliceReporterImp)
END_CONNECTION_POINT_MAP()


// IAgent
public:
	STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker);
   STDMETHOD(RegInterfaces)();
	STDMETHOD(Init)();
	STDMETHOD(Reset)();
	STDMETHOD(ShutDown)();
   STDMETHOD(Init2)();
   STDMETHOD(GetClassID)(CLSID* pCLSID);

protected:
   // CReporterBase implementation
   virtual CTitlePageBuilder* CreateTitlePageBuilder(LPCTSTR strName,bool bFullVersion=true);

private:
   DECLARE_EAF_AGENT_DATA;

   HRESULT InitReportBuilders();
};
