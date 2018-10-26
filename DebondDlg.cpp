///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2012  Washington State Department of Transportation
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

// DebondDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "DebondDlg.h"
#include "GirderDescDlg.h"
#include "PGSuperColors.h"
#include <DesignConfigUtil.h>
#include <IFace\Bridge.h>
#include "HtmlHelp\HelpTopics.hh"

#include <MFCTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NO_DEBOND_FILL_COLOR GREY90

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage dialog


CGirderDescDebondPage::CGirderDescDebondPage()
	: CPropertyPage(CGirderDescDebondPage::IDD)
{
	//{{AFX_DATA_INIT(CGirderDescDebondPage)
	//}}AFX_DATA_INIT

}

void CGirderDescDebondPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   //{{AFX_DATA_MAP(CGirderDescDebondPage)
	//}}AFX_DATA_MAP

	DDX_Check_Bool(pDX, IDC_SYMMETRIC_DEBOND, pParent->m_GirderData.PrestressData.bSymmetricDebond);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   if ( pDX->m_bSaveAndValidate )
   {
      GET_IFACE2(pBroker, IBridge,pBridge);
      Float64 gdr_length2 = pBridge->GetGirderLength(pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx)/2.0;

      m_Grid.GetData(pParent->m_GirderData);

      for (std::vector<CDebondInfo>::iterator iter = pParent->m_GirderData.PrestressData.Debond[pgsTypes::Straight].begin(); 
           iter != pParent->m_GirderData.PrestressData.Debond[pgsTypes::Straight].end(); iter++ )
      {
         CDebondInfo& debond_info = *iter;
         if (gdr_length2 <= debond_info.Length1 || gdr_length2 <= debond_info.Length2)
         {
            HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_DEBOND_GRID);
	         AfxMessageBox( _T("Debond length cannot exceed one half of girder length."), MB_ICONEXCLAMATION);
	         pDX->Fail();
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CGirderDescDebondPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescDebondPage)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_SYMMETRIC_DEBOND, OnSymmetricDebond)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondPage message handlers
BOOL CGirderDescDebondPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();

	m_Grid.SubclassDlgItem(IDC_DEBOND_GRID, this);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   m_Grid.CustomInit(pParent->m_GirderData.PrestressData.bSymmetricDebond ? TRUE : FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CGirderDescDebondPage::OnSetActive() 
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   bool bCanDebond = pStrandGeometry->CanDebondStrands(pParent->m_strGirderName.c_str(),pgsTypes::Straight);
   m_Grid.CanDebond(bCanDebond);
   GetDlgItem(IDC_SYMMETRIC_DEBOND)->ShowWindow(bCanDebond ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_NUM_DEBONDED)->ShowWindow(bCanDebond ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_NOTE)->ShowWindow(bCanDebond ? SW_SHOW : SW_HIDE);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   GetDlgItem(IDC_NUM_EXTENDED_LEFT)->ShowWindow(pSpecEntry->AllowStraightStrandExtensions() ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_NUM_EXTENDED_RIGHT)->ShowWindow(pSpecEntry->AllowStraightStrandExtensions() ? SW_SHOW : SW_HIDE);

   StrandIndexType nStrands = pParent->GetStraightStrandCount();
   ConfigStrandFillVector strtvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   ReconcileDebonding(strtvec, pParent->m_GirderData.PrestressData.Debond[pgsTypes::Straight]); 

   for ( int i = 0; i < 2; i++ )
   {
      std::vector<GridIndexType> extStrands = pParent->m_GirderData.PrestressData.GetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i);
      bool bChanged = ReconcileExtendedStrands(strtvec, extStrands);

      if ( bChanged )
         pParent->m_GirderData.PrestressData.SetExtendedStrands(pgsTypes::Straight,(pgsTypes::MemberEndType)i,extStrands);
   }

   // fill up the grid
   m_Grid.FillGrid(pParent->m_GirderData);

   BOOL enab = nStrands>0 ? TRUE:FALSE;
   GetDlgItem(IDC_SYMMETRIC_DEBOND)->EnableWindow(enab);

   m_Grid.SelectRange(CGXRange().SetTable(), FALSE);

   OnChange();

   return CPropertyPage::OnSetActive();
}

BOOL CGirderDescDebondPage::OnKillActive()
{
   this->SetFocus();  // prevents artifacts from grid list controls (not sure why)

   return CPropertyPage::OnKillActive();
}

std::vector<CDebondInfo> CGirderDescDebondPage::GetDebondInfo() const
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   return pParent->m_GirderData.PrestressData.Debond[pgsTypes::Straight];
}


