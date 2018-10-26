///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2016  Washington State Department of Transportation
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

// AnalysisAgentImp.cpp : Implementation of CAnalysisAgent
#include "stdafx.h"
#include "AnalysisAgent.h"
#include "AnalysisAgent_i.h"
#include "AnalysisAgentImp.h"
#include "StatusItems.h"
#include "..\PGSuperException.h"

#include <IFace\Intervals.h>

#include <WBFLSTL.h>
#include <MathEx.h>

#include <PgsExt\LoadFactors.h>
#include <PgsExt\DebondUtil.h>
#include <PgsExt\GirderModelFactory.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderMaterial.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\ClosureJointData.h>

#include <PgsExt\GirderLabel.h>

#pragma Reminder("NOTES: Possible New Features") 
// 1) IExternalLoading: Add CreateDistributedLoad. 
//    Distributed load is the more general version and it is supported by the FEM model...

//////////////////////////////////////////////////////////////////////////////////
// NOTES:
//
// The pedestrian live load distribution factor is the pedestrian load.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DECLARE_LOGFILE;

// FEM Loading IDs
const LoadCaseIDType g_lcidGirder          = 1;
const LoadCaseIDType g_lcidStraightStrand  = 2;
const LoadCaseIDType g_lcidHarpedStrand    = 3;
const LoadCaseIDType g_lcidTemporaryStrand = 4;

#define TOP 0
#define BOT 1

CAnalysisAgentImp::CAnalysisAgentImp()
{
   m_Level = 0;
}

UINT CAnalysisAgentImp::DeleteSegmentModelManager(LPVOID pParam)
{
   WATCH(_T("Begin: DeleteSegmentModelManager"));

   CSegmentModelManager* pModelMgr = (CSegmentModelManager*)(pParam);
   pModelMgr->Clear();
   delete pModelMgr;

   WATCH(_T("End: DeleteSegmentModelManager"));

   return 0; // success... returning terminates the thread
}

UINT CAnalysisAgentImp::DeleteGirderModelManager(LPVOID pParam)
{
   WATCH(_T("Begin: DeleteGirderModelManager"));

   CGirderModelManager* pModelMgr = (CGirderModelManager*)(pParam);
   pModelMgr->Clear();
   delete pModelMgr;

   WATCH(_T("End: DeleteGirderModelManager"));

   return 0; // success... returning terminates the thread
}

HRESULT CAnalysisAgentImp::FinalConstruct()
{
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisAgent
void CAnalysisAgentImp::Invalidate(bool clearStatus)
{
   // use a worker thread to delete the models
   CSegmentModelManager* pOldSegmentModels = m_pSegmentModelManager.release();
   m_pSegmentModelManager = std::auto_ptr<CSegmentModelManager>(new CSegmentModelManager(LOGGER,m_pBroker));

   CGirderModelManager* pOldGirderModels = m_pGirderModelManager.release();
   m_pGirderModelManager = std::auto_ptr<CGirderModelManager>(new CGirderModelManager(LOGGER,m_pBroker,m_StatusGroupID));

#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CAnalysisAgentImp::DeleteSegmentModelManager,(LPVOID)(pOldSegmentModels));
   m_ThreadManager.CreateThread(CAnalysisAgentImp::DeleteGirderModelManager, (LPVOID)(pOldGirderModels));
#else
   CAnalysisAgentImp::DeleteSegmentModelManager((LPVOID)pOldSegmentModels);
   CAnalysisAgentImp::DeleteGirderModelManager((LPVOID)pOldGirderModels);
#endif

   InvalidateCamberModels();

   for ( int i = 0; i < 6; i++ )
   {
      m_CreepCoefficientDetails[CREEP_MINTIME][i].clear();
      m_CreepCoefficientDetails[CREEP_MAXTIME][i].clear();
   }

   if (clearStatus)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByStatusGroupID(m_StatusGroupID);
   }
}

void CAnalysisAgentImp::InvalidateCamberModels()
{
   m_PrestressDeflectionModels.clear();
   m_InitialTempPrestressDeflectionModels.clear();
   m_ReleaseTempPrestressDeflectionModels.clear();
}

void CAnalysisAgentImp::ValidateCamberModels(const CSegmentKey& segmentKey)
{
   GDRCONFIG dummy_config;

   CamberModelData camber_model_data;
   BuildCamberModel(segmentKey,false,dummy_config,&camber_model_data);

   m_PrestressDeflectionModels.insert( std::make_pair(segmentKey,camber_model_data) );

   CamberModelData initial_temp_beam;
   CamberModelData release_temp_beam;
   BuildTempCamberModel(segmentKey,false,dummy_config,&initial_temp_beam,&release_temp_beam);
   m_InitialTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,initial_temp_beam) );
   m_ReleaseTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,release_temp_beam) );
}

CAnalysisAgentImp::CamberModelData CAnalysisAgentImp::GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models)
{
   CamberModels::iterator found;
   found = models.find( segmentKey );

   if ( found == models.end() )
   {
      ValidateCamberModels(segmentKey);
      found = models.find( segmentKey );
   }

   // Model should have already been created in ValidateCamberModels
   ATLASSERT( found != models.end() );

   return (*found).second;
}

std::vector<EquivPretensionLoad> CAnalysisAgentImp::GetEquivPretensionLoads(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,pgsTypes::StrandType strandType,bool bTempStrandInstallation)
{
   std::vector<EquivPretensionLoad> equivLoads;

   Float64 Ms;    // Concentrated moments at straight strand debond location
   Float64 Msl;   // Concentrated moments at straight strand bond locations (left)
   Float64 Msr;   // Concentrated moments at straight strand bond locations (right)
   Float64 Mhl;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (left)
   Float64 Mhr;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (right)
   Float64 Mtl;   // Concentrated moments at temporary straight strand bond locations (left)
   Float64 Mtr;   // Concentrated moments at temporary straight strand bond locations (right)
   Float64 Nl;    // Vertical loads at left harping point
   Float64 Nr;    // Vertical loads at right harping point
   Float64 Ps;    // Force in straight strands (varies with location due to debonding)
   Float64 PsStart;  // Force in straight strands (varies with location due to debonding)
   Float64 PsEnd;    // Force in straight strands (varies with location due to debonding)
   Float64 Ph;    // Force in harped strands
   Float64 Pt;    // Force in temporary strands
   Float64 ecc_harped_start; // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_end;  // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_hp1;  // Eccentricity of harped strand at harping point (left)
   Float64 ecc_harped_hp2;  // Eccentricity of harped strand at harping point (right)
   Float64 ecc_straight_start;  // Eccentricity of straight strands (left)
   Float64 ecc_straight_end;    // Eccentricity of straight strands (right)
   Float64 ecc_straight_debond; // Eccentricity of straight strands (location varies)
   Float64 ecc_temporary_start; // Eccentricity of temporary strands (left)
   Float64 ecc_temporary_end;   // Eccentricity of temporary strands (right)
   Float64 hp1; // Location of left harping point
   Float64 hp2; // Location of right harping point
   Float64 Ls;  // Length of segment

   // These are the interfaces we will be using
   GET_IFACE_NOCHECK(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);


   Ls = pBridge->GetSegmentLength(segmentKey);

   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_0L | POI_5L | POI_10L) );
   ATLASSERT( vPoi.size() == 3 );
   pgsPointOfInterest poiStart(vPoi[0]);
   pgsPointOfInterest poiMiddle(vPoi[1]);
   pgsPointOfInterest poiEnd(vPoi[2]);

#if defined _DEBUG
   ATLASSERT( poiMiddle.IsMidSpan(POI_RELEASED_SEGMENT) == true );
#endif

   StrandIndexType Ns, Nh, Nt;
   if ( bUseConfig )
   {
      Ns = config.PrestressConfig.GetStrandCount(pgsTypes::Straight);
      Nh = config.PrestressConfig.GetStrandCount(pgsTypes::Harped);
      Nt = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);
   }
   else
   {
      Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
      Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
      Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
   }


   if ( strandType == pgsTypes::Harped && 0 < Nh )
   {
      hp1 = 0;
      hp2 = 0;
      Nl  = 0;
      Nr  = 0;
      Mhl = 0;
      Mhr = 0;

      // Determine the prestress force
      GET_IFACE(IPretensionForce,pPrestressForce);
      if ( bUseConfig )
      {
         Ph = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Harped,releaseIntervalIdx,pgsTypes::End,config);
      }
      else
      {
         Ph = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Harped,releaseIntervalIdx,pgsTypes::End);
      }

      // get harping point locations
      vPoi.clear(); // recycle the vector
      vPoi = pIPoi->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
      ATLASSERT( 0 <= vPoi.size() && vPoi.size() < 3 );
      pgsPointOfInterest hp1_poi;
      pgsPointOfInterest hp2_poi;
      if ( vPoi.size() == 0 )
      {
         hp1_poi.SetSegmentKey(segmentKey);
         hp1_poi.SetDistFromStart(0.0);
         hp2_poi.SetSegmentKey(segmentKey);
         hp2_poi.SetDistFromStart(0.0);
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else if ( vPoi.size() == 1 )
      { 
         std::vector<pgsPointOfInterest>::const_iterator iter( vPoi.begin() );
         hp1_poi = *iter++;
         hp2_poi = hp1_poi;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else
      {
         std::vector<pgsPointOfInterest>::const_iterator iter( vPoi.begin() );
         hp1_poi = *iter++;
         hp2_poi = *iter++;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }

      // Determine eccentricity of harped strands at end and harp point
      // (assumes eccentricities are the same at each harp point - which they are because
      // of the way the input is defined)
      Float64 nHs_effective;

      if ( bUseConfig )
      {
         ecc_harped_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, config, pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp1_poi,  config, pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp2_poi,  config, pgsTypes::Harped, &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   config, pgsTypes::Harped, &nHs_effective);
      }
      else
      {
         ecc_harped_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp1_poi,  pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp2_poi,  pgsTypes::Harped, &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Harped, &nHs_effective);
      }

      // Determine equivalent loads

      // moment
      Mhl = Ph*ecc_harped_start;
      Mhr = Ph*ecc_harped_end;

      // upward force
      Float64 e_prime_start, e_prime_end;
      e_prime_start = ecc_harped_hp1 - ecc_harped_start;
      e_prime_start = IsZero(e_prime_start) ? 0 : e_prime_start;

      e_prime_end = ecc_harped_hp2 - ecc_harped_end;
      e_prime_end = IsZero(e_prime_end) ? 0 : e_prime_end;

      Nl = IsZero(hp1)    ? 0 : Ph*e_prime_start/hp1;
      Nr = IsZero(Ls-hp2) ? 0 : Ph*e_prime_end/(Ls-hp2);

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P  = Ph;
      startMoment.N  = 0;
      startMoment.M  = Mhl;

      EquivPretensionLoad leftHpLoad;
      leftHpLoad.Xs = hp1;
      leftHpLoad.P  = 0;
      leftHpLoad.N  = Nl;
      leftHpLoad.M  = 0;

      EquivPretensionLoad rightHpLoad;
      rightHpLoad.Xs = hp2;
      rightHpLoad.P  = 0;
      rightHpLoad.N  = Nr;
      rightHpLoad.M  = 0;

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P = -Ph;
      endMoment.N = 0;
      endMoment.M = -Mhr;

      equivLoads.push_back(startMoment);
      equivLoads.push_back(leftHpLoad);
      equivLoads.push_back(rightHpLoad);
      equivLoads.push_back(endMoment);
   }
   else if ( strandType == pgsTypes::Straight && 0 < Ns )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      Float64 nSsEffective;
      if ( bUseConfig )
      {
         Float64 Ps = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,config);
         ecc_straight_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, config, pgsTypes::Straight, &nSsEffective);
         PsStart = Ps*nSsEffective/Ns;

         ecc_straight_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   config, pgsTypes::Straight, &nSsEffective);
         PsEnd = Ps*nSsEffective/Ns;
      }
      else
      {
         Ps = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End);

         ecc_straight_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Straight, &nSsEffective);
         PsStart = Ps*nSsEffective/Ns;

         ecc_straight_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Straight, &nSsEffective);
         PsEnd = Ps*nSsEffective/Ns;
      }

      Msl = PsStart*ecc_straight_start;
      Msr = PsEnd*ecc_straight_end;

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P  = PsStart;
      startMoment.N  = 0;
      startMoment.M  = Msl;

      equivLoads.push_back(startMoment);

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P  = -PsEnd;
      endMoment.N  = 0;
      endMoment.M  = -Msr;

      equivLoads.push_back(endMoment);

      // debonding
      if ( bUseConfig )
      {
         // use tool to extract section data
         CDebondSectionCalculator dbcomp(config.PrestressConfig.Debond[pgsTypes::Straight], Ls);

         // left end first
         Float64 sign = 1;
         SectionIndexType nSections = dbcomp.GetNumLeftSections();
         SectionIndexType sectionIdx = 0;
         for ( sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
         {
            StrandIndexType nDebondedAtSection;
            Float64 location;
            dbcomp.GetLeftSectionInfo(sectionIdx,&location,&nDebondedAtSection);

            // nDebonded is to be interpreted as the number of strands that become bonded at this section
            // (ok, not at this section but lt past this section)
            Float64 nSsEffective;

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(poiMiddle,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,config);
            ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location),config, pgsTypes::Straight, &nSsEffective);

            Ms = sign*Ps*ecc_straight_debond;

            EquivPretensionLoad leftEndMoment;
            leftEndMoment.Xs = location;
            leftEndMoment.P  = Ps;
            leftEndMoment.N  = 0;
            leftEndMoment.M  = Ms;

            equivLoads.push_back(leftEndMoment);
         }

         // right end 
         sign = -1;
         nSections = dbcomp.GetNumRightSections();
         for ( sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
         {
            StrandIndexType nDebondedAtSection;
            Float64 location;
            dbcomp.GetRightSectionInfo(sectionIdx,&location,&nDebondedAtSection);

            Float64 nSsEffective;

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(poiMiddle,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,config);
            ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location),config, pgsTypes::Straight, &nSsEffective);

            Ms = sign*Ps*ecc_straight_debond;

            EquivPretensionLoad rightEndMoment;
            rightEndMoment.Xs = location;
            rightEndMoment.P  = Ps;
            rightEndMoment.N  = 0;
            rightEndMoment.M  = Ms;

            equivLoads.push_back(rightEndMoment);
         }

      }
      else
      {
         for (int end = 0; end < 2; end++)
         {
            // left end first, right second
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)end;
            Float64 sign = (end == 0 ?  1 : -1);
            IndexType nSections = pStrandGeom->GetNumDebondSections(segmentKey,endType,pgsTypes::Straight);
            for ( IndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
            {
               Float64 location = pStrandGeom->GetDebondSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);
               if ( location < 0 || Ls < location )
               {
                  continue; // bond occurs after the end of the segment... skip this one
               }

               StrandIndexType nDebondedAtSection = pStrandGeom->GetNumDebondedStrandsAtSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);

               // nDebonded is to be interperted as the number of strands that start to become bonded at this section
               // (full bonding occurs at lt past this section)
               Float64 PsDebond = nDebondedAtSection*Ps/Ns;

               // using actual poi is faster than using one created on the fly
               pgsPointOfInterest poi = pIPoi->GetPointOfInterest(segmentKey,location); // there should be a poi at the debond point...
               ATLASSERT(poi.GetID() != INVALID_ID);
               ATLASSERT(poi.HasAttribute(POI_DEBOND));

               Float64 nSsEffective;
               ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Straight, &nSsEffective);

               Ms = PsDebond*ecc_straight_debond;

               EquivPretensionLoad debondLocationLoad;
               debondLocationLoad.Xs = location;
               debondLocationLoad.P  = sign*PsDebond;
               debondLocationLoad.N  = 0;
               debondLocationLoad.M  = sign*Ms;

               equivLoads.push_back(debondLocationLoad);
            }
         }
      }
   }
   else if ( strandType == pgsTypes::Temporary && 0 < Nt )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType tsIntervalIdx;
      pgsTypes::IntervalTimeType time;
      if ( bTempStrandInstallation )
      {
         tsIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
         time = pgsTypes::End;
      }
      else
      {
         tsIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
         time = pgsTypes::Start;
      }
      ATLASSERT(tsIntervalIdx != INVALID_INDEX);

      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

      pgsTypes::TTSUsage tempStrandUsage = (bUseConfig ? config.PrestressConfig.TempStrandUsage : pStrands->GetTemporaryStrandUsage());

      Float64 nTsEffective;
      if ( bUseConfig )
      {
         Pt = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Temporary,tsIntervalIdx,time,config);
         ecc_temporary_start = pStrandGeom->GetEccentricity(releaseIntervalIdx,poiStart, config, pgsTypes::Temporary, &nTsEffective);
         ecc_temporary_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx,poiEnd,   config, pgsTypes::Temporary, &nTsEffective);
      }
      else
      {
         Pt = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Temporary,tsIntervalIdx,time);
         ecc_temporary_start = pStrandGeom->GetEccentricity(releaseIntervalIdx,poiStart,pgsTypes::Temporary, &nTsEffective);
         ecc_temporary_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx,poiEnd,  pgsTypes::Temporary, &nTsEffective);
      }

      if ( !bTempStrandInstallation )
      {
         // temp strands are being removed so the force is in the opposite direction
         Pt *= -1.0;
      }

      Pt *= nTsEffective/Nt;

      Mtl = Pt*ecc_temporary_start;
      Mtr = Pt*ecc_temporary_end;

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P  = Pt;
      startMoment.N  = 0;
      startMoment.M  = Mtl;
      equivLoads.push_back(startMoment);

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P  = -Pt;
      endMoment.N  = 0;
      endMoment.M  = -Mtr;

      equivLoads.push_back(endMoment);
   }
   else
   {
      ATLASSERT(strandType != pgsTypes::Permanent); // can't be permanent
   }

   return equivLoads;
}

Float64 g_Ls;
bool RemoveOffSegmentPOI(const pgsPointOfInterest& poi)
{
   return !::InRange(0.0,poi.GetDistFromStart(),g_Ls);
}

void CAnalysisAgentImp::BuildCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pModelData)
{
   Float64 Ls;  // Length of segment

   // These are the interfaces we will be using
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 E;
   if ( bUseConfig )
   {
      if ( config.bUserEci )
      {
         E = config.Eci;
      }
      else
      {
         E = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
      }
   }
   else
   {
      E = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);
   }

   //
   // Create the FEM model (includes girder dead load)
   //
   Ls = pBridge->GetSegmentLength(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( pIPoi->GetPointsOfInterest(segmentKey) );
   g_Ls = Ls;
   vPOI.erase(std::remove_if(vPOI.begin(),vPOI.end(),RemoveOffSegmentPOI),vPOI.end());

   GET_IFACE(ISegmentLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI( pLiftPOI->GetLiftingPointsOfInterest(segmentKey,0) );

   GET_IFACE(ISegmentHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI( pHaulPOI->GetHaulingPointsOfInterest(segmentKey,0) );

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());

   vPOI.erase(std::unique(vPOI.begin(),vPOI.end()), vPOI.end() );

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,releaseIntervalIdx,segmentKey,0.0,Ls,E,g_lcidGirder,false,false,vPOI,&pModelData->Model,&pModelData->PoiMap);

   CComPtr<IFem2dLoadingCollection> loadings;
   pModelData->Model->get_Loadings(&loadings);

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      std::vector<EquivPretensionLoad> vLoads = GetEquivPretensionLoads(segmentKey,bUseConfig,config,strandType);

      CComPtr<IFem2dLoading> loading;
      CComPtr<IFem2dPointLoadCollection> pointLoads;

      LoadCaseIDType lcid;
      if ( strandType == pgsTypes::Straight )
      {
         lcid = g_lcidStraightStrand;
      }
      else if ( strandType == pgsTypes::Harped )
      {
         lcid = g_lcidHarpedStrand;
      }

      loadings->Create(lcid,&loading);
      loading->get_PointLoads(&pointLoads);
      LoadIDType ptLoadID;
      pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

      std::vector<EquivPretensionLoad>::iterator iter(vLoads.begin());
      std::vector<EquivPretensionLoad>::iterator iterEnd(vLoads.end());
      for ( ; iter != iterEnd; iter++ )
      {
         EquivPretensionLoad& equivLoad = *iter;

         CComPtr<IFem2dPointLoad> ptLoad;
         MemberIDType mbrID;
         Float64 x;
         pgsGirderModelFactory::FindMember(pModelData->Model,equivLoad.Xs,&mbrID,&x);
         pointLoads->Create(ptLoadID++,mbrID,x,equivLoad.P,equivLoad.N,equivLoad.M,lotGlobal,&ptLoad);
      }
   }
}

void CAnalysisAgentImp::BuildTempCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData)
{
   std::vector<pgsPointOfInterest> vPOI; // Vector of points of interest

   // These are the interfaces we will be using
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   // Build models
   Float64 L;
   Float64 Ec;
   Float64 Eci;
   L = pBridge->GetSegmentLength(segmentKey);

   if ( bUseConfig )
   {
      if ( config.bUserEci )
      {
         Eci = config.Eci;
      }
      else
      {
         Eci = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
      }

      if ( config.bUserEc )
      {
         Ec = config.Ec;
      }
      else
      {
         Ec = pMaterial->GetEconc(config.Fc,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
      }
   }
   else
   {
      Eci = pMaterial->GetSegmentEc(segmentKey,tsInstallationIntervalIdx);
      Ec  = pMaterial->GetSegmentEc(segmentKey,tsRemovalIntervalIdx);
   }

   vPOI = pIPoi->GetPointsOfInterest(segmentKey);
   vPOI.erase(std::remove_if(vPOI.begin(),vPOI.end(),RemoveOffSegmentPOI),vPOI.end());

   GET_IFACE(ISegmentLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI( pLiftPOI->GetLiftingPointsOfInterest(segmentKey,0) );

   GET_IFACE(ISegmentHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI( pHaulPOI->GetHaulingPointsOfInterest(segmentKey,0) );

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());
   vPOI.erase(std::unique(vPOI.begin(),vPOI.end()), vPOI.end() );

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,tsInstallationIntervalIdx, segmentKey,0.0,L,Eci,g_lcidGirder,false,false,vPOI,&pInitialModelData->Model,&pInitialModelData->PoiMap);
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,tsRemovalIntervalIdx,      segmentKey,0.0,L,Ec, g_lcidGirder,false,false,vPOI,&pReleaseModelData->Model,&pReleaseModelData->PoiMap);

   std::vector<EquivPretensionLoad> vInstallationLoads = GetEquivPretensionLoads(segmentKey,bUseConfig,config,pgsTypes::Temporary,true);
   std::vector<EquivPretensionLoad> vRemovalLoads = GetEquivPretensionLoads(segmentKey,bUseConfig,config,pgsTypes::Temporary,false);

   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   pInitialModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidTemporaryStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);
   LoadIDType ptLoadID;
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   BOOST_FOREACH(EquivPretensionLoad& equivLoad,vInstallationLoads)
   {
      CComPtr<IFem2dPointLoad> ptLoad;
      MemberIDType mbrID;
      Float64 x;
      pgsGirderModelFactory::FindMember(pInitialModelData->Model,equivLoad.Xs,&mbrID,&x);
      pointLoads->Create(ptLoadID++,mbrID,x,equivLoad.P,equivLoad.N,equivLoad.M,lotGlobal,&ptLoad);
   }


   loadings.Release();
   loading.Release();
   pReleaseModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidTemporaryStrand,&loading);

   pointLoads.Release();
   loading->get_PointLoads(&pointLoads);
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   BOOST_FOREACH(EquivPretensionLoad& equivLoad,vRemovalLoads)
   {
      CComPtr<IFem2dPointLoad> ptLoad;
      MemberIDType mbrID;
      Float64 x;
      pgsGirderModelFactory::FindMember(pReleaseModelData->Model,equivLoad.Xs,&mbrID,&x);
      pointLoads->Create(ptLoadID++,mbrID,x,equivLoad.P,equivLoad.N,equivLoad.M,lotGlobal,&ptLoad);
   }
}

