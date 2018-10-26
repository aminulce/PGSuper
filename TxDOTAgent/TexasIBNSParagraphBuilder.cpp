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

#include "StdAfx.h"
#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include "TexasIBNSParagraphBuilder.h"

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\BridgeDescription2.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\GirderLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Inline functions
inline bool IsNonStandardStrands(StrandIndexType nperm, bool isHarpedDesign, int sdtType)
{
   if (nperm>0)
   {
      return sdtType == CStrandData::sdtDirectSelection ||
         (isHarpedDesign && sdtType != CStrandData::sdtTotal );
   }
   else
      return false;
}

static void WriteGirderScheduleTable(rptParagraph* p, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,
                                     const std::vector<CSegmentKey>& segments, ColumnIndexType startIdx, ColumnIndexType endIdx,
                                     IStrandGeometry* pStrandGeometry, ISegmentData* pSegmentData, IPointOfInterest* pPointOfInterest,
                                     const CBridgeDescription2* pBridgeDesc, IArtifact* pIArtifact, ILiveLoadDistributionFactors* pDistFact,
                                     IMaterials* pMaterial, IMomentCapacity* pMomentCapacity,
                                     bool bUnitsSI, bool areAnyTempStrandsInTable, bool areAllHarpedStrandsStraightInTable, 
                                     bool areAnyHarpedStrandsInTable, bool areAnyDebondingInTable);



/****************************************************************************
CLASS	TxDOTIBNSDebondWriter
****************************************************************************/

