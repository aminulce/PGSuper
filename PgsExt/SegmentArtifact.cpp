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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SegmentArtifact.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsSegmentArtifact::pgsSegmentArtifact(const CSegmentKey& segmentKey) :
m_SegmentKey(segmentKey)
{
}

pgsSegmentArtifact::pgsSegmentArtifact(const pgsSegmentArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSegmentArtifact::~pgsSegmentArtifact()
{
}

pgsSegmentArtifact& pgsSegmentArtifact::operator= (const pgsSegmentArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool pgsSegmentArtifact::operator<(const pgsSegmentArtifact& rOther) const
{
   return m_SegmentKey < rOther.m_SegmentKey;
}

void pgsSegmentArtifact::SetStrandStressArtifact(const pgsStrandStressArtifact& artifact)
{
   m_StrandStressArtifact = artifact;
}

const pgsStrandStressArtifact* pgsSegmentArtifact::GetStrandStressArtifact() const
{
   return &m_StrandStressArtifact;
}

pgsStrandStressArtifact* pgsSegmentArtifact::GetStrandStressArtifact()
{
   return &m_StrandStressArtifact;
}

void pgsSegmentArtifact::SetStrandSlopeArtifact(const pgsStrandSlopeArtifact& artifact)
{
   m_StrandSlopeArtifact = artifact;
}

const pgsStrandSlopeArtifact* pgsSegmentArtifact::GetStrandSlopeArtifact() const
{
   return &m_StrandSlopeArtifact;
}

pgsStrandSlopeArtifact* pgsSegmentArtifact::GetStrandSlopeArtifact()
{
   return &m_StrandSlopeArtifact;
}

void pgsSegmentArtifact::SetHoldDownForceArtifact(const pgsHoldDownForceArtifact& artifact)
{
   m_HoldDownForceArtifact = artifact;
}

const pgsHoldDownForceArtifact* pgsSegmentArtifact::GetHoldDownForceArtifact() const
{
   return &m_HoldDownForceArtifact;
}

pgsHoldDownForceArtifact* pgsSegmentArtifact::GetHoldDownForceArtifact()
{
   return &m_HoldDownForceArtifact;
}

void pgsSegmentArtifact::AddFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,
                                                   const pgsFlexuralStressArtifact& artifact)
{
   // Need to be using a valid ID in the artifact
   ATLASSERT(artifact.GetPointOfInterest().GetID() != INVALID_ID);

   std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   artifacts.push_back(artifact);
   std::sort(artifacts.begin(),artifacts.end());
}

CollectionIndexType pgsSegmentArtifact::GetFlexuralStressArtifactCount(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress) const
{
   const std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   return artifacts.size();
}

const pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,CollectionIndexType idx) const
{
   const std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   return &artifacts[idx];
}

pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,CollectionIndexType idx)
{
   std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   return &artifacts[idx];
}

const pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifactAtPoi(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,PoiIDType poiID) const
{
   const std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   std::vector<pgsFlexuralStressArtifact>::const_iterator iter(artifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::const_iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsFlexuralStressArtifact& artifact(*iter);
      if ( artifact.GetPointOfInterest().GetID() == poiID )
         return &artifact;
   }

   return NULL;
}

const pgsStirrupCheckArtifact* pgsSegmentArtifact::GetStirrupCheckArtifact() const
{
   return &m_StirrupCheckArtifact;
}

pgsStirrupCheckArtifact* pgsSegmentArtifact::GetStirrupCheckArtifact()
{
   return &m_StirrupCheckArtifact;
}

const pgsPrecastIGirderDetailingArtifact* pgsSegmentArtifact::GetPrecastIGirderDetailingArtifact() const
{
   return &m_PrecastIGirderDetailingArtifact;
}

pgsPrecastIGirderDetailingArtifact* pgsSegmentArtifact::GetPrecastIGirderDetailingArtifact()
{
   return &m_PrecastIGirderDetailingArtifact;
}

void pgsSegmentArtifact::SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact)
{
   m_ConstructabilityArtifact = artifact;
}

const pgsConstructabilityArtifact* pgsSegmentArtifact::GetConstructabilityArtifact() const
{
   return &m_ConstructabilityArtifact;
}

pgsConstructabilityArtifact* pgsSegmentArtifact::GetConstructabilityArtifact()
{
   return &m_ConstructabilityArtifact;
}

void pgsSegmentArtifact::SetLiftingAnalysisArtifact(pgsLiftingAnalysisArtifact* artifact)
{
   m_pLiftingAnalysisArtifact = std::auto_ptr<pgsLiftingAnalysisArtifact>(artifact);
}

