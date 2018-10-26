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

// SegmentSpacingGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "SegmentSpacingGrid.h"
#include "GirderSegmentSpacingPage.h"
#include "TemporarySupportDlg.h"

#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PierData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CSegmentSpacingGrid, CS_DBLCLKS, 0, 0, 0);

void DDV_SpacingGrid(CDataExchange* pDX,int nIDC,CSegmentSpacingGrid* pGrid)
{
   if (!pDX->m_bSaveAndValidate )
      return;

   pDX->PrepareCtrl(nIDC);
   if ( !pGrid->ValidateSpacing() )
   {
      pDX->Fail();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CSegmentSpacingGrid

CSegmentSpacingGrid::CSegmentSpacingGrid()
{
//   RegisterClass();
   m_bEnabled = TRUE;
}

CSegmentSpacingGrid::~CSegmentSpacingGrid()
{
}

BEGIN_MESSAGE_MAP(CSegmentSpacingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderNameGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(IDC_EXPAND, OnExpand)
	ON_COMMAND(IDC_JOIN, OnJoin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CGirderSpacingData2& CSegmentSpacingGrid::GetSpacingData()
{
   return m_SpacingData;
}

void CSegmentSpacingGrid::SetSpacingData(const CGirderSpacingData2& spacingData)
{
   m_SpacingData = spacingData;
}

void CSegmentSpacingGrid::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   m_SpacingData.SetMeasurementType(mt);
}

void CSegmentSpacingGrid::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   m_SpacingData.SetMeasurementLocation(ml);
}

void CSegmentSpacingGrid::SetSkewAngle(Float64 skewAngle)
{
   m_SkewAngle = skewAngle;
   FillGrid();
}

bool CSegmentSpacingGrid::InputSpacing() const
{
   ATLASSERT(m_MinGirderSpacing.size() == m_MaxGirderSpacing.size());
   std::vector<Float64>::const_iterator minIter, maxIter;
   for ( minIter  = m_MinGirderSpacing.begin(), maxIter  = m_MaxGirderSpacing.begin();
         minIter != m_MinGirderSpacing.end(),   maxIter != m_MaxGirderSpacing.end();
         minIter++, maxIter++ )
   {
      Float64 v = (*maxIter) - (*minIter);
      if ( !IsZero(v) )
         return true;
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////
// CSegmentSpacingGrid message handlers
void CSegmentSpacingGrid::CustomInit()
{
   CGirderSegmentSpacingPage* pParent = (CGirderSegmentSpacingPage*)GetParent();

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IBridge,pBridge);

   pBridge->GetSkewAngle(pParent->GetStation(),pParent->GetOrientation(),&m_SkewAngle);
   m_GirderSpacingType = pParent->GetBridgeDescription().GetGirderSpacingType();
   m_SpacingData = pParent->GetGirderSpacing();

	Initialize( );

   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELROW));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   FillGrid();

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();
   SetStyleRange(CGXRange(0,0,GetRowCount(),GetColCount()),
      CGXStyle()
      .SetWrapText(TRUE)
      .SetAutoSize(TRUE)
      .SetUserAttribute(GX_IDS_UA_TOOLTIPTEXT,"To regroup girder spacing, select column headings, right click over the grid, and select Expand or Join from the menu")
      );
	
   GetParam()->EnableUndo(TRUE);

	SetFocus();
}

void CSegmentSpacingGrid::FillGrid(const CGirderSpacing2* pGirderSpacing)
{
   // why is this commented out???
//   m_GridData.m_GirderSpacing = *pGirderSpacing;

   FillGrid();
}

void CSegmentSpacingGrid::FillGrid()
{
   if ( GetSafeHwnd() == NULL )
      return; // grid isn't ready for filling

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& spacingUnit = IsGirderSpacing(m_GirderSpacingType) // if
                                        ? pDisplayUnits->GetXSectionDimUnit()     // then
                                        : pDisplayUnits->GetComponentDimUnit();   // else

   GroupIndexType nSpacingGroups = m_SpacingData.GetSpacingGroupCount();
   GirderIndexType nGirders      = m_SpacingData.GetSpacingCount() + 1;

   // Compute the girder spacing correction for skew angle
   Float64 skewCorrection;
   if ( m_SpacingData.GetMeasurementType() == pgsTypes::NormalToItem )
   {
      skewCorrection = 1;
   }
   else
   {
      skewCorrection = fabs(1/cos(m_SkewAngle));
   }

   const ROWCOL num_rows = 2;
   const ROWCOL num_cols = (ROWCOL)nSpacingGroups;

   SetRowCount(num_rows);
	SetColCount(num_cols);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   CGirderSegmentSpacingPage* pParent = (CGirderSegmentSpacingPage*)GetParent();

   const CBridgeDescription2& bridgeDesc = pParent->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType  = bridgeDesc.GetDeckDescription()->DeckType;

   m_MinGirderSpacing.clear();
   m_MaxGirderSpacing.clear();
   for ( GroupIndexType grpIdx = 0; grpIdx < nSpacingGroups; grpIdx++ )
   {
      Float64 spacing;
      GirderIndexType firstGdrIdx, lastGdrIdx;
      m_SpacingData.GetSpacingGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

      // if this is the last group and the girder spacing is uniform
      // then the last girder index needs to be forced to nGirders-1
      if ( grpIdx == nSpacingGroups-1 && IsBridgeSpacing(m_GirderSpacingType) )
      {
         lastGdrIdx = nGirders-1;
      }

      CString strHeading;
      if ( firstGdrIdx == lastGdrIdx )
      {
         strHeading.Format(_T("%s (%s)"),LABEL_GIRDER(firstGdrIdx),spacingUnit.UnitOfMeasure.UnitTag().c_str());
      }
      else
      {
         strHeading.Format(_T("%s-%s (%s)"),LABEL_GIRDER(firstGdrIdx),LABEL_GIRDER(lastGdrIdx),spacingUnit.UnitOfMeasure.UnitTag().c_str());
      }

      UserData* pUserData = new UserData(firstGdrIdx,lastGdrIdx); // the grid will delete this

      SetStyleRange(CGXRange(0,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetValue(strHeading)
         .SetItemDataPtr((void*)pUserData)
         );

      // get valid girder spacing for this group
      Float64 minGirderSpacing = -MAX_GIRDER_SPACING;
      Float64 maxGirderSpacing =  MAX_GIRDER_SPACING;
      for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
      {
         const GirderLibraryEntry* pGdrEntry = bridgeDesc.GetGirderLibraryEntry();
         const IBeamFactory::Dimensions& dimensions = pGdrEntry->GetDimensions();
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         // save spacing range to local class data
         Float64 minGS, maxGS;
         factory->GetAllowableSpacingRange(dimensions,deckType,m_GirderSpacingType,&minGS, &maxGS);
         minGS *= skewCorrection;
         maxGS *= skewCorrection;

         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            // girder spacing
            minGirderSpacing = _cpp_max(minGirderSpacing,minGS);
            maxGirderSpacing = _cpp_min(maxGirderSpacing,maxGS);
         }
         else
         {
            // joint spacing
            minGirderSpacing = 0;
            maxGirderSpacing = _cpp_min(maxGirderSpacing-minGirderSpacing,maxGS-minGS);
         }
      }

      m_MinGirderSpacing.push_back(minGirderSpacing);
      m_MaxGirderSpacing.push_back(maxGirderSpacing);

      ATLASSERT( minGirderSpacing <= maxGirderSpacing );

      spacing = ForceIntoRange(minGirderSpacing,spacing,maxGirderSpacing);
      m_SpacingData.SetGirderSpacing(grpIdx,spacing);

      CString strSpacing;
      strSpacing.Format(_T("%s"),FormatDimension(spacing,spacingUnit,false));
      SetStyleRange(CGXRange(1,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue( strSpacing )
         );

      if (maxGirderSpacing < MAX_GIRDER_SPACING)
      {
         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            // girder spacing
            strSpacing.Format(_T("%s - %s"), 
               FormatDimension(minGirderSpacing,spacingUnit,false),
               FormatDimension(maxGirderSpacing,spacingUnit,false));
         }
         else
         {
            // joint spacing
            strSpacing.Format(_T("%s - %s"), 
               FormatDimension(0.0,spacingUnit,false),
               FormatDimension(maxGirderSpacing-minGirderSpacing,spacingUnit,false));
         }
      }
      else
      {
         strSpacing.Format(_T("%s or more"), 
            FormatDimension(minGirderSpacing,spacingUnit,false));
      }

      SetStyleRange(CGXRange(2,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue( strSpacing )
         );
   }

   SetStyleRange(CGXRange(0,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(_T(""))
      );

   SetStyleRange(CGXRange(1,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(_T("Spacing"))
      );

   SetStyleRange(CGXRange(2,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(_T("Allowable"))
      );

   // make it so that text fits correctly in header row
   ResizeColWidthsToFit(CGXRange(0,0,num_rows,num_cols),FALSE);

   // this will get the styles of all the cells to be correct if the size of the grid changed
   Enable(m_bEnabled);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CSegmentSpacingGrid::SetGirderSpacingType(pgsTypes::SupportedBeamSpacing girderSpacingType)
{
   m_GirderSpacingType = girderSpacingType;

   //// if we are moving to general spacing, then the girder spacing isn't associated with a span 
   //// (make the span null so the spacing object can't go to the parent bridge and get spacing)
   //if ( IsSpanSpacing(girderSpacingType) )
   //   m_GridData.m_GirderSpacing.SetSpan(NULL);

   FillGrid();
}

BOOL CSegmentSpacingGrid::OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState)
{
   if ( nHitState & GX_HITSTART )
      return TRUE;

   if ( nHitCol != 0 )
   {
      CRowColArray selCols;
      ROWCOL nSelected = GetSelectedCols(selCols);

      if ( 0 < nSelected )
      {
         CMenu menu;
         VERIFY( menu.LoadMenu(IDR_GIRDER_GRID_CONTEXT) );
         ClientToScreen(&point);
         menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

         return TRUE;
      }
   }

   return FALSE;
}

void CSegmentSpacingGrid::OnExpand()
{
   CRowColArray selCols;
   ROWCOL nSelCols = GetSelectedCols(selCols);

   if ( nSelCols == 0 )
      return;

   if ( nSelCols == GetColCount() )
   {
      m_SpacingData.ExpandAll();
   }
   else
   {
      for ( int i = nSelCols-1; 0 <= i; i-- )
      {
         ROWCOL col = selCols[i];
         GroupIndexType grpIdx = GroupIndexType(col-1);

         m_SpacingData.Expand(grpIdx);
      }
   }

   FillGrid();
}

void CSegmentSpacingGrid::OnJoin()
{
   CRowColArray selLeftCols,selRightCols;
   GetSelectedCols(selLeftCols,selRightCols,FALSE,FALSE);
   ROWCOL nSelRanges = (ROWCOL)selLeftCols.GetSize();
   ASSERT( 0 < nSelRanges ); // must be more that one selected column to join

   for (ROWCOL i = 0; i < nSelRanges; i++ )
   {
      ROWCOL firstCol = selLeftCols[i];
      ROWCOL lastCol  = selRightCols[i];

      CGXStyle firstColStyle, lastColStyle;
      GetStyleRowCol(0,firstCol,firstColStyle);
      GetStyleRowCol(0,lastCol, lastColStyle);

      UserData* pFirstColUserData = (UserData*)firstColStyle.GetItemDataPtr();
      UserData* pLastColUserData =  (UserData*)lastColStyle.GetItemDataPtr();

      GirderIndexType firstGdrIdx = pFirstColUserData->first;
      GirderIndexType lastGdrIdx  = pLastColUserData->second;

      m_SpacingData.Join(firstGdrIdx,lastGdrIdx,firstGdrIdx);
   }

   FillGrid();
}

void CSegmentSpacingGrid::Enable(BOOL bEnable)
{
   m_bEnabled = bEnable;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if ( bEnable )
   {
      // Girder/Joint Spacing

      style.SetEnabled(TRUE)
           .SetReadOnly(FALSE)
           .SetInterior(::GetSysColor(COLOR_WINDOW))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      if ( 0 < GetColCount() )
      {
         for ( ROWCOL col = 1; col <= GetColCount(); col++ )
         {
            // all of the spacing values
            range = CGXRange(1,col);
            if ( IsEqual(m_MinGirderSpacing[col-1],m_MaxGirderSpacing[col-1]) )
            {
               // disable if there isn't a "range" of input
               style.SetEnabled(FALSE)
                    .SetReadOnly(TRUE)
                    .SetInterior(::GetSysColor(COLOR_BTNFACE))
                    .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
            }

            SetStyleRange(range,style);
         }
      }

      // Column Headings
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetEnabled(FALSE)
           .SetReadOnly(TRUE);

      // across the top
      range = CGXRange(0,0,0,GetColCount());
      SetStyleRange(range,style);

      // the "Ahead/Back" box
      range = CGXRange(1,0);
      SetStyleRange(range,style);

      // the "Allowable" box + the actual allowable spacing cells
      range = CGXRange(2,0,2,GetColCount());
      SetStyleRange(range,style);
   }
   else
   {
      style.SetEnabled(FALSE)
           .SetReadOnly(TRUE)
           .SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0,0,GetRowCount(),GetColCount());
      SetStyleRange(range,style);
   }


   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}

BOOL CSegmentSpacingGrid::ValidateSpacing()
{
   ROWCOL nCols = GetColCount();
   for ( ROWCOL col = 1; col <= nCols; col++ )
   {
      if ( !OnValidateCell(1,col) )
      {
         DisplayWarningText();
         return FALSE;
      }
   }

   return TRUE;
}

BOOL CSegmentSpacingGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   CGXControl* pControl = GetControl(nRow,nCol);
   CWnd* pWnd = (CWnd*)pControl;

   CString strText;
   if ( pControl->IsInit() )
   {
      pControl->GetCurrentText(strText);
   }
   else
   {
      strText = GetValueRowCol(nRow,nCol);
   }


   Float64 spacing = _tstof(strText); // returns zero if error
   if ( spacing <= 0 )
   {
      if ( IsGirderSpacing(m_GirderSpacingType) )
      {
  	      SetWarningText (_T("Invalid girder spacing value"));
         return FALSE;
      }
      else
      {
         // zero is a valid joint spacing so let it go... if text string is not a number, it will be treated as zero for joints
         if ( !IsZero(spacing) )
         {
  	         SetWarningText (_T("Invalid joint spacing value"));
            return FALSE;
         }
      }
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   if ( IsGirderSpacing(m_GirderSpacingType) )
   {
      spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
      Float64 minGirderSpacing = m_MinGirderSpacing[nCol-1];
      Float64 maxGirderSpacing = m_MaxGirderSpacing[nCol-1];
      if ( IsLT(spacing,minGirderSpacing) || IsLT(maxGirderSpacing,spacing) )
      {
         SetWarningText(_T("Girder spacing is out of range"));
         return FALSE;
      }
   }
   else
   {
      spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      Float64 minGirderSpacing = m_MinGirderSpacing[nCol-1];
      Float64 maxGirderSpacing = m_MaxGirderSpacing[nCol-1];
      if ( spacing < 0 || IsGT(maxGirderSpacing-minGirderSpacing,spacing) )
      {
         SetWarningText(_T("Joint spacing is out of range"));
         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

BOOL CSegmentSpacingGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   if ( nRow == 1 && 1 <= nCol )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strValue;
      GetCurrentCellControl()->GetCurrentText(strValue);

      Float64 spacing = _tstof(strValue);

      if ( IsGirderSpacing(m_GirderSpacingType) )
      {
         // girder spacing
         spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
      }
      else
      {
         // joint spacing
         spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      }

      m_SpacingData.SetGirderSpacing(GroupIndexType(nCol-1),spacing);
      FillGrid();
   }

   return CGXGridWnd::OnEndEditing(nRow,nCol);
}
