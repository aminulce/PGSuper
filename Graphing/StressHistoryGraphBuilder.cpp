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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\StressHistoryGraphBuilder.h>
#include "StressHistoryGraphController.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PointOfInterest.h>

#include <EAF\EAFGraphView.h>

#include <GraphicsLib\AxisXY.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool    DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CStressHistoryGraphBuilder, CEAFGraphBuilderBase)
   ON_BN_CLICKED(IDC_TOPDECK,OnTopDeck)
   ON_BN_CLICKED(IDC_TOPGIRDER,OnTopGirder)
   ON_BN_CLICKED(IDC_BOTTOMGIRDER,OnBottomGirder)
   ON_BN_CLICKED(IDC_GRID, OnShowGrid)
END_MESSAGE_MAP()


CStressHistoryGraphBuilder::CStressHistoryGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_bTopDeck(false),
m_bTopGirder(false),
m_bBottomGirder(false),
m_XAxisType(X_AXIS_TIME_LOG)
{
   m_pGraphController = new CStressHistoryGraphController;

   SetName(_T("Stress History"));

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CStressHistoryGraphBuilder::CStressHistoryGraphBuilder(const CStressHistoryGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pTimeFormat(0),
m_pIntervalFormat(0),
m_pYFormat(0),
m_bTopDeck(false),
m_bTopGirder(false),
m_bBottomGirder(false),
m_XAxisType(X_AXIS_TIME_LOG)
{
   m_pGraphController = new CStressHistoryGraphController;

   m_Scalar.Width = 7;
   m_Scalar.Precision = 0;
   m_Scalar.Format = sysNumericFormatTool::Fixed;
}

CStressHistoryGraphBuilder::~CStressHistoryGraphBuilder()
{
   if ( m_pGraphController != NULL )
   {
      delete m_pGraphController;
      m_pGraphController = NULL;
   }

   if ( m_pTimeFormat != NULL )
   {
      delete m_pTimeFormat;
      m_pTimeFormat = NULL;
   }

   if ( m_pIntervalFormat != NULL )
   {
      delete m_pIntervalFormat;
      m_pIntervalFormat = NULL;
   }

   if ( m_pYFormat != NULL )
   {
      delete m_pYFormat;
      m_pYFormat = NULL;
   }

}

CEAFGraphControlWindow* CStressHistoryGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

CGraphBuilder* CStressHistoryGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CStressHistoryGraphBuilder(*this);
}

int CStressHistoryGraphBuilder::CreateControls(CWnd* pParent,UINT nID)
{
   // create the graph definitions before creating the graph controller.
   // our graph controller will call GetLoadCaseNames to populate the 
   // list of load cases
   EAFGetBroker(&m_pBroker);

   // let the base class do its thing
   CEAFAutoCalcGraphBuilder::CreateControls(pParent,nID);

   // create our controls
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_pGraphController->Create(pParent,IDD_STRESSHISTORY_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return -1; // failed to create
   }

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);

   m_Graph.SetTitle(_T("Stress History"));

   m_pGraphController->CheckDlgButton(IDC_TIME_LOG,BST_CHECKED);

   // x axis
   m_pTimeFormat = new ScalarTool(m_Scalar);
   m_pIntervalFormat = new ScalarTool(m_Scalar);
   m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   m_Graph.SetXAxisNumberOfMajorTics(11);
   m_XAxisType = X_AXIS_TIME_LOG;

   // y axis
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
   m_Graph.SetYAxisTitle(_T("Stress (")+m_pYFormat->UnitTag()+_T(")"));
   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.SetDoDrawGrid(); // show grid by default
   m_Graph.SetGridPenStyle(PS_DOT, 1, GRID_COLOR);

   return 0;
}

