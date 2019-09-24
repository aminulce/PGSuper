///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2019  Washington State Department of Transportation
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

// DrawTendonsControl.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperColors.h"
#include "DrawTendonsControl.h"
#include <IFace\Alignment.h>
#include <IFace\Bridge.h>

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CDrawTendonsControl

IMPLEMENT_DYNAMIC(CDrawTendonsControl, CWnd)

CDrawTendonsControl::CDrawTendonsControl()
{
   m_pGirder = nullptr;
   m_pPTData = nullptr;
   m_MapMode = grlibPointMapper::Isotropic;
   m_DuctIdx = ALL_DUCTS;
}

CDrawTendonsControl::~CDrawTendonsControl()
{
}


BEGIN_MESSAGE_MAP(CDrawTendonsControl, CWnd)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CDrawTendonsControl message handlers
void CDrawTendonsControl::CustomInit(const CGirderKey& girderKey,const CSplicedGirderData* pGirder,const CPTData* pPTData)
{
   m_GirderKey = girderKey;
   m_pGirder = pGirder;
   m_pPTData = pPTData;
}

void CDrawTendonsControl::SetMapMode(grlibPointMapper::MapMode mm)
{
   if (m_MapMode != mm)
   {
      m_MapMode = mm;
      Invalidate();
      UpdateWindow();
   }
}

grlibPointMapper::MapMode CDrawTendonsControl::GetMapMode() const
{
   return m_MapMode;
}

void CDrawTendonsControl::SetDuct(DuctIndexType ductIdx)
{
   if (m_DuctIdx != ductIdx)
   {
      m_DuctIdx = ductIdx;
      Invalidate();
      UpdateWindow();
   }
}

DuctIndexType CDrawTendonsControl::GetDuct() const
{
   return m_DuctIdx;
}

BOOL CDrawTendonsControl::OnEraseBkgnd(CDC* pDC)
{
   CWnd* pParent = GetParent();
   int color = COLOR_BTNFACE;
   if (pParent->IsKindOf(RUNTIME_CLASS(CPropertyPage)))
   {
      color = COLOR_WINDOW;
   }

   CBrush brush(::GetSysColor(color));
   brush.UnrealizeObject();

   CPen pen(PS_SOLID, 1, ::GetSysColor(color));
   pen.UnrealizeObject();

   CBrush* pOldBrush = pDC->SelectObject(&brush);
   CPen* pOldPen = pDC->SelectObject(&pen);

   CRect rect;
   GetClientRect(&rect);
   rect.InflateRect(1, 1);
   pDC->Rectangle(rect);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   return TRUE;

   // default isn't working so we have to do our own erasing
   //return CWnd::OnEraseBkgnd(pDC);
}

