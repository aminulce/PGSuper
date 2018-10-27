///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2018  Washington State Department of Transportation
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

// TxDOTCadExporter.h : Declaration of the CTxDOTCadExporter

#pragma once
#include "resource.h"       // main symbols


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CTxDOTCadExporter

class ATL_NO_VTABLE CTxDOTCadExporter :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTxDOTCadExporter, &CLSID_TxDOTCadExporter>,
	public IPGSDataExporter
{
public:
	CTxDOTCadExporter()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TXDOTCADEXPORTER)

DECLARE_NOT_AGGREGATABLE(CTxDOTCadExporter)

BEGIN_COM_MAP(CTxDOTCadExporter)
	COM_INTERFACE_ENTRY(IPGSDataExporter)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

// IPGSDataImporter
public:
   STDMETHOD(Init)(UINT nCmdID) override;
   STDMETHOD(GetMenuText)(/*[out,retval]*/BSTR*  bstrText) override;
   STDMETHOD(GetBitmapHandle)(/*[out]*/HBITMAP* phBmp) override;
   STDMETHOD(GetCommandHintText)(BSTR*  bstrText) override;
   STDMETHOD(Export)(/*[in]*/IBroker* pBroker) override;
};

OBJECT_ENTRY_AUTO(__uuidof(TxDOTCadExporter), CTxDOTCadExporter)
