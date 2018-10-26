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

// BridgeDescFramingGrid2.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "BridgeDescFramingGrid.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescDlg.h"
#include "BridgeDescFramingPage.h"

#include "SpanDetailsDlg.h"
#include "PierDetailsDlg.h"
#include "PGSuperAppPlugin\InsertSpanDlg.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"

#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\ClosureJointData.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CRowType : public CGXAbstractUserAttribute
{
public:
   enum RowType { Span, Pier, TempSupport };
   CRowType(RowType type,CollectionIndexType idx)
   {
      m_Type = type;
      m_Index = idx;
   }

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CRowType(m_Type,m_Index);
   }

   RowType m_Type;
   CollectionIndexType m_Index;
};

GRID_IMPLEMENT_REGISTER(CBridgeDescFramingGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid

CBridgeDescFramingGrid::CBridgeDescFramingGrid()
{
//   RegisterClass();
   HRESULT hr = m_objStation.CoCreateInstance(CLSID_Station);
   hr = m_objAngle.CoCreateInstance(CLSID_Angle);
   hr = m_objDirection.CoCreateInstance(CLSID_Direction);
}

CBridgeDescFramingGrid::~CBridgeDescFramingGrid()
{
}

BEGIN_MESSAGE_MAP(CBridgeDescFramingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CBridgeDescFramingGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

std::vector<Float64> CBridgeDescFramingGrid::GetSpanLengths()
{
   std::vector<Float64> spanLengths;

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   SpanIndexType nSpans = pDlg->m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      spanLengths.push_back( pDlg->m_BridgeDesc.GetSpan(spanIdx)->GetSpanLength() );
   }

   return spanLengths;
}

void CBridgeDescFramingGrid::SetPierOrientation(LPCTSTR strOrientation)
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );


   PierIndexType nPiers = pDlg->m_BridgeDesc.GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CPierData2* pPier = pDlg->m_BridgeDesc.GetPier(pierIdx);
      pPier->SetOrientation(strOrientation);
   }

   SupportIndexType nTS = pDlg->m_BridgeDesc.GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      CTemporarySupportData ts = *pDlg->m_BridgeDesc.GetTemporarySupport(tsIdx);
      ts.SetOrientation(strOrientation);
      pDlg->m_BridgeDesc.SetTemporarySupportByIndex(tsIdx,ts);
   }

   FillGrid(pDlg->m_BridgeDesc);
}

void CBridgeDescFramingGrid::SetSpanLengths(const std::vector<Float64>& spanLengths,PierIndexType fixedPierIdx)
{
   ROWCOL row = GetPierRow(fixedPierIdx);
   CPierData2* pFixedPierData = GetPierRowData(row);
   Float64 station = pFixedPierData->GetStation();

   // work backwards to the first pier
   PierIndexType idx = fixedPierIdx-1;
   while (0 <= idx && idx != ALL_PIERS)
   {
      station -= spanLengths[idx];
      idx--;
   }

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // holding first pier constant
   CPierData2* pPrevPier = pDlg->m_BridgeDesc.GetPier(0);
   pPrevPier->SetStation( station );

   ASSERT(spanLengths.size() == pDlg->m_BridgeDesc.GetSpanCount());

   std::vector<Float64>::const_iterator iter;
   for ( iter = spanLengths.begin(); iter != spanLengths.end(); iter++ )
   {
      Float64 L = *iter;

      CSpanData2* pNextSpan = pPrevPier->GetNextSpan();
      ASSERT(pNextSpan);

      CPierData2* pNextPier = pNextSpan->GetNextPier();
      pNextPier->SetStation( pPrevPier->GetStation() + L );

      pPrevPier = pNextPier;
   }

   FillGrid(pDlg->m_BridgeDesc);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid message handlers

BOOL CBridgeDescFramingGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_FRAMING_GRID2_CONTEXT));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != NULL );

   // deal with disabling delete since update stuff doesn't seem to work right
   UINT bCanDelete = EnableRemovePierBtn() ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
   pPopup->EnableMenuItem(IDC_REMOVE_PIER, bCanDelete);

   bCanDelete = EnableRemoveTemporarySupportBtn() ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
   pPopup->EnableMenuItem(IDC_REMOVE_TEMP_SUPPORT, bCanDelete);


	// display the menu
	ClientToScreen(&pt);
   CWnd* pParent = GetParent();
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, pParent);

	// we processed the message
	return TRUE;
}

void CBridgeDescFramingGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );
   pParent->EnableRemovePierBtn(EnableRemovePierBtn());
   pParent->EnableRemoveTemporarySupportBtn(EnableRemoveTemporarySupportBtn());
}

void CBridgeDescFramingGrid::InsertRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;
	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);

   ScrollCellInView(nRow+1, GetLeftCol());
}

void CBridgeDescFramingGrid::OnAddTemporarySupport()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   if ( nSelRows == 0 )
   {
      nSelRows = 1;
      sel_rows.Add(GetRowCount());
   }

   CTemporarySupportDlg dlg(&pDlg->m_BridgeDesc,INVALID_INDEX,pDlg->GetExtensionPages(),this);
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::OnAddPier()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // add at end of bridge
   CInsertSpanDlg dlg(&pDlg->m_BridgeDesc);;
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc.InsertSpan(dlg.m_RefPierIdx,dlg.m_PierFace,dlg.m_SpanLength,NULL,NULL,dlg.m_bCreateNewGroup,dlg.m_EventIndex);
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::OnRemovePier()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   PierIndexType nPiers = GetPierCount();

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];
      PierIndexType pierIdx = GetPierIndex(selRow);
      if ( pierIdx != INVALID_INDEX )
      {
         SpanIndexType spanIdx = (SpanIndexType)pierIdx;
         pgsTypes::RemovePierType removePier = (spanIdx == 0) ? pgsTypes::PrevPier : pgsTypes::NextPier;
         if (pierIdx == nPiers-1 )
         {
            spanIdx--;
         }

         pDlg->m_BridgeDesc.RemoveSpan(spanIdx,removePier);
      }
   }

   FillGrid(pDlg->m_BridgeDesc);
}

void CBridgeDescFramingGrid::OnRemoveTemporarySupport()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];
      SupportIndexType tsIdx = GetTemporarySupportIndex(selRow);
      if ( 0 <= tsIdx)
      {
         pDlg->m_BridgeDesc.RemoveTemporarySupportByIndex(tsIdx);
      }
   }

   FillGrid(pDlg->m_BridgeDesc);
}

bool CBridgeDescFramingGrid::EnableRemovePierBtn()
{
	if (GetParam() == NULL)
   {
		return false;
   }

   if ( GetPierCount() == 2 )
   {
      return false; // can't delete the last span
   }

   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
   {
      return false; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   PierIndexType pierIdx = GetPierIndex(selRow);

   if ( pierIdx == INVALID_INDEX )
   {
      return false;
   }

   return true;
}

bool CBridgeDescFramingGrid::EnableRemoveTemporarySupportBtn()
{
	if (GetParam() == NULL)
   {
		return false;
   }

   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
   {
      return false; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   SupportIndexType tsIdx = GetTemporarySupportIndex(selRow);

   if ( tsIdx == INVALID_INDEX )
   {
      return false;
   }

   return true;
}

void CBridgeDescFramingGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetMergeCellsMode(gxnMergeDelayEval);

   const int num_rows = 0;
   const int num_cols = 6;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE & ~GX_SELMULTIPLE & ~GX_SELSHIFT & ~GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
   int col = 0;
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   CString cv = _T("Station");
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = _T("Orientation");
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Segments"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Spans"))
      );

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Spans"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Temp Support"))
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

   GetParam()->SetLockReadOnly(TRUE);
	GetParam( )->EnableUndo(TRUE);
}

