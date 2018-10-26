///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2011  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void ConfigureSpanLengthUnitGroup(IDisplayUnitMgr2* pDisplayUnitsMgr)
{
   HRESULT hr; 

   CComPtr<IDisplayUnitGroup> dispUnitGroup;
   hr = pDisplayUnitsMgr->Add(CComBSTR("SpanLength"),CComBSTR("Length"),&dispUnitGroup);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IDisplayUnitFormatter> formatter;
   formatter.CoCreateInstance(CLSID_DisplayUnitFormatter);

   CComPtr<IDisplayUnit> duMeter;
   hr = dispUnitGroup->Add(CComBSTR("m"),unitsSI,formatter,VARIANT_TRUE,&duMeter);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IDisplayUnit> duFeet;
   hr = dispUnitGroup->Add(CComBSTR("ft"),unitsUS,formatter,VARIANT_TRUE,&duFeet);
   ATLASSERT(SUCCEEDED(hr));
}

void ConfigureStationUnitGroup(IDisplayUnitMgr2* pDisplayUnitsMgr)
{
   CComPtr<IDisplayUnitGroup> dispUnitGroup;
   HRESULT hr = pDisplayUnitsMgr->Add(CComBSTR("Station"),CComBSTR("Length"),&dispUnitGroup);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IAnnotatedDisplayUnitFormatter> siStationFormatter;
   siStationFormatter.CoCreateInstance(CLSID_AnnotatedDisplayUnitFormatter);
   siStationFormatter->put_OffsetDigits(3);
   siStationFormatter->put_Multiplier(1.0);
   siStationFormatter->put_Annotation(CComBSTR("+"));
   siStationFormatter->FormatSpecifiers(6,3,tjRight,nftFixed,0.0001);

   CComPtr<IAnnotatedDisplayUnitFormatter> usStationFormatter;
   usStationFormatter.CoCreateInstance(CLSID_AnnotatedDisplayUnitFormatter);
   usStationFormatter->put_OffsetDigits(2);
   usStationFormatter->put_Multiplier(1.0);
   usStationFormatter->put_Annotation(CComBSTR("+"));
   usStationFormatter->FormatSpecifiers(4,2,tjRight,nftFixed,0.0001);


   CComPtr<IDisplayUnit> duMeter;
   hr = dispUnitGroup->Add(CComBSTR("m"),unitsSI,siStationFormatter,VARIANT_TRUE,&duMeter);
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IDisplayUnit> duFeet;
   hr = dispUnitGroup->Add(CComBSTR("ft"),unitsUS,usStationFormatter,VARIANT_TRUE,&duFeet);
   ATLASSERT(SUCCEEDED(hr));
}

BOOL CreateAppUnitSystem(IAppUnitSystem** ppAppUnitSystem)
{
   CComPtr<IAppUnitSystem> appUnitSystem;
   HRESULT hr = appUnitSystem.CoCreateInstance(CLSID_AppUnitSystem);
   if ( FAILED(hr) )
   {
      AfxMessageBox(_T("Error initializing Units system"),MB_OK | MB_ICONEXCLAMATION);
      return FALSE;
   }

   CComPtr<IUnitServer> unitServer;
   appUnitSystem->get_UnitServer(&unitServer);

   hr = unitServer->SetBaseUnits(CComBSTR("kg"),CComBSTR("m"),CComBSTR("sec"),CComBSTR("C"),CComBSTR("rad"));
   ATLASSERT(SUCCEEDED(hr));

   CComPtr<IDisplayUnitMgr2> dispUnitMgr;
   appUnitSystem->get_DisplayUnitMgr(&dispUnitMgr);

   ConfigureSpanLengthUnitGroup(dispUnitMgr);
   ConfigureStationUnitGroup(dispUnitMgr);


   (*ppAppUnitSystem) = appUnitSystem;
   (*ppAppUnitSystem)->AddRef();

   return TRUE;
}

void CreateDocUnitSystem(IAppUnitSystem* pAppUnitSystem,IDocUnitSystem** ppDocUnitSystem)
{
   CComPtr<IDocUnitSystem> docUnitSystem;
   docUnitSystem.CoCreateInstance(CLSID_DocUnitSystem);

   docUnitSystem->putref_AppUnitSystem(pAppUnitSystem);

   (*ppDocUnitSystem) = docUnitSystem;
   (*ppDocUnitSystem)->AddRef();
}