/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CAnalysisAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IProductLoads,             this );
   pBrokerInit->RegInterface( IID_IProductForces,            this );
   pBrokerInit->RegInterface( IID_IProductForces2,           this );
   pBrokerInit->RegInterface( IID_ICombinedForces,           this );
   pBrokerInit->RegInterface( IID_ICombinedForces2,          this );
   pBrokerInit->RegInterface( IID_ILimitStateForces,         this );
   pBrokerInit->RegInterface( IID_ILimitStateForces2,        this );
   pBrokerInit->RegInterface( IID_IExternalLoading,          this );
   pBrokerInit->RegInterface( IID_IPretensionStresses,       this );
   pBrokerInit->RegInterface( IID_ICamber,                   this );
   pBrokerInit->RegInterface( IID_IContraflexurePoints,      this );
   pBrokerInit->RegInterface( IID_IContinuity,               this );
   pBrokerInit->RegInterface( IID_IBearingDesign,            this );
   pBrokerInit->RegInterface( IID_IPrecompressedTensileZone, this );
   pBrokerInit->RegInterface( IID_IReactions,                this );

   return S_OK;
};

STDMETHODIMP CAnalysisAgentImp::Init()
{
   CREATE_LOGFILE("AnalysisAgent");

   EAF_AGENT_INIT;

   // Register status callbacks that we want to use
   m_scidVSRatio = pStatusCenter->RegisterCallback(new pgsVSRatioStatusCallback(m_pBroker));

   m_pSegmentModelManager = std::auto_ptr<CSegmentModelManager>(new CSegmentModelManager(LOGGER,m_pBroker));
   m_pGirderModelManager  = std::auto_ptr<CGirderModelManager> (new CGirderModelManager (LOGGER,m_pBroker,m_StatusGroupID));

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CAnalysisAgentImp::Init2()
{
   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the bridge description
   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the specification
   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the rating specification
   hr = pBrokerInit->FindConnectionPoint( IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwRatingSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the load modifiers
   hr = pBrokerInit->FindConnectionPoint( IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLoadModifierCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the loss parameters
   hr = pBrokerInit->FindConnectionPoint( IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_AnalysisAgent;
   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::Reset()
{
   LOG("Reset");
   Invalidate(true);
   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::ShutDown()
{
   LOG("AnalysisAgent Log Closed");

   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwRatingSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLoadModifierCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;

   return S_OK;
}

void CAnalysisAgentImp::GetSidewalkLoadFraction(const CSegmentKey& segmentKey,Float64* pSidewalkLoad,Float64* pFraLeft,Float64* pFraRight)
{
   m_pGirderModelManager->GetSidewalkLoadFraction(segmentKey,pSidewalkLoad,pFraLeft,pFraRight);
}

void CAnalysisAgentImp::GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,Float64* pFraExtLeft, Float64* pFraIntLeft,Float64* pFraExtRight,Float64* pFraIntRight)
{
   m_pGirderModelManager->GetTrafficBarrierLoadFraction(segmentKey,pBarrierLoad,pFraExtLeft,pFraIntLeft,pFraExtRight,pFraIntRight);
}


PoiIDType CAnalysisAgentImp::AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi)
{
   PoiIDType femPoiID = pgsGirderModelFactory::AddPointOfInterest(models.Model,poi);
   models.PoiMap.AddMap( poi, femPoiID );
   return femPoiID;
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces
//
pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization)
{
   pgsTypes::BridgeAnalysisType bat;
   if ( analysisType == pgsTypes::Simple )
   {
      bat = pgsTypes::SimpleSpan;
   }
   else if ( analysisType == pgsTypes::Continuous )
   {
      bat = pgsTypes::ContinuousSpan;
   }
   else
   {
      if ( optimization == pgsTypes::Maximize )
      {
         bat = pgsTypes::MaxSimpleContinuousEnvelope;
      }
      else
      {
         bat = pgsTypes::MinSimpleContinuousEnvelope;
      }
   }

   return bat;
}

pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::OptimizationType optimization)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   return GetBridgeAnalysisType(analysisType,optimization);
}


Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,pfType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,bIncludeElevationAdjustment);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,bIncludeSlopeAdjustment);
   ATLASSERT(results.size() == 1);
   return results[0];
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

////////////////////////////////////////////////////////////////////////////////////////////
// IProductLoads
LPCTSTR CAnalysisAgentImp::GetProductLoadName(pgsTypes::ProductForceType pfType)
{
   // these are the names that are displayed to the user in the UI and reports
   // this must be in the same order as the pgsTypes::ProductForceType enum
   static LPCTSTR strNames[] = 
   {
      _T("Girder"),
      _T("Construction"),
      _T("Slab"),
      _T("Slab Pad"),
      _T("Slab Panel"),
      _T("Diaphragm"),
      _T("Overlay"),
      _T("Sidewalk"),
      _T("Railing System"),
      _T("User DC"),
      _T("User DW"),
      _T("User LLIM"),
      _T("Shear Key"),
      _T("Pre-tensioning"),
      _T("Post-tensioning"),
      _T("Secondary Effects"),
      _T("Creep"),
      _T("Shrinkage"),
      _T("Relaxation"),
      _T("Total Post-tensioning"),
      _T("Overlay (rating)")
   };

   // the direct lookup in the array is faster, however if the enum changes (number of values or order of values)
   // it isn't easily detectable... the switch/case below is slower but it can detect errors that result
   // from changing the enum
#if defined _DEBUG
   std::_tstring strName;
   switch(pfType)
   {
   case pgsTypes::pftGirder:
      strName = _T("Girder");
      break;

   case pgsTypes::pftConstruction:
      strName = _T("Construction");
      break;

   case pgsTypes::pftSlab:
      strName = _T("Slab");
      break;

   case pgsTypes::pftSlabPad:
      strName = _T("Slab Pad");
      break;

   case pgsTypes::pftSlabPanel:
      strName = _T("Slab Panel");
      break;

   case pgsTypes::pftDiaphragm:
      strName = _T("Diaphragm");
      break;

   case pgsTypes::pftOverlay:
      strName = _T("Overlay");
      break;

   case pgsTypes::pftSidewalk:
      strName = _T("Sidewalk");
      break;

   case pgsTypes::pftTrafficBarrier:
      strName = _T("Railing System");
      break;

   case pgsTypes::pftUserDC:
      strName = _T("User DC");
      break;

   case pgsTypes::pftUserDW:
      strName = _T("User DW");
      break;

   case pgsTypes::pftUserLLIM:
      strName = _T("User LLIM");
      break;

   case pgsTypes::pftShearKey:
      strName = _T("Shear Key");
      break;

   case pgsTypes::pftPretension:
      strName = _T("Pre-tensioning");
      break;

   case pgsTypes::pftPostTensioning:
      strName = _T("Post-tensioning");
      break;

   case pgsTypes::pftSecondaryEffects:
      strName = _T("Secondary Effects");
      break;

   case pgsTypes::pftCreep:
      strName = _T("Creep");
      break;

   case pgsTypes::pftShrinkage:
      strName = _T("Shrinkage");
      break;

   case pgsTypes::pftRelaxation:
      strName = _T("Relaxation");
      break;

   case pgsTypes::pftOverlayRating:
      strName = _T("Overlay (rating)");
      break;

   default:
      ATLASSERT(false); // is there a new type?
      strName = _T("");
      break;
   }

   ATLASSERT(strName == std::_tstring(strNames[pfType]));
#endif

   return strNames[pfType];
}

LPCTSTR CAnalysisAgentImp::GetLoadCombinationName(LoadingCombinationType loadCombo)
{
   // these are the names that are displayed to the user in the UI and reports
   // this must be in the same order as the LoadingCombinationType enum
   static LPCTSTR strNames[] =
   {
   _T("DC"), 
   _T("DW"), 
   _T("DWRating"), 
   _T("DWp"), 
   _T("DWf"), 
   _T("CR"), 
   _T("SH"), 
   _T("RE"), 
   _T("PS")
   };

   // the direct lookup in the array is faster, however if the enum changes (number of values or order of values)
   // it isn't easily detectable... the switch/case below is slower but it can detect errors that result
   // from changing the enum
#if defined _DEBUG
   std::_tstring strName;
   switch(loadCombo)
   {
   case lcDC:
      strName = _T("DC");
      break; 
   
   case lcDW:
      strName = _T("DW");
      break;

   case lcDWRating:
      strName = _T("DWRating");
      break;

   case lcDWp:
      strName = _T("DWp");
      break;

   case lcDWf:
      strName = _T("DWf");
      break;

   case lcCR:
      strName = _T("CR");
      break;

   case lcSH:
      strName = _T("SH");
      break;

   case lcRE:
      strName = _T("RE");
      break;

   case lcPS:
      strName = _T("PS");
      break;

   default:
      ATLASSERT(false);
   }

   ATLASSERT(strName == std::_tstring(strNames[loadCombo]));
#endif

   return strNames[loadCombo];
}

LPCTSTR CAnalysisAgentImp::GetLimitStateName(pgsTypes::LimitState limitState)
{
   return ::GetLimitStateName(limitState); // (must use the scope resolution operator to call the global version of this function.. if not, we recursion)
}

bool CAnalysisAgentImp::ReportAxialResults()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 1; pierIdx < nPiers-1; pierIdx++ )
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
      {
         return true;
      }
   }

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         if ( 0 < pGirder->GetPostTensioning()->GetDuctCount() )
         {
            return true;
         }
      }
   }

   return false;
}

void CAnalysisAgentImp::GetGirderSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad)
{
   m_pGirderModelManager->GetGirderSelfWeightLoad(segmentKey,pDistLoad,pPointLoad);
}

std::vector<EquivPretensionLoad> CAnalysisAgentImp::GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation)
{
   GDRCONFIG dummy;
   return GetEquivPretensionLoads(segmentKey,false,dummy,strandType,bTempStrandInstallation);
}

Float64 CAnalysisAgentImp::GetTrafficBarrierLoad(const CSegmentKey& segmentKey)
{
   return m_pGirderModelManager->GetTrafficBarrierLoad(segmentKey);
}

Float64 CAnalysisAgentImp::GetSidewalkLoad(const CSegmentKey& segmentKey)
{
   return m_pGirderModelManager->GetSidewalkLoad(segmentKey);
}

void CAnalysisAgentImp::GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads)
{
   m_pGirderModelManager->GetOverlayLoad(segmentKey,pOverlayLoads);
}

bool  CAnalysisAgentImp::HasConstructionLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasConstructionLoad(girderKey);
}

void CAnalysisAgentImp::GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads)
{
   m_pGirderModelManager->GetConstructionLoad(segmentKey,pConstructionLoads);
}

void CAnalysisAgentImp::GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads)
{
   m_pGirderModelManager->GetMainSpanSlabLoad(segmentKey,pSlabLoads);
}

bool CAnalysisAgentImp::HasShearKeyLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasShearKeyLoad(girderKey);
}

void CAnalysisAgentImp::GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads)
{
   m_pGirderModelManager->GetShearKeyLoad(segmentKey,pLoads);
}

bool CAnalysisAgentImp::HasPedestrianLoad()
{
   return m_pGirderModelManager->HasPedestrianLoad();
}

bool CAnalysisAgentImp::HasSidewalkLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasSidewalkLoad(girderKey);
}

bool CAnalysisAgentImp::HasPedestrianLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasPedestrianLoad(girderKey);
}

Float64 CAnalysisAgentImp::GetPedestrianLoad(const CSegmentKey& segmentKey)
{
   return m_pGirderModelManager->GetPedestrianLoad(segmentKey);
}

Float64 CAnalysisAgentImp::GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation)
{
   return m_pGirderModelManager->GetPedestrianLoadPerSidewalk(orientation);
}

void CAnalysisAgentImp::GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   m_pGirderModelManager->GetCantileverSlabLoad(segmentKey,pP1,pM1,pP2,pM2);
}

void CAnalysisAgentImp::GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   m_pGirderModelManager->GetCantileverSlabPadLoad(segmentKey,pP1,pM1,pP2,pM2);
}

void CAnalysisAgentImp::GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads)
{
   m_pGirderModelManager->GetPrecastDiaphragmLoads(segmentKey,pLoads);
}

void CAnalysisAgentImp::GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads)
{
   m_pGirderModelManager->GetIntermediateDiaphragmLoads(spanKey,pLoads);
}

void CAnalysisAgentImp::GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead)
{
   m_pGirderModelManager->GetPierDiaphragmLoads(pierIdx,gdrIdx,pPback,pMback,pPahead,pMahead);
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc)
{
   // NOTE: This function assumes that deflection due to girder self-weight is the only loading
   // that goes into the camber calculations
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // we want the largest downward deflection. With (+) being up and (-) being down
   // we want the minimum (most negative) deflection
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   // The initial camber occurs while the girder is sitting in storage
   // get the deflection while in storage
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   *pDyStorage = GetDeflection(storageIntervalIdx,pgsTypes::pftGirder,poi,bat,rtCumulative,false);
   *pRzStorage = GetRotation(storageIntervalIdx,pgsTypes::pftGirder,poi,bat,rtCumulative,false);

   // This is the girder deflection after it has been erected
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   *pDyErected = GetDeflection(erectionIntervalIdx,pgsTypes::pftGirder,poi,bat,rtCumulative,false);
   *pRzErected = GetRotation(erectionIntervalIdx,pgsTypes::pftGirder,poi,bat,rtCumulative,false);

   // If the storage support locations and the support locations after the girder is erected aren't at the same
   // location there is an incremental change in moment which causes an additional camber increment
   //
   // We must use the actual loading, "Girder_Incremental", to get the incremental deformation due to the
   // change in support location. Using the product load type pftGirder gives use both the incremental deformation
   // and a deflection adjustment because of the change in the deformation measurement datum. During storage
   // deformations are measured relative to the storage supports and after erection, deformations are measured
   // relative to the final bearing locations. This translation between measurement datums is not subject to
   // creep and camber.
   *pDyInc = GetDeflection(erectionIntervalIdx,_T("Girder_Incremental"),poi,bat,rtCumulative,false);
   *pRzInc = GetRotation(erectionIntervalIdx,_T("Girder_Incremental"),poi,bat,rtCumulative,false);
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi)
{
   Float64 dyStorage,rzStorage;
   Float64 dyErected,rzErected;
   Float64 dy_inc,rz_inc;
   GetGirderDeflectionForCamber(poi,&dyStorage,&rzStorage,&dyErected,&rzErected,&dy_inc,&rz_inc);
   return dyStorage; // by definition we want deflection during storage
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dyStorage,rzStorage;
   Float64 dyErected,rzErected;
   Float64 dy_inc,rz_inc;
   GetGirderDeflectionForCamber(poi,config,&dyStorage,&rzStorage,&dyErected,&rzErected,&dy_inc,&rz_inc);
   return dyStorage; // by definition we want deflection during storage
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc)
{
   // this is the deflection during storage based on the concrete properties input by the user
   Float64 dStorage, rStorage;
   Float64 dErected, rErected;
   Float64 d_inc, r_inc;
   GetGirderDeflectionForCamber(poi,&dStorage,&rStorage,&dErected,&rErected,&d_inc,&r_inc);

   ATLASSERT(IsEqual(dStorage+d_inc,dErected));
   //ATLASSERT(IsEqual(rStorage+r_inc,rErected));

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // we need to adjust the deflections for the concrete properties in config
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);

   // get Eci used to compute delta and rotation
   Float64 Eci_original = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);

   // get Eci for the concrete properties in the config object
   Float64 Eci_config;
   if ( config.bUserEci )
   {
      Eci_config = config.Eci;
   }
   else
   {
      Eci_config = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                  pMaterial->GetSegmentEccK1(segmentKey),
                                                  pMaterial->GetSegmentEccK2(segmentKey));
   }

   // adjust the deflections
   // deltaOriginal = K/Ix*Eoriginal
   // deltaConfig   = K/Ix*Econfig = (K/Ix*Eoriginal)(Eoriginal/Econfig) = deltaOriginal*(Eoriginal/Econfig)
   dStorage *= (Eci_original/Eci_config);
   rStorage *= (Eci_original/Eci_config);

   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   // get Ec used to compute delta and rotation
   Float64 Ec_original = pMaterial->GetSegmentEc(segmentKey,erectionIntervalIdx);

   // get Ec for the concrete properties in the config object
   Float64 Ec_config;
   if ( config.bUserEc )
   {
      Ec_config = config.Ec;
   }
   else
   {
      Ec_config = pMaterial->GetEconc(config.Fc,pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                  pMaterial->GetSegmentEccK1(segmentKey),
                                                  pMaterial->GetSegmentEccK2(segmentKey));
   }

   d_inc *= (Ec_original/Ec_config);
   r_inc *= (Ec_original/Ec_config);

   dErected = dStorage + d_inc;
   rErected = rStorage + r_inc;

   *pDyStorage = dStorage;
   *pRzStorage = rStorage;
   *pDyErected = dErected;
   *pRzErected = rErected;
   *pDyInc = d_inc;
   *pRzInc = r_inc;
}

void CAnalysisAgentImp::GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   m_pGirderModelManager->GetLiveLoadAxial(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   return m_pGirderModelManager->GetLiveLoadShear(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   m_pGirderModelManager->GetLiveLoadMoment(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadDeflection(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   return m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,pierIdx,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,pierIdx,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig,VehicleIndexType* pTopMaxConfig,VehicleIndexType* pBotMinConfig,VehicleIndexType* pBotMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadStress(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pTopMinConfig,pTopMaxConfig,pBotMinConfig,pBotMaxConfig);
}

std::vector<std::_tstring> CAnalysisAgentImp::GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey)
{
   return m_pGirderModelManager->GetVehicleNames(llType,girderKey);
}

void CAnalysisAgentImp::GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadAxial(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,AxleConfiguration* pMinLeftAxleConfig,AxleConfiguration* pMinRightAxleConfig,AxleConfiguration* pMaxLeftAxleConfig,AxleConfiguration* pMaxRightAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinLeftAxleConfig,pMinRightAxleConfig,pMaxLeftAxleConfig,pMaxRightAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop,AxleConfiguration* pMaxAxleConfigTop,AxleConfiguration* pMinAxleConfigBot,AxleConfiguration* pMaxAxleConfigBot)
{
   m_pGirderModelManager->GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pMinAxleConfigTop,pMaxAxleConfigTop,pMinAxleConfigBot,pMaxAxleConfigBot);
}

void CAnalysisAgentImp::GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   m_pGirderModelManager->GetDeflLiveLoadDeflection(type,poi,bat,pDmin,pDmax);
}

Float64 CAnalysisAgentImp::GetDesignSlabMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // returns the difference in moment between the slab moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;
   sysSectionValue M = beam.ComputeMoment(x);

   ATLASSERT( IsEqual(M.Left(),M.Right()) );
   return M.Left();
}

Float64 CAnalysisAgentImp::GetDesignSlabDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDesignSlabDeflectionAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetDesignSlabStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot)
{
   GET_IFACE(ISectionProperties,pSectProp);
   // returns the difference in top and bottom girder stress between the stresses caused by the current slab
   // and the input value.
   Float64 M = GetDesignSlabMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckInterval = pIntervals->GetCastDeckInterval();

   Float64 Sbg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::BottomGirder,fcgdr);
   Float64 Stg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::TopGirder,   fcgdr);

   *pfTop = M/Stg;
   *pfBot = M/Sbg;
}

rkPPPartUniformLoad CAnalysisAgentImp::GetDesignSlabModel(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(ISectionProperties,pSectProp);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Ig = pSectProp->GetIx( castDeckIntervalIdx, poi, fcgdr );

   Float64 E = pMaterial->GetEconc(fcgdr,pMaterial->GetSegmentStrengthDensity(segmentKey), 
                                         pMaterial->GetSegmentEccK1(segmentKey), 
                                         pMaterial->GetSegmentEccK2(segmentKey) );

   Float64 L = pBridge->GetSegmentSpanLength(segmentKey);

   Float64 w;
   bool bIsInteriorGirder = pBridge->IsInteriorGirder(segmentKey);
   if ( bIsInteriorGirder )
   {
      // change in main span loading only occurs for exterior girders
      w = 0;
   }
   else
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
      if ( pDeck->DeckType == pgsTypes::sdtNone || pDeck->OverhangTaper == pgsTypes::dotNone )
      {
         // there isn't a deck, or the overhang doesn't taper, so there isn't
         // a change in loading
         w = 0;
      }
      else
      {
         ATLASSERT( pDeck->OverhangTaper == pgsTypes::dotTopTopFlange || pDeck->OverhangTaper == pgsTypes::dotBottomTopFlange );
         // There is a triangular shape piece of deck concrete that isn't accounted for
         // in the main slab loading based on the input value because the "A" dimension has changed.
         // The slab overhang tapers to either the top or bottom of the top flange and the
         // distance to the top/bottom of the top flange depends on "A"

         // Compute the change in loading due to the change in "A" dimension

         PierIndexType startPierIdx, endPierIdx;
         pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
         ATLASSERT(endPierIdx == startPierIdx+1);

         // current value of "A" dimension
         Float64 AoStart = pBridge->GetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex);
         Float64 AoEnd   = pBridge->GetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex);

         // change in overhang depth at the flange tip at the start/end of the girder
         // due to the design "A" dimension
         Float64 delta_overhang_depth_at_flange_tip_start = startSlabOffset - AoStart;
         Float64 delta_overhang_depth_at_flange_tip_end   = endSlabOffset   - AoEnd;

         // change in flange overhang depth at this poi
         // assuming a linear variation between start and end of span
         Float64 delta_overhang_depth_at_flange_tip = ::LinInterp(poi.GetDistFromStart(),delta_overhang_depth_at_flange_tip_start,delta_overhang_depth_at_flange_tip_end,L);

         // Determine the slab overhang at this poi
         Float64 station,offset;
         pBridge->GetStationAndOffset(poi,&station,&offset);
      
         GET_IFACE(IPointOfInterest,pPoi);
         Float64 Xb = pPoi->ConvertRouteToBridgeLineCoordinate(station);

         // slab overhang from CL of girder (normal to alignment)
         Float64 slab_overhang = (segmentKey.girderIndex == 0 ? pBridge->GetLeftSlabOverhang(Xb) : pBridge->GetRightSlabOverhang(Xb));

         if (slab_overhang < 0.0)
         {
            // negative overhang - girder probably has no slab over it
            slab_overhang = 0.0;
         }
         else
         {
            GET_IFACE(IGirder,pGdr);
            Float64 top_width = pGdr->GetTopWidth(poi);

            // slab overhang from edge of girder (normal to alignment)
            slab_overhang -= top_width/2;
         }

         // cross sectional area of the missing dead load
         Float64 delta_slab_overhang_area = slab_overhang*delta_overhang_depth_at_flange_tip/2;

         Float64 density = pMaterial->GetDeckWeightDensity(castDeckIntervalIdx) ;

         w = delta_slab_overhang_area*density*unitSysUnitsMgr::GetGravitationalAcceleration();
      }
   }

#pragma Reminder("UPDATE: This is incorrect if girders are made continuous before slab casting")
#pragma Reminder("UPDATE: Should be using a trapezoidal load.")
   rkPPPartUniformLoad beam(0,L,-w,L,E,Ig);
   return beam;
}

Float64 CAnalysisAgentImp::GetDesignSlabPadMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // returns the difference in moment between the slab pad moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;
   sysSectionValue M = beam.ComputeMoment(x);

   ATLASSERT( IsEqual(M.Left(),M.Right()) );
   return M.Left();
}

Float64 CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDesignSlabPadDeflectionAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetDesignSlabPadStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot)
{
   GET_IFACE(ISectionProperties,pSectProp);
   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   Float64 M = GetDesignSlabPadMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckInterval = pIntervals->GetCastDeckInterval();

   Float64 Sbg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::BottomGirder,fcgdr);
   Float64 Stg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::TopGirder,   fcgdr);

   *pfTop = M/Stg;
   *pfBot = M/Sbg;
}