void CStressHistoryGraphBuilder::OnTopDeck()
{
   m_bTopDeck = !m_bTopDeck;
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnTopGirder()
{
   m_bTopGirder = !m_bTopGirder;
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnBottomGirder()
{
   m_bBottomGirder = !m_bBottomGirder;
   Update();
   GetView()->Invalidate();
}

void CStressHistoryGraphBuilder::OnShowGrid()
{
   m_Graph.SetDoDrawGrid( !m_Graph.GetDoDrawGrid() );
   GetView()->Invalidate();
}

bool CStressHistoryGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   m_XAxisType = m_pGraphController->GetXAxisType();
   UpdateXAxis();


   // Update graph properties
   pgsPointOfInterest poi = m_pGraphController->GetLocation();
   UpdateGraphTitle(poi);
   UpdateGraphData(poi);
   return true;
}

void CStressHistoryGraphBuilder::UpdateGraphTitle(const pgsPointOfInterest& poi)
{
   CString strGraphTitle;
   strGraphTitle.Format(_T("Stress History"));
   m_Graph.SetTitle(std::_tstring(strGraphTitle));

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IPointOfInterest,pPoi);
   Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
   CString strSubtitle;
   std::_tstring strAttributes = poi.GetAttributes(POI_ERECTED_SEGMENT,false);
   if ( strAttributes.size() == 0 )
   {
      strSubtitle.Format(_T("Group %d Girder %s Segment %d, %s (X=%s)"),
         LABEL_GROUP(segmentKey.groupIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         LABEL_SEGMENT(segmentKey.segmentIndex),
         FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit()),
         FormatDimension(Xg,pDisplayUnits->GetSpanLengthUnit()));
   }
   else
   {
      strSubtitle.Format(_T("Group %d Girder %s Segment %d, %s (%s) (X=%s)"),
         LABEL_GROUP(segmentKey.groupIndex),
         LABEL_GIRDER(segmentKey.girderIndex),
         LABEL_SEGMENT(segmentKey.segmentIndex),
         FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit()),
         strAttributes.c_str(),
         FormatDimension(Xg,pDisplayUnits->GetSpanLengthUnit()));
   }

   m_Graph.SetSubtitle(std::_tstring(strSubtitle));
}

