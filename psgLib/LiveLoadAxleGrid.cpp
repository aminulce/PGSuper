///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// DiaphramGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "LiveLoadAxleGrid.h"
#include "LiveLoadDlg.h"
#include <system\tokenizer.h>
#include <Units\sysUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CLiveLoadAxleGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadAxleGrid

CLiveLoadAxleGrid::CLiveLoadAxleGrid()
{
//   RegisterClass();
}

CLiveLoadAxleGrid::~CLiveLoadAxleGrid()
{
}

BEGIN_MESSAGE_MAP(CLiveLoadAxleGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CLiveLoadAxleGrid)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoverows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLiveLoadAxleGrid message handlers

int CLiveLoadAxleGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );

   return rect.Width( )/3;
}

BOOL CLiveLoadAxleGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CLiveLoadDlg* pdlg = (CLiveLoadDlg*)GetParent();
   ASSERT (pdlg);

	if (GetParam() == NULL)
	{
      pdlg->OnEnableDelete(false);
		return TRUE;
	}

	CGXRangeList* pSelList = GetParam()->GetRangeList();

   // don't allow row 1 to be deleted unless it's the only row
   int cnt = GetRowCount();
   if (cnt>1)
   {
	   bool res = !pSelList->IsAnyCellFromRow(1) && pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1;
      pdlg->OnEnableDelete(res);
   }
   else if (cnt==1)
   {
	   bool res = pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1;
      pdlg->OnEnableDelete(res);
   }

   return TRUE;
}


void CLiveLoadAxleGrid::Appendrow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	Invalidate();
}

void CLiveLoadAxleGrid::Removerows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
	}

   CLiveLoadDlg* pdlg = (CLiveLoadDlg*)GetParent();
   ASSERT (pdlg);
   pdlg->OnEnableDelete(false);
}

void CLiveLoadAxleGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	this->Initialize( );
	this->EnableIntelliMouse();

   ResetGrid();

}

void CLiveLoadAxleGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	this->SetStyleRange(CGXRange(nRow,1), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
		);

   if (nRow>1 )
   {
	   this->SetStyleRange(CGXRange(nRow,2), CGXStyle()
            .SetHorizontalAlignment(DT_RIGHT));
   }
   else
   {
	   this->SetStyleRange(CGXRange(nRow,2), CGXStyle()
            .SetEnabled(FALSE).
            SetReadOnly(TRUE).
            SetInterior( RGB(150,150,150) ));
   }


	GetParam()->EnableUndo(TRUE);
}

CString CLiveLoadAxleGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
  }
    else
        return GetValueRowCol(nRow, nCol);
}


// validate input
BOOL CLiveLoadAxleGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

	if (nCol==1)
	{
      double d;
      if (!sysTokenizer::ParseDouble(s, &d))
		{
			SetWarningText (_T("Value must be a number"));
			return FALSE;
		}

      if (d<=0.0)
      {
			SetWarningText (_T("Axle weights must greater than zero"));
			return FALSE;
      }

		return TRUE;
	}
	else if (nCol==2)
	{
      const char *delims[] = {"-"," ", 0};
      sysTokenizer tokizerd(delims);
      tokizerd.push_back(s);

      sysTokenizer::size_type npd = tokizerd.size();
      if (npd==1 || npd==2)
      {
         double d;
         if (!sysTokenizer::ParseDouble(tokizerd[0].c_str(), &d))
		   {
			   SetWarningText (_T("Axle spacing value(s) must be a single number or two numbers separated by a dash"));
			   return FALSE;
		   }

         if (d<=0.0)
         {
			   SetWarningText (_T("First axle spacing must greater than zero"));
			   return FALSE;
         }

         if (npd==2)
         {
            double dmax;
            if (!sysTokenizer::ParseDouble(tokizerd[1].c_str(), &dmax))
		      {
			      SetWarningText (_T("Axle spacing value(s) must be a single number or two numbers separated by a dash"));
			      return FALSE;
		      }

            if (dmax<=0.0)
            {
			      SetWarningText (_T("Max axle spacing must greater than zero"));
			      return FALSE;
            }

            if (dmax<=d)
            {
			      SetWarningText (_T("Max axle spacing must greater than Min axle spacing"));
			      return FALSE;
            }
         }
      }
      else
      {
			SetWarningText (_T("Axle spacing value(s) must be a single number or two numbers separated by a dash"));
			return FALSE;
      }

		return TRUE;
	}

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