rkPPPartUniformLoad CAnalysisAgentImp::GetDesignSlabPadModel(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // Creates a simple beam analysis object. The loading is the change in slab pad weight due to the difference
   // in the slab pad weight from the user input and the slab pad weight due to the "A" dimension being
   // used in a design trial.
   //
   // The slab offset parameters passed into this method are the trial values.
   //
   // For example, the moment in the girder due to slab pad dead load during design is the moment due to the
   // original slab pad, minus the change in moment due to a smaller or larger slab pad. Instead of getting
   // change in moments, we model a change in loading.
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectionProperties,pSectProp);

   // Design "A" dimensions
   Float64 AdStart = startSlabOffset;
   Float64 AdEnd   = endSlabOffset;

   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
   ATLASSERT(endPierIdx == startPierIdx+1);

   // Original "A" dimensions
   Float64 AoStart = pBridge->GetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex);
   Float64 AoEnd   = pBridge->GetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex);

   Float64 top_flange_width = pGdr->GetTopFlangeWidth( poi );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Ig = pSectProp->GetIx( castDeckIntervalIdx, poi, fcgdr );
   
   Float64 E = pMaterial->GetEconc(fcgdr,pMaterial->GetSegmentStrengthDensity(segmentKey), 
                                         pMaterial->GetSegmentEccK1(segmentKey), 
                                         pMaterial->GetSegmentEccK2(segmentKey) );

   Float64 L = pBridge->GetSegmentSpanLength(segmentKey);

#pragma Reminder("NOTE: This is incorrect if girders are made continuous before slab casting")
   // a simple span model is being created. if the girders are continuous before slab casting
   // the change in load due to the change in "A" dimension should be applied to a continuous model

   // Get change in "A" dimension at this poi
   Float64 density = pMaterial->GetDeckWeightDensity(castDeckIntervalIdx) ;
   Float64 deltaA = ::LinInterp(poi.GetDistFromStart(),AdStart-AoStart,AdEnd-AoEnd,L);
   Float64 w = deltaA * top_flange_width * density * unitSysUnitsMgr::GetGravitationalAcceleration();

#pragma Reminder("NOTE: Should be using a trapezoidal load.")

   rkPPPartUniformLoad beam(0,L,-w,L,E,Ig);
   return beam;
}

void CAnalysisAgentImp::DumpAnalysisModels(GirderIndexType gdrIdx)
{
   m_pSegmentModelManager->DumpAnalysisModels(gdrIdx);
   m_pGirderModelManager->DumpAnalysisModels(gdrIdx);
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot)
{
   m_pGirderModelManager->GetDeckShrinkageStresses(poi,pftop,pfbot);
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr,Float64* pftop,Float64* pfbot)
{
   m_pGirderModelManager->GetDeckShrinkageStresses(poi,fcGdr,pftop,pfbot);
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepPrestressAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   ATLASSERT(pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects);
   ATLASSERT(bat == pgsTypes::ContinuousSpan);

   std::vector<Float64> P;
   P.reserve(vPoi.size());


   GET_IFACE(ILosses,pILosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      const LOSSDETAILS* pLossDetails = pILosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

      Float64 dP = 0;
      if ( resultsType == rtIncremental )
      {
         dP = tsDetails.dPi[pfType]; // incremental
      }
      else
      {
         dP = tsDetails.Pi[pfType]; // cumulative
      }
      P.push_back(dP);
   }

   return P;
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepPrestressMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   ATLASSERT(pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects);
   ATLASSERT(bat == pgsTypes::ContinuousSpan);

   std::vector<Float64> M;
   M.reserve(vPoi.size());


   GET_IFACE(ILosses,pILosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      const LOSSDETAILS* pLossDetails = pILosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

      Float64 dM = 0;
      if ( resultsType == rtIncremental )
      {
         dM = tsDetails.dMi[pfType]; // incremental
      }
      else
      {
         dM = tsDetails.Mi[pfType]; // cumulative
      }
      M.push_back(dM);
   }

   return M;
}

void CAnalysisAgentImp::ApplyElevationAdjustment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2)
{
   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64> vAdjustments;
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Float64 elevAdj = pBridge->GetElevationAdjustment(intervalIdx,poi);
      vAdjustments.push_back(elevAdj);
   }

   if ( pDeflection1 )
   {
      std::transform(pDeflection1->begin(),pDeflection1->end(),vAdjustments.begin(),pDeflection1->begin(),std::plus<Float64>());
   }

   if ( pDeflection2 )
   {
      std::transform(pDeflection2->begin(),pDeflection2->end(),vAdjustments.begin(),pDeflection2->begin(),std::plus<Float64>());
   }
}

void CAnalysisAgentImp::ApplyRotationAdjustment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2)
{
   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64> vAdjustments;
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Float64 slopeAdj = pBridge->GetRotationAdjustment(intervalIdx,poi);
      vAdjustments.push_back(slopeAdj);
   }

   if ( pDeflection1 )
   {
      std::transform(pDeflection1->begin(),pDeflection1->end(),vAdjustments.begin(),pDeflection1->begin(),std::plus<Float64>());
   }

   if ( pDeflection2 )
   {
      std::transform(pDeflection2->begin(),pDeflection2->end(),vAdjustments.begin(),pDeflection2->begin(),std::plus<Float64>());
   }
}

std::_tstring CAnalysisAgentImp::GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx)
{
   return m_pGirderModelManager->GetLiveLoadName(llType,vehicleIdx);
}

pgsTypes::LiveLoadApplicabilityType CAnalysisAgentImp::GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx)
{
   return m_pGirderModelManager->GetLiveLoadApplicability(llType,vehicleIdx);
}

VehicleIndexType CAnalysisAgentImp::GetVehicleCount(pgsTypes::LiveLoadType llType)
{
   return m_pGirderModelManager->GetVehicleCount(llType);
}

Float64 CAnalysisAgentImp::GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx)
{
   return m_pGirderModelManager->GetVehicleWeight(llType,vehicleIdx);
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces2
//
std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;

   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         return GetTimeStepPrestressAxial(intervalIdx,pfType,vPoi,bat,resultsType);
      }
      else
      {
#if defined _DEBUG
         GET_IFACE(IPointOfInterest,pPoi);
         std::vector<CSegmentKey> vSegmentKeys(pPoi->GetSegmentKeys(vPoi));
         ATLASSERT(vSegmentKeys.size() == 1); // this method assumes all the poi are for the same segment
#endif
         // This is not time-step analysis.
         // The pretension effects are handled in the segment and girder models for
         // elastic analysis... we want to use the code further down in this method.
         if ( pfType == pgsTypes::pftPretension )
         {
            // for elastic analysis, force effects due to pretensioning are always those at release
            if ( resultsType == rtCumulative )
            {
               CSegmentKey segmentKey = vPoi.front().GetSegmentKey();
               GET_IFACE(IIntervals,pIntervals);
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
               intervalIdx = releaseIntervalIdx;
            }
         }

         // Post-tensioning is not modeled for linear elastic analysis, so the
         // results are zero
         if ( pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects )
         {
            results.resize(vPoi.size(),0.0);
            return results;
         }
      }
   }

   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
         
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<Float64> fx = GetAxial(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),fx.begin(),results.begin(),std::plus<Float64>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetAxial(intervalIdx,pfType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Aprev = GetAxial(intervalIdx-1,pfType,vPoi,bat,rtCumulative);
         std::vector<Float64> Athis = GetAxial(intervalIdx,  pfType,vPoi,bat,rtCumulative);
         std::transform(Athis.begin(),Athis.end(),Aprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,pfType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<sysSectionValue> results;

   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      // pre and post-tensioning don't cause external shear forces.
      // There is a vertical component of prestress, Vp, that is used
      // in shear analysis, but that isn't this...
      results.resize(vPoi.size(),sysSectionValue(0,0));
      return results;
   }


   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
         
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),sysSectionValue(0,0));
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<sysSectionValue> fy = GetShear(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),fy.begin(),results.begin(),std::plus<sysSectionValue>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),sysSectionValue(0,0));
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),sysSectionValue(0,0));
      }
      return results;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetShear(intervalIdx,pfType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<sysSectionValue> Vprev = GetShear(intervalIdx-1,pfType,vPoi,bat,rtCumulative);
         std::vector<sysSectionValue> Vthis = GetShear(intervalIdx,  pfType,vPoi,bat,rtCumulative);
         std::transform(Vthis.begin(),Vthis.end(),Vprev.begin(),std::back_inserter(results),std::minus<sysSectionValue>());
      }
      else
      {
         // after erection - results are in the girder models
         results = m_pGirderModelManager->GetShear(intervalIdx,pfType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;

   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         return GetTimeStepPrestressMoment(intervalIdx,pfType,vPoi,bat,resultsType);
      }
      else
      {
#if defined _DEBUG
         //GET_IFACE(IPointOfInterest,pPoi);
         //std::vector<CSegmentKey> vSegmentKeys(pPoi->GetSegmentKeys(vPoi));
         //ATLASSERT(vSegmentKeys.size() == 1); // this method assumes all the poi are for the same segment
#endif
         // This is not time-step analysis.
         // The pretension effects are handled in the segment and girder models for
         // elastic analysis... we want to use the code further down in this method.
         if ( pfType == pgsTypes::pftPretension )
         {
            // for elastic analysis, force effects due to pretensioning are always those at release
            if ( resultsType == rtCumulative )
            {
               CSegmentKey segmentKey = vPoi.front().GetSegmentKey();
               GET_IFACE(IIntervals,pIntervals);
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
               intervalIdx = releaseIntervalIdx;
            }
         }

         // Post-tensioning is not modeled for linear elastic analysis, so the
         // results are zero
         if ( pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects )
         {
            results.resize(vPoi.size(),0.0);
            return results;
         }
      }
   }

   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
      
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<Float64> mz = GetMoment(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),mz.begin(),results.begin(),std::plus<Float64>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetMoment(intervalIdx,pfType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,pfType,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  pfType,vPoi,bat,rtCumulative);
         std::transform(Mthis.begin(),Mthis.end(),Mprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
      }

   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   std::vector<Float64> deflections;
   if ( pfType == pgsTypes::pftSecondaryEffects )
   {
      deflections.resize(vPoi.size(),0.0);
   }
   else if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE(ILosses,pLosses);
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);

      if ( resultsType == rtCumulative )
      {
         deflections.resize(vPoi.size(),0);

         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
         for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(iIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
               std::vector<Float64> dy = GetDeflection(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
               std::transform(deflections.begin(),deflections.end(),dy.begin(),deflections.begin(),std::plus<Float64>());
            }
         }
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         if ( 0 < pIntervals->GetDuration(intervalIdx) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
            deflections = GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType,false);
         }
         else
         {
            deflections.resize(vPoi.size(),0);
         }
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());

         if ( intervalIdx < erectionIntervalIdx )
         {
            // before erection - results are in the segment models
            deflections = m_pSegmentModelManager->GetDeflection(intervalIdx,pfType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,pfType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetDeflection(intervalIdx,  pfType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(deflections),std::minus<Float64>());
         }
         else
         {
            deflections = m_pGirderModelManager->GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&deflections,NULL);
   }

   return deflections;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   std::vector<Float64> rotations;
   if ( pfType == pgsTypes::pftSecondaryEffects )
   {
      rotations.resize(vPoi.size(),0.0);
      return rotations;
   }
   else if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE(ILosses,pLosses);
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
      
      if ( resultsType == rtCumulative )
      {
         rotations.resize(vPoi.size(),0);

         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
         for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(iIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
               std::vector<Float64> rz = GetRotation(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
               std::transform(rotations.begin(),rotations.end(),rz.begin(),rotations.begin(),std::plus<Float64>());
            }
         }
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         if ( 0 < pIntervals->GetDuration(intervalIdx) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
            rotations = GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType,false);
         }
         else
         {
            rotations.resize(vPoi.size(),0);
         }
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
         if ( intervalIdx < erectionIntervalIdx )
         {
            // before erection - results are in the segment models
            rotations = m_pSegmentModelManager->GetRotation(intervalIdx,pfType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Rprev = GetRotation(intervalIdx-1,pfType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Rthis = GetRotation(intervalIdx,  pfType,vPoi,bat,rtCumulative,false);
            std::transform(Rthis.begin(),Rthis.end(),Rprev.begin(),std::back_inserter(rotations),std::minus<Float64>());
         }
         else
         {
            rotations = m_pGirderModelManager->GetRotation(intervalIdx,pfType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&rotations,NULL);
   }

   return rotations;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GetTimeStepStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      GetElasticStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
}

void CAnalysisAgentImp::GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck)
{
   m_pGirderModelManager->GetLiveLoadAxial(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMinTruck,std::vector<VehicleIndexType>* pMaxTruck)
{
   m_pGirderModelManager->GetLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinTruck,pMaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck)
{
   m_pGirderModelManager->GetLiveLoadMoment(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadDeflection(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex,std::vector<VehicleIndexType>* pTopMaxIndex,std::vector<VehicleIndexType>* pBotMinIndex,std::vector<VehicleIndexType>* pBotMaxIndex)
{
   m_pGirderModelManager->GetLiveLoadStress(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pTopMinIndex,pTopMaxIndex,pBotMinIndex,pBotMaxIndex);
}

void CAnalysisAgentImp::GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadAxial(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<AxleConfiguration>* pMinLeftAxleConfig,std::vector<AxleConfiguration>* pMinRightAxleConfig,std::vector<AxleConfiguration>* pMaxLeftAxleConfig,std::vector<AxleConfiguration>* pMaxRightAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinLeftAxleConfig,pMinRightAxleConfig,pMaxLeftAxleConfig,pMaxRightAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop,std::vector<AxleConfiguration>* pMaxAxleConfigTop,std::vector<AxleConfiguration>* pMinAxleConfigBot,std::vector<AxleConfiguration>* pMaxAxleConfigBot)
{
   m_pGirderModelManager->GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pMinAxleConfigTop,pMaxAxleConfigTop,pMinAxleConfigBot,pMaxAxleConfigBot);
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces
//
Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,comboType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> V( GetShear(intervalIdx,comboType,vPoi,bat,resultsType) );

   ATLASSERT(V.size() == 1);

   return V.front();
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,comboType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(intervalIdx,comboType,vPoi,bat,resultsType,bIncludeElevationAdjustment) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,comboType,vPoi,bat,resultsType,bIncludeSlopeAdjustment) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);
   
   ATLASSERT(fTop.size() == 1 && fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Pmin, Pmax;
   GetCombinedLiveLoadAxial(intervalIdx,llType,vPoi,bat,&Pmin,&Pmax);

   ATLASSERT(Pmin.size() == 1 && Pmax.size() == 1);
   *pMin = Pmin.front();
   *pMax = Pmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,&Vmin,&Vmax);

   ATLASSERT( Vmin.size() == 1 && Vmax.size() == 1 );
   *pVmin = Vmin.front();
   *pVmax = Vmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1 && Mmax.size() == 1);
   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT( Dmin.size() == 1 && Dmax.size() == 1 );
   *pDmin = Dmin.front();
   *pDmax = Dmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,&Rmin,&Rmax);

   ATLASSERT( Rmin.size() == 1 && Rmax.size() == 1 );
   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   GetCombinedLiveLoadStress(intervalIdx,llType,vPoi,bat,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax);

   ATLASSERT( fTopMin.size() == 1 && fTopMax.size() == 1 && fBotMin.size() == 1 && fBotMax.size() == 1 );

   *pfTopMin = fTopMin.front();
   *pfTopMax = fTopMax.front();
   *pfBotMin = fBotMin.front();
   *pfBotMax = fBotMax.front();
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces2
//
std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;
   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<Float64> A = GetAxial(intervalIdx,pfType,vPoi,bat,resultsType);

         // add A to results and assign answer to results
         std::transform(A.begin(),A.end(),results.begin(),results.begin(),std::plus<Float64>());
      }

      return results;
   }

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);

         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);

            GET_IFACE(ILosses,pLosses);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<Float64> fx = GetAxial(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),fx.begin(),results.begin(),std::plus<Float64>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx < erectionIntervalIdx )
      {
         results = m_pSegmentModelManager->GetAxial(intervalIdx,comboType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Aprev = GetAxial(intervalIdx-1,comboType,vPoi,bat,rtCumulative);
         std::vector<Float64> Athis = GetAxial(intervalIdx,  comboType,vPoi,bat,rtCumulative);
         std::transform(Athis.begin(),Athis.end(),Aprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<sysSectionValue> results;
   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<sysSectionValue> V = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);

         // add V to results and assign answer to results
         std::transform(V.begin(),V.end(),results.begin(),results.begin(),std::plus<sysSectionValue>());
      }

      return results;
   }

   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
         
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),sysSectionValue(0,0));
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<sysSectionValue> fy = GetShear(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),fy.begin(),results.begin(),std::plus<sysSectionValue>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),sysSectionValue(0,0));
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),sysSectionValue(0,0));
      }
      return results;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         results =  m_pSegmentModelManager->GetShear(intervalIdx,comboType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<sysSectionValue> Vprev = GetShear(intervalIdx-1,comboType,vPoi,bat,rtCumulative);
         std::vector<sysSectionValue> Vthis = GetShear(intervalIdx,  comboType,vPoi,bat,rtCumulative);
         std::transform(Vthis.begin(),Vthis.end(),Vprev.begin(),std::back_inserter(results),std::minus<sysSectionValue>());
      }
      else
      {
         results = m_pGirderModelManager->GetShear(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;
   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<Float64> M = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);

         // add M to results and assign answer to results
         std::transform(M.begin(),M.end(),results.begin(),results.begin(),std::plus<Float64>());
      }

      return results;
   }

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);

         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<Float64> mz = GetMoment(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),mz.begin(),results.begin(),std::plus<Float64>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
      if (intervalIdx < erectionIntervalIdx )
      {
         results = m_pSegmentModelManager->GetMoment(intervalIdx,comboType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,comboType,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  comboType,vPoi,bat,rtCumulative);
         std::transform(Mthis.begin(),Mthis.end(),Mprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<Float64> deflection;
   if ( comboType == lcPS )
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      deflection.resize(vPoi.size(),0.0);
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
      {
         std::vector<Float64> delta = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,false);
         std::transform(delta.begin(),delta.end(),deflection.begin(),deflection.begin(),std::plus<Float64>());
      }
   }
   else if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILosses,pLosses);
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);

      if ( resultsType == rtCumulative )
      {
         deflection.resize(vPoi.size(),0);
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
         for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(iIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
               std::vector<Float64> dy = GetDeflection(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
               std::transform(deflection.begin(),deflection.end(),dy.begin(),deflection.begin(),std::plus<Float64>());
            }
         }
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         if ( 0 < pIntervals->GetDuration(intervalIdx) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
            deflection = GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType,false);
         }
         else
         {
            deflection.resize(vPoi.size(),0.0);
         }
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
         if ( intervalIdx < erectionIntervalIdx )
         {
            deflection = m_pSegmentModelManager->GetDeflection(intervalIdx,comboType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,comboType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetDeflection(intervalIdx,  comboType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(deflection),std::minus<Float64>());
         }
         else
         {
            deflection = m_pGirderModelManager->GetDeflection(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&deflection,NULL);
   }

   return deflection;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<Float64> rotation;
   if ( comboType == lcPS )
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      rotation.resize(vPoi.size(),0.0);
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
      {
         std::vector<Float64> delta = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,false);
         std::transform(delta.begin(),delta.end(),rotation.begin(),rotation.begin(),std::plus<Float64>());
      }
   }
   else if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILosses,pLosses);
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
      
      if ( resultsType == rtCumulative )
      {
         rotation.resize(vPoi.size(),0);
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().GetSegmentKey());
         for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(iIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
               std::vector<Float64> rz = GetRotation(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
               std::transform(rotation.begin(),rotation.end(),rz.begin(),rotation.begin(),std::plus<Float64>());
            }
         }
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         if ( 0 < pIntervals->GetDuration(intervalIdx) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
            rotation = GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType,false);
         }
         else
         {
            rotation.resize(vPoi.size(),0.0);
         }
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
         if ( intervalIdx < erectionIntervalIdx )
         {
            rotation = m_pSegmentModelManager->GetRotation(intervalIdx,comboType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetRotation(intervalIdx-1,comboType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetRotation(intervalIdx,  comboType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(rotation),std::minus<Float64>());
         }
         else
         {
            rotation = m_pGirderModelManager->GetRotation(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&rotation,NULL);
   }

   return rotation;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GetTimeStepStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      GetElasticStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadAxial(intervalIdx,llType,vPoi,bat,pMmin,pMmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,pVmin,pVmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,pMmin,pMmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,pDmin,pDmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,pRmin,pRmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax)
{
   m_pGirderModelManager->GetCombinedLiveLoadStress(intervalIdx,llType,vPoi,bat,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax);
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces
//
void CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Pmin, Pmax;
   GetAxial(intervalIdx,limitState,vPoi,bat,&Pmin,&Pmax);

   ATLASSERT(Pmin.size() == 1);
   ATLASSERT(Pmax.size() == 1);

   *pMin = Pmin.front();
   *pMax = Pmax.front();
}

void CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetShear(intervalIdx,limitState,vPoi,bat,&Vmin,&Vmax);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pMin = Vmin.front();
   *pMax = Vmax.front();
}

void CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetMoment(intervalIdx,limitState,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncPrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetDeflection(intervalIdx,limitState,vPoi,bat,bIncPrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin.front();
   *pMax = Dmax.front();
}

void CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetRotation(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,bIncludeSlopeAdjustment,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin.front();
   *pMax = Dmax.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Fmin, Fmax;
   GetStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,&Fmin,&Fmax);

   ATLASSERT(Fmin.size() == 1);
   ATLASSERT(Fmax.size() == 1);

   *pMin = Fmin.front();
   *pMax = Fmax.front();
}

Float64 CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> M = GetSlabDesignMoment(limitState,vPoi,bat);

   ATLASSERT(M.size() == vPoi.size());

   return M.front();
}