CString CBridgeDescFramingGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
    }
    else
    {
        return GetValueRowCol(nRow, nCol);
    }
}

CPierData2* CBridgeDescFramingGrid::GetPierRowData(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   PierIndexType pierIdx = GetPierIndex(nRow);
   ASSERT( pierIdx != ALL_PIERS );

   CPierData2* pPier = pDlg->m_BridgeDesc.GetPier(pierIdx);

   // update pier using the data in the grid

   // Station
   CString strStation = GetCellValue(nRow,1);
   UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
   m_objStation->FromString(CComBSTR(strStation),unitMode);
   Float64 station;
   m_objStation->get_Value(&station);
   pPier->SetStation( ::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure) );

   // Orientation
   CString strOrientation = GetCellValue(nRow,2);
   strOrientation.MakeUpper();
   strOrientation.TrimLeft();
   strOrientation.TrimRight();
   if ( strOrientation.GetLength() == 1 && strOrientation.GetAt(0) == 'N')
   {
      // if user input N for normal, fill out the whole word
      strOrientation = _T("NORMAL");
   }
   pPier->SetOrientation( strOrientation );

   return pPier;
}

CTemporarySupportData CBridgeDescFramingGrid::GetTemporarySupportRowData(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   SupportIndexType tsIdx = GetTemporarySupportIndex(nRow);
   ASSERT( tsIdx != INVALID_INDEX );

   CTemporarySupportData tsData = *pDlg->m_BridgeDesc.GetTemporarySupport(tsIdx);

   // update temporary support using the data in the grid

   // Station
   CString strStation = GetCellValue(nRow,1);
   UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
   m_objStation->FromString(CComBSTR(strStation),unitMode);
   Float64 station;
   m_objStation->get_Value(&station);
   station = ::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   tsData.SetStation(station);

   // Orientation
   CString strOrientation = GetCellValue(nRow,2);
   strOrientation.MakeUpper();
   strOrientation.TrimLeft();
   strOrientation.TrimRight();
   if ( strOrientation.GetLength() == 1 && strOrientation.GetAt(0) == 'N')
   {
      // if user input N for normal, fill out the whole word
      strOrientation = _T("NORMAL");
   }
   tsData.SetOrientation( strOrientation );

   return tsData;
}