void CStressHistoryGraphBuilder::UpdateGraphData(const pgsPointOfInterest& poi)
{
   // clear graph
   m_Graph.ClearData();

   IndexType topSlabMinDataSeries   = m_Graph.CreateDataSeries(_T("Top of Slab"),     PS_SOLID,1,ORANGE);
   IndexType topSlabMaxDataSeries   = m_Graph.CreateDataSeries(_T(""),                PS_SOLID,1,ORANGE);
   IndexType topGirderMinDataSeries = m_Graph.CreateDataSeries(_T("Top of Girder"),   PS_SOLID,1,GREEN);
   IndexType topGirderMaxDataSeries = m_Graph.CreateDataSeries(_T(""),                PS_SOLID,1,GREEN);
   IndexType botGirderMinDataSeries = m_Graph.CreateDataSeries(_T("Bottom of Girder"),PS_SOLID,1,BLUE);
   IndexType botGirderMaxDataSeries = m_Graph.CreateDataSeries(_T(""),                PS_SOLID,1,BLUE);

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IIntervals,pIntervals);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType startIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 xStart, xEnd;
      if ( m_XAxisType == X_AXIS_TIME_LINEAR || m_XAxisType == X_AXIS_TIME_LOG )
      {
         xStart = pIntervals->GetStart(intervalIdx);
         xEnd   = pIntervals->GetEnd(intervalIdx);
      }
      else
      {
         xEnd   = (Float64)LABEL_INTERVAL(intervalIdx);
         xStart = xEnd - 1;
      }

      bool bIncludePrestress = true;

      if ( m_bTopDeck )
      {
         Float64 fMin, fMax;
         pForces->GetStress(pgsTypes::ServiceI,intervalIdx-1,poi,pgsTypes::TopDeck,bIncludePrestress,pgsTypes::ContinuousSpan,&fMin,&fMax);
         AddGraphPoint(topSlabMinDataSeries,xStart,fMin);
         AddGraphPoint(topSlabMaxDataSeries,xStart,fMax);

         pForces->GetStress(pgsTypes::ServiceI,intervalIdx,poi,pgsTypes::TopDeck,bIncludePrestress,pgsTypes::ContinuousSpan,&fMin,&fMax);
         AddGraphPoint(topSlabMinDataSeries,xEnd,fMin);
         AddGraphPoint(topSlabMaxDataSeries,xEnd,fMax);
      }

      if ( m_bTopGirder )
      {
         Float64 fMin, fMax;
         pForces->GetStress(pgsTypes::ServiceI,intervalIdx-1,poi,pgsTypes::TopGirder,bIncludePrestress,pgsTypes::ContinuousSpan,&fMin,&fMax);
         AddGraphPoint(topGirderMinDataSeries,xStart,fMin);
         AddGraphPoint(topGirderMaxDataSeries,xStart,fMax);

         pForces->GetStress(pgsTypes::ServiceI,intervalIdx,poi,pgsTypes::TopGirder,bIncludePrestress,pgsTypes::ContinuousSpan,&fMin,&fMax);
         AddGraphPoint(topGirderMinDataSeries,xEnd,fMin);
         AddGraphPoint(topGirderMaxDataSeries,xEnd,fMax);
      }

      if ( m_bBottomGirder )
      {
         Float64 fMin, fMax;
         pForces->GetStress(pgsTypes::ServiceI,intervalIdx-1,poi,pgsTypes::BottomGirder,bIncludePrestress,pgsTypes::ContinuousSpan,&fMin,&fMax);
         AddGraphPoint(botGirderMinDataSeries,xStart,fMin);
         AddGraphPoint(botGirderMaxDataSeries,xStart,fMax);

         pForces->GetStress(pgsTypes::ServiceI,intervalIdx,poi,pgsTypes::BottomGirder,bIncludePrestress,pgsTypes::ContinuousSpan,&fMin,&fMax);
         AddGraphPoint(botGirderMinDataSeries,xEnd,fMin);
         AddGraphPoint(botGirderMaxDataSeries,xEnd,fMax);
      }
   }
}

void CStressHistoryGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   const arvPhysicalConverter* pcx = dynamic_cast<const arvPhysicalConverter*>(m_Graph.GetXAxisValueFormat());
   ASSERT(pcx);
   const arvPhysicalConverter* pcy = dynamic_cast<const arvPhysicalConverter*>(m_Graph.GetYAxisValueFormat());
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   Float64 y = pcy->Convert(yval);
   m_Graph.AddPoint(series, gpPoint2d(x,y));
}

void CStressHistoryGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int save = pDC->SaveDC();

   // The graph is valided and there was not an error
   // updating data.... draw the graph
   CRect rect = GetView()->GetDrawingRect();

   m_Graph.SetOutputRect(rect);
   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());
   m_Graph.Draw(pDC->GetSafeHdc());

   pDC->RestoreDC(save);
}

void CStressHistoryGraphBuilder::UpdateXAxis()
{
   if ( m_XAxisType == X_AXIS_TIME_LINEAR )
   {
      m_Graph.SetXAxisScale(grAxisXY::LINEAR);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.SetXAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_TIME_LOG )
   {
      m_Graph.SetXAxisScale(grAxisXY::LOGARITHMIC);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.SetXAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(*m_pTimeFormat);
   }
   else
   {
      m_Graph.SetXAxisScale(grAxisXY::INTEGRAL);
      m_Graph.SetXAxisTitle(_T("Interval"));
      m_Graph.SetXAxisNiceRange(false);
      m_Graph.SetXAxisNumberOfMinorTics(0);
      m_Graph.SetXAxisValueFormat(*m_pIntervalFormat);
   }
}