bool CAnalysisAgentImp::IsStrengthIIApplicable(const CGirderKey& girderKey)
{
   // If we have permit truck, we're golden
   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
   if (bPermit)
   {
      return true;
   }
   else
   {
      // last chance is if this girder has pedestrian load and it is turned on for permit states
      ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);
      if (ILiveLoads::PedDontApply != PermitPedLoad)
      {
         return HasPedestrianLoad(girderKey);
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces2
//
void CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
      if (intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetAxial(intervalIdx,limitState,vPoi,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetAxial(intervalIdx,limitState,vPoi,bat,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         gCRMax = pLoadFactors->CRmax[limitState];
         gCRMin = pLoadFactors->CRmin[limitState];
         gSHMax = pLoadFactors->SHmax[limitState];
         gSHMin = pLoadFactors->SHmin[limitState];
         gREMax = pLoadFactors->REmax[limitState];
         gREMin = pLoadFactors->REmin[limitState];
      }

      std::vector<Float64> vPcr = GetAxial(intervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<Float64> vPsh = GetAxial(intervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<Float64> vPre = GetAxial(intervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<Float64> vPcrMin;
         std::transform(vPcr.begin(),vPcr.end(),std::back_inserter(vPcrMin),std::bind1st(std::multiplies<Float64>(),gCRMin));
         std::transform(pMin->begin(),pMin->end(),vPcrMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vPcr.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vPcrMax;
         std::transform(vPcr.begin(),vPcr.end(),std::back_inserter(vPcrMax),std::bind1st(std::multiplies<Float64>(),gCRMax));
         std::transform(pMax->begin(),pMax->end(),vPcrMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vPcr.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<Float64> vPshMin;
         std::transform(vPsh.begin(),vPsh.end(),std::back_inserter(vPshMin),std::bind1st(std::multiplies<Float64>(),gSHMin));
         std::transform(pMin->begin(),pMin->end(),vPshMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vPsh.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vPshMax;
         std::transform(vPsh.begin(),vPsh.end(),std::back_inserter(vPshMax),std::bind1st(std::multiplies<Float64>(),gSHMax));
         std::transform(pMax->begin(),pMax->end(),vPshMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vPsh.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<Float64> vPreMin;
         std::transform(vPre.begin(),vPre.end(),std::back_inserter(vPreMin),std::bind1st(std::multiplies<Float64>(),gREMin));
         std::transform(pMin->begin(),pMin->end(),vPreMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vPre.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vPreMax;
         std::transform(vPre.begin(),vPre.end(),std::back_inserter(vPreMax),std::bind1st(std::multiplies<Float64>(),gREMax));
         std::transform(pMax->begin(),pMax->end(),vPreMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vPre.begin(),pMax->begin(),std::plus<Float64>());
      }
   }
}

void CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetShear(intervalIdx,limitState,vPoi,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetShear(intervalIdx,limitState,vPoi,bat,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         gCRMax = pLoadFactors->CRmax[limitState];
         gCRMin = pLoadFactors->CRmin[limitState];
         gSHMax = pLoadFactors->SHmax[limitState];
         gSHMin = pLoadFactors->SHmin[limitState];
         gREMax = pLoadFactors->REmax[limitState];
         gREMin = pLoadFactors->REmin[limitState];
      }

      std::vector<sysSectionValue> vVcr = GetShear(intervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<sysSectionValue> vVsh = GetShear(intervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<sysSectionValue> vVre = GetShear(intervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<sysSectionValue> vVcrMin;
         std::transform(vVcr.begin(),vVcr.end(),std::back_inserter(vVcrMin),std::bind1st(std::multiplies<sysSectionValue>(),gCRMin));
         std::transform(pMin->begin(),pMin->end(),vVcrMin.begin(),pMin->begin(),std::plus<sysSectionValue>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vVcr.begin(),pMin->begin(),std::plus<sysSectionValue>());
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<sysSectionValue> vVcrMax;
         std::transform(vVcr.begin(),vVcr.end(),std::back_inserter(vVcrMax),std::bind1st(std::multiplies<sysSectionValue>(),gCRMax));
         std::transform(pMax->begin(),pMax->end(),vVcrMax.begin(),pMax->begin(),std::plus<sysSectionValue>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vVcr.begin(),pMax->begin(),std::plus<sysSectionValue>());
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<sysSectionValue> vVshMin;
         std::transform(vVsh.begin(),vVsh.end(),std::back_inserter(vVshMin),std::bind1st(std::multiplies<sysSectionValue>(),gSHMin));
         std::transform(pMin->begin(),pMin->end(),vVshMin.begin(),pMin->begin(),std::plus<sysSectionValue>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vVsh.begin(),pMin->begin(),std::plus<sysSectionValue>());
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<sysSectionValue> vVshMax;
         std::transform(vVsh.begin(),vVsh.end(),std::back_inserter(vVshMax),std::bind1st(std::multiplies<sysSectionValue>(),gSHMax));
         std::transform(pMax->begin(),pMax->end(),vVshMax.begin(),pMax->begin(),std::plus<sysSectionValue>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vVsh.begin(),pMax->begin(),std::plus<sysSectionValue>());
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<sysSectionValue> vVreMin;
         std::transform(vVre.begin(),vVre.end(),std::back_inserter(vVreMin),std::bind1st(std::multiplies<sysSectionValue>(),gREMin));
         std::transform(pMin->begin(),pMin->end(),vVreMin.begin(),pMin->begin(),std::plus<sysSectionValue>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vVre.begin(),pMin->begin(),std::plus<sysSectionValue>());
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<sysSectionValue> vVreMax;
         std::transform(vVre.begin(),vVre.end(),std::back_inserter(vVreMax),std::bind1st(std::multiplies<sysSectionValue>(),gREMax));
         std::transform(pMax->begin(),pMax->end(),vVreMax.begin(),pMax->begin(),std::plus<sysSectionValue>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vVre.begin(),pMax->begin(),std::plus<sysSectionValue>());
      }
   }
}

void CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
      if (intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetMoment(intervalIdx,limitState,vPoi,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetMoment(intervalIdx,limitState,vPoi,bat,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         gCRMax = pLoadFactors->CRmax[limitState];
         gCRMin = pLoadFactors->CRmin[limitState];
         gSHMax = pLoadFactors->SHmax[limitState];
         gSHMin = pLoadFactors->SHmin[limitState];
         gREMax = pLoadFactors->REmax[limitState];
         gREMin = pLoadFactors->REmin[limitState];
      }

      std::vector<Float64> vMcr = GetMoment(intervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<Float64> vMsh = GetMoment(intervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<Float64> vMre = GetMoment(intervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<Float64> vMcrMin;
         std::transform(vMcr.begin(),vMcr.end(),std::back_inserter(vMcrMin),std::bind1st(std::multiplies<Float64>(),gCRMin));
         std::transform(pMin->begin(),pMin->end(),vMcrMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vMcr.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vMcrMax;
         std::transform(vMcr.begin(),vMcr.end(),std::back_inserter(vMcrMax),std::bind1st(std::multiplies<Float64>(),gCRMax));
         std::transform(pMax->begin(),pMax->end(),vMcrMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vMcr.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<Float64> vMshMin;
         std::transform(vMsh.begin(),vMsh.end(),std::back_inserter(vMshMin),std::bind1st(std::multiplies<Float64>(),gSHMin));
         std::transform(pMin->begin(),pMin->end(),vMshMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vMsh.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vMshMax;
         std::transform(vMsh.begin(),vMsh.end(),std::back_inserter(vMshMax),std::bind1st(std::multiplies<Float64>(),gSHMax));
         std::transform(pMax->begin(),pMax->end(),vMshMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vMsh.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<Float64> vMreMin;
         std::transform(vMre.begin(),vMre.end(),std::back_inserter(vMreMin),std::bind1st(std::multiplies<Float64>(),gREMin));
         std::transform(pMin->begin(),pMin->end(),vMreMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vMre.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vMreMax;
         std::transform(vMre.begin(),vMre.end(),std::back_inserter(vMreMax),std::bind1st(std::multiplies<Float64>(),gREMax));
         std::transform(pMax->begin(),pMax->end(),vMreMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vMre.begin(),pMax->begin(),std::plus<Float64>());
      }
   }
}

std::vector<Float64> CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<Float64> Msd;
   try
   {
      // need to do the time-step analysis because it adds loads to the LBAM
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),INVALID_INDEX);

      Msd = m_pGirderModelManager->GetSlabDesignMoment(limitState,vPoi,bat);
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return Msd;
}

void CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetDeflection(intervalIdx,limitState,vPoi,bIncludePrestress,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetDeflection(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         gCRMax = pLoadFactors->CRmax[limitState];
         gCRMin = pLoadFactors->CRmin[limitState];
         gSHMax = pLoadFactors->SHmax[limitState];
         gSHMin = pLoadFactors->SHmin[limitState];
         gREMax = pLoadFactors->REmax[limitState];
         gREMin = pLoadFactors->REmin[limitState];
      }

      std::vector<Float64> vDcr = GetDeflection(intervalIdx,lcCR,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vDsh = GetDeflection(intervalIdx,lcSH,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vDre = GetDeflection(intervalIdx,lcRE,vPoi,bat,rtCumulative,false);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<Float64> vDcrMin;
         std::transform(vDcr.begin(),vDcr.end(),std::back_inserter(vDcrMin),std::bind1st(std::multiplies<Float64>(),gCRMin));
         std::transform(pMin->begin(),pMin->end(),vDcrMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vDcr.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vDcrMax;
         std::transform(vDcr.begin(),vDcr.end(),std::back_inserter(vDcrMax),std::bind1st(std::multiplies<Float64>(),gCRMax));
         std::transform(pMax->begin(),pMax->end(),vDcrMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vDcr.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<Float64> vDshMin;
         std::transform(vDsh.begin(),vDsh.end(),std::back_inserter(vDshMin),std::bind1st(std::multiplies<Float64>(),gSHMin));
         std::transform(pMin->begin(),pMin->end(),vDshMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vDsh.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vDshMax;
         std::transform(vDsh.begin(),vDsh.end(),std::back_inserter(vDshMax),std::bind1st(std::multiplies<Float64>(),gSHMax));
         std::transform(pMax->begin(),pMax->end(),vDshMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vDsh.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<Float64> vDreMin;
         std::transform(vDre.begin(),vDre.end(),std::back_inserter(vDreMin),std::bind1st(std::multiplies<Float64>(),gREMin));
         std::transform(pMin->begin(),pMin->end(),vDreMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vDre.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vDreMax;
         std::transform(vDre.begin(),vDre.end(),std::back_inserter(vDreMax),std::bind1st(std::multiplies<Float64>(),gREMax));
         std::transform(pMax->begin(),pMax->end(),vDreMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vDre.begin(),pMax->begin(),std::plus<Float64>());
      }
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetRotation(intervalIdx,limitState,vPoi,bIncludePrestress,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetRotation(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         gCRMax = pLoadFactors->CRmax[limitState];
         gCRMin = pLoadFactors->CRmin[limitState];
         gSHMax = pLoadFactors->SHmax[limitState];
         gSHMin = pLoadFactors->SHmin[limitState];
         gREMax = pLoadFactors->REmax[limitState];
         gREMin = pLoadFactors->REmin[limitState];
      }

      std::vector<Float64> vRcr = GetRotation(intervalIdx,lcCR,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vRsh = GetRotation(intervalIdx,lcSH,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vRre = GetRotation(intervalIdx,lcRE,vPoi,bat,rtCumulative,false);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<Float64> vRcrMin;
         std::transform(vRcr.begin(),vRcr.end(),std::back_inserter(vRcrMin),std::bind1st(std::multiplies<Float64>(),gCRMin));
         std::transform(pMin->begin(),pMin->end(),vRcrMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vRcr.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vRcrMax;
         std::transform(vRcr.begin(),vRcr.end(),std::back_inserter(vRcrMax),std::bind1st(std::multiplies<Float64>(),gCRMax));
         std::transform(pMax->begin(),pMax->end(),vRcrMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vRcr.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<Float64> vRshMin;
         std::transform(vRsh.begin(),vRsh.end(),std::back_inserter(vRshMin),std::bind1st(std::multiplies<Float64>(),gSHMin));
         std::transform(pMin->begin(),pMin->end(),vRshMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vRsh.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vRshMax;
         std::transform(vRsh.begin(),vRsh.end(),std::back_inserter(vRshMax),std::bind1st(std::multiplies<Float64>(),gSHMax));
         std::transform(pMax->begin(),pMax->end(),vRshMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vRsh.begin(),pMax->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<Float64> vRreMin;
         std::transform(vRre.begin(),vRre.end(),std::back_inserter(vRreMin),std::bind1st(std::multiplies<Float64>(),gREMin));
         std::transform(pMin->begin(),pMin->end(),vRreMin.begin(),pMin->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),vRre.begin(),pMin->begin(),std::plus<Float64>());
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vRreMax;
         std::transform(vRre.begin(),vRre.end(),std::back_inserter(vRreMax),std::bind1st(std::multiplies<Float64>(),gREMax));
         std::transform(pMax->begin(),pMax->end(),vRreMax.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMax->begin(),pMax->end(),vRre.begin(),pMax->begin(),std::plus<Float64>());
      }
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GetTimeStepStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,pMin,pMax);
   }
   else
   {
      GetElasticStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
{
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IBridge,pBridge);

      CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx,girderKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(intervalIdx,limitState,pierIdx,girderKey,bIncludeImpact,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetReaction(intervalIdx,limitState,pierIdx,girderKey,bat,bIncludeImpact,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

///////////////////////////////////////////////////////
// IExternalLoading
bool CAnalysisAgentImp::CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName)
{
   if ( !m_pSegmentModelManager->CreateLoading(girderLineIdx,strLoadingName) )
   {
      return false;
   }

   if ( !m_pGirderModelManager->CreateLoading(girderLineIdx,strLoadingName) )
   {
      return false;
   }

   return true;
}

bool CAnalysisAgentImp::AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo)
{
   if ( !m_pSegmentModelManager->AddLoadingToLoadCombination(girderLineIdx,strLoadingName,lcCombo) )
   {
      return false;
   }

   if ( !m_pGirderModelManager->AddLoadingToLoadCombination(girderLineIdx,strLoadingName,lcCombo) )
   {
      return false;
   }

   return true;
}

bool CAnalysisAgentImp::CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   if ( IsZero(Fx) && IsZero(Fy) && IsZero(Mz) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateConcentratedLoad(intervalIdx,strLoadingName,poi,Fx,Fy,Mz);
   }
   else
   {
      return m_pGirderModelManager->CreateConcentratedLoad(intervalIdx,strLoadingName,poi,Fx,Fy,Mz);
   }
}

bool CAnalysisAgentImp::CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   if ( IsZero(Fx) && IsZero(Fy) && IsZero(Mz) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateConcentratedLoad(intervalIdx,pfType,poi,Fx,Fy,Mz);
   }
   else
   {
      return m_pGirderModelManager->CreateConcentratedLoad(intervalIdx,pfType,poi,Fx,Fy,Mz);
   }
}

bool CAnalysisAgentImp::CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   if ( IsZero(wx) && IsZero(wy) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateUniformLoad(intervalIdx,strLoadingName,poi1,poi2,wx,wy);
   }
   else
   {
      return m_pGirderModelManager->CreateUniformLoad(intervalIdx,strLoadingName,poi1,poi2,wx,wy);
   }
}

bool CAnalysisAgentImp::CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   if ( IsZero(wx) && IsZero(wy) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateUniformLoad(intervalIdx,pfType,poi1,poi2,wx,wy);
   }
   else
   {
      return m_pGirderModelManager->CreateUniformLoad(intervalIdx,pfType,poi1,poi2,wx,wy);
   }
}

bool CAnalysisAgentImp::CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
#if defined _DEBUG
   // POI's must belong to the same girder
   const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
   const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
   ATLASSERT(segmentKey1.groupIndex  == segmentKey2.groupIndex);
   ATLASSERT(segmentKey1.girderIndex == segmentKey2.girderIndex);
#endif

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateInitialStrainLoad(intervalIdx,strLoadingName,poi1,poi2,e,r);
   }
   else
   {
      return m_pGirderModelManager->CreateInitialStrainLoad(intervalIdx,strLoadingName,poi1,poi2,e,r);
   }
}

bool CAnalysisAgentImp::CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
#if defined _DEBUG
   // POI's must belong to the same girder
   const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
   const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
   ATLASSERT(segmentKey1.groupIndex  == segmentKey2.groupIndex);
   ATLASSERT(segmentKey1.girderIndex == segmentKey2.girderIndex);
#endif

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateInitialStrainLoad(intervalIdx,pfType,poi1,poi2,e,r);
   }
   else
   {
      return m_pGirderModelManager->CreateInitialStrainLoad(intervalIdx,pfType,poi1,poi2,e,r);
   }
}

Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results( GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType,bIncludeElevationAdjustment) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType,bIncludeSlopeAdjustment) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);
   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);
   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetAxial(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Aprev = GetAxial(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<Float64> Athis = GetAxial(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(Athis.begin(),Athis.end(),Aprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<sysSectionValue> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetShear(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<sysSectionValue> Vprev = GetShear(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<sysSectionValue> Vthis = GetShear(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(Vthis.begin(),Vthis.end(),Vprev.begin(),std::back_inserter(results),std::minus<sysSectionValue>());
      }
      else
      {
         results = m_pGirderModelManager->GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetMoment(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(Mthis.begin(),Mthis.end(),Mprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetDeflection(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative,false);
         std::vector<Float64> Dthis = GetDeflection(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative,false);
         std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&results,NULL);
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetRotation(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Rprev = GetRotation(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative,false);
         std::vector<Float64> Rthis = GetRotation(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative,false);
         std::transform(Rthis.begin(),Rthis.end(),Rprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&results,NULL);
   }

   return results;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         m_pSegmentModelManager->GetStress(intervalIdx,strLoadingName,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> fTopPrev, fBotPrev;
         GetStress(intervalIdx-1,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTopPrev,&fBotPrev);

         std::vector<Float64> fTopThis, fBotThis;
         GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTopThis,&fBotThis);

         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
         std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright)
{
   std::vector<CSegmentKey> vSegmentKeys;
   vSegmentKeys.push_back(segmentKey);
   std::vector<Float64> vFyLeft, vFyRight;
   GetSegmentReactions(vSegmentKeys,intervalIdx,strLoadingName,bat,resultsType,&vFyLeft,&vFyRight);
   *pRleft = vFyLeft.front();
   *pRright = vFyRight.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& vSegmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright)
{
   pRleft->reserve(vSegmentKeys.size());
   pRright->reserve(vSegmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(vSegmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(vSegmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 Rleft(0), Rright(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,strLoadingName,bat,resultsType,&Rleft,&Rright);
      }
      pRleft->push_back(Rleft);
      pRright->push_back(Rright);
   }
}

Float64 CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> vReactions = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,resultsType);
   return vReactions.front();
}

std::vector<Float64> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      std::vector<Float64> reactions;
      reactions.resize(vSupports.size(),0);
      return reactions;
   }
   else
   {
      if ( resultsType == rtCumulative )
      {
         std::vector<Float64> reactions;
         reactions.resize(vSupports.size(),0);
         for ( IntervalIndexType iIdx = erectionIntervalIdx; iIdx <= intervalIdx; iIdx++)
         {
            std::vector<Float64> Fy = m_pGirderModelManager->GetReaction(girderKey,vSupports,iIdx,strLoadingName,bat,rtIncremental);
            std::transform(reactions.begin(),reactions.end(),Fy.begin(),reactions.begin(),std::plus<Float64>());
         }
         return reactions;
      }
      else
      {
         return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
      }
   }
}

///////////////////////////
void CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   // Computes design-time stresses due to external loads
   ATLASSERT(IsGirderStressLocation(loc)); // expecting stress location to be on the girder

   // figure out which stage the girder loading is applied
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx           = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx          = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx     = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType trafficBarrierIntervalIdx    = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx           = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx          = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx == releaseIntervalIdx )
   {
      GetStress(intervalIdx,limitState,poi,bat,false,loc,pMin,pMax);
      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // can't use this method with strength limit states
   ATLASSERT( !IsStrengthLimitState(limitState) );

   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Stc = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::TopGirder);
   Float64 Sbc = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::BottomGirder);

   Float64 Stop = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::TopGirder,   fcgdr);
   Float64 Sbot = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::BottomGirder,fcgdr);

   Float64 k_top = Stc/Stop; // scale factor that converts top stress of composite section to a section with this f'c
   Float64 k_bot = Sbc/Sbot; // scale factor that converts bot stress of composite section to a section with this f'c

   Float64 ftop1, ftop2, ftop3Min, ftop3Max;
   Float64 fbot1, fbot2, fbot3Min, fbot3Max;

   ftop1 = ftop2 = ftop3Min = ftop3Max = 0;
   fbot1 = fbot2 = fbot3Min = fbot3Max = 0;

   // Load Factors
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   Float64 dc = pLoadFactors->DCmax[limitState];
   Float64 dw = pLoadFactors->DWmax[limitState];
   Float64 ll = pLoadFactors->LLIMmax[limitState];

   Float64 ft,fb;

   // Erect Segment (also covers temporary strand removal)
   if ( erectSegmentIntervalIdx <= intervalIdx )
   {
      GetStress(erectSegmentIntervalIdx,pgsTypes::pftGirder,poi,bat,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 = dc*ft;   
      fbot1 = dc*fb;
   }

   // Casting Deck (non-composite girder carrying deck dead load)
   if ( castDeckIntervalIdx <= intervalIdx )
   {
      GetStress(castDeckIntervalIdx,pgsTypes::pftConstruction,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftSlab,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetDesignSlabStressAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftSlabPad,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftSlabPanel,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetDesignSlabPadStressAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftDiaphragm,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftShearKey,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftUserDC,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pgsTypes::pftUserDW,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dw*ft;   
      fbot1 += dw*fb;
   }

   // Composite Section Carrying Superimposed Dead Loads
   if ( compositeDeckIntervalIdx <= intervalIdx )
   {
      GetStress(trafficBarrierIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 = dc*k_top*ft;   
      fbot2 = dc*k_bot*fb;

      GetStress(trafficBarrierIntervalIdx,pgsTypes::pftSidewalk,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dc*k_top*ft;   
      fbot2 += dc*k_bot*fb;

      if ( overlayIntervalIdx != INVALID_INDEX && overlayIntervalIdx <= intervalIdx )
      {
         GetStress(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
         ftop2 += dw*k_top*ft;   
         fbot2 += dw*k_bot*fb;
      }

      GetStress(compositeDeckIntervalIdx,pgsTypes::pftUserDC,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dc*k_top*ft;   
      fbot2 += dc*k_bot*fb;

      GetStress(compositeDeckIntervalIdx,pgsTypes::pftUserDW,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dw*k_top*ft;   
      fbot2 += dw*k_bot*fb;

      // slab shrinkage stresses
      Float64 ft_ss, fb_ss;
      GetDeckShrinkageStresses(poi,fcgdr,&ft_ss,&fb_ss);
      ftop2 += ft_ss;
      fbot2 += fb_ss;
   }

   // Open to traffic, carrying live load
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      ftop3Min = ftop3Max = 0.0;   
      fbot3Min = fbot3Max = 0.0;   

      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pGirderMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

      Float64 fc_lldf = fcgdr;
      if ( pGirderMaterial->Concrete.bUserEc )
      {
         fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Concrete.Ec, pGirderMaterial->Concrete.StrengthDensity );
      }


      Float64 ftMin,ftMax,fbMin,fbMax;
      if ( limitState == pgsTypes::FatigueI )
      {
         GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltFatigue, poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
      }
      else
      {
         GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltDesign,  poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
      }

      ftop3Min += ll*k_top*ftMin;   
      fbot3Min += ll*k_bot*fbMin;

      ftop3Max += ll*k_top*ftMax;   
      fbot3Max += ll*k_bot*fbMax;

      GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltPedestrian,  poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);

      ftop3Min += ll*k_top*ftMin;   
      fbot3Min += ll*k_bot*fbMin;

      ftop3Max += ll*k_top*ftMax;   
      fbot3Max += ll*k_bot*fbMax;

      GetStress(liveLoadIntervalIdx,pgsTypes::pftUserLLIM,poi,bat,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop3Min += ll*k_top*ft;   
      fbot3Min += ll*k_bot*fb;

      ftop3Max += ll*k_top*ft;   
      fbot3Max += ll*k_bot*fb;
   }

   if ( loc == pgsTypes::TopGirder )
   {
      *pMin = ftop1 + ftop2 + ftop3Min;
      *pMax = ftop1 + ftop2 + ftop3Max;
   }
   else
   {
      *pMin = fbot1 + fbot2 + fbot3Min;
      *pMax = fbot1 + fbot2 + fbot3Max;
   }

   if (*pMax < *pMin )
   {
      std::swap(*pMin,*pMax);
   }

   *pMax = (IsZero(*pMax) ? 0 : *pMax);
   *pMin = (IsZero(*pMin) ? 0 : *pMin);
}

std::vector<Float64> CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludeLiveLoad)
{
   std::vector<Float64> stresses;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      Float64 stress = GetStress(intervalIdx,poi,stressLocation,bIncludeLiveLoad);
      stresses.push_back(stress);
   }

   return stresses;
}

//////////////////////////////////////////////////////////////////////
void CAnalysisAgentImp::GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   try
   {
      m_pGirderModelManager->GetConcurrentShear(intervalIdx,limitState,poi,bat,pMin,pMax);
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax)
{
   m_pGirderModelManager->GetViMmax(intervalIdx,limitState,poi,bat,pVi,pMmax);
}

/////////////////////////////////////////////////////////////////////////////
// ICamber
//
Uint32 CAnalysisAgentImp::GetCreepMethod()
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   return pSpecEntry->GetCreepMethod();
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,creepPeriod,constructionRate);
   return details.Ct;
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,config,creepPeriod,constructionRate);
   return details.Ct;
}

CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details;

   GET_IFACE(IEnvironment,pEnvironment);

   GET_IFACE(ISectionProperties,pSectProp);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // if fc < 0 use current fc girder
   LoadingEvent le = GetLoadingEvent(creepPeriod);
   Float64 fc = GetConcreteStrengthAtTimeOfLoading(config,le);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   details.Method = pSpecEntry->GetCreepMethod();
   ATLASSERT(details.Method == CREEP_LRFD); // only supporting LRFD method... the old WSDOT method is out

   details.Spec = (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ) ? CREEP_SPEC_PRE_2005 : CREEP_SPEC_2005;

   if ( details.Spec == CREEP_SPEC_PRE_2005 )
   {
      //  LRFD 3rd edition and earlier
      try
      {
         lrfdCreepCoefficient cc;
         cc.SetRelHumidity( pEnvironment->GetRelHumidity() );
         cc.SetSurfaceArea( pSectProp->GetSegmentSurfaceArea(segmentKey) );
         cc.SetVolume( pSectProp->GetSegmentVolume(segmentKey) );
         cc.SetFc(fc);
         cc.SetCuringMethodTimeAdjustmentFactor(pSpecEntry->GetCuringMethodTimeAdjustmentFactor());

         switch( creepPeriod )
         {
            case cpReleaseToDiaphragm:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) - cc.GetAdjustedInitialAge() );
               break;

            case cpReleaseToDeck:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpReleaseToFinal:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDiaphragmToDeck:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity(   constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpDiaphragmToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDeckToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            default:
               ATLASSERT(false);
         }

         details.ti           = cc.GetAdjustedInitialAge();
         details.t            = cc.GetMaturity();
         details.Fc           = cc.GetFc();
         details.H            = cc.GetRelHumidity();
         details.kf           = cc.GetKf();
         details.kc           = cc.GetKc();
         details.VSratio      = cc.GetVolume() / cc.GetSurfaceArea();
         details.CuringMethod = cc.GetCuringMethod();

         details.Ct           = cc.GetCreepCoefficient();
      }
#if defined _DEBUG
      catch( lrfdXCreepCoefficient& ex )
#else
      catch( lrfdXCreepCoefficient& /*ex*/ )
#endif // _DEBUG
      {
         ATLASSERT( ex.GetReason() == lrfdXCreepCoefficient::VSRatio );

         std::_tstring strMsg(_T("V/S Ratio exceeds maximum value per C5.4.2.3.2. Use a different method for estimating creep"));
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(segmentKey,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }
   else
   {
      // LRFD 3rd edition with 2005 interims and later
      try
      {
         lrfdCreepCoefficient2005 cc;
         cc.SetRelHumidity( pEnvironment->GetRelHumidity() );
         cc.SetSurfaceArea( pSectProp->GetSegmentSurfaceArea(segmentKey) );
         cc.SetVolume( pSectProp->GetSegmentVolume(segmentKey) );
         cc.SetFc(fc);

         cc.SetCuringMethodTimeAdjustmentFactor(pSpecEntry->GetCuringMethodTimeAdjustmentFactor());

         GET_IFACE(IMaterials,pMaterial);
         cc.SetK1( pMaterial->GetSegmentCreepK1(segmentKey) );
         cc.SetK2( pMaterial->GetSegmentCreepK2(segmentKey) );


         switch( creepPeriod )
         {
            case cpReleaseToDiaphragm:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) - cc.GetAdjustedInitialAge() );
               break;

            case cpReleaseToDeck:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max()) - cc.GetAdjustedInitialAge() );
               break;

            case cpReleaseToFinal:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() - cc.GetAdjustedInitialAge() );
               break;

            case cpDiaphragmToDeck:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity(  (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max()) - cc.GetAdjustedInitialAge());
               break;

            case cpDiaphragmToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) - cc.GetAdjustedInitialAge() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDeckToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() - cc.GetAdjustedInitialAge() );
               break;

            default:
               ATLASSERT(false);
         }

         details.ti           = cc.GetAdjustedInitialAge();
         details.t            = cc.GetMaturity();
         details.Fc           = cc.GetFc();
         details.H            = cc.GetRelHumidity();
         details.kvs          = cc.GetKvs();
         details.khc          = cc.GetKhc();
         details.kf           = cc.GetKf();
         details.ktd          = cc.GetKtd();
         details.VSratio      = cc.GetVolume() / cc.GetSurfaceArea();
         details.CuringMethod = cc.GetCuringMethod();
         details.K1           = cc.GetK1();
         details.K2           = cc.GetK2();

         details.Ct           = cc.GetCreepCoefficient();
      }
#if defined _DEBUG
      catch( lrfdXCreepCoefficient& ex )
#else
      catch( lrfdXCreepCoefficient& /*ex*/ )
#endif // _DEBUG
      {
         ATLASSERT( ex.GetReason() == lrfdXCreepCoefficient::VSRatio );

         std::_tstring strMsg(_T("V/S Ratio exceeds maximum value per C5.4.2.3.2. Use a different method for estimating creep"));
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(segmentKey,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }

   return details;
}


CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate)
{
   std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS>::iterator found = m_CreepCoefficientDetails[constructionRate][creepPeriod].find(segmentKey);
   if ( found != m_CreepCoefficientDetails[constructionRate][creepPeriod].end() )
   {
      return (*found).second;
   }

   GET_IFACE(IPointOfInterest, IPoi);
   std::vector<pgsPointOfInterest> pois( IPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L,POIFIND_AND) );
   ATLASSERT(pois.size() == 1);
   pgsPointOfInterest poi = pois[0];

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   CREEPCOEFFICIENTDETAILS ccd = GetCreepCoefficientDetails(segmentKey,config,creepPeriod,constructionRate);
   m_CreepCoefficientDetails[constructionRate][creepPeriod].insert(std::make_pair(segmentKey,ccd));
   return ccd;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum)
{
   Float64 dy,rz;
   GetPrestressDeflection(poi,datum,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum)
{
   Float64 dy,rz;
   GetPrestressDeflection(poi,config,datum,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum)
{
   Float64 dy,rz;
   GetInitialTempPrestressDeflection(poi,datum,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum)
{
   Float64 dy,rz;
   GetInitialTempPrestressDeflection(poi,config,datum,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetReleaseTempPrestressDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetReleaseTempPrestressDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum)
{
   Float64 dy,rz;
   GetCreepDeflection(poi,creepPeriod,constructionRate,datum,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum)
{
   Float64 dy,rz;
   GetCreepDeflection(poi,config,creepPeriod,constructionRate,datum,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi)
{
   Float64 dsy,rsz,dspy,rspz;
   GetDeckDeflection(poi,&dsy,&rsz,&dspy,&rspz);
   return dsy + dspy;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dsy,rsz,dspy,rspz;
   GetDeckDeflection(poi,config,&dsy,&rsz,&dspy,&rspz);
   return dsy + dspy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetShearKeyDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetShearKeyDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetShearKeyDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetConstructionLoadDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetConstructionLoadDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetConstructionLoadDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) 
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetScreedCamber(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetScreedCamber(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,Int16 time)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
#if defined _DEBUG
      // this is slower, but use it as a check on the more direct method used below
      Float64 D = GetDCamberForGirderSchedule(poi,time);
      Float64 C = GetScreedCamber(poi);
      Float64 excess = D - C;
#endif

      // excess camber is the camber that remains in the girder when the bridge is open
      // to traffic. we simply need to get the deflection when the bridge goes into
      // service.
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      Float64 Dmin, Dmax;
      GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,poi,bat,true,false/*exclude live load deflection*/,true,&Dmin,&Dmax);
      ATLASSERT(IsEqual(Dmin,Dmax)); // no live load so these should be the same

      ATLASSERT(IsEqual(Dmax,excess));

      return Dmax;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model         = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
      CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
      CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

      Float64 Dy, Rz;
      GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&Dy,&Rz);
      return Dy;
   }
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);
   
   Float64 Dy, Rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&Dy, &Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model         = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   Float64 dy,rz;
   GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);
   
   Float64 dy,rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   return GetSidlDeflection(poi,config);
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType nIntervals               = pIntervals->GetIntervalCount();

   // NOTE: No need to validate camber models
   Float64 delta_trafficbarrier  = 0;
   Float64 delta_sidewalk        = 0;
   Float64 delta_overlay         = 0;
   Float64 delta_diaphragm       = 0;
   Float64 delta_user            = 0;

   // adjustment factor for fcgdr that is different that current value
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,compositeDeckIntervalIdx);

   delta_diaphragm      = GetDiaphragmDeflection(poi,config);
   delta_trafficbarrier = k2*GetDeflection(compositeDeckIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
   delta_sidewalk       = k2*GetDeflection(compositeDeckIntervalIdx, pgsTypes::pftSidewalk,       poi, bat, rtIncremental, false);

   for ( IntervalIndexType intervalIdx = compositeDeckIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 dUser = GetUserLoadDeflection(intervalIdx,poi,config);
      delta_user += dUser;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      delta_overlay = k2*GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
   }

   Float64 dSIDL;
   bool bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) && 
                        config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   if ( bTempStrands )
   {
      dSIDL = delta_trafficbarrier + delta_sidewalk + delta_overlay + delta_user;
   }
   else
   {
      // for SIP decks, diaphagms are applied before the cast portion of the slab so they don't apply to screed camber
      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         delta_diaphragm = 0;
      }

      dSIDL = delta_trafficbarrier + delta_sidewalk + delta_overlay + delta_user + delta_diaphragm;
   }

   return dSIDL;
}

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      Float64 Dmin, Dmax;
      GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,poi,bat,true,false,true,&Dmin,&Dmax);
      ATLASSERT(IsEqual(Dmin,Dmax)); // no live load so these should be the same

      return Dmax;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model            = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
      CamberModelData initTempModel    = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
      CamberModelData releaseTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

      Float64 Dy, Rz;
      GetDCamberForGirderSchedule(poi,model,initTempModel,releaseTempModel,time,&Dy,&Rz);
      return Dy;
   }
}

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1, tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);

   Float64 Dy, Rz;
   GetDCamberForGirderSchedule(poi,config,initModel,tempModel1, tempModel2, time, &Dy, &Rz);
   return Dy;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GDRCONFIG dummy_config;

   CamberModelData model  = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData model1 = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData model2 = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);
   GetCreepDeflection(poi,false,dummy_config,model,model1,model2, creepPeriod, constructionRate, datum, pDy, pRz);
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model;
   BuildCamberModel(segmentKey,true,config,&model);

   CamberModelData model1,model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);
   
   GetCreepDeflection(poi,true,config,model,model1,model2, creepPeriod, constructionRate, datum, pDy,pRz);
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   // this version is only for PGSuper design mode
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   // NOTE: No need to validate camber models
   Float64 Dslab            = 0;
   Float64 Dpanel           = 0;
   Float64 DslabPad         = 0;
   Float64 Dtrafficbarrier  = 0;
   Float64 Dsidewalk        = 0;
   Float64 Doverlay         = 0;
   Float64 Duser1           = 0;
   Float64 Duser2           = 0;

   Float64 Rslab            = 0;
   Float64 Rpanel           = 0;
   Float64 RslabPad         = 0;
   Float64 Rtrafficbarrier  = 0;
   Float64 Rsidewalk        = 0;
   Float64 Roverlay         = 0;
   Float64 Ruser1           = 0;
   Float64 Ruser2           = 0;

   GetDeckDeflection(poi,&Dslab,&Rslab,&DslabPad,&RslabPad);
   Dtrafficbarrier = GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
   Rtrafficbarrier = GetRotation(  railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
   Dsidewalk = GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);
   Rsidewalk = GetRotation(  railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);

   if ( deckType == pgsTypes::sdtCompositeSIP )
   {
      GetDeckPanelDeflection( poi,&Dpanel,&Rpanel );
   }

   // Only get deflections for user defined loads that occur during deck placement and later
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
   std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(spanKey));
   vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(),vUserLoadIntervals.end(),std::bind2nd(std::less<IntervalIndexType>(),castDeckIntervalIdx)),vUserLoadIntervals.end());
   std::vector<IntervalIndexType>::iterator userLoadIter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator userLoadIterEnd(vUserLoadIntervals.end());
   Uint32 ucnt=0;
   for ( ; userLoadIter != userLoadIterEnd; userLoadIter++ )
   {
      IntervalIndexType intervalIdx = *userLoadIter;
      Float64 D,R;

      GetUserLoadDeflection(intervalIdx,poi,&D,&R);

      // For PGSuper there are two user load stages and they are factored differently. User1 occurs when slab is cast. For splice,
      // the user load stages are always factored by 1.0, so this does not matter (now)
      if (intervalIdx==castDeckIntervalIdx)
      {
         Duser1 += D;
         Ruser1 += R;
      }
      else
      {
         Duser2 += D;
         Ruser2 += R;
      }

      ucnt++;
   }

   // This is an old convention in PGSuper: If there is no deck, user1 is included in the D camber
   if ( deckType == pgsTypes::sdtNone )
   {
      Duser1 = 0.0;
      Ruser1 = 0.0;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      Doverlay = GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
      Roverlay = GetRotation(  overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

   *pDy = cm.SlabUser1Factor*(Dslab + Duser1) + cm.SlabPadLoadFactor*DslabPad + cm.DeckPanelFactor*Dpanel
        + cm.BarrierSwOverlayUser2Factor*(Dtrafficbarrier + Dsidewalk + Doverlay + Duser2);

   *pRz = cm.SlabUser1Factor*(Rslab + Ruser1) + cm.SlabPadLoadFactor*RslabPad + cm.DeckPanelFactor*Rpanel
        + cm.BarrierSwOverlayUser2Factor*(Rtrafficbarrier + Rsidewalk + Roverlay + Ruser2);

   // Switch the sign. Negative deflection creates positive screed camber
   (*pDy) *= -1;
   (*pRz) *= -1;
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // this version is only for PGSuper design mode
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   // NOTE: No need to validate camber models
   Float64 Dslab            = 0;
   Float64 Dpanel           = 0;
   Float64 DslabPad         = 0;
   Float64 Dslab_adj        = 0;
   Float64 Dslab_pad_adj    = 0;
   Float64 Dtrafficbarrier  = 0;
   Float64 Dsidewalk        = 0;
   Float64 Doverlay         = 0;
   Float64 Duser1           = 0;
   Float64 Duser2           = 0;

   Float64 Rslab            = 0;
   Float64 Rpanel           = 0;
   Float64 RslabPad         = 0;
   Float64 Rslab_adj        = 0;
   Float64 Rslab_pad_adj    = 0;
   Float64 Rtrafficbarrier  = 0;
   Float64 Rsidewalk        = 0;
   Float64 Roverlay         = 0;
   Float64 Ruser1           = 0;
   Float64 Ruser2           = 0;

   // deflections are computed based on current parameters for the bridge.
   // E in the config object may be different than E used to compute the deflection.
   // The deflection adjustment factor accounts for the differences in E.
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,railingSystemIntervalIdx);
   GetDeckDeflection(poi,config,&Dslab,&Rslab,&DslabPad,&RslabPad); 
   GetDesignSlabDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_adj,&Rslab_adj);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad_adj,&Rslab_pad_adj);

   if ( deckType == pgsTypes::sdtCompositeSIP )
   {
      GetDeckPanelDeflection(poi,config,&Dpanel,&Rpanel );
   }
   
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
   Rtrafficbarrier = k2*GetRotation(  railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);
   Rsidewalk = k2*GetRotation(  railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);

   // Only get deflections for user defined loads that occur during deck placement and later
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
   std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(spanKey));
   vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(),vUserLoadIntervals.end(),std::bind2nd(std::less<IntervalIndexType>(),castDeckIntervalIdx)),vUserLoadIntervals.end());
   std::vector<IntervalIndexType>::iterator userLoadIter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator userLoadIterEnd(vUserLoadIntervals.end());
   Uint32 ucnt=0;
   for ( ; userLoadIter != userLoadIterEnd; userLoadIter++ )
   {
      IntervalIndexType intervalIdx = *userLoadIter;
      Float64 D,R;

      k2 = GetDeflectionAdjustmentFactor(poi,config,intervalIdx);
      GetUserLoadDeflection(intervalIdx,poi,config,&D,&R);

      // For PGSuper there are two user load stages and they are factored differently. User1 occurs when slab is cast. For splice,
      // the user load stages are always factored by 1.0, so this does not matter (now)
      if (intervalIdx==castDeckIntervalIdx)
      {
         Duser1 += k2*D;
         Ruser1 += k2*R;
      }
      else
      {
         Duser2 += k2*D;
         Ruser2 += k2*R;
      }
   }

   // This is an old convention in PGSuper: If there is no deck, user1 is included in the D camber
   if ( deckType == pgsTypes::sdtNone )
   {
      Duser1 = 0.0;
      Ruser1 = 0.0;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      k2 = GetDeflectionAdjustmentFactor(poi,config,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
      Roverlay = k2*GetRotation(  overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

   *pDy = cm.SlabUser1Factor*(Dslab + Duser1) + cm.SlabPadLoadFactor*DslabPad + cm.DeckPanelFactor*Dpanel
        + cm.BarrierSwOverlayUser2Factor*(Dtrafficbarrier + Dsidewalk + Doverlay + Duser2);

   *pRz = cm.SlabUser1Factor*(Rslab + Ruser1) + cm.SlabPadLoadFactor*RslabPad + cm.DeckPanelFactor*Rpanel 
        + cm.BarrierSwOverlayUser2Factor*(Rtrafficbarrier + Rsidewalk + Roverlay + Ruser2);

   // Switch the sign. Negative deflection creates positive screed camber
   (*pDy) *= -1;
   (*pRz) *= -1;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 dy_slab = GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlab,poi,bat, rtIncremental, false);
   Float64 rz_slab = GetRotation(  castDeckIntervalIdx,pgsTypes::pftSlab,poi,bat, rtIncremental, false);

   Float64 dy_slab_pad = GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlabPad,poi,bat, rtIncremental, false);
   Float64 rz_slab_pad = GetRotation(  castDeckIntervalIdx,pgsTypes::pftSlabPad,poi,bat, rtIncremental, false);

   *pSlabDy    = dy_slab;
   *pSlabRz    = rz_slab;
   *pSlabPadDy = dy_slab_pad;
   *pSlabPadRz = rz_slab_pad;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GetDeckDeflection(poi,pSlabDy,pSlabRz,pSlabPadDy,pSlabPadRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);
   (*pSlabDy) *= k;
   (*pSlabRz) *= k;
   (*pSlabPadDy) *= k;
   (*pSlabPadRz) *= k;
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // NOTE: it is assumed that deck panels are placed at the same time the
   // cast-in-place topping is installed.
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   *pDy = GetDeflection(castDeckIntervalIdx,pgsTypes::pftSlabPanel,poi,bat, rtIncremental, false);
   *pRz = GetRotation(  castDeckIntervalIdx,pgsTypes::pftSlabPanel,poi,bat, rtIncremental, false);
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // NOTE: it is assumed that deck panels are placed at the same time the
   // cast-in-place topping is installed.
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GetDeckPanelDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetShearKeyDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   if (HasShearKeyLoad(poi.GetSegmentKey()))
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

      *pDy = GetDeflection(castDeckIntervalIdx,pgsTypes::pftShearKey,poi,bat, rtIncremental, false);
      *pRz = GetRotation(  castDeckIntervalIdx,pgsTypes::pftShearKey,poi,bat, rtIncremental, false);
   }
   else
   {
      *pDy = 0.0;
      *pRz = 0.0;
   }
}

