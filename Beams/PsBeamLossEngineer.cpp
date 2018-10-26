///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2013  Washington State Department of Transportation
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

// PsBeamLossEngineer.cpp : Implementation of CPsBeamLossEngineer
#include "stdafx.h"
#include "PsBeamLossEngineer.h"
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDesignLosses::CDesignLosses()
{
   Invalidate();
}

void CDesignLosses::Invalidate()
{
   m_Losses.clear();
}

const LOSSDETAILS* CDesignLosses::GetFromCache(const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   Losses* pLosses = 0;

   std::map<pgsPointOfInterest,Losses>::iterator iter(m_Losses.begin() );
   std::map<pgsPointOfInterest,Losses>::iterator end(m_Losses.end() );
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& p = iter->first;
      // check equality based only on location, not POI ID... sometimes temporary POIs (ID < 0) are 
      // used during design
      if ( p.GetSegmentKey() == poi.GetSegmentKey() && IsEqual(p.GetDistFromStart(),poi.GetDistFromStart()) )
      {
         // Found it!
         pLosses = &(iter->second);
         break;
      }
   }

   if ( pLosses == NULL )
      return NULL; // not found... we don't have it cached

   // have results for this POI cached, but was it for the same configuration?
   if ( config.IsFlexuralDataEqual(pLosses->m_Config) )
   {
      // yep... return the results
      return &(pLosses->m_Details);
   }
   else
   {
      // the one that was found doesn't match for this POI, so remove it because
      // we have new values for this POI
      m_Losses.erase(iter);
      return NULL;
   }

   ATLASSERT(false); // should never get here
}

void CDesignLosses::SaveToCache(const pgsPointOfInterest& poi, const GDRCONFIG& config, const LOSSDETAILS& losses)
{
   Losses l;
   l.m_Config = config;
   l.m_Details = losses;

   std::pair<std::map<pgsPointOfInterest,Losses>::iterator,bool> result = m_Losses.insert( std::make_pair(poi,l) );
   ATLASSERT( result.second == true );
}

/////////////////////////////////////////////////////////////////////////////
// CUBeamPsLossEngineer
HRESULT CPsBeamLossEngineer::FinalConstruct()
{
   return S_OK;
}

void CPsBeamLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   m_Engineer.Init(m_pBroker,m_StatusGroupID);
}

const LOSSDETAILS* CPsBeamLossEngineer::GetLosses(const pgsPointOfInterest& poi)
{
   std::map<PoiIDKey,LOSSDETAILS>::const_iterator found;
   PoiIDKey key(poi.GetID(),poi);
   found = m_PsLosses.find( key );
   if ( found == m_PsLosses.end() )
   {
      // losses not found... compute them
      LOSSDETAILS details = m_Engineer.ComputeLosses((CPsLossEngineer::BeamType)m_BeamType,poi);
      m_PsLosses.insert(std::make_pair(key,details));
      found = m_PsLosses.find( key );
      ATLASSERT(found != m_PsLosses.end());
   }

   return &(*found).second;
}

const LOSSDETAILS* CPsBeamLossEngineer::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   const LOSSDETAILS* pLossDetails = m_DesignLosses.GetFromCache(poi,config);
   if ( pLossDetails == NULL )
   {
      LOSSDETAILS details = m_Engineer.ComputeLossesForDesign((CPsLossEngineer::BeamType)m_BeamType,poi,config);
      m_DesignLosses.SaveToCache(poi,config,details);
      pLossDetails = m_DesignLosses.GetFromCache(poi,config);
   }

   return pLossDetails;
}

void CPsBeamLossEngineer::ClearDesignLosses()
{
   m_DesignLosses.Invalidate();
}

void CPsBeamLossEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_Engineer.BuildReport((CPsLossEngineer::BeamType)m_BeamType,girderKey,pChapter,pDisplayUnits);
}

void CPsBeamLossEngineer::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_Engineer.ReportFinalLosses((CPsLossEngineer::BeamType)m_BeamType,girderKey,pChapter,pDisplayUnits);
}

const ANCHORSETDETAILS* CPsBeamLossEngineer::GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   return NULL;
}

Float64 CPsBeamLossEngineer::GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   return 0;
}