void CGirderDescDebondPage::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CProperyPage::OnPaint() for painting messages

   // Draw the girder cross section and label the strand locations
   // The basic logic from this code is take from
   // Programming Microsoft Visual C++, Fifth Edition
   // Kruglinski, Shepherd, and Wingo
   // Page 129
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CWnd* pWnd = GetDlgItem(IDC_PICTURE);
   CRect redit;
   pWnd->GetClientRect(&redit);
   CRgn rgn;
   VERIFY(rgn.CreateRectRgn(redit.left,redit.top,redit.right,redit.bottom));
   CDC* pDC = pWnd->GetDC();
   pDC->SelectClipRgn(&rgn);
   pWnd->Invalidate();
   pWnd->UpdateWindow();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());

   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);

   CComPtr<IGirderSection> gdrSection;
   factory->CreateGirderSection(pBroker,-1,pParent->m_CurrentSpanIdx,pParent->m_CurrentGirderIdx,pGdrEntry->GetDimensions(),&gdrSection);

   CComQIPtr<IShape> shape(gdrSection);

   CComQIPtr<IXYPosition> position(shape);
   CComPtr<IPoint2d> lp;
   position->get_LocatorPoint(lpBottomCenter,&lp);
   lp->Move(0,0);
   position->put_LocatorPoint(lpBottomCenter,lp);


   // Get the world height to be equal to the height of the area 
   // occupied by the strands
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   Float64 y_min =  DBL_MAX;
   Float64 y_max = -DBL_MAX;
   StrandIndexType nStrands = GetNumStrands();

   ConfigStrandFillVector  fillvec = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   PRESTRESSCONFIG config;
   config.SetStrandFill(pgsTypes::Straight, fillvec);

   CComPtr<IPoint2dCollection> points;
   pStrandGeometry->GetStrandPositionsEx(pParent->m_strGirderName.c_str(),config,pgsTypes::Straight,pgsTypes::metStart,&points);
   for ( StrandIndexType strIdx = 0; strIdx < nStrands; strIdx++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strIdx,&point);
      double y;
      point->get_Y(&y);
      y_min = _cpp_min(y,y_min);
      y_max = _cpp_max(y,y_max);
   }
   gpSize2d size;
   
   Float64 bottom_width;
   gdrSection->get_BottomWidth(&bottom_width);
   size.Dx() = bottom_width;

   size.Dy() = (y_max - y_min);
   if ( IsZero(size.Dy()) )
      size.Dy() = size.Dx()/2;

   CSize csize = redit.Size();

   CComPtr<IRect2d> box;
   shape->get_BoundingBox(&box);

   CComPtr<IPoint2d> objOrg;
   box->get_BottomCenter(&objOrg);

   gpPoint2d org;
   Float64 x,y;
   objOrg->get_X(&x);
   objOrg->get_Y(&y);
   org.X() = x;
   org.Y() = y;

   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
   mapper.SetWorldExt(size);
   mapper.SetWorldOrg(org);
   mapper.SetDeviceExt(csize.cx-10,csize.cy-10);
   mapper.SetDeviceOrg(csize.cx/2,csize.cy-5);


   CPen solid_pen(PS_SOLID,1,GIRDER_BORDER_COLOR);
   CBrush solid_brush(GIRDER_FILL_COLOR);

   CPen void_pen(PS_SOLID,1,VOID_BORDER_COLOR);
   CBrush void_brush(GetSysColor(COLOR_WINDOW));

   CPen* pOldPen     = pDC->SelectObject(&solid_pen);
   CBrush* pOldBrush = pDC->SelectObject(&solid_brush);

   CComQIPtr<ICompositeShape> compshape(shape);
   if ( compshape )
   {
      CollectionIndexType nShapes;
      compshape->get_Count(&nShapes);
      for ( CollectionIndexType idx = 0; idx < nShapes; idx++ )
      {
         CComPtr<ICompositeShapeItem> item;
         compshape->get_Item(idx,&item);

         CComPtr<IShape> s;
         item->get_Shape(&s);

         VARIANT_BOOL bVoid;
         item->get_Void(&bVoid);

         if ( bVoid )
         {
            pDC->SelectObject(&void_pen);
            pDC->SelectObject(&void_brush);
         }
         else
         {
            pDC->SelectObject(&solid_pen);
            pDC->SelectObject(&solid_brush);
         }

         DrawShape(pDC,s,mapper);
      }
   }
   else
   {
      DrawShape(pDC,shape,mapper);
   }

   DrawStrands(pDC,mapper);

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   pWnd->ReleaseDC(pDC);
}

