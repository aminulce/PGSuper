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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsFlexuralStressArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsFlexuralStressArtifact::pgsFlexuralStressArtifact()
{
   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      m_bIsApplicable[stressLocation] = false;
      m_fPretension[stressLocation]   = 0.0;
      m_fPosttension[stressLocation]  = 0.0;
      m_fExternal[stressLocation]     = 0.0;
      m_fDemand[stressLocation]       = 0.0;
      m_fAllowable[stressLocation]    = 0.0;
      m_bIsInPTZ[stressLocation]      = false;

      m_Yna[i]                           = 0.0;
      m_At[i]                            = 0.0;
      m_T[i]                             = 0.0;
      m_AsProvided[i]                    = 0.0;
      m_AsRequired[i]                    = 0.0;
      m_fAltAllowableStress[i]           = 0.0;
      m_bIsAltTensileStressApplicable[i] = false;

      m_FcReqd[i] = -99999;
   }
}

pgsFlexuralStressArtifact::pgsFlexuralStressArtifact(const pgsPointOfInterest& poi):
m_Poi(poi)
{
   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      m_bIsApplicable[stressLocation] = false;
      m_fPretension[stressLocation]   = 0.0;
      m_fPosttension[stressLocation]  = 0.0;
      m_fExternal[stressLocation]     = 0.0;
      m_fDemand[stressLocation]       = 0.0;
      m_fAllowable[stressLocation]    = 0.0;
      m_bIsInPTZ[stressLocation]      = false;

      m_Yna[i]                           = 0.0;
      m_At[i]                            = 0.0;
      m_T[i]                             = 0.0;
      m_AsProvided[i]                    = 0.0;
      m_AsRequired[i]                    = 0.0;
      m_fAltAllowableStress[i]           = 0.0;
      m_bIsAltTensileStressApplicable[i] = false;

      m_FcReqd[i] = -99999;
   }
}