void CDrawTendonsControl::OnPaint()
{
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirder,pIGirder);

   SegmentIndexType nSegments = m_pGirder->GetSegmentCount();

   std::vector<CComPtr<IShape>> segmentShapes;
   CComPtr<IRect2d> bounding_box;
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(m_GirderKey,segIdx);

      CComPtr<IShape> shape;
      pIGirder->GetSegmentProfile( segmentKey, m_pGirder, true, &shape);

      if ( segIdx == 0 )
      {
         shape->get_BoundingBox(&bounding_box);
      }
      else
      {
         CComPtr<IRect2d> bbox;
         shape->get_BoundingBox(&bbox);
         bounding_box->Union(bbox);
      }

      segmentShapes.push_back(shape);
   }

   // Create a poly line for each tendon. 
   std::vector<CComPtr<IPoint2dCollection>> ducts;
   GET_IFACE2_NOCHECK(pBroker,ITendonGeometry,pTendonGeometry); // only used if there are ducts/tendons
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pIPoi); // only used if there are ducts/tendons
   DuctIndexType nDucts = m_pPTData->GetDuctCount();
   DuctIndexType startDuctIdx = (m_DuctIdx == ALL_DUCTS ? 0 : m_DuctIdx);
   DuctIndexType endDuctIdx = (m_DuctIdx == ALL_DUCTS ? nDucts - 1 : startDuctIdx);
   for (DuctIndexType ductIdx = startDuctIdx; ductIdx <= endDuctIdx; ductIdx++)
   {
      CComPtr<IPoint2dCollection> ductPoints;
      const CDuctData* pDuct = m_pPTData->GetDuct(ductIdx);
      pTendonGeometry->GetDuctCenterline(m_GirderKey,ductIdx,pDuct,&ductPoints);

      IndexType nPoints;
      ductPoints->get_Count(&nPoints);
      for ( IndexType pntIdx = 0; pntIdx < nPoints; pntIdx++ )
      {
         CComPtr<IPoint2d> pnt;
         ductPoints->get_Item(pntIdx,&pnt);
         Float64 X;
         pnt->get_X(&X);
         X = pIPoi->ConvertGirderCoordinateToGirderPathCoordinate(m_GirderKey,X);
         pnt->put_X(X);
      }

      ducts.push_back(ductPoints);
   }


   //
   // Set up coordinate mapping
   //
   Float64 left,right,top,bottom;
   bounding_box->get_Left(&left);
   bounding_box->get_Right(&right);
   bounding_box->get_Top(&top);
   bounding_box->get_Bottom(&bottom);

   gpRect2d box(left,bottom,right,top);
   gpSize2d size = box.Size();
   gpPoint2d org = box.Center();

   CRect rClient;
   GetClientRect(&rClient);
   CSize sClient = rClient.Size();

   grlibPointMapper mapper;
   mapper.SetMappingMode(m_MapMode);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(sClient.cx,sClient.cy);
   mapper.SetDeviceOrg(sClient.cx/2,sClient.cy/2);

   CPen pen(PS_SOLID,1,SEGMENT_BORDER_COLOR);
   CBrush brush(SEGMENT_FILL_COLOR);
   CPen* pOldPen = dc.SelectObject(&pen);
   CBrush* pOldBrush = dc.SelectObject(&brush);

   CFont* pFont = GetParent()->GetFont();
   CFont* pOldFont = dc.SelectObject(pFont);

   dc.SetBkMode(TRANSPARENT);
   dc.SetTextAlign(TA_BOTTOM | TA_CENTER);

   // Draw the precast girder segments
   SegmentIndexType segIdx = 0;
   std::vector<CComPtr<IShape>>::iterator shapeIter(segmentShapes.begin());
   std::vector<CComPtr<IShape>>::iterator shapeIterEnd(segmentShapes.end());
   for ( ; shapeIter != shapeIterEnd; shapeIter++ )
   {
      CComPtr<IShape> shape = *shapeIter;
      DrawShape(&dc,mapper,shape);


      CComPtr<IShapeProperties> props;
      shape->get_ShapeProperties(&props);

      CComPtr<IPoint2d> cg;
      props->get_Centroid(&cg);

      long dx,dy;
      mapper.WPtoDP(cg,&dx,&dy);
      CString str;
      str.Format(_T("%d"),LABEL_SEGMENT(segIdx++));
      dc.TextOut(dx,dy,str);
   }

   // Draw the tendons
   CPen tendonColor(PS_SOLID,1,TENDON_LINE_COLOR);
   dc.SelectObject(&tendonColor);
   std::vector<CComPtr<IPoint2dCollection>>::iterator ductIter(ducts.begin());
   std::vector<CComPtr<IPoint2dCollection>>::iterator ductIterEnd(ducts.end());
   for ( ; ductIter != ductIterEnd; ductIter++ )
   {
      CComPtr<IPoint2dCollection> points = *ductIter;
      Draw(&dc,mapper,points,FALSE);
   }

   // Clean up
   dc.SelectObject(pOldPen);
   dc.SelectObject(pOldBrush);
   dc.SelectObject(pOldFont);
}

void CDrawTendonsControl::DrawShape(CDC* pDC,grlibPointMapper& mapper,IShape* pShape)
{
   CComPtr<IPoint2dCollection> polypoints;
   pShape->get_PolyPoints(&polypoints);

   Draw(pDC,mapper,polypoints,TRUE);
}

void CDrawTendonsControl::Draw(CDC* pDC,grlibPointMapper& mapper,IPoint2dCollection* pPolyPoints,BOOL bPolygon)
{
   CollectionIndexType nPoints;
   pPolyPoints->get_Count(&nPoints);

   IPoint2d** points = new IPoint2d*[nPoints];

   CComPtr<IEnumPoint2d> enum_points;
   pPolyPoints->get__Enum(&enum_points);

   ULONG nFetched;
   enum_points->Next((ULONG)nPoints,points,&nFetched);
   ATLASSERT(nFetched == nPoints);

   CPoint* dev_points = new CPoint[nPoints];
   for ( CollectionIndexType i = 0; i < nPoints; i++ )
   {
      LONG dx,dy;
      mapper.WPtoDP(points[i],&dx,&dy);
      dev_points[i] = CPoint(dx,dy);

      points[i]->Release();
   }

   if ( bPolygon )
      pDC->Polygon(dev_points,(int)nPoints);
   else
      pDC->Polyline(dev_points,(int)nPoints);

   delete[] points;
   delete[] dev_points;
}