void CGirderDescDebondPage::DrawShape(CDC* pDC,IShape* shape,grlibPointMapper& mapper)
{
   CComPtr<IPoint2dCollection> objPoints;
   shape->get_PolyPoints(&objPoints);

   CollectionIndexType nPoints;
   objPoints->get_Count(&nPoints);

   CPoint* points = new CPoint[nPoints];

   CComPtr<IPoint2d> point;
   long dx,dy;

   long i = 0;
   CComPtr<IEnumPoint2d> enumPoints;
   objPoints->get__Enum(&enumPoints);
   while ( enumPoints->Next(1,&point,NULL) != S_FALSE )
   {
      mapper.WPtoDP(point,&dx,&dy);

      points[i] = CPoint(dx,dy);

      point.Release();
      i++;
   }

   pDC->Polygon(points,(int)nPoints);

   delete[] points;
}

void CGirderDescDebondPage::DrawStrands(CDC* pDC,grlibPointMapper& mapper)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CPen strand_pen(PS_SOLID,1,STRAND_BORDER_COLOR);
   CPen no_debond_pen(PS_SOLID,1,NO_DEBOND_FILL_COLOR);
   CPen debond_pen(PS_SOLID,1,DEBOND_FILL_COLOR);
   CPen extended_pen(PS_SOLID,1,EXTENDED_FILL_COLOR);
   CPen* old_pen = (CPen*)pDC->SelectObject(&strand_pen);

   CBrush strand_brush(STRAND_FILL_COLOR);
   CBrush no_debond_brush(NO_DEBOND_FILL_COLOR);
   CBrush debond_brush(DEBOND_FILL_COLOR);
   CBrush extended_brush(EXTENDED_FILL_COLOR);
   CBrush* old_brush = (CBrush*)pDC->SelectObject(&strand_brush);

   pDC->SetTextAlign(TA_CENTER);
   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);
   CFont* old_font = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   // Draw all the strands bonded
   StrandIndexType nStrands = GetNumStrands();

   ConfigStrandFillVector  straightStrandFill = pParent->ComputeStrandFillVector(pgsTypes::Straight);
   PRESTRESSCONFIG config;
   config.SetStrandFill(pgsTypes::Straight, straightStrandFill);

   CComPtr<IPoint2dCollection> points;
   pStrandGeometry->GetStrandPositionsEx(pParent->m_strGirderName.c_str(),config,pgsTypes::Straight,pgsTypes::metStart,&points);

   CComPtr<IIndexArray> debondables;
   pStrandGeometry->ListDebondableStrands(pParent->m_strGirderName.c_str(), straightStrandFill,pgsTypes::Straight, &debondables); 

   const int strand_size = 2;
   for ( StrandIndexType strIdx = 0; strIdx <nStrands; strIdx++ )
   {
      CComPtr<IPoint2d> point;
      points->get_Item(strIdx,&point);

      StrandIndexType is_debondable = 0;
      debondables->get_Item(strIdx, &is_debondable);

      LONG dx,dy;
      mapper.WPtoDP(point,&dx,&dy);

      CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

      if (is_debondable)
      {
         pDC->SelectObject(&strand_pen);
         pDC->SelectObject(&strand_brush);
      }
      else
      {
         pDC->SelectObject(&no_debond_pen);
         pDC->SelectObject(&no_debond_brush);
      }

      pDC->Ellipse(&rect);

      CString strLabel;
      strLabel.Format(_T("%d"),strIdx+1);
      pDC->TextOut(dx,dy,strLabel);
   }

   // Redraw the debonded strands
   pDC->SelectObject(&debond_pen);
   pDC->SelectObject(&debond_brush);

   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(pParent->m_strGirderName.c_str());

   std::vector<CDebondInfo>::iterator debond_iter;
   CGirderData girderData = pParent->m_GirderData;
   m_Grid.GetData(girderData);
   for ( debond_iter = girderData.PrestressData.Debond[pgsTypes::Straight].begin(); debond_iter != girderData.PrestressData.Debond[pgsTypes::Straight].end(); debond_iter++ )
   {
      CDebondInfo& debond_info = *debond_iter;

      if ( debond_info.strandTypeGridIdx == INVALID_INDEX )
      {
         ATLASSERT(0); // we should be protecting against this
         continue;
      }

      // Library entry uses grid indexing (same as debonding)
      Float64 xs, xe, ys, ye;
      bool candb;
      pGdrEntry->GetStraightStrandCoordinates( debond_info.strandTypeGridIdx, &xs, &ys, &xe, &ye, &candb);

      long dx,dy;
      mapper.WPtoDP(xs, ys, &dx,&dy);

      CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

      pDC->Ellipse(&rect);

      if ( xs > 0.0 )
      {
         mapper.WPtoDP(-xs, ys, &dx,&dy);

         CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

         pDC->Ellipse(&rect);
      }
   }


   // Redraw the extended strands
   pDC->SelectObject(&extended_pen);
   pDC->SelectObject(&extended_brush);

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      const std::vector<GridIndexType>& extStrandsStart(girderData.PrestressData.GetExtendedStrands(pgsTypes::Straight,endType));
      std::vector<GridIndexType>::const_iterator ext_iter(extStrandsStart.begin());
      std::vector<GridIndexType>::const_iterator ext_iter_end(extStrandsStart.end());
      for ( ; ext_iter != ext_iter_end; ext_iter++ )
      {
         GridIndexType gridIdx = *ext_iter;

         // Library entry uses grid indexing (same as debonding)
         Float64 xs, xe, ys, ye;
         bool candb;
         pGdrEntry->GetStraightStrandCoordinates( gridIdx, &xs, &ys, &xe, &ye, &candb);

         long dx,dy;
         mapper.WPtoDP(xs, ys, &dx,&dy);

         CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

         pDC->Ellipse(&rect);

         if ( xs > 0.0 )
         {
            mapper.WPtoDP(-xs, ys, &dx,&dy);

            CRect rect(dx-strand_size,dy-strand_size,dx+strand_size,dy+strand_size);

            pDC->Ellipse(&rect);
         }
      }
   }

   pDC->SelectObject(old_pen);
   pDC->SelectObject(old_brush);
   pDC->SelectObject(old_font);

}

