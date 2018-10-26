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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "InplaceTemporarySupportStationEditEvents.h"
#include "EditTemporarySupportStation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInplaceTemporarySupportStationEditEvents::CInplaceTemporarySupportStationEditEvents(IBroker* pBroker,SupportIndexType tsIdx) :
CInplaceEditDisplayObjectEvents(pBroker), m_TSIdx(tsIdx)
{
}

void CInplaceTemporarySupportStationEditEvents::Handle_OnChanged(iDisplayObject* pDO)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComQIPtr<iEditableUnitValueTextBlock> pTextBlock(pDO);
   ATLASSERT(pTextBlock);

   Float64 old_station = pTextBlock->GetValue();
   Float64 new_station = pTextBlock->GetEditedValue();

   if ( IsEqual(old_station,new_station) )
      return;

   txnEditTemporarySupportStation* pTxn = new txnEditTemporarySupportStation(m_TSIdx,old_station,new_station);
   txnTxnManager::GetInstance()->Execute(pTxn);
}