void CBridgeDescFramingGrid::FillGrid(const CBridgeDescription2& bridgeDesc)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // remove all rows
   ROWCOL rows = GetRowCount();
   if ( 1 < rows )
   {
      RemoveRows(1, rows);
   }

   const CPierData2* pPier = bridgeDesc.GetPier(0);

   ROWCOL row = 1;

   // first pier
   InsertRow();
   FillPierRow(row++,pPier);

   Float64 last_station = pPier->GetStation();

   PierIndexType nPiers = bridgeDesc.GetPierCount();
   SupportIndexType nTS = bridgeDesc.GetTemporarySupportCount();

   PierIndexType pierIdx = 1;
   SupportIndexType tsIdx = 0;

   while ( pierIdx < nPiers )
   {
      pPier = bridgeDesc.GetPier(pierIdx);
      Float64 next_pier_station = pPier->GetStation();

      while ( tsIdx < nTS && bridgeDesc.GetTemporarySupport(tsIdx)->GetStation() < next_pier_station )
      {
         // temporary support is before the next pier... add it
         const CTemporarySupportData* pTS = bridgeDesc.GetTemporarySupport(tsIdx);
         InsertRow();
         Float64 ts_station = pTS->GetStation();
         FillSegmentRow(row++);
         last_station = ts_station;

         InsertRow();
         FillTemporarySupportRow(row++,pTS);

         tsIdx++;
      }

      InsertRow();
      FillSegmentRow(row++);
      last_station = pPier->GetStation();

      InsertRow();
      FillPierRow(row++,pPier);

      pierIdx++;
   }

   FillSegmentColumn();
   FillSpanColumn();

   HideCols(6,6,nTS == 0 ? TRUE : FALSE);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillPierRow(ROWCOL row,const CPierData2* pPierData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strStation = FormatStation(pDisplayUnits->GetStationFormat(),pPierData->GetStation());

   CString strPierLabel;
   strPierLabel.Format(_T("%s %d"),pPierData->IsAbutment() ? _T("Abut") : _T("Pier"),LABEL_PIER(pPierData->GetIndex()));

   int col = 0;
   // left column header
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetHorizontalAlignment(DT_LEFT)
      .SetValue(strPierLabel)
      .SetUserAttribute(0,CRowType(CRowType::Pier,pPierData->GetIndex()))
   );

   // station
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(strStation)
   );

   // orientation
   SetStyleRange(CGXRange(row,col++), CGXStyle()
        .SetHorizontalAlignment(DT_RIGHT)
        .SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
        .SetReadOnly(FALSE)
        .SetEnabled(TRUE)
        .SetValue(pPierData->GetOrientation())
      );

   col++; // segment column
   col++; // spans column
   col++; // spans column

   // temp support
   SetStyleRange(CGXRange(row,col++), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         );

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillTemporarySupportRow(ROWCOL row,const CTemporarySupportData* pTSData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   int col = 0;

   // label
   CString strLabel;
   strLabel.Format(_T("TS %d"),LABEL_TEMPORARY_SUPPORT(pTSData->GetIndex()));
   SetStyleRange(CGXRange(row,col++),CGXStyle()
      .SetValue(strLabel)
      .SetUserAttribute( 0, CRowType(CRowType::TempSupport,pTSData->GetIndex()) )
      );

   // station
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strStation = FormatStation(pDisplayUnits->GetStationFormat(),pTSData->GetStation());
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(strStation)
   );

   // orientation
   CGXStyle style;
   style.SetHorizontalAlignment(DT_RIGHT);
   style.SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
        .SetReadOnly(FALSE)
        .SetEnabled(TRUE)
        .SetValue(pTSData->GetOrientation());
   
   SetStyleRange(CGXRange(row,col++), style );

   col++; // segment column
   col++; // spans column
   col++; // spans column

   // edit button
   SetStyleRange(CGXRange(row,col++), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
         .SetUserAttribute( 0, CRowType(CRowType::TempSupport,pTSData->GetIndex()) )
         );

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSegmentRow(ROWCOL row)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetStyleRange(CGXRange(row,0,row,3), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
      .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
      .SetValue(_T(""))
   );

   // temp support
   SetStyleRange(CGXRange(row,6), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         );

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSegmentColumn()
{
   int col = 3;

   // This column not see for PGSuper
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      HideCols(col,col);
   }

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   GroupIndexType nGroups = pDlg->m_BridgeDesc.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CSplicedGirderData* pGirder = pDlg->m_BridgeDesc.GetGirderGroup(grpIdx)->GetGirder(0);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         ROWCOL startRow;
         CString strStartSupportLabel;
         Float64 startStation;
         if ( pSegment->GetLeftClosure() )
         {
            if ( pSegment->GetLeftClosure()->GetTemporarySupport() )
            {
               startRow = GetTemporarySupportRow(pSegment->GetLeftClosure()->GetTemporarySupport()->GetIndex());
               strStartSupportLabel.Format(_T("TS %d"),LABEL_TEMPORARY_SUPPORT(pSegment->GetLeftClosure()->GetTemporarySupport()->GetIndex()));
               startStation = pSegment->GetLeftClosure()->GetTemporarySupport()->GetStation();
            }
            else
            {
               startRow = GetPierRow(pSegment->GetLeftClosure()->GetPier()->GetIndex());
               strStartSupportLabel.Format(_T("Pier %d"),LABEL_PIER(pSegment->GetLeftClosure()->GetPier()->GetIndex()));
               startStation = pSegment->GetLeftClosure()->GetPier()->GetStation();
            }
         }
         else
         {
            startRow = GetPierRow(pGirder->GetPier(pgsTypes::metStart)->GetIndex());
            strStartSupportLabel.Format(_T("Pier %d"),LABEL_PIER(pGirder->GetPier(pgsTypes::metStart)->GetIndex()));
            startStation = pGirder->GetPier(pgsTypes::metStart)->GetStation();
         }

         ROWCOL endRow;
         CString strEndSupportLabel;
         Float64 endStation;
         if ( pSegment->GetRightClosure() )
         {
            if ( pSegment->GetRightClosure()->GetTemporarySupport() )
            {
               endRow = GetTemporarySupportRow(pSegment->GetRightClosure()->GetTemporarySupport()->GetIndex());
               strEndSupportLabel.Format(_T("TS %d"),LABEL_TEMPORARY_SUPPORT(pSegment->GetRightClosure()->GetTemporarySupport()->GetIndex()));
               endStation = pSegment->GetRightClosure()->GetTemporarySupport()->GetStation();
            }
            else
            {
               endRow = GetPierRow(pSegment->GetRightClosure()->GetPier()->GetIndex());
               strEndSupportLabel.Format(_T("Pier %d"),LABEL_PIER(pSegment->GetRightClosure()->GetPier()->GetIndex()));
               endStation = pSegment->GetRightClosure()->GetPier()->GetStation();
            }
         }
         else
         {
            endRow = GetPierRow(pGirder->GetPier(pgsTypes::metEnd)->GetIndex());
            strEndSupportLabel.Format(_T("Pier %d"),LABEL_PIER(pGirder->GetPier(pgsTypes::metEnd)->GetIndex()));
            endStation = pGirder->GetPier(pgsTypes::metEnd)->GetStation();
         }

         SetStyleRange(CGXRange(startRow,col), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
            .SetValue(strStartSupportLabel)
         );

         SetStyleRange(CGXRange(endRow,col), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
            .SetValue(strEndSupportLabel)
         );

         Float64 segmentLength = endStation - startStation;
         CString strSegmentLength;
         strSegmentLength.Format(_T("%s"),FormatDimension(segmentLength,pDisplayUnits->GetSpanLengthUnit()));
         for ( ROWCOL row = startRow+1; row < endRow; row++ )
         {
            SetStyleRange(CGXRange(row,col), CGXStyle()
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_VCENTER)
               .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
               .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
               .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
               .SetValue(strSegmentLength)
            );
         }
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSpanColumn()
{
   int col = 4;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   SpanIndexType nSpans = pDlg->m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      const CSpanData2* pSpan = pDlg->m_BridgeDesc.GetSpan(spanIdx);
      const CPierData2* pPrevPier = pSpan->GetPrevPier();
      const CPierData2* pNextPier = pSpan->GetNextPier();
      
      // Get row values for prev and next pier
      ROWCOL prevPierRow = GetPierRow(pPrevPier->GetIndex());
      ROWCOL nextPierRow = GetPierRow(pNextPier->GetIndex());

      // create prev and next pier labels
      CString strPrevPierLabel;
      strPrevPierLabel.Format(_T("Pier %d"),LABEL_PIER(pPrevPier->GetIndex()));

      CString strNextPierLabel;
      strNextPierLabel.Format(_T("Pier %d"),LABEL_PIER(pNextPier->GetIndex()));

      // label prev pier
      SetStyleRange(CGXRange(prevPierRow,col), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
         .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         .SetValue(strPrevPierLabel)
      );

      // prev pier edit button
      SetStyleRange(CGXRange(prevPierRow,col+1), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
         .SetUserAttribute(0,CRowType(CRowType::Pier,pPrevPier->GetIndex()))
      );
      
      // label next pier
      SetStyleRange(CGXRange(nextPierRow,col), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
         .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         .SetValue(strNextPierLabel)
      );
      
      // next pier edit button
      SetStyleRange(CGXRange(nextPierRow,col+1), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
         .SetUserAttribute(0,CRowType(CRowType::Pier,pNextPier->GetIndex()))
      );

      // fill in all rows between prev and next pier with span length
      Float64 spanLength = pNextPier->GetStation() - pPrevPier->GetStation();
      CString strSpanLength;
      strSpanLength.Format(_T("%s"),FormatDimension(spanLength,pDisplayUnits->GetSpanLengthUnit()));
      for ( ROWCOL row = prevPierRow+1; row < nextPierRow; row++ )
      {
         SetStyleRange(CGXRange(row,col), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor(spanLength <= 0 ? RGB(255,0,0) : ::GetSysColor(COLOR_WINDOWTEXT))
            .SetReadOnly(TRUE)
            .SetEnabled(FALSE)
            .SetValue(strSpanLength)
            );

         SetStyleRange(CGXRange(row,col+1), CGXStyle()
            .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
            .SetControl(GX_IDS_CTRL_PUSHBTN)
			   .SetChoiceList(_T("Edit"))
            .SetHorizontalAlignment(DT_LEFT)
            .SetEnabled(TRUE)
            .SetValue(0L)
            .SetUserAttribute(0,CRowType(CRowType::Span,pPrevPier->GetNextSpan()->GetIndex()))
            );
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 5 && nCol != 6 )
   {
      return;
   }
 
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );
   pParent->UpdateData();

   CGXStyle style;
   GetStyleRowCol(nRow,nCol,style);
   if ( style.GetIncludeUserAttribute(0) )
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));

      if ( rowType.m_Type == CRowType::Pier )
      {
         EditPier(rowType.m_Index);
      }
      else if ( rowType.m_Type == CRowType::Span )
      {
         EditSpan(rowType.m_Index);
      }
      else if ( rowType.m_Type == CRowType::TempSupport )
      {
         EditTemporarySupport(rowType.m_Index);
      }
      else
      {
         ATLASSERT(false); // is there a new row type?
      }
   }
}

