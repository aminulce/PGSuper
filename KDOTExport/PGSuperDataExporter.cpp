// PGSuperExporter.cpp : Implementation of CPGSuperExporter
#include "stdafx.h"
#include "KDOTExport_i.h"
#include "PGSuperDataExporter.h"
#include "ExportDlg.h"

#include <MFCTools\Prompts.h>
#include "PGSuperInterfaces.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFDocument.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>

#include <pgsExt\DeckDescription.h>
#include <PgsExt\GirderData.h>


   enum Type { A615  = 0x1000,  // ASTM A615
               A706  = 0x2000,  // ASTM A706
               A1035 = 0x4000   // ASTM A1035
   };
static std::_tstring GenerateReinfTypeName(matRebar::Type rtype)
{
   switch(rtype)
   {
   case matRebar::A615:
      return _T("A615");
      break;
   case matRebar::A706:
      return _T("A706");
      break;
   case matRebar::A1035:
      return _T("A1035");
      break;
   default:
      ATLASSERT(0); // new rebar grade?
      return _T("A615");
   }
}

static std::_tstring GenerateReinfGradeName(matRebar::Grade grade)
{
   switch(grade)
   {
   case matRebar::Grade40:
      return _T("40");
      break;
   case matRebar::Grade60:
      return _T("60");
      break;
   case matRebar::Grade75:
      return _T("75");
      break;
   case matRebar::Grade80:
      return _T("80");
      break;
   default:
      ATLASSERT(0); // new rebar grade?
      return _T("40");
   }
}

HRESULT CPGSuperDataExporter::FinalConstruct()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDataExporter

STDMETHODIMP CPGSuperDataExporter::Init(UINT nCmdID)
{

   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetMenuText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("KDOT CAD Data...");
   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetBitmapHandle(HBITMAP* phBmp)
{
   *phBmp = m_bmpLogo;
   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetCommandHintText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("Export PGSuper data to KDOT CAD XML format\nExport PGSuper data to KDOT CAD format");
   return S_OK;   
}

STDMETHODIMP CPGSuperDataExporter::Export(IBroker* pBroker)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE2(pBroker,ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   spanIdx = spanIdx == ALL_SPANS ? 0 : spanIdx; // default to 0
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();
   gdrIdx = gdrIdx == ALL_GIRDERS ? 0 : gdrIdx;

   // Get girders to be exported
   SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
   std::vector<SpanGirderHashType> gdrlist;
   gdrlist.push_back(hash);

   CExportDlg  caddlg (pBroker, NULL);
   caddlg.m_SelGdrs = gdrlist;

   // Open the ExportCADData dialog box
   INT_PTR stf = caddlg.DoModal();
   if (stf == IDOK)
   {
	   // Get user's span & beam id values
	   gdrlist = caddlg.m_SelGdrs;
   }
   else
   {
	   // Just do nothing if CANCEL
       return S_OK;
   }

   // use the PGSuper document file name, with the extension changed, as the default file name
   CEAFDocument* pDoc = EAFGetDocument();
   CString strDefaultFileName = pDoc->GetPathName();
   // default name is file name with .xml
   strDefaultFileName.Replace(_T(".pgs"),_T(".xml"));

   CFileDialog dlgFile(FALSE,_T("*.xml"),strDefaultFileName,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,_T("KDOT CAD Export(*.xml)|*.xml"));
   if ( dlgFile.DoModal() == IDCANCEL )
   {
      return S_OK;
   }

   CString strFileName = dlgFile.GetPathName();

   HRESULT hr = Export(pBroker,strFileName,gdrlist);

   if ( SUCCEEDED(hr) )
   {
      CString strMsgSuccess;
      strMsgSuccess.Format(_T("Data for was successfully exported to the File \n\n %s"),strFileName);
      AfxMessageBox(strMsgSuccess,MB_OK | MB_ICONINFORMATION);
   }
   else
   {
      CString strMsgSuccess;
      strMsgSuccess.Format(_T("An unknown error occured when exporting to the File \n\n %s"),strFileName);
      AfxMessageBox(strMsgSuccess,MB_OK | MB_ICONINFORMATION);
   }

   return S_OK;
}

HRESULT CPGSuperDataExporter::Export(IBroker* pBroker,CString& strFileName, const std::vector<SpanGirderHashType>& gdrList)
{
   USES_CONVERSION;

   { // scope the progress window
   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress autoProgress(pProgress,0);
   pProgress->UpdateMessage(_T("Exporting PGSuper data for KDOT CAD"));

   try
   {
      KDOT::KDOTExport kdot_export;

      GET_IFACE2(pBroker,IBridge,pBridge);
      GET_IFACE2(pBroker,IBridgeDescription,pBridgeDescription);
      GET_IFACE2(pBroker, IGirder,pGirder);
      GET_IFACE2(pBroker, IGirderData, pGirderData);
      GET_IFACE2(pBroker, IRoadway,pAlignment);

      const CBridgeDescription* pBridgeDescr = pBridgeDescription->GetBridgeDescription();

      // Bridge level data
      KDOT::BridgeDataType brdata;

      std::_tstring type = pBridgeDescr->GetLeftRailingSystem()->GetExteriorRailing()->GetName();
      brdata.LeftRailingType(T2A(type.c_str()));

      type = pBridgeDescr->GetRightRailingSystem()->GetExteriorRailing()->GetName();
      brdata.RightRailingType(T2A(type.c_str()));

      GET_IFACE2(pBroker,IBridgeMaterial,pBridgeMaterial);
      GET_IFACE2(pBroker,IBridgeMaterialEx,pMaterial);

      Float64 dval = pBridgeMaterial->GetFcSlab();
      dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
      brdata.SlabFc(dval);

      // Assume uniform deck thickness
      const CDeckDescription* pDeckDescription = pBridgeDescr->GetDeckDescription();

      Float64 tDeck(0.0);
      if ( pDeckDescription->DeckType == pgsTypes::sdtCompositeSIP )
      {
         tDeck = pDeckDescription->GrossDepth + pDeckDescription->PanelDepth;
      }
      else
      {
         tDeck = pDeckDescription->GrossDepth;
      }

      dval = ::ConvertFromSysUnits(tDeck, unitMeasure::Inch);
      brdata.SlabThickness(dval);

      dval = pDeckDescription->OverhangEdgeDepth;
      dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
      brdata.OverhangThickness(dval);

      SpanIndexType ns = pBridge->GetSpanCount();
      brdata.NumberOfSpans(ns);

      KDOT::BridgeDataType::SpanLengths_sequence span_lengths;
      KDOT::BridgeDataType::NumberOfGirdersPerSpan_sequence ngs_count;
      for (SpanIndexType is=0; is<ns; is++)
      {
         Float64 sl = pBridge->GetSpanLength(is);
         sl = ::ConvertFromSysUnits(sl, unitMeasure::Inch); 
         span_lengths.push_back(sl);

         GirderIndexType ng = pBridge->GetGirderCount(is);
         ngs_count.push_back(ng);
      }

      brdata.SpanLengths(span_lengths);
      brdata.NumberOfGirdersPerSpan(ngs_count);

      // Pier data
      KDOT::BridgeDataType::PierData_sequence pds;

      for (SpanIndexType ip=0; ip<ns+1; ip++)
      {
         KDOT::BridgeDataType::PierData_type pd;

         dval = pBridge->GetPierStation(ip);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         pd.Station(dval);

         CComPtr<IAngle> angle;
         pBridge->GetPierSkew(ip, &angle);
         angle->get_Value(&dval);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Radian);
         pd.Skew(dval);

         // End offsets are measured at girder 0
         Float64 backEndOffset(0.0);
         if (ip>0)
         {
            backEndOffset  = pBridge->GetCLPierToCLBearingDistance(ip-1,0, pgsTypes::Back, pgsTypes::AlongItem)
                           - pBridge->GetCLPierToGirderEndDistance(ip-1,0, pgsTypes::Back, pgsTypes::AlongItem);
         }

         dval = ::ConvertFromSysUnits(backEndOffset, unitMeasure::Inch);
         pd.backGirderEndOffset(dval);

         Float64 aheadEndOffset(0.0);
         if (ip<ns)
         {
            aheadEndOffset = pBridge->GetCLPierToCLBearingDistance(ip, 0, pgsTypes::Ahead, pgsTypes::AlongItem)
                           - pBridge->GetCLPierToGirderEndDistance(ip, 0, pgsTypes::Ahead, pgsTypes::AlongItem);
         }

         dval = ::ConvertFromSysUnits(aheadEndOffset, unitMeasure::Inch);
         pd.aheadGirderEndOffset(dval);

         pds.push_back(pd);
      }

      brdata.PierData(pds);

      GET_IFACE2(pBroker,ICamber,pCamber);
      GET_IFACE2(pBroker, IGirderLifting, pLifting);
      GET_IFACE2(pBroker, IGirderHauling, pHauling);
      GET_IFACE2(pBroker,ISectProp2,pSectProp);
      GET_IFACE2(pBroker,IPointOfInterest,pPoi);
      GET_IFACE2(pBroker,ILongitudinalRebar,pLongRebar);
      GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,IProductForces,pProduct);
      GET_IFACE2(pBroker,IPrestressForce,pPrestressForce);

      // Girder Data
      Float64 bridgeHaunchVolume = 0.0; // will add haunches from all girders to compute this
      KDOT::BridgeDataType::GirderData_sequence gds;

      for (std::vector<SpanGirderHashType>::const_iterator itg = gdrList.begin(); itg != gdrList.end(); itg++)
      {
         SpanIndexType is;
         GirderIndexType ig;

         UnhashSpanGirder(*itg,&is,&ig);

         PierIndexType prevPierIdx = is;
         PierIndexType nextPierIdx = is+1;

         GirderIndexType ng = pBridge->GetGirderCount(is);

         KDOT::BridgeDataType::GirderData_type gd;

         ::KDOT::GirderKeyType gkey;
         gkey.SpanIndex(is);
         gkey.GirderIndex(ig);
         gd.GirderKey(gkey);

         // have to dig into library to get information
         GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
         const GirderLibraryEntry* pGirderEntry = pBridgeDesc->GetSpan(is)->GetGirderTypes()->GetGirderLibraryEntry(ig);
         const GirderLibraryEntry::Dimensions& dimensions = pGirderEntry->GetDimensions();
         std::_tstring strGirderName = pGirderEntry->GetName();

         gd.GirderType(T2A(strGirderName.c_str()));

         KDOT::BridgeDataType::GirderData_type::SectionDimensions_sequence sds;

         // section dimensions
         const GirderLibraryEntry::Dimensions& dims = pGirderEntry->GetDimensions();
         for(GirderLibraryEntry::Dimensions::const_iterator itd=dims.begin(); itd!=dims.end(); itd++)
         {
            const GirderLibraryEntry::Dimension& dim = *itd;

            KDOT::BridgeDataType::GirderData_type::SectionDimensions_type sd;

            sd.ParameterName(T2A(dim.first.c_str()));

            // assumes are dimensions are reals. This may not be the case (e.g., voided slab has #voids)
            dval = ::ConvertFromSysUnits(dim.second, unitMeasure::Inch);
            sd.Value(dval);

            sds.push_back(sd);
         }

         gd.SectionDimensions(sds);

         // Girder material
         dval = pBridgeMaterial->GetFciGdr(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Fci(dval);

         dval = pBridgeMaterial->GetFcGdr(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Fc(dval);

         dval = pBridgeMaterial->GetEciGdr(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Eci(dval);

         dval = pBridgeMaterial->GetEcGdr(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Ec(dval);

         // Girder length 
         Float64 girderLength = pBridge->GetGirderLength(is,ig);
         Float64 girderSpanLength = pBridge->GetSpanLength(is,ig);

         dval = ::ConvertFromSysUnits(girderLength, unitMeasure::Inch);
         gd.GirderLength(dval);

         // Station and offset of girder line ends
         CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
         pGirder->GetGirderEndPoints(is,ig,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

         Float64 startStation, startOffset, endStation, endOffset;
         pAlignment->GetStationAndOffset(pntPier1,&startStation,&startOffset);
         pAlignment->GetStationAndOffset(pntPier2,&endStation,  &endOffset);

         Float64 startPierStation = pBridge->GetPierStation(0);
         Float64 startBrDistance = startStation - startPierStation;
         Float64 endBrDistance   = endStation   - startPierStation;

         // Girder spacing
         std::vector<Float64> prevPierSpac = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtPierLine,    pgsTypes::NormalToItem);
         std::vector<Float64> nextPierSpac = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back,  pgsTypes::AtPierLine,    pgsTypes::NormalToItem);

         // left start
         if (ig==0)
         {
            dval = pBridge->GetLeftSlabOverhang(startBrDistance);
         }
         else
         {
            dval = prevPierSpac[ig-1];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingLeftStart(dval);

         // Right start
         if (ig==ng-1)
         {
            dval = pBridge->GetRightSlabOverhang(startBrDistance);
         }
         else
         {
            dval = prevPierSpac[ig];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingRightStart(dval);

         // left end
         if (ig==0)
         {
            dval = pBridge->GetLeftSlabOverhang(endBrDistance);
         }
         else
         {
            dval = nextPierSpac[ig-1];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingLeftEnd(dval);

         // Right end
         if (ig==ng-1)
         {
            dval = pBridge->GetRightSlabOverhang(endBrDistance);
         }
         else
         {
            dval = nextPierSpac[ig];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingRightEnd(dval);

         // Harping points
         KDOT::GirderDataType::HarpingPoints_sequence hpseq;
         IndexType nhp = pStrandGeom->GetNumHarpPoints(is,ig);
         gd.NumberOfHarpingPoints(nhp);

         Float64 holddownforce = pPrestressForce->GetHoldDownForce(is,ig);

         for (IndexType ihp=0; ihp<nhp; ihp++)
         {
            KDOT::HarpingPointDataType hpdata;
            Float64 lhp, rhp;
            pStrandGeom->GetHarpingPointLocations(is,ig,&lhp,&rhp);

            dval = ::ConvertFromSysUnits(ihp==0 ? lhp:rhp, unitMeasure::Inch);
            hpdata.Location(dval);

            dval = ::ConvertFromSysUnits(holddownforce/nhp, unitMeasure::Kip);
            hpdata.HoldDownForce(dval);

            hpseq.push_back(hpdata);
         }

         gd.HarpingPoints(hpseq);

         // Lifting location
         dval = pLifting->GetLeftLiftingLoopLocation(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.LiftingLocation(dval);

         // Hauling locations
         dval = pHauling->GetLeadingOverhang(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.LeadingHaulingLocation(dval);

         dval = pHauling->GetTrailingOverhang(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.TrailingHaulingLocation(dval);

         // A dimensions
         Float64 adstart = pBridge->GetSlabOffset(is, ig, pgsTypes::metStart);
         Float64 adend   = pBridge->GetSlabOffset(is, ig, pgsTypes::metEnd);

         dval = ::ConvertFromSysUnits(adstart, unitMeasure::Inch);
         gd.StartADimension(dval);

         dval = ::ConvertFromSysUnits(adend, unitMeasure::Inch);
         gd.EndADimension(dval);

         // Section properties
         bool bIsPrismatic_Final  = pGirder->IsPrismatic(pgsTypes::CastingYard, is,ig);
         gd.IsPrismatic(bIsPrismatic_Final);

         // Take sample at mid-span
         std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(is,ig,pgsTypes::CastingYard,POI_MIDSPAN));
         ATLASSERT(vPoi.size() == 1);
         pgsPointOfInterest mid_poi(vPoi.front());

         pgsPointOfInterest end_poi(is,ig,0.0); // Left end of girder

          // Non-composite properties
         dval = pSectProp->GetAg(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch2);
         gd.Area(dval);

         dval = pSectProp->GetIx(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Ix(dval);

         dval = pSectProp->GetIy(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Iy(dval);

         Float64 Hg = pSectProp->GetHg(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(Hg, unitMeasure::Inch);
         gd.d(dval);

         dval = pSectProp->GetYt(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yt(dval);

         dval = pSectProp->GetYb(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yb(dval);

         dval = pSectProp->GetSt(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.St(dval);

         dval = pSectProp->GetSb(pgsTypes::CastingYard,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.Sb(dval);

         dval = pSectProp->GetPerimeter(mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.P(dval);

         dval = pSectProp->GetGirderWeightPerLength(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KipPerInch);
         gd.W(dval);

         dval = pSectProp->GetGirderWeight(is,ig);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Kip);
         gd.Wtotal(dval);

          // Composite properties
         dval = pSectProp->GetAg(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch2);
         gd.Area_c(dval);

         dval = pSectProp->GetIx(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Ix_c(dval);

         dval = pSectProp->GetIy(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Iy_c(dval);

         dval = pSectProp->GetHg(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.d_c(dval);

         dval = pSectProp->GetYt(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yt_c(dval);

         dval = pSectProp->GetYb(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yb_c(dval);

         dval = pSectProp->GetSt(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.St_c(dval);

         dval = pSectProp->GetSb(pgsTypes::BridgeSite3,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.Sb_c(dval);

         // Strand eccentricities
         Float64 nEff;
         dval = pStrandGeom->GetEccentricity(end_poi, false /*no temporary strands*/, &nEff );
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.StrandEccentricityAtEnds(dval);

         dval = pStrandGeom->GetEccentricity( mid_poi, false /*no temporary strands*/, &nEff );
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.StrandEccentricityAtHPs(dval);

         // prestressing strand material type
         KDOT::PrestressingStrandType pstype;
         const matPsStrand* pmatps = pGirderData->GetStrandMaterial(is, ig, pgsTypes::Straight);

         std::_tstring name = pmatps->GetName();
         pstype.Name(T2A(name.c_str()));

         dval = pmatps->GetNominalDiameter();
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         pstype.NominalDiameter(dval);
         
         gd.PrestressingStrandMaterial(pstype);

         // Strand data
         // Straight strands
         StrandIndexType sival;
         sival = pStrandGeom->GetNumStrands(is,ig, pgsTypes::Straight);
         gd.NumberOfStraightStrands(sival);

         KDOT::GirderDataType::StraightStrandCoordinates_sequence strCoords;

         CComPtr<IPoint2dCollection> strPoints;
         pStrandGeom->GetStrandPositions(mid_poi, pgsTypes::Straight, &strPoints);

         StrandIndexType sicnt;
         strPoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> strPoint;
            strPoints->get_Item(istrand, &strPoint);
            Float64 x, y;
            strPoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            strCoords.push_back(kpnt);
         }

         gd.StraightStrandCoordinates(strCoords);

         // Debonding
         gd.NumberOfDebondedStraightStrands( pStrandGeom->GetNumDebondedStrands(is, ig, pgsTypes::Straight) );

         KDOT::GirderDataType::StraightStrandDebonding_sequence debonds;

         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            Float64 dstart, dend;
            if (pStrandGeom->IsStrandDebonded(is, ig, istrand, pgsTypes::Straight, &dstart, &dend))
            {
               dstart = ::ConvertFromSysUnits(dstart, unitMeasure::Inch);
               dend   = ::ConvertFromSysUnits(dend,   unitMeasure::Inch);

               ::KDOT::DebondDataType debond(istrand+1, dstart, dend);
               debonds.push_back(debond);
            }
         }

         gd.StraightStrandDebonding(debonds);

         // Extended strands
         KDOT::GirderDataType::StraightStrandExtensions_sequence extends;

         StrandIndexType next(0);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            bool extstart = pStrandGeom->IsExtendedStrand(is, ig, pgsTypes::metStart, istrand, pgsTypes::Straight);
            bool extend = pStrandGeom->IsExtendedStrand(is, ig, pgsTypes::metEnd, istrand, pgsTypes::Straight);
            if (extstart || extend)
            {
               ::KDOT::StrandExtensionDataType sed(istrand+1, extstart, extend);
               
               extends.push_back(sed);
               next++;
            }
         }

         gd.StraightStrandExtensions(extends);
         gd.NumberOfExtendedStraightStrands( next );

         // Harped strands
         sival = pStrandGeom->GetNumStrands(is, ig, pgsTypes::Harped);
         gd.NumberOfHarpedStrands(sival);

         // coords at ends
         KDOT::GirderDataType::HarpedStrandCoordinatesAtEnds_sequence heCoords;

         CComPtr<IPoint2dCollection> hePoints;
         pStrandGeom->GetStrandPositions(end_poi, pgsTypes::Harped, &hePoints);

         hePoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> hePoint;
            hePoints->get_Item(istrand, &hePoint);
            Float64 x, y;
            hePoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            heCoords.push_back(kpnt);
         }

         gd.HarpedStrandCoordinatesAtEnds(heCoords);

         // coords at HP
         KDOT::GirderDataType::HarpedStrandCoordinatesAtHP_sequence hpCoords;

         CComPtr<IPoint2dCollection> hpPoints;
         pStrandGeom->GetStrandPositions(mid_poi, pgsTypes::Harped, &hpPoints);

         hpPoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> hpPoint;
            hpPoints->get_Item(istrand, &hpPoint);
            Float64 x, y;
            hpPoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            hpCoords.push_back(kpnt);
         }

         gd.HarpedStrandCoordinatesAtHP(hpCoords);

         // Temporary strands
         sival = pStrandGeom->GetNumStrands(is, ig, pgsTypes::Temporary);
         gd.NumberOfTemporaryStrands(sival);

         KDOT::GirderDataType::TemporaryStrandCoordinates_sequence tmpCoords;

         CComPtr<IPoint2dCollection> tmpPoints;
         pStrandGeom->GetStrandPositions(mid_poi, pgsTypes::Temporary, &tmpPoints);

         tmpPoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> tmpPoint;
            tmpPoints->get_Item(istrand, &tmpPoint);
            Float64 x, y;
            tmpPoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            tmpCoords.push_back(kpnt);
         }

         gd.TemporaryStrandCoordinates(tmpCoords);

         // Long. rebar materials
         KDOT::RebarMaterialType lrbrmat;

         matRebar::Type rebarType;
         matRebar::Grade rebarGrade;
         pBridgeMaterial->GetLongitudinalRebarMaterial(is, ig, rebarType, rebarGrade);

         std::_tstring grd = GenerateReinfGradeName(rebarGrade);
         lrbrmat.Grade(T2A(grd.c_str()));

         std::_tstring typ = GenerateReinfTypeName(rebarType);
         lrbrmat.Type(T2A(typ.c_str()));

         gd.LongitudinalRebarMaterial(lrbrmat);

         // Longitudinal bar rows
         CLongitudinalRebarData rebarData = pLongRebar->GetLongitudinalRebarData(is, ig);
         const std::vector<CLongitudinalRebarData::RebarRow>& rebar_rows = rebarData.RebarRows;
         CollectionIndexType rowcnt = rebar_rows.size();

         gd.NumberOfLongitudinalRebarRows(rowcnt);

         Float64 segment_length = pBridge->GetGirderLength(is, ig);

         KDOT::GirderDataType::LongitudinalRebarRows_sequence rows;
         std::vector<CLongitudinalRebarData::RebarRow>::const_iterator iter(rebar_rows.begin());
         std::vector<CLongitudinalRebarData::RebarRow>::const_iterator end(rebar_rows.end());
         for ( ; iter != end; iter++ )
         {
            const CLongitudinalRebarData::RebarRow& rowData = *iter;

            Float64 startLoc, endLoc;
            bool onGirder = rowData.GetRebarStartEnd(segment_length, &startLoc, &endLoc);

            const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(rebarData.BarType, rebarData.BarGrade, rowData.BarSize);
            if (pRebar)
            {
               KDOT::RebarRowInstanceType rebarRow;

               dval = ::ConvertFromSysUnits(startLoc, unitMeasure::Inch);
               rebarRow.BarStart(dval);

               dval = ::ConvertFromSysUnits(endLoc, unitMeasure::Inch);
               rebarRow.BarEnd(dval);

               rebarRow.Face(rowData.Face==pgsTypes::GirderTop ? "Top" : "Bottom");

               dval = ::ConvertFromSysUnits(rowData.Cover, unitMeasure::Inch);
               rebarRow.Cover(dval);

               rebarRow.NumberOfBars(rowData.NumberOfBars);

               dval = ::ConvertFromSysUnits(rowData.BarSpacing, unitMeasure::Inch);
               rebarRow.Spacing(dval);

               rebarRow.Size(T2A(pRebar->GetName().c_str()));

               rows.push_back(rebarRow);
            }
         }

         gd.LongitudinalRebarRows(rows);

         // stirrups
         KDOT::RebarMaterialType srbrmat;

         pBridgeMaterial->GetTransverseRebarMaterial(is, ig, rebarType, rebarGrade);

         grd = GenerateReinfGradeName(rebarGrade);
         srbrmat.Grade(T2A(grd.c_str()));

         typ = GenerateReinfTypeName(rebarType);
         srbrmat.Type(T2A(typ.c_str()));

         gd.TransverseReinforcementMaterial(srbrmat);

         // stirrup zones
         ZoneIndexType nz = pStirrupGeometry->GetNumPrimaryZones(is, ig);
         gd.NumberOfStirrupZones(nz);

         KDOT::GirderDataType::StirrupZones_sequence szones;

         for (ZoneIndexType iz=0; iz<nz; iz++)
         {
            KDOT::StirrupZoneType szone;

            Float64 zoneStart, zoneEnd;
            pStirrupGeometry->GetPrimaryZoneBounds(is, ig, iz, &zoneStart, &zoneEnd);

            dval = ::ConvertFromSysUnits(zoneStart, unitMeasure::Inch);
            szone.StartLocation(dval);

            dval = ::ConvertFromSysUnits(zoneEnd, unitMeasure::Inch);
            szone.EndLocation(dval);

            matRebar::Size barSize;
            Float64 spacing;
            Float64 nStirrups;
            pStirrupGeometry->GetPrimaryVertStirrupBarInfo(is, ig,iz,&barSize,&nStirrups,&spacing);

            szone.BarSize(T2A(lrfdRebarPool::GetBarSize(barSize).c_str()));

            dval = ::ConvertFromSysUnits(spacing, unitMeasure::Inch);
            szone.BarSpacing(dval);

            szone.NumVerticalLegs(nStirrups);

            Float64 num_legs = pStirrupGeometry->GetPrimaryHorizInterfaceBarCount(is, ig,iz);
            szone.NumLegsExtendedIntoDeck(num_legs);

            barSize = pStirrupGeometry->GetPrimaryConfinementBarSize(is, ig,iz);
            szone.ConfinementBarSize(T2A(lrfdRebarPool::GetBarSize(barSize).c_str()));

            szones.push_back(szone);
         }

         gd.StirrupZones(szones);

         // Camber
         KDOT::GirderDataType::CamberResults_sequence camberResults;

         // First need to compute POI locations - results are at tenth points between pier/girder intersections
         // This is different than normal PGSuper POI's
         // &pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2
         Float64 distFromPierToPier, distFromPierToStartGdr, distFromPierToStartBrg, distFromPierToEndBrg;
         pntPier1->DistanceEx(pntPier2, &distFromPierToPier);
         pntPier1->DistanceEx(pntEnd1, &distFromPierToStartGdr);
         pntPier1->DistanceEx(pntBrg1, &distFromPierToStartBrg);
         pntPier1->DistanceEx(pntBrg2, &distFromPierToEndBrg);

         struct PoiLocType
         {
            Float64 fracLoc;
            Float64 distFromPierLine;
            Float64 distFromBrg;
            Float64 Adim; // A dim at location
         };

#pragma Reminder("In PGSuper 3.0 we can compute cambers at the correct locations. For now, we use the model between bearings only.")

         // Use collapsed model in PGSuper 2.x
         Float64 BrgOffset = pBridge->GetGirderStartConnectionLength(is, ig);
         Float64 GdrOffset = pBridge->GetGirderStartBearingOffset(is, ig);

         std::vector<PoiLocType> poiLocs; // possible POI locations
         poiLocs.reserve(11); // 11 possible pnts
         for(Int32 i=0; i<=10; i++) 
         {
            Float64 fracLoc = (Float64)i/10;
            Float64 loc = girderSpanLength * fracLoc; // dist from bearing

            PoiLocType ploc;
            ploc.fracLoc          = fracLoc;
            ploc.distFromPierLine = loc; // These values are same for collapsed model
            ploc.distFromBrg      = loc; 

            // A dimension height at location
            ploc.Adim = ::LinInterp( ploc.distFromBrg, adstart, adend, girderSpanLength);

            poiLocs.push_back(ploc);
         }

         for(std::vector<PoiLocType>::const_iterator itl=poiLocs.begin(); itl!=poiLocs.end(); itl++)
         {
            const PoiLocType& poiloc = *itl;

            KDOT::CamberResultType camberResult;
            camberResult.FractionalLocation(poiloc.fracLoc);

            dval = ::ConvertFromSysUnits(poiloc.distFromPierLine, unitMeasure::Inch);
            camberResult.Location(dval);

            Float64 locFromEndOfGirder = poiloc.distFromBrg + BrgOffset;

            dval = ::ConvertFromSysUnits(locFromEndOfGirder, unitMeasure::Inch);
            camberResult.LocationFromEndOfGirder(dval);

            pgsPointOfInterest mpoi(is, ig, locFromEndOfGirder); // poi's are measured from end of girder

            Float64 sta, offset;
            pBridge->GetStationAndOffset(mpoi,&sta,&offset);

            Float64 elev = pAlignment->GetElevation(sta,offset);
            dval = ::ConvertFromSysUnits(elev, unitMeasure::Inch);

            camberResult.TopOfDeckElevation(dval);

            // Girder chord elevation
            Float64 topOfGirderChord = elev - poiloc.Adim;
            dval = ::ConvertFromSysUnits(topOfGirderChord, unitMeasure::Inch);
            camberResult.TopOfGirderChordElevation(dval);

            // Get cambers at POI
            Float64 DgdrRelease = pProduct->GetGirderDeflectionForCamber(mpoi);
            Float64 DpsRelease  = pCamber->GetPrestressDeflection(mpoi,true);
            Float64 Dcreep = pCamber->GetCreepDeflection( mpoi, ICamber::cpReleaseToDeck, CREEP_MAXTIME );

            Float64 releaseCamber = DpsRelease + DgdrRelease;
            Float64 slabCastingCamber = releaseCamber + Dcreep;
            Float64 excessCamber = pCamber->GetExcessCamber(mpoi,CREEP_MAXTIME);

            Float64 topAtSlabCasting = topOfGirderChord + slabCastingCamber;
            dval = ::ConvertFromSysUnits(topAtSlabCasting, unitMeasure::Inch);
            camberResult.TopOfGirderElevationPriorToSlabCasting(dval);

            Float64 topAtFinal = topOfGirderChord + excessCamber;
            dval = ::ConvertFromSysUnits(topAtFinal, unitMeasure::Inch);
            camberResult.TopOfGirderElevationAtFinal(dval);

            dval = ::ConvertFromSysUnits(releaseCamber, unitMeasure::Inch);
            camberResult.GirderCamberAtRelease(dval);

            dval = ::ConvertFromSysUnits(slabCastingCamber, unitMeasure::Inch);
            camberResult.GirderCamberPriorToDeckCasting(dval);

            dval = ::ConvertFromSysUnits(excessCamber, unitMeasure::Inch);
            camberResult.GirderCamberAtFinal(dval);

            camberResults.push_back(camberResult);
         }

         gd.CamberResults(camberResults);

         // Lastly, compute girder haunch volume and sum for bridge haunch weight
         Float64 haunchWidth = pGirder->GetTopFlangeWidth(mid_poi);

         // volume of haunch assuming flat girder
         Float64 haunchVolNoCamber = girderLength * haunchWidth * ((adstart+adend)/2 - tDeck);

         // assume that camber makes a parabolic shape along entire length of girder
         Float64 midSpanExcessCamber = pCamber->GetExcessCamber(mid_poi,CREEP_MAXTIME);

         // Area under parabolic segment is 2/3(width)(height) 
         Float64 camberVolume = 2.0/3.0 * haunchWidth * midSpanExcessCamber * girderLength;

         // camber adjusted haunch volume
         Float64 haunchVol = haunchVolNoCamber - camberVolume;
         dval = ::ConvertFromSysUnits(haunchVol, unitMeasure::Inch3);
         gd.GirderHaunchVolume(dval);

         bridgeHaunchVolume += haunchVol;

         // The Collection of girders
         gds.push_back(gd);
      }


      brdata.GirderData(gds);

      // total bridge haunch
      dval = ::ConvertFromSysUnits(bridgeHaunchVolume, unitMeasure::Inch3);
      brdata.HaunchVolumeForAllSelectedGirders(dval);

      // Now can compute haunch weight for entire bridge
      Float64 haunchWDensity = pBridgeMaterial->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();
      Float64 bridgeHaunchWeight = bridgeHaunchVolume * haunchWDensity;

      dval = ::ConvertFromSysUnits(bridgeHaunchWeight, unitMeasure::Kip);
      brdata.HaunchWeightForAllSelectedGirders(dval);

      // Set data for main export class
      kdot_export.BridgeData(brdata);

      // save the XML to a file
      xml_schema::namespace_infomap map;
      map[""].name = "";
      map[""].schema = "KDOTExport.xsd"; // get this from a compiled resource if possible

      std::ofstream ofile(T2A(strFileName.GetBuffer()));

      KDOT::KDOTExport_(ofile,kdot_export,map);
   }
   catch ( const xml_schema::exception& e)
   {
      // an xml schema error has occured. Give the user a nice message and include the message from the exception so we have a
      // fighting chance of resolving the issue.
      CString strMsg;
      strMsg.Format(_T("An error has occured. Your PGSuper data could not be exported for KDOT CAD.\n\n(%s)"),e.what());
      AfxMessageBox(strMsg);
      return S_OK;
   }
   catch (... )
   {
      // an error outside our scope has occured. tell the user we can't export their data... then re-throw the exception and let PGSuper deal with it
      // this happens sometimes if live load distribution factors couldn't be computed or some other THROW_UNWIND type exception
      AfxMessageBox(_T("An error has occured. Your PGSuper data could not be exported for KDOT CAD.\n\nLook in the PGSuper Status Center for additional information."));
      throw;
   }
   } // progress window scope

   return S_OK;
}