const pgsLiftingAnalysisArtifact* pgsSegmentArtifact::GetLiftingAnalysisArtifact() const
{
   return m_pLiftingAnalysisArtifact.get();
}

void pgsSegmentArtifact::SetHaulingAnalysisArtifact(pgsHaulingAnalysisArtifact* artifact)
{
   m_pHaulingAnalysisArtifact = std::auto_ptr<pgsHaulingAnalysisArtifact>(artifact);;
}

const pgsHaulingAnalysisArtifact* pgsSegmentArtifact::GetHaulingAnalysisArtifact() const
{
   return m_pHaulingAnalysisArtifact.get();
}

void pgsSegmentArtifact::SetCastingYardCapacityWithMildRebar(Float64 fAllow)
{
    m_CastingYardAllowable = fAllow;
}

Float64 pgsSegmentArtifact::GetCastingYardCapacityWithMildRebar() const
{
    return m_CastingYardAllowable;
}

pgsDebondArtifact* pgsSegmentArtifact::GetDebondArtifact(pgsTypes::StrandType strandType)
{
   return &m_DebondArtifact[strandType];
}

const pgsDebondArtifact* pgsSegmentArtifact::GetDebondArtifact(pgsTypes::StrandType strandType) const
{
   return &m_DebondArtifact[strandType];
}

bool pgsSegmentArtifact::Passed() const
{
   bool bPassed = true;

   bPassed &= m_ConstructabilityArtifact.Pass();
   bPassed &= m_HoldDownForceArtifact.Passed();
   bPassed &= m_StrandSlopeArtifact.Passed();
   bPassed &= m_StrandStressArtifact.Passed();
   bPassed &= DidFlexuralStressesPass();

   bPassed &= m_StirrupCheckArtifact.Passed();

   bPassed &= m_PrecastIGirderDetailingArtifact.Passed();

   if (m_pLiftingAnalysisArtifact.get()!=NULL)
      bPassed &= m_pLiftingAnalysisArtifact->Passed();

   if (m_pHaulingAnalysisArtifact.get()!=NULL)
      bPassed &= m_pHaulingAnalysisArtifact->Passed();

   for ( Uint16 i = 0; i < 3; i++ )
      bPassed &= m_DebondArtifact[i].Passed();

   return bPassed;
}

bool pgsSegmentArtifact::DidFlexuralStressesPass() const
{
   bool bPassed = true;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator  i2;
   for ( i2 = m_FlexuralStressArtifacts.begin(); i2 != m_FlexuralStressArtifacts.end(); i2++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *i2;
      std::vector<pgsFlexuralStressArtifact>::const_iterator iter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator end(item.second.end());
      for ( ; iter != end; iter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*iter);
         bPassed &= artifact.Passed();
      }
   }

   return bPassed;
}

Float64 pgsSegmentArtifact::GetRequiredConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   Float64 fc_reqd = 0;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx == intervalIdx && key.ls == ls )
      {
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
         for ( ; artifactIter != artifactIterEnd; artifactIter++ )
         {
            const pgsFlexuralStressArtifact& artifact(*artifactIter);
            Float64 fc = artifact.GetRequiredConcreteStrength();

            if ( fc < 0 ) 
               return fc;

            if ( 0 < fc )
               fc_reqd = _cpp_max(fc,fc_reqd);
         }
      }
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredConcreteStrength() const
{
   Float64 fc_reqd = 0;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(m_SegmentKey);

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx < haulingIntervalIdx )
         continue; // don't check if this is before hauling (basically we want final concrete strength cases)

      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
      for ( ; artifactIter != artifactIterEnd; artifactIter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*artifactIter);
         Float64 fc = artifact.GetRequiredConcreteStrength();

         if ( fc < 0 ) 
            return fc;

         if ( 0 < fc )
            fc_reqd = _cpp_max(fc,fc_reqd);
      }
   }