// validate input
BOOL CBridgeDescFramingGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

	if (nCol==1)
	{
      // station
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      HRESULT hr = m_objStation->FromString( CComBSTR(s), (UnitModeType)(pDisplayUnits->GetUnitMode()));
      if ( FAILED(hr) )
      {
			SetWarningText (_T("Invalid Station Value"));
         return FALSE;
      }
      else
      {
         return TRUE;
      }
   }
	else if (nCol==2)
	{
         // orientation
      CString strOrientation(s);
      strOrientation.MakeUpper(); // do comparisons in upper case

      if (strOrientation.IsEmpty())
      {
			SetWarningText (_T("Orientation must not be blank"));
         return FALSE;
      }

      if ( strOrientation == _T("NORMAL") || (strOrientation.GetLength() == 1 && strOrientation[0] == 'N') )
      {
         return TRUE;
      }

      HRESULT hr_angle = m_objAngle->FromString(CComBSTR(strOrientation));
      if ( SUCCEEDED(hr_angle) )
      {
         Float64 value;
         m_objAngle->get_Value(&value);
         if ( value < -MAX_SKEW_ANGLE || MAX_SKEW_ANGLE < value )
         {
   		   SetWarningText (_T("Skew angle must be less than 88 deg"));
            return FALSE;
         }
         else
         {
            return TRUE;
         }
      }

      HRESULT hr_direction = m_objDirection->FromString(CComBSTR( strOrientation ));
      if ( SUCCEEDED(hr_direction) )
      {
         return TRUE;
      }
      else
      {
 		   SetWarningText (_T("Orientation is invalid"));
         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

BOOL CBridgeDescFramingGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   // The function FillGrid(), called below, causes recursion into
   // this function. To avoid that problem, we'll keep a static
   // variable to keep track if this function has already been entered
   static bool bHasThisMethodBeenCalled = false;
   if ( bHasThisMethodBeenCalled )
   {
      return TRUE;
   }

   bHasThisMethodBeenCalled = true;
   if ( nCol == 1 || nCol == 2 )
   {
      // Pier or temporary support station changed... update segment and span lengths
      CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
      ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

      CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
      ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );


      PierIndexType pierIdx = GetPierIndex(nRow);
      if ( pierIdx != INVALID_INDEX )
      {
         CPierData2* pPierData = GetPierRowData(nRow);
      }
      else
      {
         SupportIndexType tsIdx = GetTemporarySupportIndex(nRow);
         CTemporarySupportData tsData = GetTemporarySupportRowData(nRow);
         pDlg->m_BridgeDesc.SetTemporarySupportByIndex(tsIdx,tsData);
      }

      FillGrid(pDlg->m_BridgeDesc);
   }

   bHasThisMethodBeenCalled = false;
   return CGXGridWnd::OnEndEditing(nRow,nCol);
}

