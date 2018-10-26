///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2010  Washington State Department of Transportation
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

#include "StdAfx.h"
#include "LibraryEntryObserver.h"
#include "ProjectAgent.h"
#include "ProjectAgent_i.h"
#include "ProjectAgentImp.h"
#include <IFace\StatusCenter.h>
#include <EAF\EAFUIIntegration.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsLibraryEntryObserver
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLibraryEntryObserver::pgsLibraryEntryObserver(CProjectAgentImp* pAgent)
{
   m_pAgent = pAgent;
}

pgsLibraryEntryObserver::~pgsLibraryEntryObserver()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLibraryEntryObserver::Update(ConcreteLibraryEntry* pSubject, Int32 hint)
{
   // no action required
   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(ConnectionLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
   {
      CPierData* pPier = m_pAgent->m_BridgeDescription.GetPier(0);
      bool bDidIt;
      while ( pPier )
      {
         if (pPier->GetConnectionLibraryEntry(pgsTypes::Back) == pSubject)
         {
            pPier->SetConnection( pgsTypes::Back, pSubject->GetName().c_str() );
            bDidIt = true;
         }

         if (pPier->GetConnectionLibraryEntry(pgsTypes::Ahead) == pSubject)
         {
            pPier->SetConnection( pgsTypes::Ahead, pSubject->GetName().c_str() );
            bDidIt = true;
         }

         if ( pPier->GetNextSpan() )
            pPier = pPier->GetNextSpan()->GetNextPier();
         else
            pPier = NULL;
      }
      ATLASSERT(bDidIt);
   }

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();

      // make sure changed library data is compatible with current project data
      m_pAgent->DealWithConnectionLibraryChanges(true);

      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(GirderLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
   {
      if ( m_pAgent->m_BridgeDescription.GetGirderLibraryEntry() == pSubject )
         m_pAgent->m_BridgeDescription.RenameGirder( pSubject->GetName().c_str() );

      CSpanData* pSpan = m_pAgent->m_BridgeDescription.GetSpan(0);
      while ( pSpan )
      {
         CGirderTypes girderTypes = *pSpan->GetGirderTypes();
         GroupIndexType nGroups = girderTypes.GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType firstGdrIdx,lastGdrIdx;
            std::string strGirderName;
            girderTypes.GetGirderGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,strGirderName);

            if ( girderTypes.GetGirderLibraryEntry(firstGdrIdx) == pSubject )
            {
               girderTypes.RenameGirder( grpIdx, pSubject->GetName().c_str() );
            }
         }

         pSpan->SetGirderTypes(girderTypes);

         pSpan = pSpan->GetNextPier()->GetNextSpan();
      }

      m_pAgent->Fire_BridgeChanged(); // if we had a lessor event, we should fire that
                                      // need to fire something so that the bridge view tooltips
                                      // are updated to display the correct name
   }

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();

      // make sure changed library data is compatible with current project data
      m_pAgent->DealWithGirderLibraryChanges(true);

      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(SpecLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
      m_pAgent->m_Spec = pSubject->GetName();

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->SpecificationChanged(true);
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(RatingLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
      m_pAgent->m_RatingSpec = pSubject->GetName();

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->RatingSpecificationChanged(true);
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(TrafficBarrierEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
   {
      if ( m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->strExteriorRailing = pSubject->GetName();

      if ( m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->strInteriorRailing = pSubject->GetName();

      if ( m_pAgent->m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetRightRailingSystem()->strExteriorRailing = pSubject->GetName();

      if ( m_pAgent->m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetRightRailingSystem()->strInteriorRailing = pSubject->GetName();
   }

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(LiveLoadLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if ( hint & LibraryHints::EntryRenamed )
   {
      for ( int i = 0; i < 2; i++ )
      {

         CProjectAgentImp::LiveLoadSelectionIterator begin = m_pAgent->m_SelectedLiveLoads[i].begin();
         CProjectAgentImp::LiveLoadSelectionIterator end   = m_pAgent->m_SelectedLiveLoads[i].end();

         CProjectAgentImp::LiveLoadSelection key;
         key.pEntry = pSubject;
         CProjectAgentImp::LiveLoadSelectionIterator found = std::find(begin,end,key);

         if ( found != end )
         {
            CProjectAgentImp::LiveLoadSelection& ll = *found;

            std::string strOldName = ll.EntryName;
            
            ll.EntryName = pSubject->GetName();
            ll.pEntry = pSubject;

            m_pAgent->Fire_LiveLoadNameChanged(strOldName.c_str(),ll.EntryName.c_str());
         }
      }

   }

   if ( hint & LibraryHints::EntryEdited )
   {
      ClearStatusItems();
      m_pAgent->Fire_LiveLoadChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

//======================== ACCESS     =======================================
void pgsLibraryEntryObserver::SetAgent(CProjectAgentImp* pAgent)
{
   m_pAgent = pAgent;
}

void pgsLibraryEntryObserver::ClearStatusItems()
{
   GET_IFACE2(m_pAgent->m_pBroker,IEAFStatusCenter,pStatusCenter);
   pStatusCenter->RemoveByStatusGroupID(m_pAgent->m_StatusGroupID);
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsLibraryEntryObserver::AssertValid() const
{
   return true;
}

void pgsLibraryEntryObserver::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsLibraryEntryObserver" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsLibraryEntryObserver::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsLibraryEntryObserver");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsLibraryEntryObserver");

   TESTME_EPILOG("LibraryEntryObserver");
}
#endif // _UNITTEST