void TxDOTIBNSDebondWriter::WriteDebondData(rptParagraph* pPara,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const std::_tstring& optionalName)
{
   *pPara<<rptNewLine; // make some space

   // build data structures
   Compute();

   StrandIndexType nss = m_pStrandGeometry->GetStrandCount(m_SegmentKey,pgsTypes::Straight);
   bool is_optional = optionalName.size() > 0;

   // see if we have an error condition - don't build table if so
   if (nss==0 || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
   {
      if (is_optional)
      {
         *pPara <<bold(ON)<< _T("Debonding Information for ") << optionalName << bold(OFF) << rptNewLine;
      }
      else
      {
         *pPara <<bold(ON)<< _T("Debonding Information for Span ") << LABEL_SPAN(m_SegmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(m_SegmentKey.girderIndex) << bold(OFF) << rptNewLine;
      }

      if(nss==0)
      {
         *pPara<< color(Red) <<_T("Warning: No straight strands in girder. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else if(m_OutCome==SectionMismatch)
      {
         *pPara<< color(Red) <<_T("Warning: Irregular, Non-standard debonding increments used. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else if (m_OutCome==TooManySections)
      {
         *pPara<< color(Red) <<_T("Warning: More than ten debonding increments exist. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else if (m_OutCome==SectionsNotSymmetrical)
      {
         *pPara<< color(Red) <<_T("Warning: Debond sections are not symmetic about girder mid-span. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else
      {
         ATLASSERT(false); // A new outcome we aren't aware of?
      }
   }
   else
   {
      // All is ok to write table
      INIT_UV_PROTOTYPE( rptLengthUnitValue, uloc, pDisplayUnits->GetSpanLengthUnit(), true);
      INIT_UV_PROTOTYPE( rptLengthUnitValue, ucomp,    pDisplayUnits->GetComponentDimUnit(), true );

      uloc.SetFormat(sysNumericFormatTool::Automatic);

      const ColumnIndexType num_cols=13;
      std::_tostringstream os;
      if (is_optional)
      {
         os << _T("NS Direct-Fill Strand Pattern for ") << optionalName;
      }
      else
      {
         os <<_T("Debonded Strand Pattern for Span ")<<LABEL_SPAN(m_SegmentKey.groupIndex)<<_T(" Girder ")<<LABEL_GIRDER(m_SegmentKey.girderIndex);
      }

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(num_cols,os.str());
      *pPara << p_table;

      // This table has a very special header
      p_table->SetNumberOfHeaderRows(2);

      RowIndexType row = 0;

      p_table->SetRowSpan(row,0,2);
      (*p_table)(row,0) << _T("Dist from Bottom");

      p_table->SetColumnSpan(row,1,2);
      (*p_table)(row,1) << _T("No. Strands");

      p_table->SetColumnSpan(row,2,10);
      (*p_table)(row,2) << _T("Number of Strands Debonded To");

      // null remaining cells in this row
      ColumnIndexType ic;
      for (ic = 3; ic < num_cols; ic++)
      {
         p_table->SetColumnSpan(row,ic,SKIP_CELL);
      }

      // next row of header
      p_table->SetColumnSpan(++row,0,SKIP_CELL); 

      (*p_table)(row,1) << Bold(_T("Total"));
      (*p_table)(row,2) << Bold(_T("Debonded"));

      Int16 loc_inc=1;
      for (ic = 3; ic < num_cols; ic++)
      {
         Float64 loc = m_SectionSpacing*loc_inc;
         (*p_table)(row,ic) <<Bold( uloc.SetValue(loc) );
         loc_inc++;
      }

      if (m_NumDebonded == 0 && !is_optional)
      {
         // no debonded strands, just write one row
         row++;

         pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);

         std::vector<StrandIndexType> vss  = m_pStrandGeometry->GetStrandsInRow(poi,0,pgsTypes::Straight);
         ATLASSERT(vss.size()>0);

         // get y of any strand in row
         CComPtr<IPoint2dCollection> coords;
         m_pStrandGeometry->GetStrandPositions(poi, pgsTypes::Straight, &coords);

         GET_IFACE2(pBroker,ISectionProperties,pSectProp);
         GET_IFACE2(pBroker,IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
         Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,poi);

         // get elevation of strand
         CComPtr<IPoint2d> point;
         coords->get_Item(vss[0],&point);
         Float64 curr_y;
         point->get_Y(&curr_y);

         (*p_table)(row,0) << ucomp.SetValue(Hg + curr_y);
         (*p_table)(row,1) << (long)vss.size();

         // rest of colums are zeros
         for (ColumnIndexType icol = 2; icol < num_cols; icol++)
         {
            (*p_table)(row,icol) << (long)0;
         }
      }
      else
      {
         // Finished writing Header, now write table, row by row
         ATLASSERT(!m_Rows.empty()); // we have debonds - we gotta have rows?

         RowIndexType nrow = 0;
         RowListIter riter = m_Rows.begin();
         while(riter != m_Rows.end())
         {
            const RowData& rowdata = *riter;

            if( !rowdata.m_Sections.empty() || is_optional) // Only write row if it has debonding, or we are writing optional design
            {
               row++; // table 

               pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);

               GET_IFACE2(pBroker,ISectionProperties,pSectProp);
               GET_IFACE2(pBroker,IIntervals,pIntervals);
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
               Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,poi);

               (*p_table)(row,0) << ucomp.SetValue(Hg + rowdata.m_Elevation);

               (*p_table)(row,1) << rowdata.m_NumTotalStrands;;

               Int16 ndbr = CountDebondsInRow(rowdata);
               (*p_table)(row,2) << ndbr;

               // we have 10 columns to write no matter what
               SectionListConstIter scit = rowdata.m_Sections.begin();

               for (ColumnIndexType icol = 3; icol < num_cols; icol++)
               {
                  Int16 db_cnt = 0;

                  if (scit!= rowdata.m_Sections.end())
                  {
                     const SectionData secdata = *scit;
                     Float64 row_loc = (icol-2)*m_SectionSpacing;

                     if (IsEqual(row_loc, secdata.m_XLoc))
                     {
                        db_cnt = secdata.m_NumDebonds;
                        scit++;
                     }
                  }

                  (*p_table)(row,icol) << db_cnt;
               }

               ATLASSERT(scit==rowdata.m_Sections.end()); // we didn't find all of our sections - bug
            }


            nrow++;
            riter++;
         }

         // write note about non-standard spacing if applicable
         if (m_OutCome==NonStandardSection)
         {
            *pPara<< color(Red)<<_T("Warning: Non-standard debonding increment of ")<<uloc.SetValue(m_SectionSpacing)<<_T(" used.")<<color(Black)<<rptNewLine;
         }
      }
   }
}

/****************************************************************************
CLASS	CTexasIBNSParagraphBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasIBNSParagraphBuilder::CTexasIBNSParagraphBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

/*--------------------------------------------------------------------*/
rptParagraph* CTexasIBNSParagraphBuilder::Build(IBroker*	pBroker, const std::vector<CSegmentKey>& segmentKeys,
                                                IEAFDisplayUnits* pDisplayUnits, Uint16 level, bool& rbEjectPage) const
{
   rbEjectPage = true; // we can just fit this and the geometry table on a page if there is no additional data


   rptParagraph* p = new rptParagraph;

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   GET_IFACE2(pBroker, ISegmentData, pSegmentData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, IMaterials, pMaterial);
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);
   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // First thing - check if we can generate the girder schedule table at all.
   // Round up data common to all tables
   bool areAnyHarpedStrandsInTable(false), areAnyBentHarpedStrandsInTable(false);
   bool areAnyTempStrandsInTable(false), areAnyDebondingInTable(false);
   std::vector<CSegmentKey>::const_iterator itsg(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator itsg_end(segmentKeys.end());
   for (; itsg != itsg_end; itsg++ )
   {
      const CSegmentKey& segmentKey(*itsg);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
      if (0 < nh)
      {
         areAnyHarpedStrandsInTable = true;

         // check that eccentricity is same at ends and mid-girder
         pgsPointOfInterest pois(segmentKey,0.0);
         std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(segmentKey,POI_5L | POI_RELEASED_SEGMENT);
         ATLASSERT(pmid.size()==1);

         Float64 nEff;
         Float64 hs_ecc_end = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pois, pgsTypes::Harped, &nEff);
         Float64 hs_ecc_mid = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pmid[0], pgsTypes::Harped, &nEff);
         if (! IsEqual(hs_ecc_end, hs_ecc_mid) )
         {
            areAnyBentHarpedStrandsInTable = true;
         }
      }

      areAnyTempStrandsInTable   |= 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary);
      areAnyDebondingInTable     |= 0 < pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
   }

   bool areAllHarpedStrandsStraightInTable = areAnyHarpedStrandsInTable && !areAnyBentHarpedStrandsInTable;

   // Cannot have Harped and Debonded strands in same report
   if (areAnyHarpedStrandsInTable && areAnyBentHarpedStrandsInTable && areAnyDebondingInTable)
   {
      *p << color(Red) << bold(ON) <<_T("Note: The TxDOT Girder Schedule report cannot be generated with mixed harped and debonded designs.")
         << _T("Please select other (similar) girders and try again.") << bold(OFF) << color(Black) << rptNewLine;
   }
   else
   {
      // First notes, if non-standard design for any girders
      bool wasNS(false);
      itsg = segmentKeys.begin();
      while(itsg != itsg_end)
      {
         const CSegmentKey& segmentKey(*itsg);

         StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
         StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);

         bool isHarpedDesign = !pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) &&
                              0 < pStrandGeometry->GetMaxStrands(segmentKey, pgsTypes::Harped);

         const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

         if ( IsNonStandardStrands( ns+nh, isHarpedDesign, pStrands->GetStrandDefinitionType()) )
         {
            SpanIndexType spanIdx = segmentKey.groupIndex;
            GirderIndexType gdrIdx = segmentKey.girderIndex;

            *p << color(Red) << _T("Note: A Non-Standard Strand Fill Was Used For Span ")
               << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << color(Black) << rptNewLine;

            wasNS = true;
         }

         itsg++;
      }

      if (wasNS)
      {
         *p << rptNewLine; // add a separator for cleaness
      }

      // Main Girder Schedule Table(s)
      // Compute a list of tables to be created. Each item in the list is the number of columns for
      // each table
      std::vector<CGirderKey> girderKeys;
      girderKeys.insert(girderKeys.begin(),segmentKeys.begin(),segmentKeys.end());
      std::list<ColumnIndexType> table_list = ComputeTableCols(girderKeys);

      bool tbfirst = true;
      ColumnIndexType start_idx, end_idx;
      for (std::list<ColumnIndexType>::iterator itcol = table_list.begin(); itcol!=table_list.end(); itcol++)
      {
         if (tbfirst)
         {
            start_idx = 0;
            end_idx = *itcol-1;
            tbfirst = false;
         }
         else
         {
            start_idx = end_idx+1;
            end_idx += *itcol;
            ATLASSERT(end_idx < table_list.size());
         }

         WriteGirderScheduleTable(p, pBroker, pDisplayUnits, segmentKeys, start_idx, end_idx,
                                     pStrandGeometry, pSegmentData, pPointOfInterest,
                                     pBridgeDesc, pIArtifact, pDistFact, pMaterial, pMomentCapacity,
                                     bUnitsSI, areAnyTempStrandsInTable, areAllHarpedStrandsStraightInTable, 
                                     areAnyHarpedStrandsInTable, areAnyDebondingInTable);
      }

      // Write debond table(s)
      if (areAnyDebondingInTable)
      {
         rbEjectPage = false;
         itsg = segmentKeys.begin();
         while(itsg != itsg_end)
         {
            const CSegmentKey& segmentKey(*itsg);

            WriteDebondTable(p, pBroker, segmentKey, pDisplayUnits);

            itsg++;
         }
      }

      // Write non-standard designs tables
      itsg = segmentKeys.begin();
      while(itsg != itsg_end)
      {
         const CSegmentKey& segmentKey(*itsg);

         StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
         StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);

         bool isHarpedDesign = !pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey) &&
                              0 < pStrandGeometry->GetMaxStrands(segmentKey, pgsTypes::Harped);

         const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

         if ( IsNonStandardStrands( ns+nh, isHarpedDesign, pStrands->GetStrandDefinitionType() ) )
         {
            rbEjectPage = false;
            // Nonstandard strands table
            std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(segmentKey,POI_5L | POI_ERECTED_SEGMENT);
            ATLASSERT(pmid.size()==1);

            OptionalDesignHarpedFillUtil::StrandRowSet strandrows = OptionalDesignHarpedFillUtil::GetStrandRowSet(pBroker, pmid[0]);

            SpanIndexType spanIdx = segmentKey.groupIndex;
            GirderIndexType gdrIdx = segmentKey.girderIndex;

            std::_tostringstream os;
            os <<_T("Non-Standard Strand Pattern for Span ")<<LABEL_SPAN(spanIdx)<<_T(" Girder ")<<LABEL_GIRDER(gdrIdx);

            rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2, os.str().c_str());
            p_table->SetColumnWidth(0,1.0);
            p_table->SetColumnWidth(1,1.8);
            *p << rptNewLine << p_table;

            RowIndexType row = 0;
            (*p_table)(row,0) << _T("Row")<<rptNewLine<<_T("(in)"); // TxDOT dosn't do metric and we need special formatting below
            (*p_table)(row++,1) << _T("Strands");

            for (OptionalDesignHarpedFillUtil::StrandRowIter srit=strandrows.begin(); srit!=strandrows.end(); srit++)
            {
               const OptionalDesignHarpedFillUtil::StrandRow& srow = *srit;
               Float64 elev_in = RoundOff(::ConvertFromSysUnits( srow.Elevation, unitMeasure::Inch ),0.001);

               (*p_table)(row,0) << elev_in; 
               (*p_table)(row++,1) << srow.fillListString << _T( " (") << srow.fillListString.size()*2 << _T(")");
            }
         }

         itsg++;
      }
   }

   return p;
}