void CAnalysisAgentImp::GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetShearKeyDeflection(poi,pDy,pRz);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetConstructionLoadDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   *pDy = GetDeflection(castDeckIntervalIdx,pgsTypes::pftConstruction,poi,bat, rtIncremental, false);
   *pRz = GetRotation(  castDeckIntervalIdx,pgsTypes::pftConstruction,poi,bat, rtIncremental, false);
}

void CAnalysisAgentImp::GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetConstructionLoadDeflection(poi,pDy,pRz);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   *pDy = GetDeflection(castDeckIntervalIdx,pgsTypes::pftDiaphragm,poi,bat, rtIncremental, false);
   *pRz = GetRotation(  castDeckIntervalIdx,pgsTypes::pftDiaphragm,poi,bat, rtIncremental, false);
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GetDiaphragmDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz) 
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 Ddc = GetDeflection(intervalIdx,pgsTypes::pftUserDC, poi,bat, rtIncremental, false);
   Float64 Ddw = GetDeflection(intervalIdx,pgsTypes::pftUserDW, poi,bat, rtIncremental, false);

   Float64 Rdc = GetRotation(intervalIdx,pgsTypes::pftUserDC, poi,bat, rtIncremental, false);
   Float64 Rdw = GetRotation(intervalIdx,pgsTypes::pftUserDW, poi,bat, rtIncremental, false);

   *pDy = Ddc + Ddw;
   *pRz = Rdc + Rdw;
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetUserLoadDeflection(intervalIdx,poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,intervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);

   GetSlabBarrierOverlayDeflection(poi,config,pDy,pRz);
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // NOTE: No need to validate camber models
   Float64 Dslab           = 0;
   Float64 Dslab_adj       = 0;
   Float64 DslabPad        = 0;
   Float64 Dslab_pad_adj   = 0;
   Float64 Dtrafficbarrier = 0;
   Float64 Dsidewalk       = 0;
   Float64 Doverlay        = 0;

   Float64 Rslab           = 0;
   Float64 Rslab_adj       = 0;
   Float64 RslabPad        = 0;
   Float64 Rslab_pad_adj   = 0;
   Float64 Rtrafficbarrier = 0;
   Float64 Rsidewalk       = 0;
   Float64 Roverlay        = 0;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GetDeckDeflection(poi,config,&Dslab,&Rslab,&DslabPad,&RslabPad);
   GetDesignSlabDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_adj,&Rslab_adj);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad_adj,&Rslab_pad_adj);
   
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,railingSystemIntervalIdx);
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental, false);
   Rtrafficbarrier = k2*GetRotation(  railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental, false);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx,pgsTypes::pftSidewalk,poi,bat, rtIncremental, false);
   Rsidewalk = k2*GetRotation(  railingSystemIntervalIdx,pgsTypes::pftSidewalk,poi,bat, rtIncremental, false);

   if ( overlayIntervalIdx != INVALID_INDEX )
   {
      Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
      Roverlay = k2*GetRotation(  overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
   }

   // Switch the sign. Negative deflection creates positive screed camber
   *pDy = Dslab + Dslab_adj + DslabPad + Dslab_pad_adj + Dtrafficbarrier + Dsidewalk + Doverlay;
   *pRz = Rslab + Rslab_adj + RslabPad + Rslab_pad_adj + Rtrafficbarrier + Rsidewalk + Roverlay;
}

Float64 CAnalysisAgentImp::GetLowerBoundCamberVariabilityFactor()const
{
   GET_IFACE(ILibrary,pLibrary);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 fac = pSpecEntry->GetCamberVariability();

   fac = 1.0-fac;
   return fac;
}

CamberMultipliers CAnalysisAgentImp::GetCamberMultipliers(const CSegmentKey& segmentKey)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(segmentKey.girderIndex);

   return pGdrEntry->GetCamberMultipliers();
}

/////////////////////////////////////////////////////////////////////////////
// IPretensionStresses
//
Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludeLiveLoad)
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation,bIncludeLiveLoad);
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad,Float64* pfTop,Float64* pfBot)
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,topLoc,botLoc,bIncludeLiveLoad,pfTop,pfBot);
}

Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 e)
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation,P,e);
}

Float64 CAnalysisAgentImp::GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation stressLocation)
{
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P = pPsForce->GetPrestressForcePerStrand(poi,strandType,intervalIdx,timeType);
   Float64 nSEffective;
   Float64 e = pStrandGeom->GetEccentricity(intervalIdx,poi,strandType,&nSEffective);

   return GetStress(intervalIdx,poi,stressLocation,P,e);
}

Float64 CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,const GDRCONFIG& config,bool bIncludeLiveLoad)
{
   // Computes design-time stresses due to prestressing
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval
   // becase the stress analysis will intrinsically include elastic effects.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P;
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      P = pPsForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,timeType,config);
   }
   else
   {
      if ( bIncludeLiveLoad )
      {
         P = pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent,limitState,config);
      }
      else
      {
         P = pPsForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,timeType,config);
      }
   }

   // NOTE: since we are doing design, the main bridge model may not have temporary strand removal
   // intervals. Use the deck casting interval as the break point for "before temporary strands are removed"
   // and "after temporary strands are removed"
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   bool bIncludeTemporaryStrands = intervalIdx < tsRemovalIntervalIdx ? true : false;
   if ( bIncludeTemporaryStrands ) 
   {
      P += pPsForce->GetPrestressForce(poi,pgsTypes::Temporary,intervalIdx,timeType,config);
   }

   Float64 nSEffective;
   pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptTransformedNoncomposite);
   Float64 e = pStrandGeom->GetEccentricity(spType,intervalIdx,poi, config, bIncludeTemporaryStrands, &nSEffective);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   return GetStress(releaseIntervalIdx,poi,stressLocation,P,e);
}

