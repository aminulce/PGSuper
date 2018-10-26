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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\PoiMgr.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsPoiMgr
****************************************************************************/



// Predicate STL objects.

class SamePlace
{
public:
   SamePlace(const pgsPointOfInterest& poi,Float64 tol) : m_Poi(poi), m_Tol(tol) {}

   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_Poi.GetSegmentKey().IsEqual(other.GetSegmentKey()) &&
           IsZero( m_Poi.GetDistFromStart() - other.GetDistFromStart(), m_Tol ) )
      {
         return true;
      }

      return false;
   }

   const pgsPointOfInterest& GetPoi() const { return m_Poi; }

private:
   const pgsPointOfInterest& m_Poi;
   Float64 m_Tol;
};

class ExactlySame
{
public:
   ExactlySame(const pgsPointOfInterest& poi,Float64 tol) : m_SamePlace(poi,tol) {}
   
   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_SamePlace.operator()(other) )
      {
         // poi are at the same place, do they have the same attributes?
         if ( other.GetNonReferencedAttributes() != m_SamePlace.GetPoi().GetNonReferencedAttributes() )
         {
            return false;
         }

         PoiAttributeType refAttribute[] = 
         {
            POI_RELEASED_SEGMENT, 
            POI_LIFT_SEGMENT, 
            POI_STORAGE_SEGMENT,
            POI_HAUL_SEGMENT, 
            POI_ERECTED_SEGMENT, 
            POI_SPAN 
         };
         for ( int i = 0; i < 6; i++ )
         {
            if (other.GetReferencedAttributes(refAttribute[i]) != m_SamePlace.GetPoi().GetReferencedAttributes(refAttribute[i]) )
            {
               return false;
            }
         }

         return true;
      }

      return false;
   }

private:
   SamePlace m_SamePlace;
};

class RemoveTest
{
public:
   RemoveTest(PoiAttributeType targetAttribute,PoiAttributeType exceptAttribute) : m_TargetAttribute(targetAttribute), m_ExceptAttribute(exceptAttribute) {}

   bool operator()(pgsPointOfInterest& poi)
   {
      // return true if the poi has the target attribute, except if it has the exception attribute
      if ( poi.HasAttribute(m_TargetAttribute) && !poi.HasAttribute(m_ExceptAttribute) )
      {
         // the poi can possibly be removed... more testing is needed
         // at minimum we can remove the target attribute, but the actual poi may need to
         // stay in the container (example, removing lifting 10th points pois and the point
         // of prestress transfer is at a tenth poi.. we want to remove the lifting attributes
         // but retain the poi)
         if ( pgsPointOfInterest::IsReferenceAttribute(m_TargetAttribute) )
         {
            poi.RemoveAttributes(m_TargetAttribute | POI_TENTH_POINTS);
         }
         else
         {
            poi.RemoveAttributes(m_TargetAttribute);
         }

         if ( poi.GetNonReferencedAttributes() != 0 )
         {
            return false; // has attributes, don't remove the poi from the container
         }

         PoiAttributeType refAttribute[] = 
         {
            POI_RELEASED_SEGMENT, 
            POI_LIFT_SEGMENT, 
            POI_STORAGE_SEGMENT,
            POI_HAUL_SEGMENT, 
            POI_ERECTED_SEGMENT, 
            POI_SPAN 
         };
         for ( int i = 0; i < 6; i++ )
         {
            if (poi.GetReferencedAttributes(refAttribute[i]) != 0 )
            {
               return false; // has attributes, don't remove the poi from the container
            }
         }

         return true; // no attributes, remove poi from the container
      }

      return false;
   }

private:
   PoiAttributeType m_TargetAttribute;
   PoiAttributeType m_ExceptAttribute;
};

class FindByID
{
public:
   FindByID(PoiIDType id) : m_ID(id) {}
   bool operator()(const pgsPointOfInterest& other) const
   {
      if ( m_ID == other.GetID() )
      {
         return true;
      }

      return false;
   }

private:
   PoiIDType m_ID;
};

