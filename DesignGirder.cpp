///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2017  Washington State Department of Transportation
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
#include "DesignGirder.h"
#include "PGSuperDoc.h"
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <LRFD\ConcreteUtil.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnDesignGirder::txnDesignGirder( std::vector<const pgsGirderDesignArtifact*>& artifacts, SlabOffsetDesignSelectionType slabOffsetDType, SpanIndexType fromSpan, GirderIndexType fromGirder)
{
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iter(artifacts.begin());
   std::vector<const pgsGirderDesignArtifact*>::const_iterator iterEnd(artifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsGirderDesignArtifact& gdrDesignArtifact(*(*iter));
      DesignData data(gdrDesignArtifact);

      m_DesignDataColl.push_back(data);
   }

   m_NewSlabOffsetType = slabOffsetDType;
   m_FromSpanIdx = fromSpan;
   m_FromGirderIdx = fromGirder;

   // Fillet design uses same input as slab offset
   if (m_NewSlabOffsetType == sodtBridge)
   {
      m_NewFilletType = pgsTypes::fttBridge;
   }
   else if (m_NewSlabOffsetType == sodtPier)
   {
      m_NewFilletType = pgsTypes::fttSpan;
   }
   else if (m_NewSlabOffsetType == sodtGirder || m_NewSlabOffsetType == sodtAllSelectedGirders)
   {
      m_NewFilletType = pgsTypes::fttGirder;
   }

   m_DidSlabOffsetDesign = false; // until we determine otherwise
   m_DidFilletDesign = false;

   m_bInit = false;
}

txnDesignGirder::~txnDesignGirder()
{
}

bool txnDesignGirder::Execute()
{
   if ( !m_bInit )
      Init();

   DoExecute(1);
   return true;
}

void txnDesignGirder::Undo()
{
   DoExecute(0);
}

txnTransaction* txnDesignGirder::CreateClone() const
{
   std::vector<const pgsGirderDesignArtifact*> artifacts;
   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      artifacts.push_back(&(iter->m_DesignArtifact));
   }

   return new txnDesignGirder(artifacts, m_NewSlabOffsetType, m_FromSpanIdx, m_FromGirderIdx);
}

std::_tstring txnDesignGirder::Name() const
{
   std::_tostringstream os;
   if ( m_DesignDataColl.size() == 1 )
   {
      const CGirderKey& girderKey = m_DesignDataColl.front().m_DesignArtifact.GetGirderKey();
      SpanIndexType spanIdx = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;
      os << _T("Design for Span ") << LABEL_SPAN(spanIdx) << _T(", Girder ") << LABEL_GIRDER(gdrIdx);
   }
   else
   {
	   os << _T("Design for (Span, Girder) =");
	   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
	   {
	      const CGirderKey& girderKey = iter->m_DesignArtifact.GetGirderKey();
	      SpanIndexType spanIdx = girderKey.groupIndex;
	      GirderIndexType gdrIdx = girderKey.girderIndex;
	      os << _T(" (") << LABEL_SPAN(spanIdx) << _T(", ") << LABEL_GIRDER(gdrIdx)<< _T(")");
	   }
   }

   return os.str();
}

bool txnDesignGirder::IsUndoable()
{
   return true;
}

bool txnDesignGirder::IsRepeatable()
{
   return false;
}