/////////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
//
HRESULT CAnalysisAgentImp::OnBridgeChanged(CBridgeChangedHint* pHint)
{
   LOG("OnBridgeChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnGirderFamilyChanged()
{
   LOG("OnGirderFamilyChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint)
{
   LOG("OnGirderChanged Event Received");

   Invalidate();

   return S_OK;
}

HRESULT CAnalysisAgentImp::OnLiveLoadChanged()
{
   LOG("OnLiveLoadChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   LOG("OnLiveLoadNameChanged Event Received");
   m_pGirderModelManager->ChangeLiveLoadName(strOldName,strNewName);
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnConstructionLoadChanged()
{
   LOG("OnConstructionLoadChanged Event Received");
   Invalidate();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ISpecificationEventSink
//
HRESULT CAnalysisAgentImp::OnSpecificationChanged()
{
   Invalidate();
   LOG("OnSpecificationChanged Event Received");
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnAnalysisTypeChanged()
{
   //Invalidate();
   // I don't think we have to do anything when the analysis type changes
   // The casting yard models wont change
   // The simple span models, if built, wont change
   // The continuous span models, if built, wont change
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IRatingSpecificationEventSink
//
HRESULT CAnalysisAgentImp::OnRatingSpecificationChanged()
{
   Invalidate();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILoadModifiersEventSink
//
HRESULT CAnalysisAgentImp::OnLoadModifiersChanged()
{
   Invalidate();
   LOG("OnLoadModifiersChanged Event Received");
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILossParametersEventSink
//
HRESULT CAnalysisAgentImp::OnLossParametersChanged()
{
   Invalidate();
   LOG("OnLossParametersChanged Event Received");
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   CamberModelData camber_model_data = GetPrestressDeflectionModel(poi.GetSegmentKey(),m_PrestressDeflectionModels);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,camber_model_data,g_lcidHarpedStrand,datum,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,camber_model_data,g_lcidStraightStrand,datum,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model_data;
   BuildCamberModel(segmentKey,true,config,&model_data);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,model_data,g_lcidHarpedStrand,datum,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,model_data,g_lcidStraightStrand,datum,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model_data = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   
   GetInitialTempPrestressDeflection(poi,model_data,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model1, model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);

   GetInitialTempPrestressDeflection(poi,model1,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   GetReleaseTempPrestressDeflection(poi,model,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model1,model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);

   GetReleaseTempPrestressDeflection(poi,model2,pDy,pRz);
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,LoadCaseIDType lcid,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      *pDy = 0;
      *pRz = 0;
      return;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // We need to compute the deflection relative to the bearings even though the prestress
   // deflection occurs over the length of the entire girder.  To accomplish this, simply
   // deduct the deflection at the bearing (when the girder is supported on its ends) from
   // the deflection at the specified location.
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;

   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   if ( femPoiID == INVALID_ID )
   {
      pgsPointOfInterest thePOI;
      if ( poi.GetID() == INVALID_ID )
      {
         GET_IFACE(IPointOfInterest,pPOI);
         thePOI = pPOI->GetPointOfInterest(segmentKey,poi.GetDistFromStart());
      }
      else
      {
         thePOI = poi;
      }

// This assert is not necessary. New pois are created by the FEM engine to deal with temporary POI's.
//      ATLASSERT( thePOI.GetID() != INVALID_ID );

      femPoiID = AddPointOfInterest(modelData,thePOI);
      ATLASSERT( 0 <= femPoiID );
   }

   HRESULT hr = results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   ATLASSERT( SUCCEEDED(hr) );

   Float64 delta = Dy;

   // we have the deflection relative to the release support locations
   if ( datum != pgsTypes::pddRelease )
   {
      // we want the deflection relative to a different datum
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IPointOfInterest,pPoi);

      Float64 left_support, right_support;
      if ( datum == pgsTypes::pddStorage )
      {
         GET_IFACE(IGirder,pGirder);
         pGirder->GetSegmentStorageSupportLocations(segmentKey,&left_support,&right_support);
      }
      else
      {
         left_support = pBridge->GetSegmentStartEndDistance(segmentKey);
         right_support = pBridge->GetSegmentEndEndDistance(segmentKey);
      }

      pgsPointOfInterest poiAtStart( pPoi->GetPointOfInterest(segmentKey,left_support) );
      ATLASSERT( 0 <= poiAtStart.GetID() );
   
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtStart);
      results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 start_delta_brg = Dy;

      Float64 Lg = pBridge->GetSegmentLength(segmentKey);
      pgsPointOfInterest poiAtEnd( pPoi->GetPointOfInterest(segmentKey,Lg-right_support) );
      ATLASSERT( 0 <= poiAtEnd.GetID() );
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtEnd);
      results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 end_delta_brg = Dy;

      Float64 delta_brg = LinInterp(poi.GetDistFromStart()-left_support,
                                    start_delta_brg,end_delta_brg,Lg);

      delta -= delta_brg;
   }

   *pDy = delta;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz)
{
   GetPrestressDeflection(poi,modelData,g_lcidTemporaryStrand,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      *pDy = 0;
      *pRz = 0;
      return;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // Get deflection at poi
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;
   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   Float64 delta_poi = Dy;

   // Get deflection at start bearing
   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   pgsPointOfInterest poi2( pPoi->GetPointOfInterest(segmentKey,start_end_size) );
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 start_delta_end_size = Dy;

   // Get deflection at end bearing
   Float64 L = pBridge->GetSegmentLength(segmentKey);
   Float64 end_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
   poi2 = pPoi->GetPointOfInterest(segmentKey,L-end_end_size);
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 end_delta_end_size = Dy;

   Float64 delta_brg = LinInterp(poi.GetDistFromStart()-start_end_size, start_delta_end_size,end_delta_end_size,L);

   Float64 delta = delta_poi - delta_brg;
   *pDy = delta;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bTempStrands = false;
   
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) && 
                       config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

      bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) && 
                      pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }

   Float64 Dcreep = 0;
   switch( deckType )
   {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
         (bTempStrands ? GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz) 
                       : GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz));
         break;

      case pgsTypes::sdtCompositeSIP:
         (bTempStrands ? GetCreepDeflection_SIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz) 
                       : GetCreepDeflection_SIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz));
         break;

      case pgsTypes::sdtNone:
         (bTempStrands ? GetCreepDeflection_NoDeck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz) 
                       : GetCreepDeflection_NoDeck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz));
         break;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastDeckInterval();


   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    pgsTypes::pddStorage, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    pgsTypes::pddStorage, &Dstraight, &Rstraight);

   Float64 Dtpsi,Rtpsi;
   GetPrestressDeflection( poi, initTempModelData, g_lcidTemporaryStrand, pgsTypes::pddStorage, &Dtpsi, &Rtpsi);

   Float64 Dps = Dharped + Dstraight + Dtpsi;
   Float64 Rps = Rharped + Rstraight + Rtpsi;

   Float64 Ct1;
   Float64 Ct2;
   Float64 Ct3;
   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc,     RgInc;

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
      Ct2 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,constructionRate);
      Ct3 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,constructionRate);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
      Ct2 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,constructionRate);
      Ct3 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,constructionRate);
   }


   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IBridge,pBridge);
      Float64 leftSupport = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 rightSupport = pBridge->GetSegmentEndEndDistance(segmentKey);
      Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

      GET_IFACE(IPointOfInterest,pPoi);
      pgsPointOfInterest poiLeft = pPoi->GetPointOfInterest(segmentKey,leftSupport);
      pgsPointOfInterest poiRight = pPoi->GetPointOfInterest(segmentKey,segmentLength - rightSupport);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_CIP_TempStrands(poiLeft, bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_CIP_TempStrands(poiRight,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      if ( bUseConfig )
      {
         GetGirderDeflectionForCamber(poiLeft,config,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,config,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      else
      {
         GetGirderDeflectionForCamber(poiLeft,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( bUseConfig )
      {
         Float64 Dharped, Rharped;
         Float64 Dstraight, Rstraight;
         Float64 Dti, Rti;
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidHarpedStrand,      pgsTypes::pddStorage, &Dharped,   &Rharped);
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidStraightStrand,    pgsTypes::pddStorage, &Dstraight, &Rstraight);
         GetPrestressDeflection( poiLeft, initTempModelData, g_lcidTemporaryStrand,   pgsTypes::pddStorage, &Dti, &Rti);
         D1 = Dharped + Dstraight + Dti;
         R1 = Rharped + Rstraight + Rti;

         GetPrestressDeflection( poiRight, initModelData,     g_lcidHarpedStrand,      pgsTypes::pddStorage, &Dharped,   &Rharped);
         GetPrestressDeflection( poiRight, initModelData,     g_lcidStraightStrand,    pgsTypes::pddStorage, &Dstraight, &Rstraight);
         GetPrestressDeflection( poiRight, initTempModelData, g_lcidTemporaryStrand,   pgsTypes::pddStorage, &Dti, &Rti);
         D2 = Dharped + Dstraight + Dti;
         R2 = Rharped + Rstraight + Rti;
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
         
         D1 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         R1 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         
         D2 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
         R2 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      *pDy = Ct1*(DgStorage + Dps);
      *pRz = Ct1*(RgStorage + Rps);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }

      return;
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      ATLASSERT(datum == pgsTypes::pddErected);

      // creep 2 - Immediately after diarphagm/temporary strands to deck
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize); // want the largest downward deflection
      Float64 Ddiaphragm = GetDeflection(castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,poi,bat, rtIncremental, false);
      Float64 Rdiaphragm = GetRotation(  castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,poi,bat, rtIncremental, false);

      Float64 Dtpsr,Rtpsr;
      GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, pgsTypes::pddErected, &Dtpsr, &Rtpsr);

      *pDy = (Ct3-Ct1)*(DgStorage + Dps) - (Ct3-Ct1)*(DgirderSupport + DpsSupport) + Ct2*(DgInc + Ddiaphragm + Dtpsr);
      *pRz = (Ct3-Ct1)*(RgStorage + Rps) - (Ct3-Ct1)*(DgirderSupport + DpsSupport) + Ct2*(RgInc + Rdiaphragm + Rtpsr);
   }
   else /*if ( creepPeriod == cpReleaseToDeck )*/
   {
      ATLASSERT(datum == pgsTypes::pddErected);
      Float64 D1, D2, R1, R2;
      GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate,datum,&D1,&R1);
      GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpDiaphragmToDeck,    constructionRate,datum,&D2,&R2);

      *pDy = D1 + D2;
      *pRz = R1 + R2;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   ATLASSERT( datum != pgsTypes::pddRelease ); // no creep at release
   ATLASSERT( creepPeriod == cpReleaseToDeck || creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   Float64 Dps, Rps; // measured relative to storage supports
   if ( bUseConfig )
   {
      Float64 Dharped, Rharped;
      GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    pgsTypes::pddStorage, &Dharped, &Rharped);

      Float64 Dstraight, Rstraight;
      GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    pgsTypes::pddStorage, &Dstraight, &Rstraight);

      Dps = Dharped + Dstraight;
      Rps = Rharped + Rstraight;
   }
   else
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
      Dps = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poi,bat,rtCumulative,false);
      Rps = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poi,bat,rtCumulative,false);
   }

   Float64 Ct1;
   Float64 Ct2;
   Float64 Ct3;
   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc,     RgInc;

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
      Ct2 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,constructionRate);
      Ct3 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,constructionRate);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
      Ct2 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,constructionRate);
      Ct3 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,constructionRate);
   }

   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IBridge,pBridge);
      Float64 leftSupport = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 rightSupport = pBridge->GetSegmentEndEndDistance(segmentKey);
      Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

      GET_IFACE(IPointOfInterest,pPoi);
      pgsPointOfInterest poiLeft = pPoi->GetPointOfInterest(segmentKey,leftSupport);
      pgsPointOfInterest poiRight = pPoi->GetPointOfInterest(segmentKey,segmentLength - rightSupport);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_CIP(poiLeft, bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_CIP(poiRight,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      if ( bUseConfig )
      {
         GetGirderDeflectionForCamber(poiLeft,config,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,config,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      else
      {
         GetGirderDeflectionForCamber(poiLeft,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( bUseConfig )
      {
         Float64 Dharped, Rharped;
         Float64 Dstraight, Rstraight;
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidHarpedStrand,      datum, &Dharped,   &Rharped);
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidStraightStrand,    datum, &Dstraight, &Rstraight);
         D1 = Dharped + Dstraight;
         R1 = Rharped + Rstraight;

         GetPrestressDeflection( poiRight, initModelData,     g_lcidHarpedStrand,      datum, &Dharped,   &Rharped);
         GetPrestressDeflection( poiRight, initModelData,     g_lcidStraightStrand,    datum, &Dstraight, &Rstraight);
         D2 = Dharped + Dstraight;
         R2 = Rharped + Rstraight;
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
         
         D1 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         R1 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         
         D2 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
         R2 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // creep deflection during storage, relative to storage supports
      *pDy = Ct1*(DgStorage + Dps);
      *pRz = Ct1*(RgStorage + Rps);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      ATLASSERT(datum == pgsTypes::pddErected);
      // (creep measured from storage supports at this poi) - (creep at final bearing locations measured from storage supports) + (creep due to incremental girder self-weight deflection)
      // |---------------------------------------------------------------------------------------------------------------------|
      //            |
      //            +- This is equal to the creep relative to the final bearing locations
      *pDy = (Ct3 - Ct1)*(DgStorage + Dps) - (Ct3 - Ct1)*(DgirderSupport + DpsSupport) + Ct2*DgInc;
      *pRz = (Ct3 - Ct1)*(RgStorage + Rps) - (Ct3 - Ct1)*(RgirderSupport + RpsSupport) + Ct2*RgInc;
   }
   else /*if ( creepPeriod == cpReleaseToDeck )*/
   {
      ATLASSERT(datum == pgsTypes::pddErected);
      Float64 D1, D2, R1, R2;
      GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate,datum,&D1,&R1);
      GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpDiaphragmToDeck,    constructionRate,datum,&D2,&R2);

      *pDy = D1 + D2;
      *pRz = R1 + R2;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    pgsTypes::pddStorage, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    pgsTypes::pddStorage, &Dstraight, &Rstraight);

   Float64 Dtpsi, Rtpsi;
   GetPrestressDeflection( poi, initTempModelData, g_lcidTemporaryStrand, pgsTypes::pddStorage, &Dtpsi, &Rtpsi);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dtpsr, Rtpsr;
   GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, pgsTypes::pddErected, &Dtpsr, &Rtpsr);

   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc, RgInc;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Dshearkey, Rshearkey;
   Float64 Dconstr, Rconstr;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;

   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

#if defined _DEBUG
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == castDeckIntervalIdx || *iter == compositeDeckIntervalIdx);
   }
#endif

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetShearKeyDeflection(poi,config,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetShearKeyDeflection(poi,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier); // includes sidewalk and overlays
   }
   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IBridge,pBridge);
      Float64 leftSupport = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 rightSupport = pBridge->GetSegmentEndEndDistance(segmentKey);
      Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

      GET_IFACE(IPointOfInterest,pPoi);
      pgsPointOfInterest poiLeft = pPoi->GetPointOfInterest(segmentKey,leftSupport);
      pgsPointOfInterest poiRight = pPoi->GetPointOfInterest(segmentKey,segmentLength - rightSupport);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_NoDeck_TempStrands(poiLeft, bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_NoDeck_TempStrands(poiRight,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      if ( bUseConfig )
      {
         GetGirderDeflectionForCamber(poiLeft,config,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,config,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      else
      {
         GetGirderDeflectionForCamber(poiLeft,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( bUseConfig )
      {
         Float64 Dharped, Rharped;
         Float64 Dstraight, Rstraight;
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidHarpedStrand,      datum, &Dharped,   &Rharped);
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidStraightStrand,    datum, &Dstraight, &Rstraight);
         D1 = Dharped + Dstraight;
         R1 = Rharped + Rstraight;

         GetPrestressDeflection( poiRight, initModelData,     g_lcidHarpedStrand,      datum, &Dharped,   &Rharped);
         GetPrestressDeflection( poiRight, initModelData,     g_lcidStraightStrand,    datum, &Dstraight, &Rstraight);
         D2 = Dharped + Dstraight;
         R2 = Rharped + Rstraight;
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
         
         D1 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         R1 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         
         D2 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
         R2 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate) 
                                : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(DgStorage + Dps + Dtpsi);
      *pRz = Ct1*(RgStorage + Rps + Rtpsi);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,   constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,     constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,   constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,     constructionRate);
      }

      ATLASSERT(datum == pgsTypes::pddErected);
      *pDy = (Ct3 - Ct1)*(DgStorage + Dps) - (Ct3 - Ct1)*(DgirderSupport + DpsSupport) + Ct2*(DgInc + Ddiaphragm + Duser1 + Dtpsr + Dconstr + Dshearkey);
      *pRz = (Ct3 - Ct1)*(RgStorage + Rps) - (Ct3 - Ct1)*(RgirderSupport + RpsSupport) + Ct2*(RgInc + Rdiaphragm + Ruser1 + Rtpsr + Rconstr + Rshearkey);
   }
   else
   {
      ATLASSERT(creepPeriod == cpDeckToFinal);
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,cpDeckToFinal,        constructionRate);
      }

      ATLASSERT(datum == pgsTypes::pddErected);
      *pDy = (Ct2 - Ct1)*(DgStorage + Dps) - (Ct2 - Ct1)*(DgirderSupport + DpsSupport) + (Ct4 - Ct3)*(DgInc + Ddiaphragm + Dtpsr + Duser1 + Dconstr + Dshearkey) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(RgStorage + Rps) - (Ct2 - Ct1)*(RgirderSupport + RpsSupport) + (Ct4 - Ct3)*(RgInc + Rdiaphragm + Rtpsr + Ruser1 + Rconstr + Rshearkey) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    pgsTypes::pddStorage, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    pgsTypes::pddStorage, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc, RgInc;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Dshearkey, Rshearkey;
   Float64 Dconstr, Rconstr;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;


   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

#if defined _DEBUG
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == castDeckIntervalIdx || *iter == compositeDeckIntervalIdx);
   }
#endif


   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetShearKeyDeflection(poi,config,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetShearKeyDeflection(poi,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IBridge,pBridge);
      Float64 leftSupport = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 rightSupport = pBridge->GetSegmentEndEndDistance(segmentKey);
      Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

      GET_IFACE(IPointOfInterest,pPoi);
      pgsPointOfInterest poiLeft = pPoi->GetPointOfInterest(segmentKey,leftSupport);
      pgsPointOfInterest poiRight = pPoi->GetPointOfInterest(segmentKey,segmentLength - rightSupport);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_NoDeck(poiLeft, bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_NoDeck(poiRight,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      if ( bUseConfig )
      {
         GetGirderDeflectionForCamber(poiLeft,config,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,config,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      else
      {
         GetGirderDeflectionForCamber(poiLeft,&D1,&R1,&D1E,&R1E,&DI1,&RI1);
         GetGirderDeflectionForCamber(poiRight,&D2,&R2,&D2E,&R2E,&DI2,&RI2);
      }
      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( bUseConfig )
      {
         Float64 Dharped, Rharped;
         Float64 Dstraight, Rstraight;
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidHarpedStrand,      datum, &Dharped,   &Rharped);
         GetPrestressDeflection( poiLeft, initModelData,     g_lcidStraightStrand,    datum, &Dstraight, &Rstraight);
         D1 = Dharped + Dstraight;
         R1 = Rharped + Rstraight;

         GetPrestressDeflection( poiRight, initModelData,     g_lcidHarpedStrand,      datum, &Dharped,   &Rharped);
         GetPrestressDeflection( poiRight, initModelData,     g_lcidStraightStrand,    datum, &Dstraight, &Rstraight);
         D2 = Dharped + Dstraight;
         R2 = Rharped + Rstraight;
      }
      else
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
         
         D1 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         R1 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiLeft,bat,rtCumulative,false);
         
         D2 = GetDeflection(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
         R2 = GetRotation(storageIntervalIdx,pgsTypes::pftPretension,poiRight,bat,rtCumulative,false);
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate)
                                : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(DgStorage + Dps);
      *pRz = Ct1*(RgStorage + Rps);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,   constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,     constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,   constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,     constructionRate);
      }

      ATLASSERT(datum == pgsTypes::pddErected);
      *pDy = (Ct3 - Ct1)*(DgStorage + Dps) - (Ct3 - Ct1)*(DgirderSupport + DpsSupport) + Ct2*(DgInc + Ddiaphragm + Duser1 + Dconstr + Dshearkey);
      *pRz = (Ct3 - Ct1)*(RgStorage + Rps) - (Ct3 - Ct1)*(RgirderSupport + RpsSupport) + Ct2*(RgInc + Rdiaphragm + Ruser1 + Rconstr + Rshearkey);
   }
   else
   {
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,cpDeckToFinal,        constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(DgStorage + Dps) - (Ct2 - Ct1)*(DgirderSupport + DpsSupport) + (Ct4 - Ct3)*(DgInc + Ddiaphragm + Duser1 + Dconstr + Dshearkey) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(RgStorage + Rps) - (Ct2 - Ct1)*(RgirderSupport + RpsSupport) + (Ct4 - Ct3)*(RgInc + Rdiaphragm + Ruser1 + Rconstr + Rshearkey) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   Float64 Dy, Dz;
   GetDCamberForGirderSchedule( poi, initModelData, initTempModelData, releaseTempModelData, time, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamber(poi,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      // apply camber multipliers
      CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,ICamber::cpDeckToFinal,time,pgsTypes::pddErected,&Dcreep3,&Rcreep3 );
      *pDy += cm.CreepFactor * Dcreep3;
      *pRz += cm.CreepFactor * Rcreep3;
   }
}

void CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   Float64 Dy, Dz;
   GetDCamberForGirderSchedule( poi, config, initModelData, initTempModelData, releaseTempModelData, time, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamber(poi,config,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      // apply camber multipliers
      CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,config,ICamber::cpDeckToFinal,time,pgsTypes::pddErected,&Dcreep3,&Rcreep3 );
      *pDy += cm.CreepFactor * Dcreep3;
      *pRz += cm.CreepFactor * Rcreep3;
   }
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GDRCONFIG dummy_config;
   GetDCamberForGirderSchedule(poi,false,dummy_config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GetDCamberForGirderSchedule(poi,true,config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bTempStrands = true;
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) && 
                      config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrand = pSegmentData->GetStrandData(segmentKey);

      bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) && 
                      pStrand->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   Float64 D, R;

   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
   case pgsTypes::sdtCompositeSIP:
      (bTempStrands ? GetD_Deck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : 
                      GetD_Deck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   case pgsTypes::sdtNone:
      (bTempStrands ? GetD_NoDeck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : 
                      GetD_NoDeck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   default:
      ATLASSERT(false);
   }

   *pDy = D;
   *pRz = R;
}

void CAnalysisAgentImp::GetD_Deck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dtpsi, Dtpsr, DgStorage, DgErected, DgInc, Dcreep1, Ddiaphragm, Dcreep2, Dshearkey, Dconstr;
   Float64 Rps, Rtpsi, Rtpsr, RgStorage, RgErected, RgInc, Rcreep1, Rdiaphragm, Rcreep2, Rshearkey, Rconstr;
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, pgsTypes::pddErected, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi, config, pgsTypes::pddErected, &Dtpsi, &Rtpsi );
      GetReleaseTempPrestressDeflection( poi, config, &Dtpsr, &Rtpsr );
      GetGirderDeflectionForCamber( poi, config, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, &Dcreep1, &Rcreep1);
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,config,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, &Dcreep2, &Rcreep2);
   }
   else
   {
      GetPrestressDeflection( poi, pgsTypes::pddErected, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi, pgsTypes::pddErected, &Dtpsi, &Rtpsi );
      GetReleaseTempPrestressDeflection( poi, &Dtpsr, &Rtpsr );
      GetGirderDeflectionForCamber( poi, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,&Dconstr,&Rconstr);
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, &Dcreep2, &Rcreep2 );
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

   Float64 D1 = cm.ErectionFactor*(DgErected + Dps + Dtpsi);
   Float64 D2 = D1 + cm.CreepFactor*Dcreep1;
   Float64 D3 = D2 + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) + cm.ErectionFactor*Dtpsr;
   Float64 D4 = D3 + cm.CreepFactor*Dcreep2;
   *pDy = D4;

   Float64 R1 = cm.ErectionFactor*(RgErected + Rps + Rtpsi);
   Float64 R2 = R1 + cm.CreepFactor*Rcreep1;
   Float64 R3 = R2 + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr) +  cm.ErectionFactor*Rtpsr;
   Float64 R4 = R3 + cm.CreepFactor*Rcreep2;

   *pRz = R4;
}