void CLiveLoadAxleGrid::ResetGrid()
{
   CLiveLoadDlg* pdlg = (CLiveLoadDlg*)GetParent();
   ASSERT(pdlg);

	this->GetParam( )->EnableUndo(FALSE);

   if ( GetRowCount() > 0 )
      RemoveRows(1,GetRowCount());

   const int num_rows = 0;
   const int num_cols = 2;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // disable left side
	this->SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
	this->SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Axle #"))
		);

   CString cv = "Weight " + pdlg->GetForceUnitString();
	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = "Spacing " + pdlg->GetLengthUnitString();
	this->SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->SetFocus();

	this->GetParam( )->EnableUndo(TRUE);
}

void CLiveLoadAxleGrid::UploadData(CDataExchange* pDX, CLiveLoadDlg* dlg)
{
   AxleIndexType nAxles = (AxleIndexType)dlg->m_Axles.size();
   for (AxleIndexType axleIdx = 0; axleIdx < nAxles; axleIdx++)
   {
      Appendrow();
   }

   ROWCOL nRow=1;
   for (LiveLoadLibraryEntry::AxleContainer::const_iterator it = dlg->m_Axles.begin(); it!=dlg->m_Axles.end(); it++)
   {
      Float64 weight = it->Weight;
      weight = ::ConvertFromSysUnits(weight, dlg->m_ForceUnit);

      Float64 spacing = it->Spacing;
      spacing = ::ConvertFromSysUnits(spacing, dlg->m_LongLengthUnit);

      SetValueRange(CGXRange(nRow, 1), weight);

      if (nRow>1)
      {
         SetValueRange(CGXRange(nRow, 2), spacing);
      }

      nRow++;
   }

   // deal with variable spacing
   ATLASSERT(dlg->m_VariableAxleIndex < nAxles || dlg->m_VariableAxleIndex == FIXED_AXLE_TRUCK);

   if (dlg->m_VariableAxleIndex != FIXED_AXLE_TRUCK)
   {
      Float64 min_spc = dlg->m_Axles[dlg->m_VariableAxleIndex].Spacing;
      min_spc = ::ConvertFromSysUnits(min_spc, dlg->m_LongLengthUnit);

      Float64 max_spc = dlg->m_MaxVariableAxleSpacing;
      max_spc = ::ConvertFromSysUnits(max_spc, dlg->m_LongLengthUnit);

      ATLASSERT(max_spc>min_spc);
      
      std::ostringstream os;
      os<< min_spc<<"-"<<max_spc;

      ROWCOL row = dlg->m_VariableAxleIndex + 1;

      SetValueRange(CGXRange(row, 2), os.str().c_str());
   }
}