class NotFindPoi
{
public:
   NotFindPoi(const CSegmentKey& segmentKey) {m_GroupIdx = segmentKey.groupIndex; m_GirderIdx = segmentKey.girderIndex; m_SegmentIdx = segmentKey.segmentIndex;}
   bool operator()(const pgsPointOfInterest& other) const
   {
      // returning true causes the poi to be excluded from the container
      // return false for those poi that match the search criteria... return false for the poi we want to keep

      const CSegmentKey& segmentKey = other.GetSegmentKey();
      GroupIndexType grpIdx = segmentKey.groupIndex;
      GirderIndexType gdrIdx = segmentKey.girderIndex;

      if ( (m_GroupIdx   == INVALID_INDEX || m_GroupIdx   == grpIdx) &&  // group matches -AND-
           (m_GirderIdx  == INVALID_INDEX || m_GirderIdx  == gdrIdx) &&  // girder matches -AND-
           (m_SegmentIdx == INVALID_INDEX || m_SegmentIdx == segmentKey.segmentIndex) // segment matches
         )
      {
         // we want to keep the POI
         return false;
      }

      return true;
   }

private:
   GroupIndexType   m_GroupIdx;
   GirderIndexType  m_GirderIdx;
   SegmentIndexType m_SegmentIdx;
};

PoiIDType pgsPoiMgr::ms_NextID = 0;


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPoiMgr::pgsPoiMgr()
{
   m_Tolerance = ::ConvertToSysUnits( 1.0, unitMeasure::Millimeter );
   pgsPointOfInterest::SetTolerance( m_Tolerance );
}

pgsPoiMgr::~pgsPoiMgr()
{
   RemoveAll();
}

PoiIDType pgsPoiMgr::AddPointOfInterest(const pgsPointOfInterest& poi)
{
   ASSERT_SEGMENT_KEY(poi.GetSegmentKey());

   // first see if we have an existing poi at this location. Just merge attributes into existing if we do
   std::vector<std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(poi.GetSegmentKey());
   ATLASSERT(vpvPoi.size() == 1);
   std::vector<pgsPointOfInterest>* pvPoi(vpvPoi.front());

   if ( poi.CanMerge() )
   {
      std::vector<pgsPointOfInterest>::iterator iter(pvPoi->begin());
      std::vector<pgsPointOfInterest>::iterator end(pvPoi->end());
      for ( ; iter != end; iter++ )
      {
         pgsPointOfInterest& curpoi = *iter;
         if ( curpoi.AtSamePlace(poi) )
         {
            if ( curpoi.CanMerge() && curpoi.MergeAttributes(poi) )
            {
               return curpoi.m_ID; // no need to re-sort vector
            }
            else
            {
               // sometimes there are multiple POI at the same location... make sure
               // the next poi is not at the same location... if it is, continue, otherwise bail out
               if ( iter+1 != end && !poi.AtSamePlace(*(iter+1)) )
               {
                  break; // poi's can't be merged... just break out of here and continue
               }
            }
         }
      }
   }

   PoiIDType id = poi.m_ID;
   if ( id == INVALID_ID )
   {
      // assert if we are about to roll over the id
      ATLASSERT(ms_NextID != MAX_ID);
      id = ms_NextID++;
   }
   else
   {
      // if in new POI has already been assigned an ID
      // make sure the next POI ID won't conflict with it
      ms_NextID = Max(ms_NextID,id+1);
   }

   // Don't copy poi and put it in because this is a very highly utilized function and copies don't come cheap
#if defined _DEBUG
   std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(pvPoi->begin(),pvPoi->end(), FindByID(id) ) );
   ATLASSERT(found == pvPoi->end());
#endif
   pvPoi->push_back(poi);
   pvPoi->back().m_ID = id;

   std::sort(pvPoi->begin(),pvPoi->end());

   return id;
}

bool pgsPoiMgr::RemovePointOfInterest(const pgsPointOfInterest& poi,bool bConsiderAttributes)
{
   ASSERT_SEGMENT_KEY(poi.GetSegmentKey());

   std::vector<std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(poi.GetSegmentKey());
   ATLASSERT(vpvPoi.size() != 0);

   std::vector<pgsPointOfInterest>* pvPoi(vpvPoi.front());

   std::vector<pgsPointOfInterest>::iterator begin(pvPoi->begin());
   std::vector<pgsPointOfInterest>::iterator end(pvPoi->end());
   std::vector<pgsPointOfInterest>::iterator found;
   if ( bConsiderAttributes )
   {
      found = std::find_if(begin, end, ExactlySame(poi,m_Tolerance) );
   }
   else
   {
      found = std::find_if(begin, end, SamePlace(poi,m_Tolerance) );
   }

   if ( found != end )
   {
      pvPoi->erase(found);
      return true;
   }

   return false;
}

