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

// ActivityGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ActivityGrid.h"

#include <PgsExt\TimelineEvent.h>
#include "TimelineEventDlg.h"
#include "ConstructSegmentsDlg.h"
#include "ErectPiersDlg.h"
#include "ErectSegmentsDlg.h"
#include "RemoveTempSupportsDlg.h"
#include "CastClosureJointDlg.h"
#include "ApplyLoadsDlg.h"
#include "StressTendonDlg.h"
#include "CastDeckDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CONSTRUCT_SEGMENTS    0
#define ERECT_PIERS           1
#define CAST_CLOSURE_JOINTS    2
#define ERECT_SEGMENTS        3
#define STRESS_TENDONS        4
#define REMOVE_TS             5
#define CAST_DECK             6
#define APPLY_LOADS           7

GRID_IMPLEMENT_REGISTER(CActivityGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CActivityGrid

CActivityGrid::CActivityGrid()
{
//   RegisterClass();
}

CActivityGrid::~CActivityGrid()
{
}

BEGIN_MESSAGE_MAP(CActivityGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CActivityGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CActivityGrid message handlers

void CActivityGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );
   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELCOL));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   const int num_rows = 0;
   const int num_cols = 2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T(""))
		);

	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Activity"))
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T(""))
		);

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();

   GetParam()->EnableUndo(TRUE);

	SetFocus();
}