void CTexasIBNSParagraphBuilder::WriteDebondTable(rptParagraph* pPara, IBroker* pBroker, const CSegmentKey& segmentKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   bool bCanDebond = pStrandGeometry->CanDebondStrands(segmentKey,pgsTypes::Straight);
   bCanDebond     |= pStrandGeometry->CanDebondStrands(segmentKey,pgsTypes::Harped);
   bCanDebond     |= pStrandGeometry->CanDebondStrands(segmentKey,pgsTypes::Temporary);

   if ( !bCanDebond )
      return;

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   // Need compute tool to decipher debond data
   TxDOTIBNSDebondWriter tx_writer(segmentKey, segment_length, pStrandGeometry);

   tx_writer.WriteDebondData(pPara, pBroker, pDisplayUnits, std::_tstring());
}

void WriteGirderScheduleTable(rptParagraph* p, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,
                              const std::vector<CSegmentKey>& segmentKeys, ColumnIndexType startIdx, ColumnIndexType endIdx,
                              IStrandGeometry* pStrandGeometry, ISegmentData* pSegmentData, IPointOfInterest* pPointOfInterest,
                              const CBridgeDescription2* pBridgeDesc, IArtifact* pIArtifact, ILiveLoadDistributionFactors* pDistFact,
                              IMaterials* pMaterial, IMomentCapacity* pMomentCapacity,
                              bool bUnitsSI, bool areAnyTempStrandsInTable, bool areAllHarpedStrandsStraightInTable, 
                              bool areAnyHarpedStrandsInTable, bool areAnyDebondingInTable)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2_NOCHECK(pBroker,ISectionProperties,pSectProp);