PierIndexType CBridgeDescFramingGrid::GetSelectedPier()
{
   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
   {
      return INVALID_INDEX; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   PierIndexType pierIdx = GetPierIndex(selRow);
   return pierIdx;
}

PierIndexType CBridgeDescFramingGrid::GetPierCount()
{
   ROWCOL nRows = GetRowCount();
   PierIndexType nPiers = 0;
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      CGXStyle style;
      GetStyleRowCol(row,0,style);
      if ( style.GetIncludeUserAttribute(0) )
      {
         const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));

         if ( rowType.m_Type == CRowType::Pier )
         {
            nPiers++;
         }
      }
   }

   return nPiers;
}

BOOL CBridgeDescFramingGrid::CanActivateGrid(BOOL bActivate)
{
   if ( !bActivate )
   {
      // make sure all the grid data is active
      std::vector<Float64> spanLengths = GetSpanLengths();
      std::vector<Float64>::iterator iter;
      for ( iter = spanLengths.begin(); iter != spanLengths.end(); iter++ )
      {
         Float64 L = *iter;
         if ( L <= 0 )
         {
   			AfxMessageBox(_T("Invalid Span Length"));
            return FALSE;
         }
      }
   }

   return CGXGridWnd::CanActivateGrid(bActivate);
}

void CBridgeDescFramingGrid::EditPier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CPierDetailsDlg dlg(&pDlg->m_BridgeDesc,pierIdx,pDlg->GetExtensionPages());
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::EditSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CSpanDetailsDlg dlg(&pDlg->m_BridgeDesc,spanIdx,pDlg->GetExtensionPages());

   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::EditTemporarySupport(SupportIndexType tsIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );


   CTemporarySupportDlg dlg(&pDlg->m_BridgeDesc,tsIdx,pDlg->GetExtensionPages(),this);
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

