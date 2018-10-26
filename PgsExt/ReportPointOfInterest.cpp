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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderLabel.h>

#include <EAF\EAFUtilities.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

rptPointOfInterest::rptPointOfInterest(const unitLength* pUnitOfMeasure,
                                       Float64 zeroTolerance,
                                       bool bShowUnitTag) :
rptLengthUnitValue(pUnitOfMeasure,zeroTolerance,bShowUnitTag),m_bPrefixAttributes(true),m_bIncludeSpanAndGirder(false)
{
}

rptPointOfInterest::rptPointOfInterest(const rptPointOfInterest& rOther) :
rptLengthUnitValue( rOther )
{
   MakeCopy( rOther );
}

rptPointOfInterest& rptPointOfInterest::operator = (const rptPointOfInterest& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment( rOther );
   }

   return *this;
}

rptReportContent* rptPointOfInterest::CreateClone() const
{
   return new rptPointOfInterest( *this );
}

void rptPointOfInterest::IncludeSpanAndGirder(bool bIncludeSpanAndGirder)
{
   m_bIncludeSpanAndGirder = bIncludeSpanAndGirder;
}

bool rptPointOfInterest::IncludeSpanAndGirder() const
{
   return m_bIncludeSpanAndGirder;
}

rptReportContent& rptPointOfInterest::SetValue(PoiAttributeType reference,const pgsPointOfInterest& poi)
{
   ATLASSERT(sysFlags<PoiAttributeType>::IsSet(reference,POI_RELEASED_SEGMENT) || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_STORAGE_SEGMENT)  || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_LIFT_SEGMENT)     || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_HAUL_SEGMENT)     || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_ERECTED_SEGMENT)  || 
             sysFlags<PoiAttributeType>::IsSet(reference,POI_SPAN));

   m_POI = poi;
   m_Reference = reference;

   // Get the location in span coordiates
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pPoi->ConvertPoiToSpanPoint(m_POI,&m_SpanKey,&m_Xspan);

   // Get the location adjustment... want to always measure from the left bearing/support point
   Float64 locationAdjustment;
   if ( m_Reference == POI_RELEASED_SEGMENT )
   {
      locationAdjustment = 0;
   }
   else if ( m_Reference == POI_STORAGE_SEGMENT )
   {
      GET_IFACE2(pBroker,IGirder,pGirder);
      Float64 XsLeft,XsRight;
      pGirder->GetSegmentStorageSupportLocations(m_POI.GetSegmentKey(),&XsLeft,&XsRight);
      locationAdjustment = XsLeft;
   }
   else if ( m_Reference == POI_LIFT_SEGMENT )
   {
      GET_IFACE2(pBroker,ISegmentLifting, pSegmentLifting);
      locationAdjustment = pSegmentLifting->GetLeftLiftingLoopLocation(m_POI.GetSegmentKey());
   }
   else if ( m_Reference == POI_HAUL_SEGMENT )
   {
      GET_IFACE2(pBroker,ISegmentHauling, pSegmentHauling);
      locationAdjustment = pSegmentHauling->GetTrailingOverhang(m_POI.GetSegmentKey());
   }
   else if ( m_Reference == POI_ERECTED_SEGMENT )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      locationAdjustment = pBridge->GetSegmentStartEndDistance(m_POI.GetSegmentKey());
   }

   if ( m_Reference == POI_SPAN )
   {
      return rptLengthUnitValue::SetValue(m_Xspan);
   }
   else
   {
      return rptLengthUnitValue::SetValue( poi.GetDistFromStart() - locationAdjustment );
   }
}

std::_tstring rptPointOfInterest::AsString() const
{
   std::_tstring strAttrib = m_POI.GetAttributes(m_Reference,true);
   std::_tstring strValue = rptLengthUnitValue::AsString();

   std::_tstring str;

   if ( m_bIncludeSpanAndGirder )
   {
      if ( m_Reference == POI_SPAN )
      {
         CString str1;
         str1.Format(_T("Span %d Girder %s, "),LABEL_SPAN(m_SpanKey.spanIndex),LABEL_GIRDER(m_SpanKey.girderIndex));
         str  = str1;
      }
      else
      {
         CString str1;

         GroupIndexType   grpIdx = m_POI.GetSegmentKey().groupIndex;
         GirderIndexType  gdrIdx = m_POI.GetSegmentKey().girderIndex;
         SegmentIndexType segIdx = m_POI.GetSegmentKey().segmentIndex;

         str1.Format(_T("Group %d Girder %s Segment %d, "),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));
         str = str1;
      }
   }

   if ( strAttrib.size() == 0 )
   {
      str += strValue;
   }
   else
   {
      if ( m_bPrefixAttributes )
      {
         str += _T("(") + strAttrib + _T(") ") + strValue;
      }
      else
      {
         str += strValue + _T(" (") + strAttrib + _T(")");
      }
   }

   return str;
}

void rptPointOfInterest::MakeCopy(const rptPointOfInterest& rOther)
{
   m_POI                   = rOther.m_POI;
   m_SpanKey               = rOther.m_SpanKey;
   m_Xspan                 = rOther.m_Xspan;
   m_Reference             = rOther.m_Reference;
   m_bPrefixAttributes     = rOther.m_bPrefixAttributes;
   m_bIncludeSpanAndGirder = rOther.m_bIncludeSpanAndGirder;
}

void rptPointOfInterest::MakeAssignment(const rptPointOfInterest& rOther)
{
   rptLengthUnitValue::MakeAssignment( rOther );
   MakeCopy( rOther );
}

void rptPointOfInterest::PrefixAttributes(bool bPrefixAttributes)
{
   m_bPrefixAttributes = bPrefixAttributes;
}

bool rptPointOfInterest::PrefixAttributes() const
{
   return m_bPrefixAttributes;
}