void CAnalysisAgentImp::GetD_Deck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, DgStorage, DgErected, DgInc, Dcreep, Ddiaphragm, Dshearkey, Dconstr;
   Float64 Rps, RgStorage, RgErected, RgInc, Rcreep, Rdiaphragm, Rshearkey, Rconstr;

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, pgsTypes::pddErected, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,config,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDeck, constructionRate, pgsTypes::pddErected, &Dcreep, &Rcreep);
   }
   else
   {
      GetPrestressDeflection( poi, pgsTypes::pddErected, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,&Dconstr,&Rconstr);
      GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate, pgsTypes::pddErected, &Dcreep, &Rcreep );
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

   *pDy = cm.ErectionFactor*(DgErected + Dps) + cm.CreepFactor*Dcreep + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) ;
   *pRz = cm.ErectionFactor*(RgErected + Rps) + cm.CreepFactor*Rcreep + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr);
}

void CAnalysisAgentImp::GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, Dtpsi, Dtpsr, DgStorage, DgErected, DgInc, Dcreep1, Ddiaphragm, Dshearkey, Dcreep2, Dconstr, Duser1;
   Float64 Rps, Rtpsi, Rtpsr, RgStorage, RgErected, RgInc, Rcreep1, Rdiaphragm, Rshearkey, Rcreep2, Rconstr, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

#if defined _DEBUG
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == castDeckIntervalIdx || *iter == compositeDeckIntervalIdx);
   }
#endif

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, pgsTypes::pddErected, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,config,pgsTypes::pddErected,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,config,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,config,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate,pgsTypes::pddErected,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi,config,&Ddiaphragm,&Rdiaphragm );
      GetShearKeyDeflection(poi,config,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate,pgsTypes::pddErected,&Dcreep2,&Rcreep2 );
   }
   else
   {
      GetPrestressDeflection( poi, pgsTypes::pddErected, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,pgsTypes::pddErected,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,&DgStorage,&RgStorage,&DgErected,&RgErected,&DgInc,&RgInc );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate,pgsTypes::pddErected,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected,&Dcreep2, &Rcreep2 );
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

   Float64 D1 = cm.ErectionFactor*(DgErected + Dps + Dtpsi);
   Float64 D2 = D1 + cm.CreepFactor*Dcreep1;
   Float64 D3 = D2 + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) + cm.ErectionFactor*Dtpsr + cm.SlabUser1Factor*Duser1;
   Float64 D4 = D3 + cm.CreepFactor*Dcreep2;
   *pDy = D4;

   Float64 R1 = cm.ErectionFactor*(RgErected + Rps + Rtpsi);
   Float64 R2 = R1 + cm.CreepFactor*Rcreep1;
   Float64 R3 = R2 + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr) + cm.ErectionFactor*Rtpsr + cm.SlabUser1Factor*Ruser1;
   Float64 R4 = R3 + cm.CreepFactor*Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetD_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, DgStorage, DgErected, DgInc, Dcreep1, Ddiaphragm, Dshearkey, Dcreep2, Dconstr, Duser1;
   Float64 Rps, RgStorage, RgErected, RgInc, Rcreep1, Rdiaphragm, Rshearkey, Rcreep2, Rconstr, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

#if defined _DEBUG
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == castDeckIntervalIdx || *iter == compositeDeckIntervalIdx);
   }
#endif

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, pgsTypes::pddErected, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,config,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,config,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, &Dcreep2, &Rcreep2 );
   }
   else
   {
      GetPrestressDeflection( poi, pgsTypes::pddErected, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetShearKeyDeflection(poi,&Dshearkey,&Rshearkey);
      GetConstructionLoadDeflection(poi,&Dconstr,&Rconstr);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, &Dcreep2, &Rcreep2 );
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliers(poi.GetSegmentKey());

   Float64 D1 = cm.ErectionFactor*(DgErected + Dps);
   Float64 D2 = D1 + cm.CreepFactor*Dcreep1;
   Float64 D3 = D2 + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) + cm.SlabUser1Factor*Duser1;
   Float64 D4 = D3 + cm.CreepFactor*Dcreep2;
   *pDy = D4;

   Float64 R1 = cm.ErectionFactor*(RgErected + Rps);
   Float64 R2 = R1 + cm.CreepFactor*Rcreep1;
   Float64 R3 = R2 + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr) + cm.SlabUser1Factor*Duser1;
   Float64 R4 = R3 + cm.CreepFactor*Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetDesignSlabDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;

   *pDy = beam.ComputeDeflection(x);
   *pRz = beam.ComputeRotation(x);
}

void CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;

   *pDy = beam.ComputeDeflection(x);
   *pRz = beam.ComputeRotation(x);
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey,LoadingEvent le)
{
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Fc;

   switch( le )
   {
   case ICamber::leRelease:
      Fc = pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx);
      break;

   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = pMaterial->GetSegmentFc(segmentKey,castDeckIntervalIdx);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config,LoadingEvent le)
{
   Float64 Fc;

   switch( le )
   {
   case ICamber::leRelease:
   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = config.Fci;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

ICamber::LoadingEvent CAnalysisAgentImp::GetLoadingEvent(CreepPeriod creepPeriod)
{
   LoadingEvent le;
   switch( creepPeriod )
   {
   case cpReleaseToDiaphragm:
   case cpReleaseToDeck:
   case cpReleaseToFinal:
      le = leRelease;
      break;

   case cpDiaphragmToDeck:
   case cpDiaphragmToFinal:
      le = leDiaphragm;
      break;

   case cpDeckToFinal:
      le = leDeck;
      break;

   default:
      ATLASSERT(false);
   }

   return le;
}

/////////////////////////////////////////////////////////////////////////
// IContraflexurePoints
void CAnalysisAgentImp::GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints)
{
   m_pGirderModelManager->GetContraflexurePoints(spanKey,cfPoints,nPoints);
}

/////////////////////////////////////////////////////////////////////////
// IContinuity
bool CAnalysisAgentImp::IsContinuityFullyEffective(const CGirderKey& girderKey)
{
   // The continuity of a girder line is fully effective if the continuity stress level
   // at all continuity piers is compressive.
   //
   // Continuity is fully effective at piers where a precast segment spans
   // across the pier.
   //
   // Check for continuity at the start and end of each girder group (that is where the continuity diaphragms are located)
   // If the continuity is not effective at any continuity diaphragm, the contininuity is not effective for the full girder line

   bool bContinuous = false;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      for (int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         PierIndexType pierIdx = pGroup->GetPierIndex(endType);

         bool bContinuousLeft, bContinuousRight;
         pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

         bool bIntegralLeft, bIntegralRight;
         pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

         if ( (bContinuousLeft && bContinuousRight) || (bIntegralLeft && bIntegralRight)  )
         {
            Float64 fb = GetContinuityStressLevel(pierIdx,thisGirderKey);
            bContinuous = (0 <= fb ? false : true);
         }
         else
         {
            bContinuous = false;
         }

         if ( bContinuous )
         {
            return bContinuous;
         }
      }
   }

   return bContinuous;
}

Float64 CAnalysisAgentImp::GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   ATLASSERT(girderKey.girderIndex != INVALID_INDEX);

   // for evaluation of LRFD 5.14.1.4.5 - Degree of continuity

   // If we are in simple span analysis mode, there is no continuity
   // no matter what the boundary conditions are
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   if ( analysisType == pgsTypes::Simple )
   {
      return 0.0;
   }

   // check the boundary conditions
   GET_IFACE(IBridge,pBridge);
   bool bIntegralLeft,bIntegralRight;
   pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);
   bool bContinuousLeft,bContinuousRight;
   pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

   if ( !bIntegralLeft && !bIntegralRight && !bContinuousLeft && !bContinuousRight )
   {
      return 0.0;
   }

   // how does this work for segments that span over a pier? I don't
   // think that 5.14.1.4.5 applies to spliced girder bridges unless we are looking
   // at a pier that is between groups
   ATLASSERT(pBridge->IsBoundaryPier(pierIdx));

   GroupIndexType backGroupIdx, aheadGroupIdx;
   pBridge->GetGirderGroupIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);

#if defined _DEBUG
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   if ( pPier->HasCantilever() )
   {
      if ( pPier->GetIndex() == 0 )
      {
         ATLASSERT(backGroupIdx == INVALID_INDEX);
         ATLASSERT(aheadGroupIdx == 0);
      }
      else
      {
         ATLASSERT(pPier->GetIndex() == pIBridgeDesc->GetPierCount()-1);
         ATLASSERT(backGroupIdx == pIBridgeDesc->GetGirderGroupCount()-1);
         ATLASSERT(aheadGroupIdx == INVALID_INDEX);
      }
   }
   else
   {
      if ( pPier->GetIndex() == 0 )
      {
         ATLASSERT(backGroupIdx == INVALID_INDEX);
         ATLASSERT(aheadGroupIdx == 0);
      }
      else if ( pPier->GetIndex() == pIBridgeDesc->GetPierCount()-1 )
      {
         ATLASSERT(backGroupIdx == pIBridgeDesc->GetGirderGroupCount()-1);
         ATLASSERT(aheadGroupIdx == INVALID_INDEX);
      }
      else
      {
         ATLASSERT(backGroupIdx == aheadGroupIdx-1);
      }
   }
#endif

   GirderIndexType gdrIdx = girderKey.girderIndex;

   // computes the stress at the bottom of the girder on each side of the pier
   // returns the greater of the two values
   GET_IFACE(IPointOfInterest,pPoi);

   // deal with girder index when there are different number of girders in each group
   GirderIndexType prev_group_gdr_idx = gdrIdx;
   GirderIndexType next_group_gdr_idx = gdrIdx;

   if ( backGroupIdx != INVALID_INDEX )
   {
      prev_group_gdr_idx = Min(gdrIdx,pBridge->GetGirderCount(backGroupIdx)-1);
   }

   if ( aheadGroupIdx != INVALID_INDEX )
   {
      next_group_gdr_idx = Min(gdrIdx,pBridge->GetGirderCount(aheadGroupIdx)-1);
   }

   CollectionIndexType nPOI = 0;
   pgsPointOfInterest vPOI[2];

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   IntervalIndexType continuity_interval[2];

   EventIndexType leftContinuityEventIndex, rightContinuityEventIndex;
   pBridge->GetContinuityEventIndex(pierIdx,&leftContinuityEventIndex,&rightContinuityEventIndex);


   // get poi at cl bearing at end of prev group
   if ( backGroupIdx != INVALID_INDEX )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(backGroupIdx,prev_group_gdr_idx);
      CSegmentKey thisSegmentKey(backGroupIdx,prev_group_gdr_idx,nSegments-1);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(thisSegmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = pIntervals->GetInterval(leftContinuityEventIndex);
      nPOI++;
   }

   // get poi at cl bearing at start of next group
   if ( aheadGroupIdx != INVALID_INDEX )
   {
      CSegmentKey thisSegmentKey(aheadGroupIdx,next_group_gdr_idx,0);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(thisSegmentKey,POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = pIntervals->GetInterval(rightContinuityEventIndex);
      nPOI++;
   }

   Float64 f[2] = {0,0};
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];
      ATLASSERT( 0 <= poi.GetID() );

      pgsTypes::BridgeAnalysisType bat = pgsTypes::ContinuousSpan;

      Float64 fbConstruction, fbSlab, fbSlabPad, fbTrafficBarrier, fbSidewalk, fbOverlay, fbUserDC, fbUserDW, fbUserLLIM, fbLLIM;

      Float64 fTop,fBottom;

      if ( continuity_interval[i] == castDeckIntervalIdx )
      {
         GetStress(castDeckIntervalIdx,pgsTypes::pftSlab,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlab = fBottom;

         GetStress(castDeckIntervalIdx,pgsTypes::pftSlabPad,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlabPad = fBottom;

         GetStress(castDeckIntervalIdx,pgsTypes::pftConstruction,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbConstruction = fBottom;
      }
      else
      {
         fbSlab         = 0;
         fbSlabPad      = 0;
         fbConstruction = 0;
      }

      GetStress(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbTrafficBarrier = fBottom;

      GetStress(railingSystemIntervalIdx,pgsTypes::pftSidewalk,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbSidewalk = fBottom;

      if ( overlayIntervalIdx != INVALID_INDEX )
      {
         GetStress(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbOverlay = fBottom;
      }
      else
      {
         fbOverlay = 0;
      }

      fbUserDC = 0;
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      std::vector<IntervalIndexType> vUserDCIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pgsTypes::pftUserDC);
      BOOST_FOREACH(IntervalIndexType intervalIdx,vUserDCIntervals)
      {
         GetStress(intervalIdx,pgsTypes::pftUserDC,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbUserDC += fBottom;
      }

      fbUserDW = 0;
      std::vector<IntervalIndexType> vUserDWIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pgsTypes::pftUserDW);
      BOOST_FOREACH(IntervalIndexType intervalIdx,vUserDWIntervals)
      {
         GetStress(intervalIdx,pgsTypes::pftUserDW,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbUserDW += fBottom;
      }

      GetStress(compositeDeckIntervalIdx,pgsTypes::pftUserLLIM,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbUserLLIM = fBottom;

      Float64 fTopMin,fTopMax,fBotMin,fBotMax;
      GetCombinedLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltDesign,poi,bat,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTopMin,&fTopMax,&fBotMin,&fBotMax);
      fbLLIM = fBotMin; // greatest compression

      fBottom = fbConstruction + fbSlab + fbSlabPad + fbTrafficBarrier + fbSidewalk + fbOverlay + fbUserDC + fbUserDW + 0.5*(fbUserLLIM + fbLLIM);

      f[i] = fBottom;
   }

   return (nPOI == 1 ? f[0] : Max(f[0],f[1]));
}

/////////////////////////////////////////////////
// IPrecompressedTensileZone
void CAnalysisAgentImp::IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ)
{
   IsInPrecompressedTensileZone(poi,limitState,topStressLocation,botStressLocation,NULL,pbTopPTZ,pbBotPTZ);
}

void CAnalysisAgentImp::IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ)
{
   bool bTopGirder = IsGirderStressLocation(topStressLocation);
   bool bBotGirder = IsGirderStressLocation(botStressLocation);
   if ( bTopGirder && bBotGirder )
   {
      IsGirderInPrecompressedTensileZone(poi,limitState,pConfig,pbTopPTZ,pbBotPTZ);
   }
   else if ( !bTopGirder && !bBotGirder )
   {
      IsDeckInPrecompressedTensileZone(poi,limitState,pbTopPTZ,pbBotPTZ);
   }
   else
   {
      bool bDummy;
      if ( bTopGirder )
      {
         IsGirderInPrecompressedTensileZone(poi,limitState,pConfig,pbTopPTZ,&bDummy);
      }
      else
      {
         IsDeckInPrecompressedTensileZone(poi,limitState,pbTopPTZ,&bDummy);
      }

      if ( bBotGirder )
      {
         IsGirderInPrecompressedTensileZone(poi,limitState,pConfig,&bDummy,pbBotPTZ);
      }
      else
      {
         IsDeckInPrecompressedTensileZone(poi,limitState,&bDummy,pbBotPTZ);
      }
   }
}

bool CAnalysisAgentImp::IsDeckPrecompressed(const CGirderKey& girderKey)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   if ( compositeDeckIntervalIdx == INVALID_INDEX )
   {
      return false; // this happens when there is not a deck
   }

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
      if ( compositeDeckIntervalIdx <= stressTendonIntervalIdx )
      {
         // this tendon is stressed after the deck is composite so the deck is considered precompressed
         return true;
      }
   }

   // didn't find a tendon that is stressed after the deck is composite... deck not precompressed
   return false;
}

/////////////////////////////////////////////////
// IReactions
void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright)
{
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   std::vector<Float64> Rleft, Rright;
   GetSegmentReactions(segmentKeys,intervalIdx,pfType,bat,resultsType,&Rleft,&Rright);

   ATLASSERT(Rleft.size()  == 1);
   ATLASSERT(Rright.size() == 1);

   *pRleft  = Rleft.front();
   *pRright = Rright.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright)
{
   pRleft->reserve(segmentKeys.size());
   pRright->reserve(segmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(segmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 Rleft(0), Rright(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,pfType,resultsType,&Rleft,&Rright);
      }
      pRleft->push_back(Rleft);
      pRright->push_back(Rright);
   }
}

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright)
{
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   std::vector<Float64> Rleft, Rright;
   GetSegmentReactions(segmentKeys,intervalIdx,comboType,bat,resultsType,&Rleft,&Rright);

   ATLASSERT(Rleft.size()  == 1);
   ATLASSERT(Rright.size() == 1);

   *pRleft  = Rleft.front();
   *pRright = Rright.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright)
{
   pRleft->reserve(segmentKeys.size());
   pRright->reserve(segmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(segmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 Rleft(0), Rright(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,comboType,resultsType,&Rleft,&Rright);
      }
      pRleft->push_back(Rleft);
      pRright->push_back(Rright);
   }
}

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax)
{
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   std::vector<Float64> RleftMin,RleftMax,RrightMin,RrightMax;
   GetSegmentReactions(segmentKeys,intervalIdx,limitState,bat,&RleftMin,&RleftMax,&RrightMin,&RrightMax);
   *pRleftMin  = RleftMin.front();
   *pRleftMax  = RleftMax.front();
   *pRrightMin = RrightMin.front();
   *pRrightMax = RrightMax.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRleftMin,std::vector<Float64>* pRleftMax,std::vector<Float64>* pRrightMin,std::vector<Float64>* pRrightMax)
{
   pRleftMin->reserve(segmentKeys.size());
   pRleftMax->reserve(segmentKeys.size());
   pRrightMin->reserve(segmentKeys.size());
   pRrightMax->reserve(segmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(segmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 RleftMin(0), RleftMax(0), RrightMin(0), RrightMax(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,limitState,&RleftMin,&RleftMax,&RrightMin,&RrightMax);
      }

      pRleftMin->push_back(RleftMin);
      pRleftMax->push_back(RleftMax);
      pRrightMin->push_back(RrightMin);
      pRrightMax->push_back(RrightMax);
   }
}

Float64 CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> reactions(GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType));

   ATLASSERT(reactions.size() == 1);

   return reactions.front();
}

std::vector<Float64> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      std::vector<Float64> results;
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(girderKey,intervalIdx);
         if ( resultsType == rtCumulative )
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
            results.resize(vSupports.size(),0);
            for ( IntervalIndexType iIdx = erectionIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<Float64> vReactions = GetReaction(girderKey,vSupports,iIdx,strLoadingName,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),vReactions.begin(),results.begin(),std::plus<Float64>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
            }
            else
            {
               results.resize(vSupports.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vSupports.size(),0);
      }
      return results;
   }
   else
   {
      return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType);
   }
}

Float64 CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> reactions(GetReaction(girderKey,vSupports,intervalIdx,comboType,bat,resultsType));

   ATLASSERT(reactions.size() == 1);

   return reactions.front();
}

std::vector<Float64> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      std::vector<Float64> results;
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE(ILosses,pLosses);
         ComputeTimeDependentEffects(girderKey,intervalIdx);
         if ( resultsType == rtCumulative )
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
            results.resize(vSupports.size(),0);
            for ( IntervalIndexType iIdx = erectionIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<Float64> vReactions = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
                  std::transform(results.begin(),results.end(),vReactions.begin(),results.begin(),std::plus<Float64>());
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
            }
            else
            {
               results.resize(vSupports.size(),0);
            }
         }
      }
      else
      {
         results.resize(vSupports.size(),0);
      }
      return results;
   }

   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      std::vector<Float64> reactions(vSupports.size(),0.0);
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<Float64> reaction = GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType);
         std::transform(reactions.begin(),reactions.end(),reaction.begin(),reactions.begin(),std::plus<Float64>());
      }
      return reactions;
   }
   else
   {
      return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,comboType,bat,resultsType);
   }
}

void CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> Rmin, Rmax;
   GetReaction(girderKey,vSupports,intervalIdx,limitState,bat,bIncludeImpact,&Rmin,&Rmax);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax)
{
   m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,limitState,bat,bIncludeImpact,pRmin,pRmax);
}

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;
   std::vector<AxleConfiguration> MinAxleConfig;
   std::vector<AxleConfiguration> MaxAxleConfig;

   GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,pMinAxleConfig ? &MinAxleConfig : NULL,pMaxAxleConfig ? &MaxAxleConfig : NULL);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
   if ( pMinAxleConfig )
   {
      *pMinAxleConfig = MinAxleConfig.front();
   }
   
   if ( pMaxAxleConfig )
   {
      *pMaxAxleConfig = MaxAxleConfig.front();
   }
}

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;
   std::vector<VehicleIndexType> vMinVehIdx;
   std::vector<VehicleIndexType> vMaxVehIdx;

   GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&vMinVehIdx,&vMaxVehIdx);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
   if ( pMinVehIdx )
   {
      *pMinVehIdx = vMinVehIdx.front();
   }
   
   if ( pMaxVehIdx )
   {
      *pMaxVehIdx = vMaxVehIdx.front();
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx,std::vector<VehicleIndexType>* pMaxVehIdx)
{
   m_pGirderModelManager->GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinVehIdx,pMaxVehIdx);
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;
   std::vector<Float64> Tmin;
   std::vector<Float64> Tmax;
   std::vector<VehicleIndexType> vMinVehIdx;
   std::vector<VehicleIndexType> vMaxVehIdx;

   GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&Tmin,&Tmax,&vMinVehIdx,&vMaxVehIdx);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();

   *pTmin = Tmin.front();
   *pTmax = Tmax.front();

   if ( pMinVehIdx )
   {
      *pMinVehIdx = vMinVehIdx.front();
   }
   
   if ( pMaxVehIdx )
   {
      *pMaxVehIdx = vMaxVehIdx.front();
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx,std::vector<VehicleIndexType>* pMaxVehIdx)
{
   m_pGirderModelManager->GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pTmin,pTmax,pMinVehIdx,pMaxVehIdx);
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;

   GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,&Rmin,&Rmax);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,pRmin,pRmax);
}

/////////////////////////////////////////////////
// IBearingDesign
std::vector<PierIndexType> CAnalysisAgentImp::GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   std::vector<PierIndexType> vPiers;

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPierIdx = (girderKey.groupIndex == INVALID_INDEX ? 0 : pBridge->GetGirderGroupStartPier(girderKey.groupIndex));
   PierIndexType endPierIdx   = (girderKey.groupIndex == INVALID_INDEX ? nPiers-1 : pBridge->GetGirderGroupEndPier(girderKey.groupIndex));
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         pgsTypes::BoundaryConditionType bcType = pBridge->GetBoundaryConditionType(pierIdx);
         if ( analysisType == pgsTypes::Simple ||
              pBridge->HasCantilever(pierIdx) || 
              bcType == pgsTypes::bctHinge || bcType == pgsTypes::bctRoller ||
              (bcType == pgsTypes::bctIntegralAfterDeckHingeAhead && pierIdx == startPierIdx) ||
              (bcType == pgsTypes::bctIntegralBeforeDeckHingeAhead && pierIdx == startPierIdx) ||
              (bcType == pgsTypes::bctIntegralAfterDeckHingeBack && pierIdx == endPierIdx) ||
              (bcType == pgsTypes::bctIntegralBeforeDeckHingeBack && pierIdx == endPierIdx) )
         {
            vPiers.push_back(pierIdx);
         }
         else
         {
            // we have a boundary pier without final bearing reactions...
            // if the interval in questions is before continuity is made, then
            // we can assume there is some type of bearing
            GET_IFACE(IIntervals,pIntervals);
            EventIndexType leftEventIdx, rightEventIdx;
            pBridge->GetContinuityEventIndex(pierIdx,&leftEventIdx,&rightEventIdx);
            EventIndexType eventIdx = (pierIdx == startPierIdx ? rightEventIdx : leftEventIdx);
            IntervalIndexType continuityIntervalIdx = pIntervals->GetInterval(eventIdx);
            if ( intervalIdx < continuityIntervalIdx )
            {
               vPiers.push_back(pierIdx);
            }
         }
      }
      else
      {
         ATLASSERT( pBridge->IsInteriorPier(pierIdx) ); // if not boundary, must be interior
         ATLASSERT( analysisType == pgsTypes::Continuous); // we only have InteriorPiers in spliced girder analysis and we only use continuous analysis mode
         pgsTypes::PierSegmentConnectionType connType = pBridge->GetPierSegmentConnectionType(pierIdx);

         // assume that any connection with a CIP closure joint uses a detail similar to WSDOT Type C connection
         // (cast in place hinge)
         // Integral segments are continuous over the pier but the diaphragm is cast around the segment making it
         // integral with the substructure
         //
         // The only connection type that is assumed to be sitting on a bearing is a continuous segment
         if ( connType == pgsTypes::psctContinuousSegment )
         {
            vPiers.push_back(pierIdx);
         }
         else if ( connType == pgsTypes::psctContinousClosureJoint || connType == pgsTypes::psctIntegralClosureJoint )
         {
            GET_IFACE(IBridgeDescription,pIBridgeDesc);
            const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
            const CClosureJointData* pClosure = pPier->GetClosureJoint(0); // same closure for all girders
            const CTemporarySupportData* pTS = pClosure->GetTemporarySupport();
            ATLASSERT(pTS->GetConnectionType() == pgsTypes::tsctClosureJoint);


            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(pTS->GetIndex());
            if ( intervalIdx < tsRemovalIntervalIdx )
            {
               vPiers.push_back(pierIdx);
            }
         }
      }
   }
   return vPiers;
}