bool pgsPoiMgr::RemovePointOfInterest(PoiIDType poiID)
{
   ATLASSERT(poiID != INVALID_ID);
   if ( poiID == INVALID_ID )
   {
      return false;
   }

   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      std::vector<pgsPointOfInterest>* pvPoi = iter->second;

      std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(pvPoi->begin(), pvPoi->end(), FindByID(poiID) ) );
      if ( found != pvPoi->end() )
      {
         pvPoi->erase(found);
         return true; // fond it... stop searching
      }
   }

   return false; // never found it
}

void pgsPoiMgr::RemovePointsOfInterest(PoiAttributeType targetAttribute,PoiAttributeType exceptionAttribute)
{
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      std::vector<pgsPointOfInterest>* pvPoi = iter->second;
      std::vector<pgsPointOfInterest>::iterator newEnd( std::remove_if(pvPoi->begin(),pvPoi->end(),RemoveTest(targetAttribute,exceptionAttribute)) );
      pvPoi->erase(newEnd,pvPoi->end());
   }
}

void pgsPoiMgr::RemoveAll()
{
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      std::vector<pgsPointOfInterest>* pvPoi = iter->second;
      delete pvPoi;
      iter->second = NULL;
   }
   m_PoiData.clear();
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) 
{
   pgsPointOfInterest poi(segmentKey,Xpoi);

   std::vector<std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   if ( vpvPoi.size() == 0 )
   {
      return poi;
   }

   ATLASSERT(vpvPoi.size() == 1);
   std::vector<pgsPointOfInterest>* pvPoi(vpvPoi.front());

   std::vector<pgsPointOfInterest>::const_iterator begin(pvPoi->begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());

   // find all poi that are within m_Tolerance of Xpoi
   std::vector<pgsPointOfInterest>::const_iterator found = begin;
   std::vector<pgsPointOfInterest> vPoi;
   while ( found != end )
   {
      found = std::find_if(begin,end,SamePlace(poi,m_Tolerance));
      if ( found != end )
      {
         vPoi.push_back(*found);
         begin = found+1;
      }
   }

   // no poi within m_Tolerance of Xpoi, just return the poi created on the fly above
   if ( vPoi.size() == 0 )
   {
      return poi;
   }

   // figure out which poi is closest to Xpoi
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   end   = vPoi.end();
   poi = (*iter);
   Float64 minDist = fabs(Xpoi - poi.GetDistFromStart());
   iter++;
   for ( ; iter != end; iter++ )
   {
      Float64 dist = fabs(Xpoi - iter->GetDistFromStart());
      if ( dist < minDist )
      {
         minDist = dist;
         poi = (*iter);
      }
   }
   return poi;
}