void CActivityGrid::Refresh()
{
   CTimelineEventDlg* pParent = (CTimelineEventDlg*)GetParent();

   if ( 0 < GetRowCount() )
   {
      RemoveRows(1,GetRowCount());
   }

   if ( pParent->m_pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
   {
      AddActivity(_T("Construct Segments"),CONSTRUCT_SEGMENTS);
   }

   if ( pParent->m_pTimelineEvent->GetErectPiersActivity().IsEnabled() )
   {
      AddActivity(_T("Erect Piers/Temporary Supports"),ERECT_PIERS);
   }

   if ( pParent->m_pTimelineEvent->GetCastClosureJointActivity().IsEnabled() )
   {
      AddActivity(_T("Cast Closure Joints"),CAST_CLOSURE_JOINTS);
   }

   if ( pParent->m_pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
   {
      AddActivity(_T("Erect Segments"),ERECT_SEGMENTS);
   }

   if ( pParent->m_pTimelineEvent->GetStressTendonActivity().IsEnabled() )
   {
      AddActivity(_T("Stress Tendons"),STRESS_TENDONS);
   }

   if ( pParent->m_pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
   {
      AddActivity(_T("Remove Temporary Supports"),REMOVE_TS);
   }

   if ( pParent->m_pTimelineEvent->GetCastDeckActivity().IsEnabled() )
   {
      AddActivity(_T("Cast Deck"),CAST_DECK);
   }

   if ( pParent->m_pTimelineEvent->GetApplyLoadActivity().IsEnabled() )
   {
      AddActivity(_T("Apply Loads"),APPLY_LOADS);
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
}

void CActivityGrid::AddActivity(LPCTSTR strName,int activityKey)
{
   ROWCOL row = GetRowCount()+1;

   GetParam()->EnableUndo(FALSE);
   InsertRows(row,1);

	SetStyleRange(CGXRange(row,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   SetStyleRange(CGXRange(row,1), CGXStyle()
         .SetHorizontalAlignment(DT_LEFT)
         .SetVerticalAlignment(DT_VCENTER)
         .SetControl(GX_IDS_CTRL_STATIC)
			.SetValue(strName)
		);

	SetStyleRange(CGXRange(row,2), CGXStyle()
		.SetControl(GX_IDS_CTRL_PUSHBTN)
		.SetChoiceList(_T("Edit"))
      .SetItemDataPtr((void*)activityKey)
		);

   GetParam()->EnableUndo(TRUE);
}

void CActivityGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 2 )
   {
      return;
   }

   CTimelineEventDlg* pParent = (CTimelineEventDlg*)GetParent();
   EventIndexType eventIdx = pParent->m_EventIndex;

   CGXStyle style;
   GetStyleRowCol(nRow,nCol,style);
   if ( (int)style.GetItemDataPtr() == CONSTRUCT_SEGMENTS )
   {
      CConstructSegmentsDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == ERECT_PIERS )
   {
      CErectPiersDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == ERECT_SEGMENTS )
   {
      CErectSegmentsDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == STRESS_TENDONS )
   {
      CStressTendonDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == REMOVE_TS )
   {
      CRemoveTempSupportsDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == CAST_CLOSURE_JOINTS )
   {
      CCastClosureJointDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == CAST_DECK )
   {
      CCastDeckDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else if ( (int)style.GetItemDataPtr() == APPLY_LOADS )
   {
      CApplyLoadsDlg dlg(pParent->m_TimelineManager,pParent->m_EventIndex);
      if ( dlg.DoModal() == IDOK )
      {
         pParent->UpdateTimelineManager(dlg.m_TimelineMgr);
      }
   }
   else
   {
      ATLASSERT(false); // is there a new activity type?
      AfxMessageBox(_T("Edit Activity"));
   }
}

void CActivityGrid::RemoveActivity()
{
   CTimelineEventDlg* pParent = (CTimelineEventDlg*)GetParent();

   CRowColArray selection;
   ROWCOL nSelRows = GetSelectedRows(selection,TRUE);

   if ( nSelRows == 0 )
   {
      return;
   }

   for ( ROWCOL i = 0; i < nSelRows; i++ )
   {
      ROWCOL row = selection.GetAt(i);

      CGXStyle style;
      GetStyleRowCol(row,2,style);
      int activityType = (int)style.GetItemDataPtr();

      if ( activityType == CONSTRUCT_SEGMENTS )
      {
         if ( pParent->m_pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
         {
            AfxMessageBox(_T("This activity cannot be directly removed. Add a Construct Segments activity to a different timeline event to remove it from this event."));
            return;
         }
         else
         {
            pParent->m_pTimelineEvent->GetConstructSegmentsActivity().Enable(false);
         }
      }

      if ( activityType == ERECT_PIERS )
      {
         if ( pParent->m_pTimelineEvent->GetErectPiersActivity().GetPierCount() == 0 &&
              pParent->m_pTimelineEvent->GetErectPiersActivity().GetTemporarySupportCount() == 0 )
         {
            pParent->m_pTimelineEvent->GetErectPiersActivity().Enable(false);
            pParent->m_pTimelineEvent->GetErectPiersActivity().Clear();
         }
         else
         {
            AfxMessageBox(_T("This activity cannot be removed. Piers/Temporary Supports are erected during this activity. Erect them in a different event in the timeline."));
            return;
         }
      }

      if ( activityType == CAST_CLOSURE_JOINTS )
      {
         if ( pParent->m_pTimelineEvent->GetCastClosureJointActivity().GetTemporarySupportCount() == 0 )
         {
            pParent->m_pTimelineEvent->GetCastClosureJointActivity().Enable(false);
            pParent->m_pTimelineEvent->GetCastClosureJointActivity().Clear();
         }
         else
         {
            AfxMessageBox(_T("This activity cannot be removed. Closure joints are cast during this activity. Cast these closure joints in a different event in the timeline."));
            return;
         }
      }

      if ( activityType == ERECT_SEGMENTS )
      {
         if ( pParent->m_pTimelineEvent->GetErectSegmentsActivity().GetSegmentCount() == 0 )
         {
            pParent->m_pTimelineEvent->GetErectSegmentsActivity().Enable(false);
            pParent->m_pTimelineEvent->GetErectSegmentsActivity().Clear();
         }
         else
         {
            AfxMessageBox(_T("This activity cannot be removed. Segments are erected during this activity. Move these segments to a different event in the timeline."));
            return;
         }
      }

      if ( activityType == STRESS_TENDONS )
      {
         if ( pParent->m_pTimelineEvent->GetStressTendonActivity().GetTendonCount() == 0 )
         {
            pParent->m_pTimelineEvent->GetStressTendonActivity().Enable(false);
            pParent->m_pTimelineEvent->GetStressTendonActivity().Clear();
         }
         else
         {
            AfxMessageBox(_T("This activity cannot be removed. Tendons are stressed during this activity. Stress these tendons in a different event in the timeline."));
            return;
         }
      }

      if ( activityType == REMOVE_TS )
      {
         if ( pParent->m_pTimelineEvent->GetRemoveTempSupportsActivity().GetTempSupportCount() == 0 )
         {
            pParent->m_pTimelineEvent->GetRemoveTempSupportsActivity().Enable(false);
            pParent->m_pTimelineEvent->GetRemoveTempSupportsActivity().Clear();
         }
         else
         {
            AfxMessageBox(_T("This activity cannot be removed. Temporary supports are removed during this activity. Remove this temporary supports in a different event in the timeline."));
            return;
         }
      }

      if ( activityType == CAST_DECK )
      {
         if ( pParent->m_pTimelineEvent->GetCastDeckActivity().IsEnabled() )
         {
            AfxMessageBox(_T("This activity cannot be removed. The deck is cast during this activity. Cast the deck in a different event in the timeline."));
            return;
         }
         else
         {
            pParent->m_pTimelineEvent->GetCastDeckActivity().Enable(false);
         }
      }

      if ( activityType == APPLY_LOADS )
      {
         if ( pParent->m_pTimelineEvent->GetApplyLoadActivity().IsEnabled() )
         {
            AfxMessageBox(_T("This activity cannot be removed. Loads are applied during this activity. Apply these loads in a different event in the timeline."));
            return;
         }
         else
         {
            pParent->m_pTimelineEvent->GetApplyLoadActivity().Enable(false);
            pParent->m_pTimelineEvent->GetApplyLoadActivity().Clear();
         }
      }
   }

   GetParam()->EnableUndo(FALSE);

   RemoveRows(selection.GetAt(0),selection.GetAt(nSelRows-1));

   GetParam()->EnableUndo(TRUE);
}