#if defined _DEBUG
   GET_IFACE2(pBroker,IGirder,pGirder);
#endif

   CollectionIndexType ng = endIdx-startIdx+1;
   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(ng+1,_T("TxDOT Girder Schedule"));

   *p << p_table;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, ecc,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dia,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       true );
   rptRcScalar df;
   df.SetFormat(sysNumericFormatTool::Fixed);
   df.SetWidth(8);
   df.SetPrecision(5);

   bool bFirst(true);
   ColumnIndexType col = 1;
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      const CSegmentKey& segmentKey(segmentKeys[gdr_idx]);

      ATLASSERT(pGirder->IsSymmetricSegment(segmentKey)); // this report assumes girders don't taper in depth

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
      StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);

      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      const matPsStrand* pstrand = pStrands->GetStrandMaterial(pgsTypes::Straight);

      // create pois at the start of girder and mid-span
      pgsPointOfInterest pois(segmentKey,0.0);
      std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT);
      ATLASSERT(pmid.size()==1);

      SpanIndexType spanIdx = segmentKey.groupIndex;
      GirderIndexType gdrIdx = segmentKey.girderIndex;
      CSpanKey spanKey(spanIdx,gdrIdx);

      RowIndexType row = 0;
      if(bFirst)
         (*p_table)(row,0) << Bold(_T("Span"));

      (*p_table)(row++,col) << Bold(LABEL_SPAN(spanIdx));

      if(bFirst)
         (*p_table)(row,0) << Bold(_T("Girder"));

      (*p_table)(row++,col) << Bold(LABEL_GIRDER(gdrIdx));

      if(bFirst)
         (*p_table)(row,0) << _T("Girder Type");

      (*p_table)(row++,col) << pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetGirderName();

      if(bFirst)
      {
         (*p_table)(row,0) << Bold(_T("Prestressing Strands"));

         if (ng==1)
            (*p_table)(row, 1) << Bold(_T("Total"));
         else
            (*p_table)(row, 1) << _T("");
      }
      else
      {
         (*p_table)(row,col) << _T("");
      }

      row++;

      // Determine if harped strands are straight by comparing harped eccentricity at end/mid
      bool are_harped_straight(false);
      if (0 < nh)
      {
         Float64 nEff;
         Float64 hs_ecc_end = pStrandGeometry->GetEccentricity(releaseIntervalIdx, pois, pgsTypes::Harped, &nEff);
         Float64 hs_ecc_mid = pStrandGeometry->GetEccentricity(releaseIntervalIdx, pmid[0], pgsTypes::Harped, &nEff);
         are_harped_straight = IsEqual(hs_ecc_end, hs_ecc_mid);
      }

      if(bFirst)
         (*p_table)(row,0) << _T("NO. (N") << Sub(_T("h")) << _T(" + N") << Sub(_T("s")) << _T(")");

      (*p_table)(row++,col) << Int16(nh + ns);

      if(bFirst)
         (*p_table)(row,0) << _T("Size");

      (*p_table)(row++,col) << dia.SetValue(pstrand->GetNominalDiameter()) << _T(" Dia.");

      if(bFirst)
         (*p_table)(row,0) << _T("Strength");

      std::_tstring strData;
      std::_tstring strGrade;
      if ( bUnitsSI )
      {
         strGrade = (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("1725") : _T("1860"));
         strData = _T("Grade ") + strGrade;
         strData += _T(" ");
         strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));
      }
      else
      {
         strGrade = (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("250") : _T("270"));
         strData = _T("Grade ") + strGrade;
         strData += _T(" ");
         strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));
      }

      (*p_table)(row++,col) << strData;


      if(bFirst)
      {
         (*p_table)(row,0) << _T("Eccentricity @ CL");
         if ( areAnyTempStrandsInTable )
            (*p_table)(row,0) << _T(" (w/o Temporary Strands)");
      }

      Float64 nEff;
      (*p_table)(row++,col) << ecc.SetValue( pStrandGeometry->GetEccentricity( releaseIntervalIdx, pmid[0], false, &nEff ) );

      if(bFirst)
      {
         (*p_table)(row,0) << _T("Eccentricity @ End");
         if ( areAnyTempStrandsInTable )
            (*p_table)(row,0) << _T(" (w/o Temporary Strands)");
      }

      (*p_table)(row++,col) << ecc.SetValue( pStrandGeometry->GetEccentricity( releaseIntervalIdx, pois, false, &nEff ) );

      StrandIndexType ndb = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);

      if(bFirst)
      {
         (*p_table)(row,0) << Bold(_T("Prestressing Strands"));
      }

      if (areAnyHarpedStrandsInTable)
      {
         if (are_harped_straight)
         {
            (*p_table)(row++,col) << Bold(_T("Straight"));

            if (bFirst)
               (*p_table)(row,0) << _T("NO. (# of Harped Strands)");

            (*p_table)(row++,col) << nh;
         }
         else
         {
            (*p_table)(row++,col) << Bold(_T("Depressed"));

            if (bFirst)
               (*p_table)(row,0) << _T("NO. (# of Harped Strands)");

            (*p_table)(row++,col) << nh;
         }

         if (bFirst)
            (*p_table)(row,0) << _T("Y")<<Sub(_T("b"))<<_T(" of Topmost Depressed Strand(s) @ End");

         Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, pois);

         Float64 TO;
         pStrandGeometry->GetHighestHarpedStrandLocationEnds(segmentKey,&TO);

         // value is measured down from top of girder... we want it measured up from the bottom
         TO += Hg;

         (*p_table)(row++,col) << ecc.SetValue(TO);
      }

      if ( areAnyDebondingInTable )
      {
//         (*p_table)(row++,col) << Bold(_T("Debonded"));

         if (bFirst)
            (*p_table)(row,0) << _T("NO. (# of Debonded Strands)");

         (*p_table)(row++,col) << ndb;
      }

      if (bFirst)
         (*p_table)(row,0) << Bold(_T("Concrete"));

      (*p_table)(row++,col) << Bold(_T(""));

      if (bFirst)
         (*p_table)(row,0) << _T("Release Strength ")<<RPT_FCI;

      (*p_table)(row++,col) << stress.SetValue(pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx));

      if (bFirst)
         (*p_table)(row,0) << _T("Minimum 28 day compressive strength ")<<RPT_FC;

      (*p_table)(row++,col) << stress.SetValue(pMaterial->GetSegmentFc28(segmentKey));

      if (bFirst)
         (*p_table)(row,0) << Bold(_T("Optional Design"));

      (*p_table)(row++,col) << Bold(_T(""));

      const pgsFlexuralStressArtifact* pArtifact;
      Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;


      pmid = pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT);
      ATLASSERT(pmid.size()==1);

      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi( liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,pmid[0].GetID() );
      fcTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
      fcBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);

      if (bFirst)
         (*p_table)(row,0) << _T("Design Load Compressive Stress (Top CL)");

      (*p_table)(row++,col) << stress.SetValue(-fcTop);

      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi( liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,pmid[0].GetID() );
      ftTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
      ftBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);

      if (bFirst)
         (*p_table)(row,0) << _T("Design Load Tensile Stress (Bottom CL)");

      (*p_table)(row++,col) << stress.SetValue(-ftBot);

      //const pgsFlexuralCapacityArtifact* pFlexureArtifact = pGdrArtifact->GetFlexuralCapacityArtifact( pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,pmid[0].GetDistFromStart()) );
      MINMOMENTCAPDETAILS mmcd;
      pMomentCapacity->GetMinMomentCapacityDetails(liveLoadIntervalIdx,pmid[0],true,&mmcd);

      if (bFirst)
         (*p_table)(row,0) << _T("Required minimum ultimate moment capacity ");

      (*p_table)(row++,col) << moment.SetValue( Max(mmcd.Mu,mmcd.MrMin) );

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Shear");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI));
      }
      else
      {
         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment (Strength and Service Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Shear (Strength and Service Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment (Fatigue Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(spanKey,pgsTypes::FatigueI));
      }

      bFirst = false;
      col++;
   }

   (*p) << rptNewLine;
   (*p) << color(Red) <<_T("NOTE: Stresses show in the above table reflect the following sign convention:") << rptNewLine 
        << _T("Compressive Stress is positive. Tensile Stress is negative") << color(Black) << rptNewLine;
}