pgsPointOfInterest pgsPoiMgr::GetNearestPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi) 
{
   std::vector<std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   if ( vpvPoi.size() == 0 )
   {
      return pgsPointOfInterest();
   }

   ATLASSERT(vpvPoi.size() == 1);
   std::vector<pgsPointOfInterest>* pvPoi(vpvPoi.front());

   // get the poi just for this segment
   std::vector<pgsPointOfInterest> vPOI;
   std::vector<pgsPointOfInterest>::const_iterator iter(pvPoi->begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSegmentKey() == segmentKey)
      {
         vPOI.push_back(poi);
      }
   }

   if ( vPOI.empty() )
   {
      return pgsPointOfInterest();
   }

   std::vector<pgsPointOfInterest>::const_iterator iter1(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator iter2(iter1+1);
   std::vector<pgsPointOfInterest>::const_iterator end2(vPOI.end());
   for ( ; iter2 != end2; iter1++, iter2++ )
   {
      const pgsPointOfInterest& prevPOI = *iter1;
      const pgsPointOfInterest& nextPOI = *iter2;

      ATLASSERT( prevPOI.GetSegmentKey() == segmentKey);
      ATLASSERT( nextPOI.GetSegmentKey() == segmentKey);

      if ( InRange(prevPOI.GetDistFromStart(),Xpoi,nextPOI.GetDistFromStart()) )
      {
         Float64 d1 = Xpoi - prevPOI.GetDistFromStart();
         Float64 d2 = nextPOI.GetDistFromStart() - Xpoi;

         if ( d1 < d2 )
         {
            return prevPOI;
         }
         else
         {
            return nextPOI;
         }
      }
   }

   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetPrevPointOfInterest(PoiIDType poiID,PoiAttributeType attrib,Uint32 mode) const
{
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi = iter->second;

      std::vector<pgsPointOfInterest>::const_iterator begin(pvPoi->begin());
      std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());
      std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(begin, end, FindByID(poiID) ) );
      if ( found == end )
      {
         continue; // poi not found... try next container
      }

      // at the beginning so we can't back up one poi
      pgsPointOfInterest foundPoi(*found);
      if ( found == begin )
      {
         return foundPoi;
      }

      pgsPointOfInterest poi;
      if ( attrib == 0 )
      {
         // unconditionally want the previous poi
         return *(found-1);
      }

      std::vector<pgsPointOfInterest>::const_reverse_iterator rend(pvPoi->rend());
      std::vector<pgsPointOfInterest>::const_reverse_iterator iter(found);
      for ( ; iter != rend; iter++ )
      {
         const pgsPointOfInterest& thisPoi(*iter);
         if ( thisPoi.GetSegmentKey().girderIndex != foundPoi.GetSegmentKey().girderIndex )
         {
            continue;
         }

         if ( mode == POIMGR_OR )
         {
            if ( OrAttributeEvaluation(thisPoi,attrib) )
            {
               poi = thisPoi;
               break;
            }
         }
         else
         {
            ATLASSERT(mode == POIMGR_AND);
            if ( AndAttributeEvaluation(thisPoi,attrib) )
            {
               poi = thisPoi;
               break;
            }
         }
      }

      if ( poi.GetID() == INVALID_ID || poi.GetSegmentKey().girderIndex != foundPoi.GetSegmentKey().girderIndex )
      {
         // couldn't find a poi with the desired attributes or we found one, but it is in a different girder
         return pgsPointOfInterest();
      }
      else
      {
         return poi;
      }
   }

   ATLASSERT(false); // poi not found
   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetNextPointOfInterest(PoiIDType poiID,PoiAttributeType attrib,Uint32 mode) const
{
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi = iter->second;

      std::vector<pgsPointOfInterest>::const_iterator begin(pvPoi->begin());
      std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());
      std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(begin, end, FindByID(poiID) ) );
      if ( found == end )
      {
         // poi not found, try next container
         continue;
      }

      pgsPointOfInterest foundPoi(*found);
      if ( found == end-1 )
      {
         // at the last poi so we can't advance to the next
         return foundPoi;
      }

      pgsPointOfInterest poi;
      if ( attrib == 0 )
      {
         // unconditionally want the next poi
         return *(found+1);
      }
      else
      {
         // search for next poi that has the desired attributes
         std::vector<pgsPointOfInterest>::const_iterator iter = found+1;
         for ( ; iter != end; iter++ )
         {
            const pgsPointOfInterest& thisPoi(*iter);
            if ( thisPoi.GetSegmentKey().girderIndex != foundPoi.GetSegmentKey().girderIndex )
            {
               continue;
            }

            if ( mode == POIMGR_OR )
            {
               if ( OrAttributeEvaluation(thisPoi,attrib) )
               {
                  poi = thisPoi;
                  break;
               }
            }
            else
            {
               ATLASSERT(mode == POIMGR_AND);
               if ( AndAttributeEvaluation(thisPoi,attrib) )
               {
                  poi = thisPoi;
                  break;
               }
            }
         }
      }

      if ( poi.GetID() == INVALID_ID || poi.GetSegmentKey().girderIndex != foundPoi.GetSegmentKey().girderIndex )
      {
         // couldn't find a poi with the desired attributes or we found one, but it is in a different girder
         return pgsPointOfInterest();
      }
      else
      {
         return poi;
      }
   }

   ATLASSERT(false); // poi not found
   return pgsPointOfInterest();
}

pgsPointOfInterest pgsPoiMgr::GetPointOfInterest(PoiIDType id) const
{
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi = iter->second;

      std::vector<pgsPointOfInterest>::const_iterator begin(pvPoi->begin());
      std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());
      std::vector<pgsPointOfInterest>::const_iterator found( std::find_if(begin, end, FindByID(id) ) );
      if ( found != end )
      {
         return (*found);
      }
   }

   return pgsPointOfInterest();
}