ROWCOL CBridgeDescFramingGrid::GetPierRow(PierIndexType pierIdx)
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      if ( GetPierIndex(row) == pierIdx )
      {
         return row;
      }
   }

   return 1;
}

PierIndexType CBridgeDescFramingGrid::GetPierIndex(ROWCOL nRow)
{
   CGXStyle style;
   GetStyleRowCol(nRow,0,style);

   if ( style.GetIncludeUserAttribute(0) )
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));
      if ( rowType.m_Type == CRowType::Pier )
      {
         return rowType.m_Index;
      }
      else
      {
         return INVALID_INDEX;
      }
   }
   else
   {
      return INVALID_INDEX;
   }
}

ROWCOL CBridgeDescFramingGrid::GetTemporarySupportRow(SupportIndexType tsIdx)
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      if ( GetTemporarySupportIndex(row) == tsIdx )
      {
         return row;
      }
   }

   return 1;
}

SupportIndexType CBridgeDescFramingGrid::GetTemporarySupportIndex(ROWCOL nRow)
{
   CGXStyle style;
   GetStyleRowCol(nRow,0,style);

   if ( style.GetIncludeUserAttribute(0) )
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));
      if ( rowType.m_Type == CRowType::TempSupport )
      {
         return rowType.m_Index;
      }
   }

   return INVALID_INDEX;
}
