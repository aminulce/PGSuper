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

#include "stdafx.h"
#include "StatusCenterImp.h"
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool StatusItemCompare::operator()(pgsStatusItem* a,pgsStatusItem* b)
{
   if ( a->IsEqual(b) )
      return false;

   return a->GetID() < b->GetID();
}

pgsStatusCenter::pgsStatusCenter()
{
   m_NextID = 1;
   m_NextAgentID = 1;
   m_pCurrentItem = NULL;
}

pgsStatusCenter::~pgsStatusCenter()
{
   Container::iterator iter;
   for ( iter = m_Items.begin(); iter != m_Items.end(); iter++ )
   {
      pgsStatusItem* pItem = *iter;
      delete pItem;
      pItem = NULL;
   }

   Callbacks::iterator iter2;
   for ( iter2 = m_Callbacks.begin(); iter2 != m_Callbacks.end(); iter2++ )
   {
      iStatusCallback* pCallback = (*iter2).second;
      delete pCallback;
      (*iter2).second = NULL;
   }
}

Int32 pgsStatusCenter::GetAgentID()
{
   return m_NextAgentID++;
}

Int32 pgsStatusCenter::Add(pgsStatusItem* pItem)
{
#if defined _DEBUG
   long callbackID = pItem->GetCallbackID();
   Callbacks::iterator found = m_Callbacks.find(callbackID);

   // If this assert fires, then an associated callback handler
   // has not been registered
   ASSERT( found != m_Callbacks.end() );
#endif // _DEBUG

   pItem->SetID(m_NextID++);

   std::pair<Container::iterator,bool> result;
   result = m_Items.insert(pItem);

   if ( result.second == true )
   {
      NotifyAdded(pItem);
      return pItem->GetID();
   }

   return -1; // failed
}

bool pgsStatusCenter::RemoveByID(Int32 id)
{
   Container::iterator iter;
   for ( iter = m_Items.begin(); iter != m_Items.end(); iter++ )
   {
      pgsStatusItem* pItem = *iter;
      if ( pItem->GetID() == id && m_pCurrentItem != pItem )
      {
         delete pItem;
         pItem = NULL;
         m_Items.erase(iter);
         NotifyRemoved(id);
         return true;
      }
      else
      {
         pItem->RemoveAfterEdit(true);
      }
   }

   return false;
}

bool pgsStatusCenter::RemoveByIndex(Uint32 index)
{
   if ( index < 0 || m_Items.size() <= index )
      return false;

   Container::iterator iter = m_Items.begin();
   for ( Uint32 i = 0; i <= index; i++ )
      iter++;

   pgsStatusItem* pItem = *iter;
   if ( m_pCurrentItem != pItem )
   {
      Int32 id = pItem->GetID();
      delete pItem;
      pItem = NULL;
      m_Items.erase(iter);
      NotifyRemoved(id);
      return true;
   }
   else
   {
      pItem->RemoveAfterEdit(true);
   }

   return false;
}

bool pgsStatusCenter::RemoveByAgentID(Int32 id)
{
   bool bItemsRemoved = false;
   Container::iterator iter;

   Container items = m_Items;
   m_Items.clear();

   for ( iter = items.begin(); iter != items.end(); iter++)
   {
      pgsStatusItem* pItem = *iter;
      if ( pItem->GetAgentID() == id )
      {
         if ( m_pCurrentItem != pItem )
         {
            Int32 itemID = pItem->GetID();
            delete pItem;
            pItem = NULL;
            NotifyRemoved(itemID);
            bItemsRemoved = true;
         }
         else
         {
            NotifyRemoved(pItem->GetID());
            pItem->RemoveAfterEdit(true);
         }
      }
      else
      {
         m_Items.insert(pItem);
      }
   }

   return bItemsRemoved;
}

pgsStatusItem* pgsStatusCenter::GetByID(Int32 id)
{
   Container::iterator iter;
   for ( iter = m_Items.begin(); iter != m_Items.end(); iter++ )
   {
      pgsStatusItem* pItem = *iter;
      if ( pItem->GetID() == id )
         return pItem;
   }

   return NULL;
}

pgsStatusItem* pgsStatusCenter::GetByIndex(Uint32 index)
{
   if ( index < 0 || m_Items.size() < index )
      return 0;

   Container::iterator iter = m_Items.begin();
   for ( Uint32 i = 0; i < index; i++ )
      iter ++;

   return *iter;
}

Uint32 pgsStatusCenter::Count()
{
   return m_Items.size();
}

Uint32 pgsStatusCenter::GetSeverity()
{
   Uint32 severity = STATUS_OK;

   Container::iterator iter;
   for ( iter = m_Items.begin(); iter != m_Items.end(); iter++ )
   {
      pgsStatusItem* pItem = *iter;
      severity = _cpp_max(severity, GetSeverity(pItem->GetCallbackID()));
   }

   return severity;
}

void pgsStatusCenter::SinkEvents(iStatusCenterEventSink* pSink)
{
   m_Sinks.insert(pSink);
}

void pgsStatusCenter::UnSinkEvents(iStatusCenterEventSink* pSink)
{
   Sinks::iterator found = m_Sinks.find(pSink);
   if ( found != m_Sinks.end() )
      m_Sinks.erase(found);
}

void pgsStatusCenter::NotifyAdded(pgsStatusItem* pNewItem)
{
   Sinks::iterator iter;
   for ( iter = m_Sinks.begin(); iter != m_Sinks.end(); iter++ )
   {
      iStatusCenterEventSink* pSink = *iter;
      pSink->OnStatusItemAdded(pNewItem);
   }
}

void pgsStatusCenter::NotifyRemoved(Int32 id)
{
   Sinks::iterator iter;
   for ( iter = m_Sinks.begin(); iter != m_Sinks.end(); iter++ )
   {
      iStatusCenterEventSink* pSink = *iter;
      pSink->OnStatusItemRemoved(id);
   }
}

void pgsStatusCenter::RegisterCallbackItem(Uint32 callbackID,iStatusCallback* pCallback)
{
   m_Callbacks.insert(std::make_pair(callbackID,pCallback));
}

Uint32 pgsStatusCenter::GetSeverity(Uint32 callbackID)
{
   iStatusCallback* pCallback = GetCallback(callbackID);
   if ( !pCallback )
      return STATUS_OK;

   return pCallback->GetSeverity();
}

iStatusCallback* pgsStatusCenter::GetCallback(Int32 callbackID)
{
   Callbacks::iterator found = m_Callbacks.find(callbackID);
   if ( found == m_Callbacks.end() )
      return NULL;

   return (*found).second;
}

void pgsStatusCenter::EditItem(Uint32 id)
{
   pgsStatusItem* pItem = GetByID(id);
   ASSERT(pItem != NULL);

   Uint32 callbackID = pItem->GetCallbackID();
   iStatusCallback* pCallback = GetCallback(callbackID);
   ASSERT(pCallback != NULL);

   m_pCurrentItem = pItem;

   pCallback->Execute(pItem);

   m_pCurrentItem = NULL;

   // remove item if required
   if (pItem->RemoveAfterEdit())
   {
      RemoveByID(id);
   }
}