void pgsPoiMgr::GetPointsOfInterest(const CSegmentKey& segmentKey,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const
{
   if ( mode == POIMGR_AND )
   {
      AndFind(segmentKey,attrib,pPois);
   }
   else
   {
      OrFind(segmentKey,attrib,pPois);
   }
}

std::vector<pgsPointOfInterest> pgsPoiMgr::GetPointsOfInterest(const CSegmentKey& segmentKey) const
{
   std::vector<pgsPointOfInterest> copy_poi;

   std::vector<const std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   std::vector<const std::vector<pgsPointOfInterest>*>::iterator iter(vpvPoi.begin());
   std::vector<const std::vector<pgsPointOfInterest>*>::iterator end(vpvPoi.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi = *iter;
      // This has to be the most poorly named function in the stl. 
      // (which really should be called copy_if_not)
      std::remove_copy_if(pvPoi->begin(), pvPoi->end(), std::back_inserter(copy_poi), NotFindPoi(segmentKey));
   }

   std::sort(copy_poi.begin(),copy_poi.end());

   return copy_poi;
}

bool pgsPoiMgr::ReplacePointOfInterest(PoiIDType ID,const pgsPointOfInterest& poi)
{
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      std::vector<pgsPointOfInterest>* pvPoi = iter->second;

      std::vector<pgsPointOfInterest>::iterator begin(pvPoi->begin());
      std::vector<pgsPointOfInterest>::iterator end(pvPoi->end());
      std::vector<pgsPointOfInterest>::iterator found(std::find_if(begin, end, FindByID(ID) ) );
      if ( found != end )
      {
         ATLASSERT(found->GetSegmentKey() == poi.GetSegmentKey()); // the replacement poi should be on the same segment
         *found = poi;
         (*found).m_ID = ID;
         return true;
      }
   }

   return false;
}

//======================== ACCESS     =======================================
Float64 pgsPoiMgr::SetTolerance(Float64 tol)
{
   Float64 oldTolerance = m_Tolerance;
   m_Tolerance = tol;
   pgsPointOfInterest::SetTolerance( m_Tolerance );
   return oldTolerance;
}

Float64 pgsPoiMgr::GetTolerance() const
{
   return m_Tolerance;
}

CollectionIndexType pgsPoiMgr::GetPointOfInterestCount() const
{
   CollectionIndexType nPoi = 0;
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator iter(m_PoiData.begin());
   std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator end(m_PoiData.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi = iter->second;
      nPoi += pvPoi->size();
   }

   return nPoi;
}

void pgsPoiMgr::GetTenthPointPOIs(PoiAttributeType reference,const CSegmentKey& segmentKey,std::vector<pgsPointOfInterest>* pPois) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   std::vector<const std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   ATLASSERT(vpvPoi.size() != 0);
   const std::vector<pgsPointOfInterest>* pvPoi(vpvPoi.front());

   if ( pvPoi == NULL )
   {
      return;
   }

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator iter(pvPoi->begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSegmentKey() == segmentKey && poi.IsTenthPoint(reference) )
      {
         pPois->push_back( poi );
      }
   }
}

void pgsPoiMgr::GetPointsOfInterestInRange(const CSegmentKey& segmentKey,Float64 xMin,Float64 xMax,std::vector<pgsPointOfInterest>* pPois) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   std::vector<const std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   if ( vpvPoi.size() == 0 )
   {
      return;
   }

   ATLASSERT(vpvPoi.size());
   const std::vector<pgsPointOfInterest>* pvPoi(vpvPoi.front());

   pPois->clear();
   std::vector<pgsPointOfInterest>::const_iterator iter(pvPoi->begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pvPoi->end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      if ( poi.GetSegmentKey() == segmentKey && ::InRange(xMin,poi.GetDistFromStart(),xMax) )
      {
         pPois->push_back( poi );
      }
   }
}