StrandIndexType CGirderDescDebondPage::GetNumStrands()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   StrandIndexType nStrands =  pParent->GetStraightStrandCount();

   return nStrands;
}

bool CGirderDescDebondPage:: CanDebondMore()
{
   // how many can be debonded?
   StrandIndexType nStrands = GetNumStrands(); 
   StrandIndexType nDebondable = 0; // number of strand that can be debonded
   for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
   {
      StrandIndexType is_debondable;
      m_Debondables->get_Item(strandIdx, &is_debondable);
      if (is_debondable)
      {
         nDebondable++;
      }
   }
   
   StrandIndexType nDebonded =  m_Grid.GetNumDebondedStrands(); // number of strands that are debonded

   return nDebonded < nDebondable; // can debond more if the number of debondable exceed number of debonded
}

void CGirderDescDebondPage::OnSymmetricDebond() 
{
   UINT checked = IsDlgButtonChecked(IDC_SYMMETRIC_DEBOND);
   m_Grid.SymmetricDebond( checked != 0 );
}

void CGirderDescDebondPage::OnHelp() 
{
	::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_DEBOND );
}

void CGirderDescDebondPage::OnChange() 
{
   StrandIndexType ns = GetNumStrands(); 
   StrandIndexType ndbs =  m_Grid.GetNumDebondedStrands();
   Float64 percent = 0.0;
   if (0 < ns && ns != INVALID_INDEX)
      percent = 100.0 * (Float64)ndbs/(Float64)ns;

   CString str;
   str.Format(_T("Straight=%d"), ns);
   CWnd* pNs = GetDlgItem(IDC_NUMSTRAIGHT);
   pNs->SetWindowText(str);

   str.Format(_T("Debonded=%d (%.1f%%)"), ndbs, percent);
   CWnd* pNdb = GetDlgItem(IDC_NUM_DEBONDED);
   pNdb->SetWindowText(str);

   StrandIndexType nExtStrands = m_Grid.GetNumExtendedStrands(pgsTypes::metStart);
   str.Format(_T("Extended Left=%d"),nExtStrands);
   CWnd* pNExt = GetDlgItem(IDC_NUM_EXTENDED_LEFT);
   pNExt->SetWindowText(str);

   nExtStrands = m_Grid.GetNumExtendedStrands(pgsTypes::metEnd);
   str.Format(_T("Extended Right=%d"),nExtStrands);
   pNExt = GetDlgItem(IDC_NUM_EXTENDED_RIGHT);
   pNExt->SetWindowText(str);

   CWnd* pPicture = GetDlgItem(IDC_PICTURE);
   CRect rect;
   pPicture->GetWindowRect(rect);
   ScreenToClient(&rect);
   InvalidateRect(rect);
   UpdateWindow();
}

HBRUSH CGirderDescDebondPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
   int ID = pWnd->GetDlgCtrlID();
   switch( ID )
   {
   case IDC_NUMSTRAIGHT:
      pDC->SetTextColor(STRAIGHT_FILL_COLOR);
      break;

   case IDC_NUM_DEBONDED:
      pDC->SetTextColor(DEBOND_FILL_COLOR);
      break;

   case IDC_NUM_EXTENDED_LEFT:
   case IDC_NUM_EXTENDED_RIGHT:
      pDC->SetTextColor(EXTENDED_FILL_COLOR);
      break;
   }

   return hbr;
}
