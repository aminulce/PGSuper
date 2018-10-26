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
#include "EditPrecastSegment.h"
#include "PGSpliceDoc.h"

#include <IFACE\Project.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


txnEditPrecastSegment::txnEditPrecastSegment(const CSegmentKey& segmentKey,const txnEditPrecastSegmentData& newData) :
m_SegmentKey(segmentKey),m_NewSegmentData(newData)
{
}

txnEditPrecastSegment::~txnEditPrecastSegment()
{
}

bool txnEditPrecastSegment::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   m_OldSegmentData.clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);

   GirderIndexType nGirders    = pGroup->GetGirderCount();
   GirderIndexType firstGdrIdx = (m_SegmentKey.girderIndex == ALL_GIRDERS ? 0 : m_SegmentKey.girderIndex);
   GirderIndexType lastGdrIdx  = (m_SegmentKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      // collect up the old girder data (we will need it for Undo)
      txnEditPrecastSegmentData oldSegmentData;
      oldSegmentData.m_SegmentKey = m_SegmentKey;
      oldSegmentData.m_SegmentKey.girderIndex = gdrIdx;

      const CSplicedGirderData* pGirder   = pGroup->GetGirder(oldSegmentData.m_SegmentKey.girderIndex);
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(oldSegmentData.m_SegmentKey.segmentIndex);

      oldSegmentData.m_SegmentData = *pSegment;

      SegmentIDType segID = pSegment->GetID();

      oldSegmentData.m_ConstructionEventIdx = pIBridgeDesc->GetTimelineManager()->GetSegmentConstructionEventIndex();
      oldSegmentData.m_ErectionEventIdx     = pIBridgeDesc->GetTimelineManager()->GetSegmentErectionEventIndex(segID);

      m_OldSegmentData.insert(oldSegmentData);

      // Copy the new segment data onto this segment
      SetSegmentData(oldSegmentData.m_SegmentKey,m_NewSegmentData);
   }

   pEvents->FirePendingEvents();

   return true;
}

void txnEditPrecastSegment::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   std::set<txnEditPrecastSegmentData>::iterator iter(m_OldSegmentData.begin());
   std::set<txnEditPrecastSegmentData>::iterator end(m_OldSegmentData.end());
   for ( ; iter != end; iter++ )
   {
      txnEditPrecastSegmentData& oldSegmentData = *iter;
      SetSegmentData(oldSegmentData.m_SegmentKey,oldSegmentData);
   }

   pEvents->FirePendingEvents();
}

txnTransaction* txnEditPrecastSegment::CreateClone() const
{
   return new txnEditPrecastSegment(m_SegmentKey,m_NewSegmentData);
}

std::_tstring txnEditPrecastSegment::Name() const
{
   std::_tostringstream os;
   os << "Edit Girder " << LABEL_GIRDER(m_NewSegmentData.m_SegmentKey.girderIndex) << " Segment " << LABEL_SEGMENT(m_NewSegmentData.m_SegmentKey.segmentIndex);
   return os.str();
}

bool txnEditPrecastSegment::IsUndoable()
{
   return true;
}

bool txnEditPrecastSegment::IsRepeatable()
{
   return false;
}

void txnEditPrecastSegment::SetSegmentData(const CSegmentKey& segmentKey,const txnEditPrecastSegmentData& data)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IEvents, pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetPrecastSegmentData(segmentKey,data.m_SegmentData);

   pIBridgeDesc->SetSegmentConstructionEventByIndex(data.m_ConstructionEventIdx);
   pIBridgeDesc->SetSegmentErectionEventByIndex(segmentKey,data.m_ErectionEventIdx);

   pEvents->FirePendingEvents();
}