void pgsPoiMgr::AndFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   std::vector<const std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   if ( vpvPoi.size() == 0 )
   {
      return;
   }

   pPois->clear();

   std::vector<const std::vector<pgsPointOfInterest>*>::iterator iter(vpvPoi.begin());
   std::vector<const std::vector<pgsPointOfInterest>*>::iterator end(vpvPoi.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi(*iter);
      std::vector<pgsPointOfInterest>::const_iterator poiIter(pvPoi->begin());
      std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(pvPoi->end());
      for ( ; poiIter != poiIterEnd; poiIter++ )
      {
         const pgsPointOfInterest& poi = *poiIter;
         if ( AndFind(poi,segmentKey,attrib) ) 
         {
            // poi has desired attributes - keep it
            pPois->push_back(poi);
         }
      }
   }
}

bool pgsPoiMgr::AndFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same segment, then
   // it checks if each flag in attrib is set. If it is, the corresponding flag in poi must
   // also be set, otherwise, the test is irrelavent (and always passes as indicated by the true)
   //
   // segmentKey can match exactly or match with ALL_XXX constants (kind of like a wildcard)

   // If the search attribute has referenced attributes, get the reference type
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   // NOTE: you can only search for POIs with referenced or non-referenced attributes. You can't search
   // for both types at the same time

   if ( segmentKey == poi.GetSegmentKey() )
   {
      // segmentKeys match....
      return AndAttributeEvaluation(poi,attrib);
   }

   return false;
}

bool pgsPoiMgr::AndAttributeEvaluation(const pgsPointOfInterest& poi,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression checks if each flag in attrib is set. If it is, the corresponding flag in poi must
   // also be set, otherwise, the test is irrelavent (and always passes as indicated by the true)

   // If the search attribute has referenced attributes, get the reference type
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   // NOTE: you can only search for POIs with referenced or non-referenced attributes. You can't search
   // for both types at the same time

   if ( targetReference == 0 )
   {
      // searching for a non-referenced attribute
      if ((sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)           ? poi.HasAttribute(POI_CRITSECTSHEAR1)           : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)           ? poi.HasAttribute(POI_CRITSECTSHEAR2)           : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)             ? poi.IsHarpingPoint()                           : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)                 ? poi.IsConcentratedLoad()                       : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DIAPHRAGM)                ? poi.HasAttribute(POI_DIAPHRAGM)                : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_H)                        ? poi.IsAtH()                                    : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_15H)                      ? poi.IsAt15H()                                  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)                   ? poi.HasAttribute(POI_PSXFER)                   : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DECKBARCUTOFF)            ? poi.HasAttribute(POI_DECKBARCUTOFF)            : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARCUTOFF)                ? poi.HasAttribute(POI_BARCUTOFF)                : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARDEVELOP)               ? poi.HasAttribute(POI_BARDEVELOP)               : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_RIGHTFACE)     ? poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE)     : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_LEFTFACE)      ? poi.HasAttribute(POI_SECTCHANGE_LEFTFACE)      : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_TRANSITION)    ? poi.HasAttribute(POI_SECTCHANGE_TRANSITION)    : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)            ? poi.HasAttribute(POI_FACEOFSUPPORT)            : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_PIER)        ? poi.HasAttribute(POI_INTERMEDIATE_PIER)        : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BOUNDARY_PIER)            ? poi.HasAttribute(POI_BOUNDARY_PIER)            : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_ABUTMENT)                 ? poi.HasAttribute(POI_ABUTMENT)                 : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_STIRRUP_ZONE)             ? poi.HasAttribute(POI_STIRRUP_ZONE)             : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_TEMPSUPPORT) ? poi.HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CLOSURE)                  ? poi.HasAttribute(POI_CLOSURE)                  : true)
         )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }
   else
   {
      // searching for a referenced attribute
      if ( attrib == targetReference )
      {
         // didn't specify which referenced attribute is being searched for... can't match them all
         return false;
      }

      if (
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT) ? poi.HasAttribute(targetReference | POI_PICKPOINT)   : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT) ? poi.HasAttribute(targetReference | POI_BUNKPOINT)   : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_0L)  ? poi.IsTenthPoint(targetReference) == 1  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_1L)  ? poi.IsTenthPoint(targetReference) == 2  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_2L)  ? poi.IsTenthPoint(targetReference) == 3  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_3L)  ? poi.IsTenthPoint(targetReference) == 4  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_4L)  ? poi.IsTenthPoint(targetReference) == 5  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_5L)  ? poi.IsTenthPoint(targetReference) == 6  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_6L)  ? poi.IsTenthPoint(targetReference) == 7  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_7L)  ? poi.IsTenthPoint(targetReference) == 8  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_8L)  ? poi.IsTenthPoint(targetReference) == 9  : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_9L)  ? poi.IsTenthPoint(targetReference) == 10 : true) &&
          (sysFlags<PoiAttributeType>::IsSet(attrib,POI_10L) ? poi.IsTenthPoint(targetReference) == 11 : true)
         )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }

   return false;
}

