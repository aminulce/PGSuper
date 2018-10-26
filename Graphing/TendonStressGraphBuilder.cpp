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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\TendonStressGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "TendonStressGraphController.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>

#include <EAF\EAFGraphView.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CTendonStressGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CTendonStressGraphBuilder::CTendonStressGraphBuilder() :
CGirderGraphBuilderBase()
{
   SetName(_T("Tendon Stresses"));
}

CTendonStressGraphBuilder::CTendonStressGraphBuilder(const CTendonStressGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CTendonStressGraphBuilder::~CTendonStressGraphBuilder()
{
}

CGraphBuilder* CTendonStressGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CTendonStressGraphBuilder(*this);
}

int CTendonStressGraphBuilder::CreateControls(CWnd* pParent,UINT nID)
{
   CGirderGraphBuilderBase::CreateControls(pParent,nID);

   m_Graph.SetXAxisTitle(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
   m_Graph.SetYAxisTitle(_T("Stress (")+m_pYFormat->UnitTag()+_T(")"));
   m_Graph.SetPinYAxisAtZero(true);

   return 0;
}

CGirderGraphControllerBase* CTendonStressGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CTendonStressGraphController;
}

BOOL CTendonStressGraphBuilder::InitGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);
   return m_pGraphController->Create(pParent,IDD_TENDONSTRESS_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

bool CTendonStressGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GroupIndexType    grpIdx      = m_pGraphController->GetGirderGroup();
   GirderIndexType   gdrIdx      = m_pGraphController->GetGirder();
   IntervalIndexType intervalIdx = m_pGraphController->GetInterval();

   DuctIndexType     ductIdx     = ((CTendonStressGraphController*)m_pGraphController)->GetDuct();

   UpdateGraphTitle(grpIdx,gdrIdx,ductIdx,intervalIdx);

   // get data to graph
   UpdateGraphData(grpIdx,gdrIdx,ductIdx,intervalIdx);

   return true;
}

void CTendonStressGraphBuilder::UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx,DuctIndexType ductIdx,IntervalIndexType intervalIdx)
{
   GET_IFACE(IIntervals,pIntervals);
   CString strInterval( pIntervals->GetDescription(intervalIdx) );

   CString strGraphTitle;
   if ( ductIdx == INVALID_INDEX )
      strGraphTitle.Format(_T("Group %d Girder %s - Interval %d %s"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_INTERVAL(intervalIdx),strInterval);
   else
      strGraphTitle.Format(_T("Group %d Girder %s Duct %d - Interval %d %s"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_DUCT(ductIdx),LABEL_INTERVAL(intervalIdx),strInterval);
   
   m_Graph.SetTitle(std::_tstring(strGraphTitle));
}

void CTendonStressGraphBuilder::UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx,DuctIndexType ductIdx,IntervalIndexType intervalIdx)
{
   // clear graph
   m_Graph.ClearData();

   ATLASSERT(ductIdx != INVALID_INDEX);

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(grpIdx,gdrIdx,ALL_SEGMENTS);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,xVals);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(CGirderKey(grpIdx,gdrIdx),ductIdx);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   IndexType dataSeries1 = m_Graph.CreateDataSeries(_T("fpj prior to anchor set"),PS_SOLID,1,ORANGE);
   IndexType dataSeries2 = m_Graph.CreateDataSeries(_T("fpj after anchor set"),PS_SOLID,1,GREEN);
   IndexType dataSeries3 = m_Graph.CreateDataSeries(_T("fpe"),PS_SOLID,1,BLUE);
   IndexType dataSeries4 = m_Graph.CreateDataSeries(_T("fpe with Max LL+IM"),PS_SOLID,1,PURPLE);
   IndexType dataSeries5 = m_Graph.CreateDataSeries(_T("fpe with Min LL+IM"),PS_SOLID,1,MAROON);

   GET_IFACE(IPosttensionForce,pPTForce);

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   std::vector<Float64>::iterator xIter(xVals.begin());
   for ( ; iter != end; iter++, xIter++ )
   {
      pgsPointOfInterest& poi = *iter;

      Float64 X = *xIter;

      Float64 fpbt = 0; // stress immediately before transfer
      Float64 fpat = 0; // stress immediately after transfer
      Float64 fpe  = 0; // stress in tendon this interval
      if ( stressTendonIntervalIdx <= intervalIdx )
      {
         fpbt = pPTForce->GetTendonStress(poi,stressTendonIntervalIdx,pgsTypes::Start,ductIdx);
         fpat = pPTForce->GetTendonStress(poi,stressTendonIntervalIdx,pgsTypes::End,  ductIdx);

         fpe  = pPTForce->GetTendonStress(poi,intervalIdx,pgsTypes::End,  ductIdx);
      }
     
      AddGraphPoint(dataSeries1,X,fpbt);
      AddGraphPoint(dataSeries2,X,fpat);

      AddGraphPoint(dataSeries3,X,fpe);

      if ( liveLoadIntervalIdx <= intervalIdx )
      {
         Float64 fpeLLMax = pPTForce->GetTendonStress(poi,intervalIdx,pgsTypes::End,ductIdx,false,true);
         AddGraphPoint(dataSeries4,X,fpeLLMax);

         Float64 fpeLLMin = pPTForce->GetTendonStress(poi,intervalIdx,pgsTypes::End,ductIdx,true,false);
         AddGraphPoint(dataSeries5,X,fpeLLMin);
      }
   }
}