pgsFlexuralStressArtifact::pgsFlexuralStressArtifact(const pgsFlexuralStressArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsFlexuralStressArtifact::~pgsFlexuralStressArtifact()
{
}

//======================== OPERATORS  =======================================
pgsFlexuralStressArtifact& pgsFlexuralStressArtifact::operator=(const pgsFlexuralStressArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsFlexuralStressArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_Poi = poi;
}

const pgsPointOfInterest& pgsFlexuralStressArtifact::GetPointOfInterest() const
{
   return m_Poi;
}

bool pgsFlexuralStressArtifact::operator<(const pgsFlexuralStressArtifact& rOther) const
{
   return m_Poi < rOther.m_Poi;
}

void pgsFlexuralStressArtifact::SetLimitState(pgsTypes::LimitState limitState)
{
   m_LimitState = limitState;
}

pgsTypes::LimitState pgsFlexuralStressArtifact::GetLimitState() const
{
   return m_LimitState;
}

void pgsFlexuralStressArtifact::SetStressType(pgsTypes::StressType type)
{
   m_StressType = type;
}

pgsTypes::StressType pgsFlexuralStressArtifact::GetStressType() const
{
   return m_StressType;
}

void pgsFlexuralStressArtifact::IsApplicable(pgsTypes::StressLocation stressLocation,bool bIsApplicable)
{
   m_bIsApplicable[stressLocation] = bIsApplicable;
}

bool pgsFlexuralStressArtifact::IsApplicable(pgsTypes::StressLocation stressLocation) const
{
   return m_bIsApplicable[stressLocation];
}

void pgsFlexuralStressArtifact::SetPretensionEffects(pgsTypes::StressLocation stressLocation,Float64 fPS)
{
   m_fPretension[stressLocation] = fPS;
}

Float64 pgsFlexuralStressArtifact::GetPretensionEffects(pgsTypes::StressLocation stressLocation) const
{
   return m_fPretension[stressLocation];
}

void pgsFlexuralStressArtifact::SetPosttensionEffects(pgsTypes::StressLocation stressLocation,Float64 fPT)
{
   m_fPosttension[stressLocation] = fPT;
}

Float64 pgsFlexuralStressArtifact::GetPosttensionEffects(pgsTypes::StressLocation stressLocation) const
{
   return m_fPosttension[stressLocation];
}

void pgsFlexuralStressArtifact::SetExternalEffects(pgsTypes::StressLocation stressLocation,Float64 f)
{
   m_fExternal[stressLocation] = f;
}

Float64 pgsFlexuralStressArtifact::GetExternalEffects(pgsTypes::StressLocation stressLocation) const
{
   return m_fExternal[stressLocation];
}

void pgsFlexuralStressArtifact::SetDemand(pgsTypes::StressLocation stressLocation,Float64 fDemand)
{
   m_fDemand[stressLocation] = fDemand;
}

Float64 pgsFlexuralStressArtifact::GetDemand(pgsTypes::StressLocation stressLocation) const
{
   return m_fDemand[stressLocation];
}

void pgsFlexuralStressArtifact::IsInPrecompressedTensileZone(pgsTypes::StressLocation stressLocation,bool bIsInPTZ)
{
   m_bIsInPTZ[stressLocation] = bIsInPTZ;
}

bool pgsFlexuralStressArtifact::IsInPrecompressedTensileZone(pgsTypes::StressLocation stressLocation) const
{
   return m_bIsInPTZ[stressLocation];
}

void pgsFlexuralStressArtifact::SetCapacity(pgsTypes::StressLocation stressLocation,Float64 fAllowable)
{
   m_fAllowable[stressLocation] = fAllowable;
}

Float64 pgsFlexuralStressArtifact::GetCapacity(pgsTypes::StressLocation stressLocation) const
{
   return m_fAllowable[stressLocation];
}


void pgsFlexuralStressArtifact::SetRequiredConcreteStrength(pgsTypes::StressLocation stressLocation,Float64 fcReqd)
{
   m_FcReqd[stressLocation] = fcReqd;
}

Float64 pgsFlexuralStressArtifact::GetRequiredConcreteStrength(pgsTypes::StressLocation stressLocation) const
{
   return m_FcReqd[stressLocation];
}

Float64 pgsFlexuralStressArtifact::GetRequiredBeamConcreteStrength() const
{
   return Max(m_FcReqd[pgsTypes::TopGirder],m_FcReqd[pgsTypes::BottomGirder]);
}

Float64 pgsFlexuralStressArtifact::GetRequiredDeckConcreteStrength() const
{
   return Max(m_FcReqd[pgsTypes::TopDeck],m_FcReqd[pgsTypes::BottomDeck]);
}

void pgsFlexuralStressArtifact::SetAlternativeTensileStressParameters(pgsTypes::StressLocation stressLocation,Float64 Yna,Float64 At,Float64 T,Float64 AsProvided,Float64 AsRequired,Float64 fHigherAllow)
{
   m_Yna[stressLocation]                 = Yna;
   m_At[stressLocation]                  = At;
   m_T[stressLocation]                   = T;
   m_AsProvided[stressLocation]          = AsProvided;
   m_AsRequired[stressLocation]          = AsRequired;
   m_fAltAllowableStress[stressLocation] = fHigherAllow;
   m_bIsAltTensileStressApplicable[stressLocation] = true;
}

void pgsFlexuralStressArtifact::GetAlternativeTensileStressParameters(pgsTypes::StressLocation stressLocation,Float64* Yna,Float64* At,Float64* T,Float64* AsProvided,Float64* AsRequired) const
{
   *Yna        = m_Yna[stressLocation];
   *At         = m_At[stressLocation];
   *T          = m_T[stressLocation];
   *AsProvided = m_AsProvided[stressLocation];
   *AsRequired = m_AsRequired[stressLocation];
}

Float64 pgsFlexuralStressArtifact::GetAlternativeAllowableTensileStress(pgsTypes::StressLocation stressLocation) const
{
   return m_fAltAllowableStress[stressLocation];
}

bool pgsFlexuralStressArtifact::IsWithRebarAllowableStressApplicable(pgsTypes::StressLocation stressLocation) const
{
   return m_bIsAltTensileStressApplicable[stressLocation];
}

bool pgsFlexuralStressArtifact::WasWithRebarAllowableStressUsed(pgsTypes::StressLocation stressLocation) const
{
   // If na<0.0, then section was in compression
   bool bWasUsed = (0.0 < m_Yna[stressLocation]) && (m_AsRequired[stressLocation] <= m_AsProvided[stressLocation]) ? true : false;
#if defined _DEBUG
   if ( bWasUsed )
   {
      ATLASSERT(m_bIsAltTensileStressApplicable[stressLocation]);
   }
#endif
   return bWasUsed;
}

bool pgsFlexuralStressArtifact::Passed(pgsTypes::StressLocation stressLocation) const
{
   return StressedPassed(stressLocation);
}

bool pgsFlexuralStressArtifact::StressedPassed(pgsTypes::StressLocation stressLocation) const
{
   Float64 fStress = GetDemand(stressLocation);
   if ( m_StressType == pgsTypes::Compression )
   {
      if ( IsTopStressLocation(stressLocation) && m_bIsApplicable[stressLocation] )
      {
         if ( (fStress < m_fAllowable[stressLocation] && !IsEqual(m_fAllowable[stressLocation],fStress,0.001)) )
         {
            return false;
         }
      }

      if ( IsBottomStressLocation(stressLocation) && m_bIsApplicable[stressLocation] )
      {
         if ( (fStress < m_fAllowable[stressLocation] && !IsEqual(m_fAllowable[stressLocation],fStress,0.001)) )
         {
            return false;
         }
      }

      // If top and bottom passed or if top and bottom are not applicable
      return true;
   }
   else
   {
      if (m_AsRequired[stressLocation] <= m_AsProvided[stressLocation] && !IsZero(m_AsProvided[stressLocation]) )
      {
        // If we have adequate rebar, we can use higher limit
         return TensionPassedWithRebar(fStress,stressLocation); 
      }
      else
      {
         return TensionPassedWithoutRebar(fStress,stressLocation);
      }
   }

   ATLASSERT(false); // should never get here
   return false;
}

bool pgsFlexuralStressArtifact::TensionPassedWithRebar(Float64 fTens,pgsTypes::StressLocation stressLocation) const
{
   if ( m_bIsApplicable[stressLocation] )
   {
      if ( (m_fAltAllowableStress[stressLocation] < fTens && !IsEqual(m_fAltAllowableStress[stressLocation],fTens,0.001) ) )
      {
         return false;
      }
   }

   return true;
}

bool pgsFlexuralStressArtifact::TensionPassedWithoutRebar(Float64 fTens,pgsTypes::StressLocation stressLocation) const
{
   if ( m_bIsApplicable[stressLocation] )
   {
      if ( ( m_fAllowable[stressLocation] < fTens && !IsEqual(m_fAllowable[stressLocation],fTens,0.001)) )
      {
         return false;
      }
   }

   return true;
}

bool pgsFlexuralStressArtifact::BeamPassed() const
{
   if ( !Passed(pgsTypes::TopGirder) )
   {
      return false;
   }

   if ( !Passed(pgsTypes::BottomGirder) )
   {
      return false;
   }

   return true;
}

bool pgsFlexuralStressArtifact::DeckPassed() const
{
   if ( !Passed(pgsTypes::TopDeck) )
   {
      return false;
   }

   if ( !Passed(pgsTypes::BottomDeck) )
   {
      return false;
   }

   return true;
}

Float64 pgsFlexuralStressArtifact::GetBeamCDRatio() const
{
   return GetCDRatio(pgsTypes::TopGirder,pgsTypes::BottomGirder);
}

Float64 pgsFlexuralStressArtifact::GetDeckCDRatio() const
{
   return GetCDRatio(pgsTypes::TopDeck,pgsTypes::BottomDeck);
}

Float64 pgsFlexuralStressArtifact::GetCDRatio(pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation) const
{
   Float64 cdr_top = GetCDRatio(topStressLocation);
   Float64 cdr_bot = GetCDRatio(botStressLocation);

   Float64 cdr;

   if ( cdr_top < 0 && cdr_bot < 0)
   {
      // top and bottom are both N/A or SKIP
      // Figure out which to return
      if ( cdr_top == cdr_bot )
      {
         return cdr_top;
      }

      if ( m_bIsApplicable[topStressLocation] && !m_bIsApplicable[botStressLocation] )
      {
         return cdr_top;
      }
      else if ( !m_bIsApplicable[topStressLocation] && m_bIsApplicable[botStressLocation] )
      {
         return cdr_bot;
      }
      else
      {
         return Min(cdr_top,cdr_bot);
      }
   }
   else if ( 0 <= cdr_top && cdr_bot < 0 )
   {
      // Top only
      cdr = cdr_top;
   }
   else if ( cdr_top < 0 && 0 <= cdr_bot )
   {
      // Bottom only
      cdr = cdr_bot;
   }
   else
   {
      // controlling of top and bottom
      cdr = Min(cdr_top,cdr_bot);
   }

   // deal with case where round-off gives a CD Ratio of 1.0
   // but the spec check doesn't actually pass
   if ( IsEqual(cdr,1.0) && !(Passed(topStressLocation) && Passed(botStressLocation)) )
   {
      cdr = 0.99;
   }

   return cdr;
}

Float64 pgsFlexuralStressArtifact::GetCDRatio(Float64 c,Float64 d) const
{
   // skip the C/D ratio if capacity or demand is non-zero
   // cases of zero capacity and/or demand are dealt with below
   if ( !IsZero(d) && ::BinarySign(c) != ::BinarySign(d) )
   {
      return CDR_SKIP; // skip CD Ratio if signs aren't the same
   }

   if ( IsZero(d) )
   {
      // c/d -> don't want to divide by zero
      if ( IsZero(c) )
      {
         // 0/0, use a C/D of 1.0
         return 1.0;
      }
      else
      {
         // c/0 -> use infinity
         return CDR_INF;
      }
   }

   Float64 cdr = c/d;
   cdr = IsZero(cdr) ? 0 : cdr;

   // if the actual CD Ratio exceeds the constant that identifies
   // infinity, tweek the value so that 10+ is reported
   if ( CDR_INF <= cdr )
   {
      cdr = CDR_LARGE;
   }

   return cdr;
}

Float64 pgsFlexuralStressArtifact::GetCDRatio(pgsTypes::StressLocation stressLocation) const
{
   if ( !m_bIsApplicable[stressLocation] )
   {
      return CDR_NA;
   }

   Float64 d = m_fDemand[stressLocation];
   Float64 c;
   if ( WasWithRebarAllowableStressUsed(stressLocation) )
   {
      c = m_fAltAllowableStress[stressLocation];
   }
   else
   {
      c = m_fAllowable[stressLocation];
   }

   return GetCDRatio(c,d);
}


void pgsFlexuralStressArtifact::MakeCopy(const pgsFlexuralStressArtifact& rOther)
{
   m_Poi                 = rOther.m_Poi;
   m_LimitState          = rOther.m_LimitState;
   m_StressType          = rOther.m_StressType;

   for ( int i = 0; i < 4; i++ )
   {
      pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      m_bIsApplicable[stressLocation] = rOther.m_bIsApplicable[stressLocation];
      m_fPretension[stressLocation]   = rOther.m_fPretension[stressLocation];
      m_fPosttension[stressLocation]  = rOther.m_fPosttension[stressLocation];
      m_fExternal[stressLocation]     = rOther.m_fExternal[stressLocation];
      m_fDemand[stressLocation]       = rOther.m_fDemand[stressLocation];
      m_fAllowable[stressLocation]    = rOther.m_fAllowable[stressLocation];
      m_bIsInPTZ[stressLocation]      = rOther.m_bIsInPTZ[stressLocation];

      m_Yna[i]                 = rOther.m_Yna[i];
      m_At[i]                  = rOther.m_At[i];
      m_T[i]                   = rOther.m_T[i];
      m_AsProvided[i]          = rOther.m_AsProvided[i];
      m_AsRequired[i]          = rOther.m_AsRequired[i];
      m_fAltAllowableStress[i] = rOther.m_fAltAllowableStress[i];

      m_bIsAltTensileStressApplicable[i] = rOther.m_bIsAltTensileStressApplicable[i];

      m_FcReqd[i]              = rOther.m_FcReqd[i];
   }
}

void pgsFlexuralStressArtifact::MakeAssignment(const pgsFlexuralStressArtifact& rOther)
{
   MakeCopy( rOther );
}