void txnDesignGirder::Init()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   BOOST_FOREACH(DesignData& rdata,m_DesignDataColl)
   {
#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pSegmentDesignArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
      const CSegmentKey& segmentKey = pSegmentDesignArtifact->GetSegmentKey();

      // old (existing) girder data
      rdata.m_SegmentData[0] = *(pIBridgeDesc->GetPrecastSegmentData(segmentKey));

      // new girder data. Artifact does the work
      rdata.m_SegmentData[1] = pSegmentDesignArtifact->GetSegmentData();

      // Slab offset design data
      if ( m_NewSlabOffsetType!=sodtDoNotDesign && pSegmentDesignArtifact->GetDesignOptions().doDesignSlabOffset != sodNoADesign )
      {
         if (m_NewSlabOffsetType==sodtAllSelectedGirders ||
             (rdata.m_DesignArtifact.GetGirderKey().groupIndex == m_FromSpanIdx && rdata.m_DesignArtifact.GetGirderKey().girderIndex == m_FromGirderIdx))
         {
            m_DidSlabOffsetDesign = true;
            m_DesignSlabOffset[pgsTypes::metStart] = pSegmentDesignArtifact->GetSlabOffset(pgsTypes::metStart);
            m_DesignSlabOffset[pgsTypes::metEnd]   = pSegmentDesignArtifact->GetSlabOffset(pgsTypes::metEnd);

            if ( pSegmentDesignArtifact->GetDesignOptions().doDesignSlabOffset == sodAandFillet )
            {
               // fillet was done too - store it
               m_DidFilletDesign = true;
               m_DesignFillet = pSegmentDesignArtifact->GetFillet();
            }
         }
      }
   }

   // Store original slab offset data. Format depends on type
   if (m_DidSlabOffsetDesign)
   {
      GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

      m_OldSlabOffsetType = pIBridgeDesc->GetSlabOffsetType();
      if (m_OldSlabOffsetType==pgsTypes::sotBridge)
      {
         m_OldBridgeSlabOffset = pIBridgeDesc->GetSlabOffset(0,0,0); // same for all
      }
      else if (m_OldSlabOffsetType==pgsTypes::sotPier)
      {
         // Store per pier
         GroupIndexType ngrp = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(igrp);
            PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
            PierIndexType endPierIdx = pGroup->GetPierIndex(pgsTypes::metEnd);
            for (PierIndexType ipier=startPierIdx; ipier<=endPierIdx; ipier++)
            {
               Float64 slabOffset = pIBridgeDesc->GetSlabOffset(igrp,ipier,0);

               m_OldSlabOffsetData.push_back( OldSlabOffsetData(igrp,ipier,INVALID_INDEX,slabOffset) );
            }
         }
      }
      else if (m_OldSlabOffsetType==pgsTypes::sotGirder)
      {
         // Store per girder
         GroupIndexType ngrp = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(igrp);
            GirderIndexType ngdrs = pGroup->GetGirderCount();

            PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
            PierIndexType endPierIdx = pGroup->GetPierIndex(pgsTypes::metEnd);
            for (PierIndexType ipier=startPierIdx; ipier<=endPierIdx; ipier++)
            {
               for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
               {
                  Float64 slabOffset = pIBridgeDesc->GetSlabOffset(igrp,ipier,igdr);
                  m_OldSlabOffsetData.push_back( OldSlabOffsetData(igrp,ipier,igdr,slabOffset) );
               }
            }
         }
      }
      else
      {
         ATLASSERT(0);
         m_DidSlabOffsetDesign = false; // this is very bad
      }
   }

   // Store original fillet data. Format depends on type
   if (m_DidFilletDesign)
   {
      GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

      m_OldFilletType = pIBridgeDesc->GetFilletType();
      if (m_OldFilletType==pgsTypes::fttBridge)
      {
         m_OldBridgeFillet = pIBridgeDesc->GetFillet(0,0); // same for all
      }
      else if (m_OldFilletType==pgsTypes::fttSpan)
      {
         // Store per span
         GroupIndexType ngrp = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(igrp);
            Float64 Fillet = pIBridgeDesc->GetFillet(igrp,0);
            m_OldFilletData.push_back( OldFilletData(igrp,INVALID_INDEX,Fillet) );
         }
      }
      else if (m_OldFilletType==pgsTypes::fttGirder)
      {
         // Store per girder
         GroupIndexType ngrp = pIBridgeDesc->GetGirderGroupCount();
         for (GroupIndexType igrp=0; igrp<ngrp; igrp++)
         {
            const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(igrp);
            GirderIndexType ngdrs = pGroup->GetGirderCount();
            for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
            {
               Float64 Fillet = pIBridgeDesc->GetFillet(igrp,igdr);
               m_OldFilletData.push_back( OldFilletData(igrp,igdr,Fillet) );
            }
         }
      }
      else
      {
         ATLASSERT(0);
         m_DidFilletDesign = false;
      }
   }

   m_bInit = true; // initialization is complete, don't do it again
}

void txnDesignGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   // Loop over all girder designs
   BOOST_FOREACH(DesignData& rdata,m_DesignDataColl)
   {
#pragma Reminder("UPDATE: assuming precast girder bridge")
      SegmentIndexType segIdx = 0;
      const pgsSegmentDesignArtifact* pSegmentDesignArtifact = rdata.m_DesignArtifact.GetSegmentDesignArtifact(segIdx);
      const CSegmentKey& segmentKey = pSegmentDesignArtifact->GetSegmentKey();

      const arDesignOptions& design_options = pSegmentDesignArtifact->GetDesignOptions();

      // Set girder data for either flexural or shear design
      if (design_options.doDesignForFlexure != dtNoDesign || design_options.doDesignForShear)
      {
         pIBridgeDesc->SetPrecastSegmentData(segmentKey,rdata.m_SegmentData[i]);
      }
   }

   if (m_DidSlabOffsetDesign)
   {
      if (i==0)
      {
         // restore old data
         pIBridgeDesc->SetSlabOffsetType(m_OldSlabOffsetType);

         if (m_OldSlabOffsetType==pgsTypes::sotBridge)
         {
            pIBridgeDesc->SetSlabOffset(m_OldBridgeSlabOffset);
         }
         else if (m_OldSlabOffsetType==pgsTypes::sotPier)
         {
            for (std::vector<OldSlabOffsetData>::const_iterator it=m_OldSlabOffsetData.begin(); it!=m_OldSlabOffsetData.end(); it++)
            {
               const OldSlabOffsetData& rdata = *it;
               pIBridgeDesc->SetSlabOffset(rdata.GroupIdx, rdata.PierIdx, rdata.SlabOffset);
            }
         }
         else if (m_OldSlabOffsetType==pgsTypes::sotGirder)
         {
            for (std::vector<OldSlabOffsetData>::const_iterator it=m_OldSlabOffsetData.begin(); it!=m_OldSlabOffsetData.end(); it++)
            {
               const OldSlabOffsetData& rdata = *it;
               pIBridgeDesc->SetSlabOffset(rdata.GroupIdx, rdata.PierIdx, rdata.GirderIdx, rdata.SlabOffset);
            }
         }
         else
         {
            ATLASSERT(0);
         }
      }
      else
      {
         // Data for new design
         if (m_NewSlabOffsetType == sodtAllSelectedGirders)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotGirder);

            for (DesignDataConstIter itdd=m_DesignDataColl.begin(); itdd!=m_DesignDataColl.end(); itdd++)
            {
               const pgsSegmentDesignArtifact* pArtifact = itdd->m_DesignArtifact.GetSegmentDesignArtifact(0);
               Float64 aStart = pArtifact->GetSlabOffset(pgsTypes::metStart);
               Float64 aEnd = pArtifact->GetSlabOffset(pgsTypes::metEnd);

               SpanIndexType span = itdd->m_DesignArtifact.GetGirderKey().groupIndex;
               SpanIndexType gdr  = itdd->m_DesignArtifact.GetGirderKey().girderIndex;

               pIBridgeDesc->SetSlabOffset(span, span,   gdr, aStart);
               pIBridgeDesc->SetSlabOffset(span, span+1, gdr, aEnd);
            }
         }
         else if (m_NewSlabOffsetType==sodtBridge)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotBridge);
            pIBridgeDesc->SetSlabOffset(m_DesignSlabOffset[0]);
         }
         else if (m_NewSlabOffsetType==sodtPier)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotPier);
            pIBridgeDesc->SetSlabOffset(m_FromSpanIdx, m_FromSpanIdx,   m_DesignSlabOffset[pgsTypes::metStart]);
            pIBridgeDesc->SetSlabOffset(m_FromSpanIdx, m_FromSpanIdx+1, m_DesignSlabOffset[pgsTypes::metEnd]);
         }
         else if (m_NewSlabOffsetType==sodtGirder)
         {
            pIBridgeDesc->SetSlabOffsetType(pgsTypes::sotGirder);
            pIBridgeDesc->SetSlabOffset(m_FromSpanIdx, m_FromSpanIdx,   m_FromGirderIdx, m_DesignSlabOffset[pgsTypes::metStart]);
            pIBridgeDesc->SetSlabOffset(m_FromSpanIdx, m_FromSpanIdx+1, m_FromGirderIdx, m_DesignSlabOffset[pgsTypes::metEnd]);
         }
         else
         {
            ATLASSERT(0);
         }
      }
   }

   if (m_DidFilletDesign)
   {
      if (i==0)
      {
         // restore old data
         pIBridgeDesc->SetFilletType(m_OldFilletType);

         if (m_OldFilletType==pgsTypes::fttBridge)
         {
            pIBridgeDesc->SetFillet(m_OldBridgeFillet);
         }
         else if (m_OldFilletType==pgsTypes::fttSpan)
         {
            for (std::vector<OldFilletData>::const_iterator it=m_OldFilletData.begin(); it!=m_OldFilletData.end(); it++)
            {
               const OldFilletData& rdata = *it;
               pIBridgeDesc->SetFillet(rdata.GroupIdx, rdata.Fillet);
            }
         }
         else if (m_OldFilletType==pgsTypes::fttGirder)
         {
            for (std::vector<OldFilletData>::const_iterator it=m_OldFilletData.begin(); it!=m_OldFilletData.end(); it++)
            {
               const OldFilletData& rdata = *it;
               pIBridgeDesc->SetFillet(rdata.GroupIdx, rdata.GirderIdx, rdata.Fillet);
            }
         }
         else
         {
            ATLASSERT(0);
         }
      }
      else
      {
         // Data for new design
         if (m_NewSlabOffsetType == sodtAllSelectedGirders)
         {
            pIBridgeDesc->SetFilletType(pgsTypes::fttGirder);

            for (DesignDataConstIter itdd=m_DesignDataColl.begin(); itdd!=m_DesignDataColl.end(); itdd++)
            {
               const pgsSegmentDesignArtifact* pArtifact = itdd->m_DesignArtifact.GetSegmentDesignArtifact(0);
               Float64 fillet = pArtifact->GetFillet();

               SpanIndexType span = itdd->m_DesignArtifact.GetGirderKey().groupIndex;
               SpanIndexType gdr  = itdd->m_DesignArtifact.GetGirderKey().girderIndex;

               pIBridgeDesc->SetFillet(span, gdr, fillet);
            }
         }
         else if (m_NewFilletType==pgsTypes::fttBridge)
         {
            pIBridgeDesc->SetFilletType(m_NewFilletType);
            pIBridgeDesc->SetFillet(m_DesignFillet);
         }
         else if (m_NewFilletType==pgsTypes::fttSpan)
         {
            pIBridgeDesc->SetFilletType(m_NewFilletType);
            pIBridgeDesc->SetFillet(m_FromSpanIdx,m_DesignFillet);
         }
         else if (m_NewFilletType==pgsTypes::fttGirder)
         {
            pIBridgeDesc->SetFilletType(m_NewFilletType);
            pIBridgeDesc->SetFillet(m_FromSpanIdx, m_FromGirderIdx, m_DesignFillet);
         }
      }
   }

  pEvents->FirePendingEvents();
}