Float64 CAnalysisAgentImp::GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      // Pretension and primary post-tension are internal self equilibriating forces
      // they don't cause external reactions
      return 0.0;
   }
   else
   {
      return m_pGirderModelManager->GetBearingProductReaction(intervalIdx,location,pfType,bat,resultsType);
   }
}

void CAnalysisAgentImp::GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx)
{
   m_pGirderModelManager->GetBearingLiveLoadReaction(intervalIdx,location,llType,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pTmin,pTmax,pMinVehIdx,pMaxVehIdx);
}

void CAnalysisAgentImp::GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx)
{
   m_pGirderModelManager->GetBearingLiveLoadRotation(intervalIdx,location,llType,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pRmin,pRmax,pMinVehIdx,pMaxVehIdx);
}

Float64 CAnalysisAgentImp::GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   Float64 R = 0;
   if ( comboType == lcPS )
   {
      std::vector<pgsTypes::ProductForceType> vpfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      BOOST_FOREACH(pgsTypes::ProductForceType pfType,vpfTypes)
      {
         Float64 r = GetBearingProductReaction(intervalIdx,location,pfType,bat,resultsType);
         R += r;
      }
   }
   else
   {
      R = m_pGirderModelManager->GetBearingCombinedReaction(intervalIdx,location,comboType,bat,resultsType);
   }
   return R;
}

void CAnalysisAgentImp::GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                           Float64* pRmin,Float64* pRmax)
{
   m_pGirderModelManager->GetBearingCombinedLiveLoadReaction(intervalIdx,location,llType,bat,bIncludeImpact,pRmin,pRmax);
}

void CAnalysisAgentImp::GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                     Float64* pRmin,Float64* pRmax)
{
   m_pGirderModelManager->GetBearingLimitStateReaction(intervalIdx,location,limitState,bat,bIncludeImpact,pRmin,pRmax);
}

Float64 CAnalysisAgentImp::GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   Float64 fc = (intervalIdx < erectionIntervalIdx ? config.Fci : config.Fc);

   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Ix          = pSectProp->GetIx(intervalIdx,poi);
   Float64 Ix_adjusted = pSectProp->GetIx(intervalIdx,poi,fc);

   GET_IFACE(IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(poi.GetSegmentKey(),intervalIdx);
   Float64 Ec_adjusted = (config.bUserEc ? config.Ec : pMaterials->GetEconc(fc,pMaterials->GetSegmentStrengthDensity(poi.GetSegmentKey()),
                                                                               pMaterials->GetSegmentEccK1(poi.GetSegmentKey()),
                                                                               pMaterials->GetSegmentEccK2(poi.GetSegmentKey())));

   Float64 EI = Ec*Ix;
   Float64 EI_adjusted = Ec_adjusted * Ix_adjusted;

   Float64 k = (IsZero(EI_adjusted) ? 0 : EI/EI_adjusted);

   return k;
}

void CAnalysisAgentImp::ComputeTimeDependentEffects(const CGirderKey& girderKey,IntervalIndexType intervalIdx)
{
   // Getting the timestep loss results, causes the creep, shrinkage, relaxation, and prestress forces
   // to be added to the LBAM model...
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      GET_IFACE(ILosses,pLosses);
      GET_IFACE(IPointOfInterest,pPoi);

      pgsPointOfInterest poi( pPoi->GetPointOfInterest(CSegmentKey(girderKey,0),0.0) );
      ATLASSERT(poi.GetID() != INVALID_ID);
      pLosses->GetLossDetails(poi,intervalIdx);
   }
}

void CAnalysisAgentImp::IsDeckInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,bool* pbTopPTZ,bool* pbBotPTZ)
{
   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      // if there is no deck, the deck can't be in the PTZ
      *pbTopPTZ = false;
      *pbBotPTZ = false;
      return;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Get the stress when the bridge is in service (that is when live load is applied)
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Tensile stresses are greatest at the top of the deck using the minimum model in
   // Envelope mode. In all other modes, Min/Max are the same
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 fMin[2], fMax[2];
   GetStress(serviceLoadIntervalIdx,limitState,poi,bat,false/*without prestress*/,pgsTypes::TopDeck,   &fMin[TOP],&fMax[TOP]);
   GetStress(serviceLoadIntervalIdx,limitState,poi,bat,false/*without prestress*/,pgsTypes::BottomDeck,&fMin[BOT],&fMax[BOT]);

   // The section is in tension, does the prestress cause compression?
   Float64 fPreTension[2];
   GetStress(serviceLoadIntervalIdx,poi,pgsTypes::TopDeck,pgsTypes::BottomDeck,false/*don't include live load*/,&fPreTension[TOP],&fPreTension[BOT]);

   Float64 fPostTension[2];
   GetStress(serviceLoadIntervalIdx,pgsTypes::pftPostTensioning,poi,bat,rtCumulative,pgsTypes::TopDeck,pgsTypes::BottomDeck,&fPostTension[TOP],&fPostTension[BOT]);

   Float64 fPS[2];
   fPS[TOP] = fPreTension[TOP] + fPostTension[TOP];
   fPS[BOT] = fPreTension[BOT] + fPostTension[BOT];

   *pbTopPTZ = 0 < fMax[TOP] && fPS[TOP] < 0 ? true : false;
   *pbBotPTZ = 0 < fMax[BOT] && fPS[BOT] < 0 ? true : false;
}

void CAnalysisAgentImp::IsGirderInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ)
{
   // The specified location is in a precompressed tensile zone if the following requirements are true
   // 1) The location is in tension in the Service III limit state for the final interval
   // 2) Prestressing causes compression at the location

   // First deal with the special cases

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Special case... this is a regular prestressed girder or a simple span spliced girder and the girder does not have cantilevered ends
   // This case isn't that special, however, we know that the bottom of the girder is in the PTZ and the top is not. There
   // is no need to do all the analytical work to figure this out.
   GET_IFACE(IBridge,pBridge);
   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
   bool bDummy;
   bool bContinuousStart, bContinuousEnd;
   bool bIntegralStart, bIntegralEnd;
   pBridge->IsContinuousAtPier(startPierIdx,&bDummy,&bContinuousStart);
   pBridge->IsIntegralAtPier(startPierIdx,&bDummy,&bIntegralStart);
   pBridge->IsContinuousAtPier(endPierIdx,&bContinuousEnd,&bDummy);
   pBridge->IsIntegralAtPier(endPierIdx,&bIntegralEnd,&bDummy);
   bool bNegMoment = (bContinuousStart || bIntegralStart || bContinuousEnd || bIntegralEnd);
   if ( startPierIdx == endPierIdx-1 && // the group is only one span long
        pBridge->GetSegmentCount(segmentKey) == 1 && // there one segment in the group
        !pBridge->HasCantilever(startPierIdx) && // start is not cantilever
        !pBridge->HasCantilever(endPierIdx) && // end is not cantilever
        !bNegMoment) // no contiuous or integral boundary conditions to cause negative moments
   {
      // we know the answer
      *pbTopPTZ = false;
      *pbBotPTZ = true;
      return;
   }


   // Special case... At the start/end of the first/last segment the stress due to 
   // externally applied loads is zero (moment is zero) for roller/hinge
   // boundary condition and the stress due to the prestressing is also zero (strands not developed). 
   // Consider the bottom of the girder to be in a precompressed tensile zone
   if ( segmentKey.segmentIndex == 0 ) // start of first segment (end of last segment is below)
   {
      bool bModelStartCantilever,bModelEndCantilever;
      pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

      if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || // start of segment at release
          (poi.IsTenthPoint(POI_ERECTED_SEGMENT)  == 1 && !bModelStartCantilever) ) // CL Brg at start of erected segment
      {
         PierIndexType pierIdx = pBridge->GetGirderGroupStartPier(segmentKey.groupIndex);
         pgsTypes::BoundaryConditionType boundaryConditionType = pBridge->GetBoundaryConditionType(pierIdx);
         if ( boundaryConditionType == pgsTypes::bctHinge || boundaryConditionType == pgsTypes::bctRoller )
         {
            *pbTopPTZ = false;
            *pbBotPTZ = true;
            return;
         }
      }
   }

   // end of last segment
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   if ( segmentKey.segmentIndex == nSegments-1 )
   {
      bool bModelStartCantilever,bModelEndCantilever;
      pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

      if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11 ||
          (poi.IsTenthPoint(POI_ERECTED_SEGMENT)  == 11 && !bModelEndCantilever) )
      {
         PierIndexType pierIdx = pBridge->GetGirderGroupEndPier(segmentKey.groupIndex);
         pgsTypes::BoundaryConditionType boundaryConditionType = pBridge->GetBoundaryConditionType(pierIdx);
         if ( boundaryConditionType == pgsTypes::bctHinge || boundaryConditionType == pgsTypes::bctRoller )
         {
            *pbTopPTZ = false;
            *pbBotPTZ = true;
            return;
         }
      }
   }

   // Special case... ends of any segment for stressing at release
   // Even though there is prestress, at the end faces of the girder there isn't
   // any prestress force because it hasn't been transfered to the girder yet. The
   // prestress force transfers over the transfer length.
   if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11 )
   {
      *pbTopPTZ = false;
      *pbBotPTZ = true;
      return;
   }

   // Special case... if there aren't any strands the notion of a precompressed tensile zone
   // gets really goofy (there is no precompression). Technically the segment is a reinforced concrete
   // beam. However, this is a precast-prestressed concrete program so we need to have something reasonable
   // for the precompressed tensile zone. If there aren't any strands (or the Pjack is zero), then
   // the bottom of the girder is in the PTZ and the top is not. This is where the PTZ is usually located
   // when there are strands
   Float64 Pjack;
   if ( pConfig )
   {
      Pjack = pConfig->PrestressConfig.Pjack[pgsTypes::Straight] + 
              pConfig->PrestressConfig.Pjack[pgsTypes::Harped]   + 
              pConfig->PrestressConfig.Pjack[pgsTypes::Temporary];
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Pjack = pStrandGeom->GetPjack(segmentKey,true/*include temp strands*/);
   }

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   if ( IsZero(Pjack) && nDucts == 0 )
   {
      *pbTopPTZ = false;
      *pbBotPTZ = true;
      return;
   }

   // Special case... if the POI is located "near" interior supports with continuous boundary
   // condition tension develops in the top of the girder and prestressing may cause compression
   // in this location (most likely from harped strands). From LRFD C5.14.1.4.6, this location is not
   // in a precompressed tensile zone. Assume that "near" means that the POI is somewhere between 
   // mid-span and the closest pier.
   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetSpansForSegment(segmentKey,&startSpanIdx,&endSpanIdx);

   GET_IFACE(IPointOfInterest,pIPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pIPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
   startPierIdx = (PierIndexType)spanKey.spanIndex;
   endPierIdx = startPierIdx+1;

   // some segments are not supported by piers at all. segments can be supported by
   // just temporary supports. an example would be the center segment in a three-segment
   // single span bridge. This special case doesn't apply to that segment.
   bool bStartAtPier = true;  // start end of segment bears on a pier
   bool bEndAtPier   = true;  // end end of segment bears on a pier

   // one of the piers must be a boundary pier or C5.14.1.4.6 doesn't apply
   // the segment must start and end in the same span (can't straddle a pier)
   if ( startSpanIdx == endSpanIdx && (pBridge->IsBoundaryPier(startPierIdx) || pBridge->IsBoundaryPier(endPierIdx)) )
   {
      Float64 Xstart, Xend; // dist from end of segment to start/end pier
      if ( pBridge->IsBoundaryPier(startPierIdx) )
      {
         // GetPierLocation returns false if the segment does not bear on the pier
         bStartAtPier = pBridge->GetPierLocation(startPierIdx,segmentKey,&Xstart);
      }
      else
      {
         // not a boundary pier so use a really big number so
         // this end doesn't control
         Xstart = DBL_MAX;
      }

      if ( pBridge->IsBoundaryPier(endPierIdx) )
      {
         // GetPierLocation returns false if the segment does not bear on the pier
         bEndAtPier = pBridge->GetPierLocation(endPierIdx,segmentKey,&Xend);
      }
      else
      {
         // not a boundary pier so use a really big number so
         // this end doesn't control
         Xend = DBL_MAX;
      }

      if ( bStartAtPier && bEndAtPier ) // both ends must bear on a pier
      {
         // distance from POI to pier at start/end of the span
         // If they are equal distances, then the POI is at mid-span
         // and we can't make a direct determination if C5.14.1.4.6 applies.
         // If they are both equal, continue with the procedure below
         Float64 offsetStart = fabs(Xstart-poi.GetDistFromStart());
         Float64 offsetEnd   = fabs(Xend-poi.GetDistFromStart());
         if ( !IsEqual(offsetStart,offsetEnd) )
         {
            // poi is closer to one pier then the other.
            // get the boundary conditions of the nearest pier
            pgsTypes::BoundaryConditionType boundaryConditionType;
            if ( offsetStart < offsetEnd )
            {
               // nearest pier is at the start of the span
               boundaryConditionType = pBridge->GetBoundaryConditionType(startPierIdx);
            }
            else
            {
               // nearest pier is at the end of the span
               boundaryConditionType = pBridge->GetBoundaryConditionType(endPierIdx);
            }

            // if hinge or roller boundary condition, C5.14.1.4.6 doesn't apply.
            if ( boundaryConditionType != pgsTypes::bctRoller && boundaryConditionType != pgsTypes::bctHinge )
            {
               // connection type is some sort of continuity/integral boundary condition
               // The top of the girder is not in the PTZ.
               *pbTopPTZ = false;
               *pbBotPTZ = true;
               return;
            }
         }
      }
   }

   // Now deal with the regular case

   // Get the stress when the bridge is in service (that is when live load is applied)
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Tensile stresses are greatest at the top of the girder using the minimum model in
   // Envelope mode. In all other modes, Min/Max are the same
   pgsTypes::BridgeAnalysisType batTop = GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batBot = GetBridgeAnalysisType(pgsTypes::Maximize);

   // Get stresses due to service loads
   Float64 fMin[2], fMax[2];
   GetStress(serviceLoadIntervalIdx,limitState,poi,batTop,false/*without prestress*/,pgsTypes::TopGirder,   &fMin[TOP],&fMax[TOP]);
   GetStress(serviceLoadIntervalIdx,limitState,poi,batBot,false/*without prestress*/,pgsTypes::BottomGirder,&fMin[BOT],&fMax[BOT]);
   //if ( fMax <= 0 )
   //{
   //   return false; // the location is not in tension so is not in the "tension zone"
   //}

   // The section is in tension, does the prestress cause compression in the service load interval?
   Float64 fPreTension[2], fPostTension[2];
   if ( pConfig )
   {
      fPreTension[TOP] = GetDesignStress(serviceLoadIntervalIdx,limitState,poi,pgsTypes::TopGirder,   *pConfig,false/*don't include live load*/);
      fPreTension[BOT] = GetDesignStress(serviceLoadIntervalIdx,limitState,poi,pgsTypes::BottomGirder,*pConfig,false/*don't include live load*/);
      fPostTension[TOP] = 0; // no post-tensioning for precast girder design
      fPostTension[BOT] = 0; // no post-tensioning for precast girder design
   }
   else
   {
      GetStress(serviceLoadIntervalIdx,poi,pgsTypes::TopGirder,pgsTypes::BottomGirder,false/*don't include live load*/,&fPreTension[TOP],&fPreTension[BOT]);

      GetStress(serviceLoadIntervalIdx,pgsTypes::pftPostTensioning,poi,batTop,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fPostTension[TOP],&fPostTension[BOT]);
   }
   
   Float64 fPS[2];
   fPS[TOP] = fPreTension[TOP] + fPostTension[TOP];
   fPS[BOT] = fPreTension[BOT] + fPostTension[BOT];

   *pbTopPTZ = 0 < fMax[TOP] && fPS[TOP] < 0 ? true : false;
   *pbBotPTZ = 0 < fMax[BOT] && fPS[BOT] < 0 ? true : false;
}


void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   pgsTypes::FaceType topFace = (IsTopStressLocation(topLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);
   pgsTypes::FaceType botFace = (IsTopStressLocation(botLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop = pTopConcreteElement->f[topFace][pfType][resultsType];
      Float64 fBot = pBotConcreteElement->f[botFace][pfType][resultsType];

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;

   pfTop->clear();
   pfBot->clear();

   if ( pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      // no direct post-tension stress in the elastic analysis
      pfTop->resize(vPoi.size(),0.0);
      pfBot->resize(vPoi.size(),0.0);
      return;
   }

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetStress(intervalIdx,pfType,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> fTopPrev, fBotPrev;
         std::vector<Float64> fTopThis, fBotThis;
         GetStress(intervalIdx-1,pfType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopPrev,&fBotPrev);
         GetStress(intervalIdx,  pfType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopThis,&fBotThis);

         std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
         std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);

  pgsTypes::FaceType topFace = (IsTopStressLocation(topLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);
  pgsTypes::FaceType botFace = (IsTopStressLocation(botLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop(0), fBot(0);
      std::vector<pgsTypes::ProductForceType>::iterator pfTypeIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfTypeIterEnd(pfTypes.end());
      for ( ; pfTypeIter != pfTypeIterEnd; pfTypeIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfTypeIter;
         fTop += pTopConcreteElement->f[topFace][pfType][resultsType];
         fBot += pBotConcreteElement->f[botFace][pfType][resultsType];
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   pfTop->clear();
   pfBot->clear();

   if ( comboType == lcPS )
   {
      // no secondary effects for elastic analysis
      pfTop->resize(vPoi.size(),0.0);
      pfBot->resize(vPoi.size(),0.0);
      return;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetStress(intervalIdx,comboType,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> fTopPrev, fBotPrev;
         std::vector<Float64> fTopThis, fBotThis;
         GetStress(intervalIdx-1,comboType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopPrev,&fBotPrev);
         GetStress(intervalIdx,  comboType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopThis,&fBotThis);

         std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
         std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pMin->clear();
   pMax->clear();

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gLLMin = pLoadFactors->LLIMmin[limitState];
   Float64 gLLMax = pLoadFactors->LLIMmax[limitState];
   Float64 gDCMin = pLoadFactors->DCmin[limitState];
   Float64 gDCMax = pLoadFactors->DCmax[limitState];
   Float64 gDWMin = pLoadFactors->DWmin[limitState];
   Float64 gDWMax = pLoadFactors->DWmax[limitState];
   Float64 gCRMax = pLoadFactors->CRmax[limitState];
   Float64 gCRMin = pLoadFactors->CRmin[limitState];
   Float64 gSHMax = pLoadFactors->SHmax[limitState];
   Float64 gSHMin = pLoadFactors->SHmin[limitState];
   Float64 gREMax = pLoadFactors->REmax[limitState];
   Float64 gREMin = pLoadFactors->REmin[limitState];
   Float64 gPSMax = pLoadFactors->PSmax[limitState]; // this is for secondary effects due to PT
   Float64 gPSMin = pLoadFactors->PSmin[limitState]; // this is for secondary effects due to PT

   // Use half prestress if Service IA or Fatigue I (See LRFD Table 5.9.4.2.1-1)
   Float64 k = (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI) ? 0.5 : 1.0;

   // Get load combination stresses
   pgsTypes::StressLocation topLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck;
   std::vector<Float64> fDCtop, fDWtop, fCRtop, fSHtop, fREtop, fPStop;
   std::vector<Float64> fDCbot, fDWbot, fCRbot, fSHbot, fREbot, fPSbot;
   GetTimeStepStress(intervalIdx,lcDC,vPoi,bat,rtCumulative,topLocation,botLocation,&fDCtop,&fDCbot);
   GetTimeStepStress(intervalIdx,lcDW,vPoi,bat,rtCumulative,topLocation,botLocation,&fDWtop,&fDWbot);
   GetTimeStepStress(intervalIdx,lcCR,vPoi,bat,rtCumulative,topLocation,botLocation,&fCRtop,&fCRbot);
   GetTimeStepStress(intervalIdx,lcSH,vPoi,bat,rtCumulative,topLocation,botLocation,&fSHtop,&fSHbot);
   GetTimeStepStress(intervalIdx,lcRE,vPoi,bat,rtCumulative,topLocation,botLocation,&fREtop,&fREbot);
   GetTimeStepStress(intervalIdx,lcPS,vPoi,bat,rtCumulative,topLocation,botLocation,&fPStop,&fPSbot);
   
   // we only want top or bottom
   std::vector<Float64>* pfDC = (IsTopStressLocation(stressLocation) ? &fDCtop : &fDCbot);
   std::vector<Float64>* pfDW = (IsTopStressLocation(stressLocation) ? &fDWtop : &fDWbot);
   std::vector<Float64>* pfCR = (IsTopStressLocation(stressLocation) ? &fCRtop : &fCRbot);
   std::vector<Float64>* pfSH = (IsTopStressLocation(stressLocation) ? &fSHtop : &fSHbot);
   std::vector<Float64>* pfRE = (IsTopStressLocation(stressLocation) ? &fREtop : &fREbot);
   std::vector<Float64>* pfPS = (IsTopStressLocation(stressLocation) ? &fPStop : &fPSbot);

   // add in prestress and live load
   GET_IFACE(ILosses,pLosses);

   std::vector<Float64>::iterator dcIter = pfDC->begin();
   std::vector<Float64>::iterator dwIter = pfDW->begin();
   std::vector<Float64>::iterator crIter = pfCR->begin();
   std::vector<Float64>::iterator shIter = pfSH->begin();
   std::vector<Float64>::iterator reIter = pfRE->begin();
   std::vector<Float64>::iterator psIter = pfPS->begin();

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++, dcIter++, dwIter++, crIter++, shIter++, reIter++, psIter++ )
   {
      Float64 fPR(0), fPT(0), fLLMin(0), fLLMax(0);
      const pgsPointOfInterest& poi(*poiIter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pConcreteElement = (IsGirderStressLocation(stressLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      pgsTypes::FaceType face = IsTopStressLocation(stressLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace;

      if ( bIncludePrestress )
      {
         fPR = pConcreteElement->f[face][pgsTypes::pftPretension           ][rtCumulative];
         fPT = pConcreteElement->f[face][pgsTypes::pftPostTensioning][rtCumulative];
      }

      fLLMin = pConcreteElement->fLLMin[face];
      fLLMax = pConcreteElement->fLLMax[face];

      if ( fLLMax < fLLMin )
      {
         std::swap(fLLMin,fLLMax);
      }

      Float64 fMin = gDCMin*(*dcIter) + gDWMin*(*dwIter) + gCRMin*(*crIter) + gSHMin*(*shIter) + gREMin*(*reIter) + gPSMin*(*psIter) + gLLMin*fLLMin + k*(fPR + fPT);
      Float64 fMax = gDCMax*(*dcIter) + gDWMax*(*dwIter) + gCRMax*(*crIter) + gSHMax*(*shIter) + gREMax*(*reIter) + gPSMax*(*psIter) + gLLMax*fLLMax + k*(fPR + fPT);

      pMin->push_back(fMin);
      pMax->push_back(fMax);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         pMin->resize(vPoi.size(),0.0);
         pMax->resize(vPoi.size(),0.0);
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetStress(intervalIdx,limitState,vPoi,stressLocation,bIncludePrestress,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,limitState,vPoi,bat,stressLocation,bIncludePrestress,pMin,pMax);
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}