void pgsPoiMgr::OrFind(const CSegmentKey& segmentKey,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const
{
   std::vector<const std::vector<pgsPointOfInterest>*> vpvPoi = GetPoiContainer(segmentKey);
   if ( vpvPoi.size() == 0 )
   {
      return;
   }

   pPois->clear();

   std::vector<const std::vector<pgsPointOfInterest>*>::iterator iter(vpvPoi.begin());
   std::vector<const std::vector<pgsPointOfInterest>*>::iterator end(vpvPoi.end());
   for ( ; iter != end; iter++ )
   {
      const std::vector<pgsPointOfInterest>* pvPoi(*iter);
      std::vector<pgsPointOfInterest>::const_iterator poiIter(pvPoi->begin());
      std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(pvPoi->end());
      for ( ; poiIter != poiIterEnd; poiIter++ )
      {
         const pgsPointOfInterest& poi = *poiIter;

         if ( OrFind(poi,segmentKey,attrib) )
         {
            // poi has desired attributes - keep it
            pPois->push_back(poi);
         }
      }
   }
}

bool pgsPoiMgr::OrFind(const pgsPointOfInterest& poi,const CSegmentKey& segmentKey,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression first check to make sure we have the same segment, then
   // it checks if each flag in attrib is set. If it is, the corresponding flag in the poi must
   // be set, otherwise, the test is irrelavent (and always passes as indicated by the true)

   // segmentKey can match exactly or match with ALL_XXX constants (kind of like a wildcard)

   if ( segmentKey != poi.GetSegmentKey() )
   {
      return false;
   }

   return OrAttributeEvaluation(poi,attrib);
}

bool pgsPoiMgr::OrAttributeEvaluation(const pgsPointOfInterest& poi,PoiAttributeType attrib) const
{
   // TRICKY CODE
   // This if expression checks if each flag in attrib is set. If it is, the corresponding flag in the poi must
   // be set, otherwise, the test is irrelavent (and always passes as indicated by the true)

   if ( attrib == 0 )
   {
      return true; // always match if we don't care what the attributes are
   }

   // If the search attribute has referenced attributes, get the reference type
   PoiAttributeType targetReference = pgsPointOfInterest::GetReference(attrib);

   // NOTE: you can only search for POIs with referenced or non-referenced attributes. You can't search
   // for both types at the same time

   if ( targetReference == 0 )
   {
      // searching for a non-referenced attribute
      if (
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR1)           ? poi.HasAttribute(POI_CRITSECTSHEAR1)           : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CRITSECTSHEAR2)           ? poi.HasAttribute(POI_CRITSECTSHEAR2)           : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_HARPINGPOINT)             ? poi.IsHarpingPoint()                           : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CONCLOAD)                 ? poi.IsConcentratedLoad()                       : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DIAPHRAGM)                ? poi.HasAttribute(POI_DIAPHRAGM)                : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_H)                        ? poi.IsAtH()                                    : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_15H)                      ? poi.IsAt15H()                                  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DEBOND)                   ? poi.HasAttribute(POI_DEBOND)                   : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PSXFER)                   ? poi.HasAttribute(POI_PSXFER)                   : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_DECKBARCUTOFF)            ? poi.HasAttribute(POI_DECKBARCUTOFF)            : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARCUTOFF)                ? poi.HasAttribute(POI_BARCUTOFF)                : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BARDEVELOP)               ? poi.HasAttribute(POI_BARDEVELOP)               : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_RIGHTFACE)     ? poi.HasAttribute(POI_SECTCHANGE_RIGHTFACE)     : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_LEFTFACE)      ? poi.HasAttribute(POI_SECTCHANGE_LEFTFACE)      : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_SECTCHANGE_TRANSITION)    ? poi.HasAttribute(POI_SECTCHANGE_TRANSITION)    : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_FACEOFSUPPORT)            ? poi.HasAttribute(POI_FACEOFSUPPORT)            : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_PIER)        ? poi.HasAttribute(POI_INTERMEDIATE_PIER)        : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BOUNDARY_PIER)            ? poi.HasAttribute(POI_BOUNDARY_PIER)            : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_ABUTMENT)                 ? poi.HasAttribute(POI_ABUTMENT)                 : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_STIRRUP_ZONE)             ? poi.HasAttribute(POI_STIRRUP_ZONE)             : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_INTERMEDIATE_TEMPSUPPORT) ? poi.HasAttribute(POI_INTERMEDIATE_TEMPSUPPORT) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_CLOSURE)                  ? poi.HasAttribute(POI_CLOSURE)                  : false)
       )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }
   else
   {
      // searching for a referenced attribute
      if ( attrib == targetReference && poi.GetReferencedAttributes(targetReference) != 0 )
      {
         // didn't specify which referenced attribute is being searched for...
         // the subject poi as at least one referenced attribute of the target type
         // so it is a match
         return true;
      }

      if (
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_PICKPOINT) ? poi.HasAttribute(targetReference | POI_PICKPOINT) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_BUNKPOINT) ? poi.HasAttribute(targetReference | POI_BUNKPOINT) : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_0L)  ? poi.IsTenthPoint(targetReference) == 1  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_1L)  ? poi.IsTenthPoint(targetReference) == 2  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_2L)  ? poi.IsTenthPoint(targetReference) == 3  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_3L)  ? poi.IsTenthPoint(targetReference) == 4  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_4L)  ? poi.IsTenthPoint(targetReference) == 5  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_5L)  ? poi.IsTenthPoint(targetReference) == 6  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_6L)  ? poi.IsTenthPoint(targetReference) == 7  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_7L)  ? poi.IsTenthPoint(targetReference) == 8  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_8L)  ? poi.IsTenthPoint(targetReference) == 9  : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_9L)  ? poi.IsTenthPoint(targetReference) == 10 : false) ||
       (sysFlags<PoiAttributeType>::IsSet(attrib,POI_10L) ? poi.IsTenthPoint(targetReference) == 11 : false)
       )
      {
         // This poi matches the selection criteria.
         return true;
      }
   }

   return false;
}

