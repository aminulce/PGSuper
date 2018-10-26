// SelectPOIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectPOIDlg.h"
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <MFCTools\CustomDDX.h>
//#include "HtmlHelp\HelpTopics.h"

// CSelectPOIDlg dialog

IMPLEMENT_DYNAMIC(CSelectPOIDlg, CDialog)

CSelectPOIDlg::CSelectPOIDlg(IBroker* pBroker,CWnd* pParent /*=NULL*/)
	: CDialog(CSelectPOIDlg::IDD, pParent)
   , m_SliderPos(0)
   , m_IntervalIdx(INVALID_INDEX)
{
   m_pBroker = pBroker;
}

CSelectPOIDlg::~CSelectPOIDlg()
{
}

void CSelectPOIDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_GROUP, m_cbGroup);
   DDX_Control(pDX, IDC_GIRDER, m_cbGirder);
   DDX_Control(pDX, IDC_SLIDER, m_Slider);
   DDX_Control(pDX, IDC_LOCATION, m_Label);

   IndexType grpIdx,gdrIdx;
   if ( pDX->m_bSaveAndValidate )
   {
      DDX_CBIndex(pDX, IDC_GROUP,  (int&)grpIdx);
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)gdrIdx);
      m_GirderKey.groupIndex = grpIdx;
      m_GirderKey.girderIndex = gdrIdx;
   }
   else
   {
      grpIdx = m_GirderKey.groupIndex;
      gdrIdx = m_GirderKey.girderIndex;
      DDX_CBIndex(pDX, IDC_GROUP,  (int&)grpIdx);
      DDX_CBIndex(pDX, IDC_GIRDER, (int&)gdrIdx);
   }

   DDX_CBItemData(pDX, IDC_INTERVAL, m_IntervalIdx);

   DDX_Slider(pDX, IDC_SLIDER, m_SliderPos);
}


BEGIN_MESSAGE_MAP(CSelectPOIDlg, CDialog)
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(IDC_GROUP,  &CSelectPOIDlg::OnGroupChanged)
   ON_CBN_SELCHANGE(IDC_GIRDER, &CSelectPOIDlg::OnGirderChanged)
   ON_COMMAND(IDC_HELPBTN,OnHelp)
END_MESSAGE_MAP()


// CSelectPOIDlg message handlers

BOOL CSelectPOIDlg::OnInitDialog()
{
   GET_IFACE( IBridge, pBridge );
   m_GirderKey = m_InitialPOI.GetSegmentKey();

   GET_IFACE(IDocumentType,pDocType);
   CString strGroup;
   if ( pDocType->IsPGSuperDocument() )
      strGroup = _T("Span");
   else
      strGroup = _T("Group");

   CComboBox* pGroupBox = (CComboBox*)GetDlgItem( IDC_GROUP );
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("%s %d"),strGroup,LABEL_GROUP(grpIdx));
      pGroupBox->AddString(strLabel);
   }
   pGroupBox->SetCurSel((int)m_GirderKey.groupIndex);
   UpdateGirderComboBox(m_GirderKey.groupIndex);

   UpdatePOI();

   CDialog::OnInitDialog();

   // initial the slider range
   m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 

   // initial the slider position to the current poi location
   CollectionIndexType pos = m_vPOI.size()/2; // default is mid-span
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = m_vPOI.begin(); iter != m_vPOI.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;
      if ( poi.GetID() == m_InitialPOI.GetID() )
      {
         pos = (iter - m_vPOI.begin());
      }
   }
   m_Slider.SetPos((int)pos);

   UpdateSliderLabel();
   FillIntervalCtrl();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

pgsPointOfInterest CSelectPOIDlg::GetPOI()
{
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_SliderPos];
   return poi;
}

void CSelectPOIDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   UpdateSliderLabel();
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSelectPOIDlg::UpdateGirderComboBox(GroupIndexType groupIdx)
{
   GET_IFACE( IBridge, pBridge );

   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pcbGirder->GetCurSel();
   pcbGirder->ResetContent();

   GirderIndexType nGirders = pBridge->GetGirderCount( groupIdx );
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(gdrIdx));
      pcbGirder->AddString( strGdr );
   }

   if ( pcbGirder->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
      pcbGirder->SetCurSel(0);
}

void CSelectPOIDlg::UpdateSliderLabel()
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   CString strLabel;
   ASSERT((int)m_SliderPos < (int)m_vPOI.size());
   pgsPointOfInterest poi = m_vPOI[m_Slider.GetPos()];

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

   rptPointOfInterest rptPoi(&pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   rptPoi.SetValue(POI_ERECTED_SEGMENT,poi,end_size);
   rptPoi.PrefixAttributes(false); // put the attributes after the location

   strLabel.Format(_T("Distance from Left Support = %s"),rptPoi.AsString().c_str());
   // remove the HTML tags
   strLabel.Replace(_T("<sub>"),_T(""));
   strLabel.Replace(_T("</sub>"),_T(""));

   m_Label.SetWindowText(strLabel);
}

void CSelectPOIDlg::OnGroupChanged()
{
   m_GirderKey.groupIndex = m_cbGroup.GetCurSel();
   UpdateGirderComboBox(m_GirderKey.groupIndex);
   UpdatePOI();
}

void CSelectPOIDlg::OnGirderChanged()
{
   m_GirderKey.girderIndex = m_cbGirder.GetCurSel();
   UpdatePOI();
}

void CSelectPOIDlg::FillIntervalCtrl()
{
   CComboBox* pcbIntervals = (CComboBox*)GetDlgItem(IDC_INTERVAL);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType startIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = startIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strInterval;
      strInterval.Format(_T("Interval %d: %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      int idx = pcbIntervals->AddString(strInterval);
      pcbIntervals->SetItemData(idx,intervalIdx);
   }

   pcbIntervals->SetCurSel(pcbIntervals->GetCount()-1);
   m_IntervalIdx = nIntervals-1;
}

void CSelectPOIDlg::UpdatePOI()
{
   GET_IFACE(IPointOfInterest,pPOI);
   m_vPOI = pPOI->GetPointsOfInterest(CSegmentKey(m_GirderKey,ALL_SEGMENTS));

   if (m_Slider.GetSafeHwnd() != NULL )
   {
      m_Slider.SetRange(0,(int)(m_vPOI.size()-1)); // the range is number of spaces along slider... 
                                                   // subtract one so we don't go past the end of the array
   }

}

void CSelectPOIDlg::OnHelp()
{
#pragma Reminder("UPDATE: Implement Help")
//   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_???);
}