#pragma Reminder("REVIEW: why is lifting used to determine f'c")
   // This came from the merge of RDP's code for mild rebar for tension
   // Why is lifting being considered for final strength? Lifting controls release strength
   if (m_pLiftingAnalysisArtifact.get() != NULL)
   {
      Float64 fc_reqd_Lifting_comp, fc_reqd_Lifting_tens, fc_reqd_Lifting_tens_wbar;
      m_pLiftingAnalysisArtifact->GetRequiredConcreteStrength(&fc_reqd_Lifting_comp,&fc_reqd_Lifting_tens, &fc_reqd_Lifting_tens_wbar);

      Float64 fc_reqd_Lifting = max(fc_reqd_Lifting_tens_wbar,fc_reqd_Lifting_comp);

      if ( fc_reqd_Lifting < 0 ) // there is no concrete strength that will work
         return fc_reqd_Lifting;

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_Lifting);
   }

   if (m_pHaulingAnalysisArtifact.get()!=NULL)
   {
      Float64 fc_reqd_hauling_comp, fc_reqd_hauling_tens, fc_reqd_hauling_tens_wbar;
      m_pHaulingAnalysisArtifact->GetRequiredConcreteStrength(&fc_reqd_hauling_comp,&fc_reqd_hauling_tens, &fc_reqd_hauling_tens_wbar);

      Float64 fc_reqd_hauling = max(fc_reqd_hauling_tens_wbar,fc_reqd_hauling_comp);

      if ( fc_reqd_hauling < 0 ) // there is no concrete strength that will work
         return fc_reqd_hauling;

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_hauling);
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredReleaseStrength() const
{
   Float64 fc_reqd = 0;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(m_SegmentKey);

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( haulingIntervalIdx <= key.intervalIdx )
         continue; // don't check if this is after hauling (basically we want release concrete strength cases)

      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
      for ( ; artifactIter != artifactIterEnd; artifactIter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*artifactIter);
         Float64 fc = artifact.GetRequiredConcreteStrength();

         if ( fc < 0 ) 
            return fc;

         if ( 0 < fc )
            fc_reqd = _cpp_max(fc,fc_reqd);
      }
   }
 
   if (m_pLiftingAnalysisArtifact.get() != NULL)
   {
      Float64 fc_reqd_lifting_comp,fc_reqd_lifting_tens_norebar,fc_reqd_lifting_tens_withrebar;
      m_pLiftingAnalysisArtifact->GetRequiredConcreteStrength(&fc_reqd_lifting_comp,&fc_reqd_lifting_tens_norebar,&fc_reqd_lifting_tens_withrebar);

      Float64 fc_reqd_lifting = Max3(fc_reqd_lifting_comp,fc_reqd_lifting_tens_norebar,fc_reqd_lifting_tens_withrebar);

      fc_reqd = _cpp_max(fc_reqd,fc_reqd_lifting);
   }

   return fc_reqd;
}

const CSegmentKey& pgsSegmentArtifact::GetSegmentKey() const
{
   return m_SegmentKey;
}

void pgsSegmentArtifact::MakeCopy(const pgsSegmentArtifact& rOther)
{
   m_SegmentKey = rOther.m_SegmentKey;

   m_StrandStressArtifact            = rOther.m_StrandStressArtifact;
   m_FlexuralStressArtifacts         = rOther.m_FlexuralStressArtifacts;
   m_StirrupCheckArtifact            = rOther.m_StirrupCheckArtifact;
   m_PrecastIGirderDetailingArtifact = rOther.m_PrecastIGirderDetailingArtifact;
   m_StrandSlopeArtifact             = rOther.m_StrandSlopeArtifact;
   m_HoldDownForceArtifact           = rOther.m_HoldDownForceArtifact;
   m_ConstructabilityArtifact        = rOther.m_ConstructabilityArtifact;

   if(rOther.m_pLiftingAnalysisArtifact.get() != NULL)
   {
      m_pLiftingAnalysisArtifact  = std::auto_ptr<pgsLiftingAnalysisArtifact>(new pgsLiftingAnalysisArtifact);
      *m_pLiftingAnalysisArtifact = *rOther.m_pLiftingAnalysisArtifact;
   }

   if(rOther.m_pHaulingAnalysisArtifact.get() != NULL)
   {
      m_pHaulingAnalysisArtifact = std::auto_ptr<pgsHaulingAnalysisArtifact>(rOther.m_pHaulingAnalysisArtifact->Clone());
   }

   m_CastingYardAllowable            = rOther.m_CastingYardAllowable;
   for ( Uint16 i = 0; i < 3; i++ )
      m_DebondArtifact[i] = rOther.m_DebondArtifact[i];
}

void pgsSegmentArtifact::MakeAssignment(const pgsSegmentArtifact& rOther)
{
   MakeCopy( rOther );
}

std::vector<pgsFlexuralStressArtifact>& pgsSegmentArtifact::GetFlexuralStressArtifacts(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress) const
{
   StressKey key;
   key.intervalIdx = intervalIdx;
   key.ls = ls;
   key.stress = stress;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::iterator found(m_FlexuralStressArtifacts.find(key));
   if ( found == m_FlexuralStressArtifacts.end() )
   {
      std::pair<std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::iterator,bool> result(m_FlexuralStressArtifacts.insert(std::make_pair(key,std::vector<pgsFlexuralStressArtifact>())));
      found = result.first;
   }

   return found->second;
}