std::vector<std::vector<pgsPointOfInterest>*> pgsPoiMgr::GetPoiContainer(const CGirderKey& girderKey)
{
   std::vector<std::vector<pgsPointOfInterest>*> vpvPoi;
   if ( girderKey.groupIndex == INVALID_INDEX || girderKey.girderIndex == INVALID_INDEX )
   {
      // want POIs for multiple groups or girders
      std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator iter = m_PoiData.begin();
      std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator end  = m_PoiData.end();
      for ( ; iter != end; iter++ )
      {
         const CGirderKey& key = iter->first;
         if ( key == girderKey )
         {
            vpvPoi.push_back(iter->second);
         }
      }
   }
   else
   {
      // want POIs for specific girder
      std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::iterator found = m_PoiData.find(girderKey);
      std::vector<pgsPointOfInterest>* pvPoi = NULL;
      if ( found == m_PoiData.end() )
      {
         pvPoi = new std::vector<pgsPointOfInterest>();
         m_PoiData.insert(std::make_pair(girderKey,pvPoi));
      }
      else
      {
         pvPoi = found->second;
      }
      ATLASSERT( pvPoi != NULL);
      vpvPoi.push_back(pvPoi);
   }

   return vpvPoi;
}

std::vector<const std::vector<pgsPointOfInterest>*> pgsPoiMgr::GetPoiContainer(const CGirderKey& girderKey) const
{
   std::vector<const std::vector<pgsPointOfInterest>*> vpvPoi;
   if ( girderKey.groupIndex == INVALID_INDEX || girderKey.girderIndex == INVALID_INDEX )
   {
      // want POIs for multiple groups or girders
      std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator iter = m_PoiData.begin();
      std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator end  = m_PoiData.end();
      for ( ; iter != end; iter++ )
      {
         const CGirderKey& key = iter->first;
         if ( girderKey == key )
         {
            vpvPoi.push_back(iter->second);
         }
      }
   }
   else
   {
      // want POIs for specific girder
      std::map<CGirderKey,std::vector<pgsPointOfInterest>*>::const_iterator found = m_PoiData.find(girderKey);
      if ( found != m_PoiData.end() )
      {
         vpvPoi.push_back(found->second);
      }
   }

   return vpvPoi;
}