void CLiveLoadAxleGrid::DownloadData(CDataExchange* pDX, CLiveLoadDlg* dlg)
{
   ROWCOL naxles = GetRowCount();

   dlg->m_Axles.clear();
   dlg->m_VariableAxleIndex = INVALID_INDEX;
   dlg->m_MaxVariableAxleSpacing  = 0.0;

   ROWCOL nRow=1;
   bool is_var_axl=false;
   for (ROWCOL iaxl=0; iaxl<naxles; iaxl++)
   {
      double weight(0), min_spacing(0), max_spacing(0);
      SpacingType stype = ParseAxleRow(nRow, pDX, &weight, &min_spacing, &max_spacing);

      weight = ::ConvertToSysUnits(weight, dlg->m_ForceUnit);
      min_spacing = ::ConvertToSysUnits(min_spacing, dlg->m_LongLengthUnit);
      max_spacing = ::ConvertToSysUnits(max_spacing, dlg->m_LongLengthUnit);

      LiveLoadLibraryEntry::Axle axle;
      if (stype==stNone)
      {
         axle.Weight = weight;
      }
      else if (stype==stFixed)
      {
         axle.Weight = weight;
         axle.Spacing = min_spacing;
      }
      else
      {
         if (is_var_axl)
         {
            // can't have more than one variable axle
            CString msg("Error - Only one variable axle may be specified"); 
            ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
            pDX->Fail();
         }

         is_var_axl  = true;

         axle.Weight  = weight;
         axle.Spacing = min_spacing;

         dlg->m_VariableAxleIndex       = AxleIndexType(iaxl);
         dlg->m_MaxVariableAxleSpacing  = max_spacing;
      }

      dlg->m_Axles.push_back(axle);

      nRow++;
   }
}


CLiveLoadAxleGrid::SpacingType CLiveLoadAxleGrid::ParseAxleRow(ROWCOL nRow, CDataExchange* pDX, double* pWeight, 
                                                               double* pSpacingMin, double* pSpacingMax)
{
   Uint32 axlno = nRow;

   SpacingType spctype = stNone;

   // weight
	CString s = GetCellValue(nRow, 1);

   double d;
   if (!sysTokenizer::ParseDouble(s, &d))
	{
      CString msg; 
      msg.Format("Error - Weight for axle %d must be a positive number", axlno);
      ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
      pDX->Fail();
	}

   if (d<=0.0)
   {
      CString msg; 
      msg.Format("Error - Weight for axle %d must be a positive number", axlno);
      ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
      pDX->Fail();
   }

   *pWeight = d;

   // spacing

   if (nRow>1)
   {
   	s = GetCellValue(nRow, 2);

      const char *delims[] = {"-"," ", 0};
      sysTokenizer tokizerd(delims);
      tokizerd.push_back(s);

      sysTokenizer::size_type npd = tokizerd.size();
      if (npd==1 || npd==2)
      {
         double d;
         if (!sysTokenizer::ParseDouble(tokizerd[0].c_str(), &d))
		   {
            CString msg; 
            msg.Format("Spacing value(s) for axle %d must be a single number or two numbers separated by a dash", axlno);
            ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
            pDX->Fail();
		   }

         if (d<=0.0)
         {
            CString msg; 
            msg.Format("Spacing value for axle %d must be greater than zero", axlno);
            ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
            pDX->Fail();
         }

         *pSpacingMin = d;
         spctype = stFixed;

         if (npd==2)
         {
            double dmax;
            if (!sysTokenizer::ParseDouble(tokizerd[1].c_str(), &dmax))
		      {
               CString msg; 
               msg.Format("Spacing value(s) for axle %d must be a single number or two numbers separated by a dash", axlno);
               ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
               pDX->Fail();
		      }

            if (dmax<=0.0)
            {
               CString msg; 
               msg.Format("Max Spacing value for axle %d must be greater than zero", axlno);
               ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
               pDX->Fail();
            }

            if (dmax<=d)
            {
               CString msg; 
               msg.Format("Max axle spacing must greater than Min axle spacing for axle %d", axlno);
               ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
               pDX->Fail();
            }

            *pSpacingMax = dmax;
            spctype = stVariable;
         }
      }
      else
      {
         CString msg; 
         msg.Format("Spacing value(s) for axle %d must be a single number or two numbers separated by a dash", axlno);
         ::AfxMessageBox(msg, MB_ICONEXCLAMATION|MB_OK);
         pDX->Fail();
      }
   }
   else
   {
      spctype = stNone;
   }

   return spctype;
}


void CLiveLoadAxleGrid::OnUpdateEditRemoverows(CCmdUI* pCmdUI) 
{
	if (GetParam() == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	pCmdUI->Enable(pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1);
}
