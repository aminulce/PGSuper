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

// CrownSlopeGrid.cpp : implementation file
//

#include "stdafx.h"
#include "CrownSlopeGrid.h"
#include "CrownSlopePage.h"
#include "PGSuperUnits.h"
#include "PGSuperDoc.h"

#include <EAF\EAFDisplayUnits.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CCrownSlopeGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopeGrid
CCrownSlopeGrid::CCrownSlopeGrid():
   m_pRoadwaySectionData(nullptr)
{
}

CCrownSlopeGrid::CCrownSlopeGrid(RoadwaySectionData* pRoadwaySectionData):
   m_pRoadwaySectionData(pRoadwaySectionData)
{
}

CCrownSlopeGrid::~CCrownSlopeGrid()
{
}

BEGIN_MESSAGE_MAP(CCrownSlopeGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CCrownSlopeGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
//	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
//	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCrownSlopeGrid::AppendRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   InitRowData(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nRow, GetLeftCol());
	Invalidate();
}

void CCrownSlopeGrid::RemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
      CGXGridWnd::RemoveRows(range.top, range.bottom);
	}
}

void CCrownSlopeGrid::InitRowData(ROWCOL row)
{
	GetParam()->EnableUndo(FALSE);

   SetValueRange(CGXRange(row,0),row-1); // row num
   SetValueRange(CGXRange(row,1),"0+00");
   SetValueRange(CGXRange(row,2),"-0.02");

   ROWCOL col = 3;
   for (IndexType ir = 0; ir < m_pRoadwaySectionData->NumberOfSegmentsPerSection - 2; ir++)
   {
      SetValueRange(CGXRange(row,col++),"10.00");
      SetValueRange(CGXRange(row,col++),"-0.02");
   }

   SetValueRange(CGXRange(row,col),"-0.02");

   GetParam()->EnableUndo(TRUE);
}

void CCrownSlopeGrid::CustomInit()
{
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(), IEAFDisplayUnits, pDisplayUnits);
   const unitmgtLengthData& alignment_unit = pDisplayUnits->GetAlignmentLengthUnit();
   std::_tstring strUnitTag = alignment_unit.UnitOfMeasure.UnitTag();

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   this->Initialize();

	this->EnableIntelliMouse();
}

void CCrownSlopeGrid::UpdateGridSizeAndHeaders(const RoadwaySectionData& data)
{
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();
   GET_IFACE2(pParent->GetBroker(), IEAFDisplayUnits,pDisplayUnits);

	this->GetParam( )->EnableUndo(FALSE);

   const ROWCOL num_rows=1;
   const ROWCOL num_cols = 2 + (ROWCOL)data.NumberOfSegmentsPerSection*2 - 2 -1;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   SetFrozenRows(1/*# frozen rows*/,1/*# extra header rows*/);

   ROWCOL col = 0;

   // disable left side
	this->SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Temp-\nlate"))
		);

	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("\n Station "))
		);

	this->SetStyleRange(CGXRange(0,col), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Segment 1"))
		);

   CString strUnitTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag().c_str();

   CString strSlope;
   strSlope.Format(_T("Slope\n(%s/%s)"),strUnitTag,strUnitTag);

   CString strLength;
   strLength.Format(_T("Length\n(%s)"),strUnitTag);

	this->SetStyleRange(CGXRange(1,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strSlope)
		);
   
   IndexType ns = 2;
   for (; ns < data.NumberOfSegmentsPerSection; ns++)
   {
      CString strSegment;
      strSegment.Format(_T("Segment %d"), ns);

      SetStyleRange(CGXRange(0, col, 0, col + 1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
         .SetEnabled(FALSE)
         .SetValue(strSegment)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

	   this->SetStyleRange(CGXRange(1,col++), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
			   .SetValue(strLength)
		   );

	   this->SetStyleRange(CGXRange(1,col++), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
			   .SetValue(strSlope)
		   );
   }

   CString strSegment;
   strSegment.Format(_T("Segment %d"), ns);

	this->SetStyleRange(CGXRange(0,col), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strSegment)
		);

	this->SetStyleRange(CGXRange(1,col), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strSlope)
		);

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
	this->ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->SetFocus();

	this->GetParam( )->EnableUndo(TRUE);
}

void CCrownSlopeGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   ROWCOL ncols = GetColCount();
   this->SetStyleRange(CGXRange(nRow,1,nRow,ncols), CGXStyle()
	      .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CCrownSlopeGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CCrownSlopeGrid::SetRowData(ROWCOL nRow, const RoadwaySectionTemplate& data)
{
   m_LengthCols.clear();
   m_SlopeCols.clear();

	GetParam()->EnableUndo(FALSE);

   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();
   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   Float64 station = data.Station;
   station = ::ConvertFromSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   ROWCOL col = 0;
   SetValueRange(CGXRange(nRow,col++),nRow-1); // row num

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->put_Value(station);
   CComBSTR bstrStation;
   objStation->AsString(unit_mode,VARIANT_FALSE,&bstrStation);
   SetValueRange(CGXRange(nRow,col++),CString(bstrStation));

   m_SlopeCols.insert(col);
   SetValueRange(CGXRange(nRow,col++),data.LeftSlope);

   for (const auto& segment : data.SegmentDataVec)
   {
      Float64 length = ::ConvertFromSysUnits(segment.Length, pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
      m_LengthCols.insert(col);
      SetValueRange(CGXRange(nRow,col++),length);
      m_SlopeCols.insert(col);
      SetValueRange(CGXRange(nRow,col++),segment.Slope);
   }

   m_SlopeCols.insert(col);
   SetValueRange(CGXRange(nRow,col),data.RightSlope);

   GetParam()->EnableUndo(TRUE);
}

bool CCrownSlopeGrid::GetRowData(ROWCOL nRow,RoadwaySectionTemplate& data)
{
   data.SegmentDataVec.clear();

   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   CString strStation = GetCellValue(nRow,1);
   CComPtr<IStation> station;
   station.CoCreateInstance(CLSID_Station);
   HRESULT hr = station->FromString(CComBSTR(strStation),unit_mode);
   if (FAILED(hr))
   {
      CString msg;
      msg.Format(_T("Invalid station data for template %d"), nRow-1);
      ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
      return false;
   }

   Float64 station_value;
   station->get_Value(&station_value);
   station_value = ::ConvertToSysUnits(station_value,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   data.Station = station_value;

   CString strVal = GetCellValue(nRow,2);
   if (!sysTokenizer::ParseDouble(strVal, &data.LeftSlope))
	{
      CString msg;
      msg.Format(_T("Leftmost slope value not a number for template %d"), nRow-1);
      ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
      return false;
	}

   ROWCOL ncols = GetColCount();
   ROWCOL col = 3;
   IndexType nsegments = (ncols - 3) / 2;
   for (IndexType ns = 0; ns < nsegments; ns++)
   {
      RoadwaySegmentData seg;
      strVal = GetCellValue(nRow,col++);
      Float64 length;
      if (!sysTokenizer::ParseDouble(strVal, &length))
	   {
         CString msg;
         msg.Format(_T("Length value not a number for template %d"), nRow-1);
         ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
         return false;
	   }

      if (length < 0.0)
      {
         CString msg;
         msg.Format(_T("A segment length is less than zero for template %d"), nRow-1);
         ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
         return false;
      }

      seg.Length = ::ConvertToSysUnits(length,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

      strVal = GetCellValue(nRow,col++);
      if (!sysTokenizer::ParseDouble(strVal, &seg.Slope))
	   {
         CString msg;
         msg.Format(_T("A slope value is not a number for template %d"), nRow-1);
         ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
         return false;
	   }

      data.SegmentDataVec.push_back(seg);
   }

   strVal = GetCellValue(nRow,col);
   if (!sysTokenizer::ParseDouble(strVal, &data.RightSlope))
	{
      CString msg;
      msg.Format(_T("Rightmost slope value not a number for template %d"), nRow-1);
      ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
      return false;
	}

   return true;
}

void CCrownSlopeGrid::InitRoadwaySectionData(bool updateHeader)
{
   // set up our width and headers
   if (updateHeader)
   {
      UpdateGridSizeAndHeaders(*m_pRoadwaySectionData);
   }

   GetParam()->EnableUndo(FALSE);

   // empty the grid
   if ( 1 < GetRowCount() )
      CGXGridWnd::RemoveRows(2,GetRowCount());

   for (const auto& templ : m_pRoadwaySectionData->RoadwaySectionTemplates)
   {
      AppendRow();
      SetRowData(GetRowCount(), templ);
   }

   GetParam()->EnableUndo(TRUE);

	this->ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
}

bool CCrownSlopeGrid::UpdateRoadwaySectionData()
{
   m_pRoadwaySectionData->RoadwaySectionTemplates.clear();

   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 2; row <= nRows; row++ )
   {
      RoadwaySectionTemplate curve_data;
      if ( !GetRowData(row, curve_data) )
         return false;

      m_pRoadwaySectionData->RoadwaySectionTemplates.push_back(curve_data);
   }

   return true;
}

bool SortByStation(const RoadwaySectionTemplate& c1,const RoadwaySectionTemplate& c2)
{
   return c1.Station < c2.Station;
}

bool CCrownSlopeGrid::SortCrossSections()
{
   if (UpdateRoadwaySectionData())
   {
      std::sort(m_pRoadwaySectionData->RoadwaySectionTemplates.begin(),m_pRoadwaySectionData->RoadwaySectionTemplates.end(),SortByStation);
      InitRoadwaySectionData(false);
      return true;
   }

   return false;
}

BOOL CCrownSlopeGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   if (m_IsACellInvalid)
   {
      return FALSE;
   }
   else
   {
      return CGXGridWnd::OnValidateCell(nRow, nCol);
   }
}

// validate input
void CCrownSlopeGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();

   // set up styles
   CGXStyle valid_style;
   CGXStyle error_style;
   valid_style.SetEnabled(TRUE)
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
   error_style.SetEnabled(FALSE)
         .SetTextColor(RGB(255,0,0));

   bool is_valid(true);
   if (nCol == 1)
   {
      CString strStation = GetCellValue(nRow, 1);

      GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
      UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());
      CComPtr<IStation> station;
      station.CoCreateInstance(CLSID_Station);
      HRESULT hr = station->FromString(CComBSTR(strStation), unit_mode);
      if (FAILED(hr))
      {
         is_valid = false;
      }
   }
   else
   {
      // only validate slope and length data on the fly
      bool docheck = m_SlopeCols.find(nCol) != m_SlopeCols.end() || m_LengthCols.find(nCol) != m_LengthCols.end();

      if (docheck)
      {
         CString s;
         CGXControl* pControl = GetControl(nRow, nCol);
         pControl->GetCurrentText(s);

         if (nCol > 1 && !s.IsEmpty())
         {
            Float64 dbl;
            if (!sysTokenizer::ParseDouble(s, &dbl))
            {
               is_valid = false;
            }
         }
      }
   }

   // set the styles
   CGXRange range(nRow, nCol);
   SetStyleRange(range,  (is_valid  ? valid_style : error_style) );

   m_IsACellInvalid = !is_valid;

   if (is_valid)
   {
      // tell parent to draw template if we have valid data
      pParent->OnChange(); 
   }

   CGXGridWnd::OnModifyCell(nRow, nCol);
}
