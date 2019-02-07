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

#ifndef INCLUDED_IFACE_BRIDGE_H_
#define INCLUDED_IFACE_BRIDGE_H_

#include <vector>

#include <PGSuperTypes.h>
#include <Details.h>

#include <WBFLCore.h>
#include <WBFLTools.h>
#include <WBFLGeometry.h>

#include <PgsExt\LongRebarInstance.h>
#include <IFace\PointOfInterest.h>

///////////////////////////////////////////////////////////////////////////////
// NOTES:
// Slab Overhang = Distance from exterior girder to edge of slab
// Slab Edge Offset = Distance from Alignment to edge of slab
// Slab Offset = "A" Dimension

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;
class matPsStrand;
class matRebar;
class matConcreteBase;
class rptChapter;
class CPrecastSegmentData;
class CSplicedGirderData;
class CStrandData;
class CStrandRow;

interface IRCBeam2Ex;
interface IEAFDisplayUnits;

interface IDirection;
interface IAngle;

interface IRebar;
interface IRebarSection;
interface IRebarSectionItem;
interface IRebarLayout;

class stbGirder;
class stbLiftingStabilityProblem;
class stbHaulingStabilityProblem;
interface ISegmentLiftingDesignPointsOfInterest;
interface ISegmentHaulingDesignPointsOfInterest;

// MISCELLANEOUS
//

struct IntermedateDiaphragm
{
   IntermedateDiaphragm() :
   m_bCompute(false),P(0),H(0),T(0),W(0),Location(0)
   {
   }
   bool m_bCompute; // if true, compuated based on H, T, and W, other use P
   Float64 P; // weight
   Float64 H; // height
   Float64 T; // thickness
   Float64 W; // width
   Float64 Location; // measured from left end of segment if precast or left end of span if cast-in-place
   bool operator<(const IntermedateDiaphragm& other) const { return Location < other.Location; }
};

struct SpaceBetweenGirder
{
   SpaceBetweenGirder() : firstGdrIdx(INVALID_INDEX),lastGdrIdx(INVALID_INDEX),spacing(0)
   {
   }
   GirderIndexType firstGdrIdx, lastGdrIdx;
   Float64 spacing;
};

struct BearingElevationDetails
{
   CGirderKey GirderKey;
   pgsTypes::PierFaceType PierFace;
   IndexType BearingIdx; // Can be more than one bearing for girder
   Float64 Station; // station where the elevations are computed
   Float64 Offset; // offset where the elevations are computed
   Float64 FinishedGradeElevation; // final design surface elevation at Station and Offset (top of overlay if overlay built with bridge, top of deck for no overlay or future overlay)
   Float64 OverlayDepth; // depth of overlay (future overlays not considered)
   Float64 SlabOffset;
   Float64 Hg; // adjusted for girder orientation
   Float64 BrgRecess;
   Float64 BrgHeight;
   Float64 SolePlateHeight;
   Float64 TopBrgElevation;  // elevation at top of bearing
   Float64 BrgSeatElevation; // elevation at bottom of bearing
   Float64 ProfileGrade; // profile grade at CL Bearing
   Float64 BasicGirderGrade; // basic slope of girder (straight line between supports, includes temporary support elevation adjustments)
   Float64 PrecamberSlope; // rotation due to precamber
   Float64 GirderGrade; // slope of the girder with precamber rotation applied
   Float64 GirderOrientation; // lateral tilt angle if superelevation and girder follows
   Float64 BearingDeduct; // TxDOT Specific value
};

/*****************************************************************************
INTERFACE
   IBridge

   Interface to get information about the bridge.

DESCRIPTION
   Interface to get information about the bridge.
*****************************************************************************/
// {3BB24886-677B-11d2-883A-006097C68A9C}
DEFINE_GUID(IID_IBridge, 
0x3bb24886, 0x677b, 0x11d2, 0x88, 0x3a, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IBridge : IUnknown
{
   ///////////////////////////////////////////////////
   // General Bridge Information
   ///////////////////////////////////////////////////

   // Returns true if the specifed girder implements the IAsymmetric interface
   virtual bool IsAsymmetricGirder(const CGirderKey& girderKey) const = 0;

   // Returns true if bridge contains any girders with an asymmetric cross section
   virtual bool HasAsymmetricGirders() const = 0;

   // Returns true if bridge contains any girders with asymmetric prestressing
   virtual bool HasAsymmetricPrestressing() const = 0;

   // Returns true if the bridge has girders that are not installed plumb
   virtual bool HasTiltedGirders() const = 0;

   // Returns the overall length of the bridge measured along the alignment between the first and last pier
   virtual Float64 GetLength() const = 0;

   // Returns the span length between piers measured along the alignment
   virtual Float64 GetSpanLength(SpanIndexType spanIdx) const = 0;

   // Returns the offset from the alignment to the bridge line
   virtual Float64 GetAlignmentOffset() const = 0;

   // Returns number of spans
   virtual SpanIndexType GetSpanCount() const = 0;

   // Returns number of permanent piers
   virtual PierIndexType GetPierCount() const = 0;

   // Returns number of temporary supports
   virtual SupportIndexType GetTemporarySupportCount() const = 0;

   // Returns number of girder groups
   virtual GroupIndexType GetGirderGroupCount() const = 0;

   // Returns number of girders in a girder group
   virtual GirderIndexType GetGirderCount(GroupIndexType grpIdx) const = 0;

   // Returns the number of girder lines in the bridge model
   // The number of girder lines is equal to the greatest number of
   // girders in each of the girder groups
   virtual GirderIndexType GetGirderlineCount() const = 0;

   // Returns the number of girders in a span
   virtual GirderIndexType GetGirderCountBySpan(SpanIndexType spanIdx) const = 0;

   // Returns the index of the pier at the start of a group
   virtual PierIndexType GetGirderGroupStartPier(GroupIndexType grpIdx) const = 0;

   // Returns the index of the pier at the end of a group
   virtual PierIndexType GetGirderGroupEndPier(GroupIndexType grpIdx) const = 0;

   // Returns the pier indices at the boundary of a group
   virtual void GetGirderGroupPiers(GroupIndexType grpIdx,PierIndexType* pStartPierIdx,PierIndexType* pEndPierIdx) const = 0;

   // Returns the index of the span at the start of a group
   virtual SpanIndexType GetGirderGroupStartSpan(GroupIndexType grpIdx) const = 0;

   // Returns the index of the span at the end of a group
   virtual SpanIndexType GetGirderGroupEndSpan(GroupIndexType grpIdx) const = 0;

   // Returns the spans indices at the boundary of a group
   virtual void GetGirderGroupSpans(GroupIndexType grpIdx,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) const = 0;

   // Returns the girder group index that a span is part of
   virtual GroupIndexType GetGirderGroupIndex(SpanIndexType spanIdx) const = 0;

   // Returns the indices of the girder groups on either side of a pier (could be the same girder group)
   virtual void GetGirderGroupIndex(PierIndexType pierIdx,GroupIndexType* pBackGroupIdx,GroupIndexType* pAheadGroupIdx) const = 0;

   // Returns the span length for the specified girder in the specified span
   // measured along the CL girder. The span length is measured between the CL Piers except at
   // group boundaries where a hinge or roller boundary condition is used, in which case the
   // span length is measured from/to the CL-Brg.
   virtual Float64 GetSpanLength(SpanIndexType spanIdx,GirderIndexType gdrIdx) const = 0;
   virtual Float64 GetSpanLength(const CSpanKey& spanKey) const = 0;

   // Returns the span length for the specified girder in the specified span
   // measured along the CL girder. The span length is measured between the CL Piers except
   // at the first and last pier in the bridge, in which case the span length is measured
   // from/to the CL-Bearing.
   virtual Float64 GetFullSpanLength(const CSpanKey& spanKey) const = 0;

   // returns the length of a girderline
   virtual Float64 GetGirderlineLength(GirderIndexType gdrLineIdx) const = 0;

   // returns the layout length of a girder
   virtual Float64 GetGirderLayoutLength(const CGirderKey& girderKey) const = 0;

   // returns the length of the girder measured along the centerline of its segments between the CL-Bearing
   // at the end piers
   virtual Float64 GetGirderSpanLength(const CGirderKey& girderKey) const = 0;

   // returns the end to end length of the girder measured along the centerline of its segments
   virtual Float64 GetGirderLength(const CGirderKey& girderKey) const = 0;

   // returns the cantilever length of a girder. If the girder end is not cantilevered, returns 0.
   // The cantilever length applies to spans where one of the end piers is designated as a cantilever.
   // This is NOT the member end distance!
   virtual Float64 GetCantileverLength(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType endType) const = 0;
   virtual Float64 GetCantileverLength(const CSpanKey& spanKey,pgsTypes::MemberEndType endType) const = 0;

   ///////////////////////////////////////////////////
   // Segment geometry
   ///////////////////////////////////////////////////

   // Returns a vector of segment index/length pairs for the length of each segment in a span for a given girder.
   // The first item in the pair is the segment index and the second item is the length of the segment
   // within the given span. Segment lengths are measured between CL-Piers and CL-Temporary Supports except for the
   // first segment in the first group and and last segment in the last group where the start and end of the segments 
   // are measured from the CL-Bearing.
   virtual std::vector<std::pair<SegmentIndexType,Float64>> GetSegmentLengths(const CSpanKey& spanKey) const = 0;

   // Returns the number of segments in a girder
   virtual SegmentIndexType GetSegmentCount(const CGirderKey& girderKey) const = 0;
   virtual SegmentIndexType GetSegmentCount(GroupIndexType grpIdx,GirderIndexType gdrIdx) const = 0;

   // End-to-end length of segment
   virtual Float64 GetSegmentLength(const CSegmentKey& segmentKey) const = 0;

   // CL Brg to CL Brg span length (measured between CL Brg of temporary supports if used)
   virtual Float64 GetSegmentSpanLength(const CSegmentKey& segmentKey) const = 0;

   // CL Pier - CL Pier length of the segment centerline
   virtual Float64 GetSegmentLayoutLength(const CSegmentKey& segmentKey) const = 0;

   // Returns the layout length of the segment, except for the first and last
   // segment in a group. For the first and last segment of the group,
   // the length is measured to the start/end face of the segment
   virtual Float64 GetSegmentFramingLength(const CSegmentKey& segmentKey) const = 0;

   // Distance from end of girder to C.L. bearing - along girder
   virtual Float64 GetSegmentStartEndDistance(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentEndEndDistance(const CSegmentKey& segmentKey) const = 0;
   
   // Distance from C.L. pier to C.L. brg - along girder
   virtual Float64 GetSegmentStartBearingOffset(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentEndBearingOffset(const CSegmentKey& segmentKey) const = 0;

   virtual Float64 GetSegmentStartSupportWidth(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentEndSupportWidth(const CSegmentKey& segmentKey) const = 0;

   // End-to-end length of segment, measured along the grade 
   virtual Float64 GetSegmentPlanLength(const CSegmentKey& segmentKey) const = 0;

   // Segment cantilevers are to be modeled if the cantilever length is
   // at least the depth of the non-composite member. This method
   // provides a uniform means of determining if cantilevers are to be modeled
   virtual void ModelCantilevers(const CSegmentKey& segmentKey,bool* pbStartCantilever,bool* pbEndCantilever) const = 0;

   // Grade of segment
   virtual Float64 GetSegmentSlope(const CSegmentKey& segmentKey) const = 0;

   // Slab Offset
   virtual Float64 GetSlabOffset(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const = 0;
   virtual void GetSlabOffset(const CSegmentKey& segmentKey, Float64* pStart, Float64* pEnd) const = 0;

   // Adjustments from temporary support elevation adjustments
   virtual Float64 GetElevationAdjustment(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetRotationAdjustment(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;

   // Distnace from CLPier to CL Bearing, measured along CL of segment
   virtual Float64 GetCLPierToCLBearingDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) const = 0;
   virtual Float64 GetCLPierToSegmentEndDistance(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::MeasurementType measure) const = 0;

   // Returns the direction of a segment
   virtual void GetSegmentBearing(const CSegmentKey& segmentKey, IDirection** ppBearing) const = 0;

   // Returns the direction of a segment normal
   virtual void GetSegmentNormal(const CSegmentKey& segmentKey, IDirection** ppNormal) const = 0;

   // Returns the angle between a segment and the CL of its supporting element (pier or temporary support)
   virtual void GetSegmentAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) const = 0;

   // Returns the angle between the plane of the end face of a segment and its centerline
   virtual void GetSegmentSkewAngle(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,IAngle** ppAngle) const = 0;

   // Returns the key for the segment that crosses the pier
   virtual CSegmentKey GetSegmentAtPier(PierIndexType pierIdx,const CGirderKey& girderKey) const = 0;

   // Gets the segments on either side of the specified pier. If there isn't a segment, the segment index part of the segment key will be INVALID_INDEX
   virtual void GetSegmentsAtPier(PierIndexType pierIdx, GirderIndexType gdrIdx, CSegmentKey* pBackSegmentKey, CSegmentKey* pAheadSegmentKey) const = 0;

   // Gets the span indices for the spans the segment starts and ends in
   virtual void GetSpansForSegment(const CSegmentKey& segmentKey,SpanIndexType* pStartSpanIdx,SpanIndexType* pEndSpanIdx) const = 0;

   // Returns normal distance from the alignment, at the given station, to the CL-Segment line (extended) 
   virtual Float64 GetSegmentOffset(const CSegmentKey& segmentKey,Float64 station) const = 0;

   // Returns a point on a segment, located at Xpoi from the start of the segment
   virtual void GetPoint(const CSegmentKey& segmentKey,Float64 Xpoi,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const = 0;

   // Returns a point on a segment at a point of interest
   virtual void GetPoint(const pgsPointOfInterest& poi,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const = 0;

   // Returns station and offset for a point on a segment
   virtual void GetStationAndOffset(const CSegmentKey& segmentKey,Float64 Xpoi,Float64* pStation,Float64* pOffset) const = 0;

   // Returns station and offset for a point on a segment
   virtual void GetStationAndOffset(const pgsPointOfInterest& poi,Float64* pStation,Float64* pOffset) const = 0;

   // Computes the intersection point of a segment and a pier. Returns true if the intersection is found
   virtual bool GetSegmentPierIntersection(const CSegmentKey& segmentKey,PierIndexType pierIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const = 0;

   // Computes the intersection point of a segment and a temporary support. Returns true if the intersection is found
   virtual bool GetSegmentTempSupportIntersection(const CSegmentKey& segmentKey,SupportIndexType tsIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppPoint) const = 0;

   // Returns true if the girder is in an interior girder
   virtual bool IsInteriorGirder(const CGirderKey& girderKey) const = 0;

   // Returns true if the girder is in an exterior girder
   virtual bool IsExteriorGirder(const CGirderKey& girderKey) const = 0;

   // Returns true if the girder is in the left exterior girder
   virtual bool IsLeftExteriorGirder(const CGirderKey& girderKey) const = 0;

   // Returns true if the girder is in the right exterior girder
   virtual bool IsRightExteriorGirder(const CGirderKey& girderKey) const = 0;

   // Returns true if the girder is in an obtuse corner of the bridge, in plan view.
   // Determination of obtuse corner is for computing the shear correction factor for skew.
   // Only the exterior and first interior girders can be in an obtuse corner.
   virtual bool IsObtuseCorner(const CSpanKey& spanKey,pgsTypes::MemberEndType endType) const = 0;

   // Returns true if the girder segment has a roughened surface
   virtual bool AreGirderTopFlangesRoughened(const CSegmentKey& segmentKey) const = 0;

   // Gets the span index for a given staiton. Returns false is the station is before or after the bridge
   virtual bool GetSpan(Float64 station,SpanIndexType* pSpanIdx) const = 0;

   // clear distance between girders. If poi is on an exterior girder, the left/right parameter will
   // be zero
   virtual void GetDistanceBetweenGirders(const pgsPointOfInterest& poi,Float64 *pLeft,Float64* pRight) const = 0;

   // clear distance between the bottom flanges of the girder. If poi is on an exterior girder, the
   // left/right parameter will be < 0 (meaning not applicable)
   virtual void GetBottomFlangeClearance(const pgsPointOfInterest& poi,Float64* pLeft,Float64* pRight) const = 0;

   // returns the spacing between girders. adjacent spaces that are the same are grouped together
   // the returned vector is empty if the spacings could not be determined (e.g. station is off the bridge)
   virtual std::vector<SpaceBetweenGirder> GetGirderSpacing(Float64 station) const = 0;

   virtual std::vector<Float64> GetGirderSpacing(SpanIndexType spanIdx,Float64 Xspan) const = 0;

   // returns girder spacing at a pier. The vector will contain nGirders-1 spaces
   virtual std::vector<Float64> GetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace, pgsTypes::MeasurementLocation measureLocation, pgsTypes::MeasurementType measureType) const = 0;

   // Returns the offset from the offset measure datum to the specified girder, measured along the CL pier
   virtual Float64 GetGirderOffset(GirderIndexType gdrIdx,PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::OffsetMeasurementType offsetMeasureDatum) const = 0;

   // Returns the left and right girder spacing for a point along a girder. If the girder is an exterior girder
   // the slab overhang is returned on the exterior side of the girder. The spacing is measured normal to the girder
   virtual void GetSpacingAlongGirder(const CGirderKey& girderKey,Float64 Xg,Float64* leftSpacing,Float64* rightSpacing) const = 0;
   virtual void GetSpacingAlongGirder(const pgsPointOfInterest& poi,Float64* leftSpacing,Float64* rightSpacing) const = 0;

   // Returns the configuration data for a segment (material properties, strands, etc)
   virtual GDRCONFIG GetSegmentConfiguration(const CSegmentKey& segmentKey) const = 0;

   virtual void ResolveSegmentVariation(const CPrecastSegmentData* pSegment, std::array<Float64, 4>& Xhp) const = 0;

   ///////////////////////////////////////////////////
   // Closure Joints
   ///////////////////////////////////////////////////

   // Returns the length of the specified closure joint
   virtual Float64 GetClosureJointLength(const CClosureKey& closureKey) const = 0;

   // Returns the left and right size of the closure joint.
   virtual void GetClosureJointSize(const CClosureKey& closureKey,Float64* pLeft,Float64* pRight) const = 0;

   // Returns the angle between segments that are joined at this closure joint
   virtual void GetAngleBetweenSegments(const CClosureKey& closureKey,IAngle** ppAngle) const = 0;

   ///////////////////////////////////////////////////
   // Diaphragms
   ///////////////////////////////////////////////////

   virtual void GetPierDiaphragmSize(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,Float64* pW,Float64* pH) const = 0;
   // return true if weight of diaphragm is carried by girder
   virtual bool DoesPierDiaphragmLoadGirder(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const = 0;
   // Get location of end diaphragm load (c.g.) measured from c.l. pier. along girder
   // Only applicable if DoesPierDiaphragmLoadGirder returns true
   virtual Float64 GetPierDiaphragmLoadLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType) const = 0;
   
   // Returns a vector of intermediate diaphragm loads for diaphragms that are precast with the
   // girder.
   virtual std::vector<IntermedateDiaphragm> GetPrecastDiaphragms(const CSegmentKey& segmentKey) const = 0;

   // Returns a vector of interemdiate diaphragm loads for diaphragms that are cast at the bridge site.
   virtual std::vector<IntermedateDiaphragm> GetCastInPlaceDiaphragms(const CSpanKey& spanKey) const = 0;

   ///////////////////////////////////////////////////
   // Slab data
   ///////////////////////////////////////////////////

   virtual pgsTypes::SupportedDeckType GetDeckType() const = 0;
   virtual pgsTypes::WearingSurfaceType GetWearingSurfaceType() const = 0;
   virtual bool IsCompositeDeck() const = 0;
   virtual bool HasOverlay() const = 0;
   virtual bool IsFutureOverlay() const = 0;
   virtual Float64 GetOverlayWeight() const = 0;
   virtual Float64 GetOverlayDepth() const = 0;
   virtual Float64 GetSacrificalDepth() const = 0;
   virtual Float64 GetFillet() const = 0;
   virtual Float64 GetAssumedExcessCamber(SpanIndexType spanIdx,GirderIndexType gdr) const = 0;
   virtual Float64 GetGrossSlabDepth(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetStructuralSlabDepth(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetCastSlabDepth(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetPanelDepth(const pgsPointOfInterest& poi) const = 0;

   // Returns distance from the left exterior girder to the edge of slab, measured normal to the alignment
   // Xb is measured in Bridge Line Coordinates and can be easily determined by station
   // at the section where the overhang is desired and the station of pier 0
   virtual Float64 GetLeftSlabOverhang(Float64 Xb) const = 0;
   // distFromStartOfSpan is measured along the alignment and can be easily determined by station
   // at the section where the overhang is desired and the station of the pier at the start of the span
   virtual Float64 GetLeftSlabOverhang(SpanIndexType spanIdx,Float64 distFromStartOfSpan) const = 0;

   // Returns distance from the right exterior girder to the edge of slab, measured normal to the alignment
   // Xb is measured in Bridge Line Coordinates and can be easily determined by station
   // at the section where the overhang is desired and the station of pier 0
   virtual Float64 GetRightSlabOverhang(Float64 Xb) const = 0;
   // distFromStartOfSpan is measured along the alignment and can be easily determined by station
   // at the section where the overhang is desired and the station of the pier at the start of the span
   virtual Float64 GetRightSlabOverhang(SpanIndexType spanIdx,Float64 distFromStartOfSpan) const = 0;

   // Returns distance from the alignment to the left slab edge, measured normal to the alignment
   // Xb is measured in Bridge Line Coordinates and can be easily determined by station
   // at the section where the end offset is desired and the station of pier 0
   virtual Float64 GetLeftSlabEdgeOffset(Float64 Xb) const = 0;
   // returns the edge offset at the location where the CL Pier intserects the alignment
   virtual Float64 GetLeftSlabEdgeOffset(PierIndexType pierIdx) const = 0;

   // Returns distance from the alignment to the right slab edge, measured normal to the alignment
   // Xb is measured in Bridge Line Coordinates and can be easily determined by station
   // at the section where the end offset is desired and the station of pier 0
   virtual Float64 GetRightSlabEdgeOffset(Float64 Xb) const = 0;
   // returns the edge offset at the location where the CL Pier intserects the alignment
   virtual Float64 GetRightSlabEdgeOffset(PierIndexType pierIdx) const = 0;

   // Returns the curb-to-curb width of the deck measured normal to the alignment along a line
   // passing through the POI
   virtual Float64 GetCurbToCurbWidth(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetCurbToCurbWidth(const CSegmentKey& segmentKey,Float64 distFromStartOfSpan) const = 0;
   // Returns the curb-to-curb width of the deck measured at Xb.
   // Xb is in Bridge Line Coordinates can be easily determined by station at the section where the end offset is
   // desired and the station of pier 0
   virtual Float64 GetCurbToCurbWidth(Float64 Xb) const = 0;
   // Returns the offset from the alignment to the left curb line measured at Xb.
   // Xb is measured in Bridge Line Coordinates and can be easily determined by station at the section where the end offset is
   // desired and the station of pier 0
   virtual Float64 GetLeftCurbOffset(Float64 Xb) const = 0;
   // Returns the offset from the alignment to the right curb line measured at Xb.
   // Xb is measured in Bridge Line Coordinates and can be easily determined by station at the section where the end offset is
   // desired and the station of pier 0
   virtual Float64 GetRightCurbOffset(Float64 Xb) const = 0;
   // Returns the offset from the alignment to the left curb line measured at distFromStartOfSpan along the alignment.
   // distFromStartOfSpan can be easily determined by station at the section where the offset is
   // desired and the station of the pier at the start of the span.
   virtual Float64 GetLeftCurbOffset(SpanIndexType spanIdx,Float64 distFromStartOfSpan) const = 0;
   // Returns the offset from the alignment to the right curb line measured at distFromStartOfSpan along the alignment.
   // distFromStartOfSpan can be easily determined by station at the section where the offset is
   // desired and the station of the pier at the start of the span.
   virtual Float64 GetRightCurbOffset(SpanIndexType spanIdx,Float64 distFromStartOfSpan) const = 0;
   // Returns the offset from the alignment to the left curb line along a line normal to the alignment
   // passing through the point where the CL pier line intersects the alignment
   virtual Float64 GetLeftCurbOffset(PierIndexType pierIdx) const = 0;
   // Returns the offset from the alignment to the right curb line along a line normal to the alignment
   // passing through the point where the CL pier line intersects the alignment
   virtual Float64 GetRightCurbOffset(PierIndexType pierIdx) const = 0;

   // Offset distances to curbline of interior barrier or sidewalk curb if present
   // Xb is in Bridge Line Coordinates
   virtual Float64 GetLeftInteriorCurbOffset(Float64 Xb) const = 0;
   virtual Float64 GetRightInteriorCurbOffset(Float64 Xb) const = 0;
   virtual Float64 GetLeftInteriorCurbOffset(PierIndexType pierIdx) const = 0;
   virtual Float64 GetRightInteriorCurbOffset(PierIndexType pierIdx) const = 0;
   virtual Float64 GetInteriorCurbToCurbWidth(Float64 Xb) const = 0;
   
   // this are the locations that the overlay butts up to
   virtual Float64 GetLeftOverlayToeOffset(Float64 Xb) const = 0;
   virtual Float64 GetRightOverlayToeOffset(Float64 Xb) const = 0;
   virtual Float64 GetLeftOverlayToeOffset(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetRightOverlayToeOffset(const pgsPointOfInterest& poi) const = 0;

   virtual void GetSlabPerimeter(CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) const = 0;
   virtual void GetSlabPerimeter(SpanIndexType startSpanIdx,SpanIndexType endSpanIdx,CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) const = 0;
   virtual void GetSpanPerimeter(SpanIndexType spanIdx,CollectionIndexType nPoints,pgsTypes::PlanCoordinateType pcType,IPoint2dCollection** points) const = 0;

   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const = 0;
   virtual void GetLeftSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const = 0;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const = 0;
   virtual void GetRightSlabEdgePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const = 0;

   virtual void GetLeftCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const = 0;
   virtual void GetLeftCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const = 0;
   virtual void GetRightCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint2d** point) const = 0;
   virtual void GetRightCurbLinePoint(Float64 station, IDirection* direction,pgsTypes::PlanCoordinateType pcType,IPoint3d** point) const = 0;

   // Distance from top of slab to top of girder - Does not account for camber
   virtual Float64 GetTopSlabToTopGirderChordDistance(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetTopSlabToTopGirderChordDistance(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) const = 0;

   ///////////////////////////////////////////////////
   // Pier data
   ///////////////////////////////////////////////////

   virtual Float64 GetPierStation(PierIndexType pierIdx) const = 0;
   virtual Float64 GetBearingStation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace) const = 0;
   virtual void GetBearingPoint(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,const CGirderKey& girderKey,Float64* pStation,Float64* pOffset) const = 0;
   virtual void GetPierDirection(PierIndexType pierIdx,IDirection** ppDirection) const = 0;
   virtual void GetPierSkew(PierIndexType pierIdx,IAngle** ppAngle) const = 0;
   virtual void GetPierPoints(PierIndexType pierIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** left,IPoint2d** alignment,IPoint2d** bridge,IPoint2d** right) const = 0;
   virtual void IsContinuousAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) const = 0;
   virtual void IsIntegralAtPier(PierIndexType pierIdx,bool* pbLeft,bool* pbRight) const = 0;

   // Returns the connection boundary condition at a pier (only valid if IsBoundaryPier returns true)
   virtual pgsTypes::BoundaryConditionType GetBoundaryConditionType(PierIndexType pierIdx) const = 0;

   // Returns the segment connection type at a pier (only valid if IsInteriorPier returns true)
   virtual pgsTypes::PierSegmentConnectionType GetPierSegmentConnectionType(PierIndexType pierIdx) const = 0;

   virtual bool IsAbutment(PierIndexType pierIdx) const = 0; // returns true if pier is an end abutment
   virtual bool IsPier(PierIndexType pierIdx) const = 0; // returns true if pier is an intermediate pier
   virtual bool HasCantilever(PierIndexType pierIdx) const = 0; // returns true if the span is cantilevered at this pier

   virtual bool IsInteriorPier(PierIndexType pierIdx) const = 0; // returns true if the pier is interior to a girder group
   virtual bool IsBoundaryPier(PierIndexType pierIdx) const = 0; // returns true if the pier is at a boundary of a girder group

   // Computes the location of the pier in the segment coordinate system. Returns false if the pier is not located on the segment
   virtual bool GetPierLocation(PierIndexType pierIdx,const CSegmentKey& segmentKey,Float64* pXs) const = 0;

   // Computes the location of the CL Pier in girder path coordinates. Returns false if the pier is not located in the group
   virtual bool GetPierLocation(const CGirderKey& girderKey,PierIndexType pierIdx,Float64* pXgp) const = 0;

   // returns the skew angle of a line define defined by the orientation string at a given station
   // this is usefuly for determing the skew angle of piers that aren't in the bridge model yet
   // returns false if there is an error in the strOrientation string
   virtual bool GetSkewAngle(Float64 station,LPCTSTR strOrientation,Float64* pSkew) const = 0;

   // returns the type of model used to define a pier
   virtual pgsTypes::PierModelType GetPierModelType(PierIndexType pierIdx) const = 0;

   // returns the number of columns at a pier.
   virtual ColumnIndexType GetColumnCount(PierIndexType pierIdx) const = 0;

   // returns the basic properties for a single column at a pier... all columns at a pier are assumed
   // to be the same. if bSkewAdjust is true, the column properties are adjusted for skew and the
   // moment of inertia about an axis normal to the alignment is returned
   virtual void GetColumnProperties(PierIndexType pierIdx,ColumnIndexType colIdx,bool bSkewAdjust,Float64* pHeight,Float64* pA,Float64* pI) const = 0;

   // negative moment calculations and results need not be processed if a simple span analysis is
   // used or if there isn't any continuity.
   // this method returns true when negative moments should be processed for a given span.
   // Use ALL_SPANS for to evaluate all spans
   virtual bool ProcessNegativeMoments(SpanIndexType spanIdx) const = 0;

   // returns the location of a temporary support within a span, measured along a girder line
   virtual void GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx,SpanIndexType* pSpanIdx,Float64* pXspan) const = 0;

   // computes the distance from the start of the segment to the temporary support. returns false if the temporary support is not located on the segment
   virtual bool GetTemporarySupportLocation(SupportIndexType tsIdx,const CSegmentKey& segmentKey,Float64* pXs) const = 0;

   // returns the location of the temporary support measured along the CL girder, measured from Girder Path Coordinates
   virtual Float64 GetTemporarySupportLocation(SupportIndexType tsIdx,GirderIndexType gdrIdx) const = 0;

   // returns the temporary support type (StrongBack or ErectionTower)
   virtual pgsTypes::TemporarySupportType GetTemporarySupportType(SupportIndexType tsIdx) const = 0;

   // returns the segment connection type at the temporary support
   virtual pgsTypes::TempSupportSegmentConnectionType GetSegmentConnectionTypeAtTemporarySupport(SupportIndexType tsIdx) const = 0;

   // gets the segment keys for the segments framing into the left and right side of a temporary support
   virtual void GetSegmentsAtTemporarySupport(GirderIndexType gdrIdx,SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey) const = 0;

   virtual void GetTemporarySupportDirection(SupportIndexType tsIdx,IDirection** ppDirection) const = 0;

   // returns true if any of the temporary supports have non-zero elevation adjustments
   virtual bool HasTemporarySupportElevationAdjustments() const = 0;

   // Compute bearing elevation data for each girder along bearing line
   virtual std::vector<BearingElevationDetails> GetBearingElevationDetails(PierIndexType pierIdx,pgsTypes::PierFaceType face) const = 0;
};

/*****************************************************************************
INTERFACE
   IMaterials

   Interface to get information about the bridge materials.

DESCRIPTION
   Interface to get information about the bridge materials.
*****************************************************************************/
// {B6904E95-0758-4fe3-A213-BFC0F3203F11}
DEFINE_GUID(IID_IMaterials, 
0xb6904e95, 0x758, 0x4fe3, 0xa2, 0x13, 0xbf, 0xc0, 0xf3, 0x20, 0x3f, 0x11);
interface IMaterials : IUnknown
{
   // Returns the concrete strength at an age of 28 days
   virtual Float64 GetSegmentFc28(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetClosureJointFc28(const CSegmentKey& closureKey) const = 0;
   virtual Float64 GetDeckFc28() const = 0;
   virtual Float64 GetRailingSystemFc28(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetPierFc28(IndexType pierIdx) const = 0;
   virtual Float64 GetLongitudinalJointFc28() const = 0;

   // Returns the secant modulus at an age of 28 days
   virtual Float64 GetSegmentEc28(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetClosureJointEc28(const CSegmentKey& closureKey) const = 0;
   virtual Float64 GetDeckEc28() const = 0;
   virtual Float64 GetRailingSystemEc28(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetPierEc28(IndexType pierIdx) const = 0;
   virtual Float64 GetLongitudinalJointEc28() const = 0;

   // Returns the weight density of the material at the specified interval. If the component has not
   // been added to the system yet, this method returns 0
   virtual Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetClosureJointWeightDensity(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetDeckWeightDensity(IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetRailingSystemWeightDensity(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetLongitudinalJointWeightDensity(IntervalIndexType intervalIdx) const = 0;

   // Returns the age of concrete. Returns 0 if the concrete has not
   // be cast or hasn't attained sufficient strength prior to this interval.
   virtual Float64 GetSegmentConcreteAge(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetClosureJointConcreteAge(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetDeckConcreteAge(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetRailingSystemAge(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetLongitudinalJointConcreteAge(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType) const = 0;

   // Returns the concrete strength at the middle of an interval
   virtual Float64 GetSegmentFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetClosureJointFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetDeckFc(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetLongitudinalJointFc(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const = 0;

   // Returns the design concrete strength at the middle of an interval
   // This differs from the concrete strength methods above in that it takes into account
   // the LimitStateConcreteStrength setting from the project criteria.
   virtual Float64 GetSegmentDesignFc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetClosureJointDesignFc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetDeckDesignFc(IntervalIndexType intervalIdx) const = 0;

   // Returns the secant modulus at the middle of an interval
   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetDeckEc(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetLongitudinalJointEc(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const = 0;

   virtual Float64 GetSegmentEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) const = 0;
   virtual Float64 GetClosureJointEc(const CClosureKey& closureKey,IntervalIndexType intervalIdx,Float64 trialFc,bool* pbChanged) const = 0;

   // Returns the concrete density modification factor (LRFD2016 5.4.2.8)
   virtual Float64 GetSegmentLambda(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetClosureJointLambda(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetDeckLambda() const = 0;
   virtual Float64 GetRailingSystemLambda(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetLongitudinalJointLambda() const = 0;

   // Returns the modulus of rupture at the middle of an interval
   virtual Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetClosureJointFlexureFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetClosureJointShearFr(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetDeckFlexureFr(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetDeckShearFr(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType=pgsTypes::Middle) const = 0;
   virtual Float64 GetLongitudinalJointFlexureFr(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const = 0;
   virtual Float64 GetLongitudinalJointShearFr(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType = pgsTypes::Middle) const = 0;

   // Returns the aging coefficient during an interval
   virtual Float64 GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetClosureJointAgingCoefficient(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetDeckAgingCoefficient(IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetLongitudinalJointAgingCoefficient(IntervalIndexType intervalIdx) const = 0;

   // Returns the age adjusted modulus from the middle to the end of an interval
   virtual Float64 GetSegmentAgeAdjustedEc(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetClosureJointAgeAdjustedEc(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetDeckAgeAdjustedEc(IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetRailingSystemAgeAdjustedEc(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetLongitudinalJointAgeAdjustedEc(IntervalIndexType intervalIdx) const = 0;

   // Returns the total free shrinkage at the relative time within the specified interval
   virtual Float64 GetTotalSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const = 0;
   virtual Float64 GetTotalClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const = 0;
   virtual Float64 GetTotalDeckFreeShrinkageStrain(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const = 0;
   virtual Float64 GetTotalRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetTotalLongitudinalJointFreeShrinkageStrain(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time) const = 0;

   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const = 0;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalClosureJointFreeShrinkageStrainDetails(const CSegmentKey& closureKey,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const = 0;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalDeckFreeShrinkageStrainDetails(IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time) const = 0;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalRailingSystemFreeShrinakgeStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual std::shared_ptr<matConcreteBaseShrinkageDetails> GetTotalLongitudinalJointFreeShrinkageStrainDetails(IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time) const = 0;

   // Returns the free shrinkage occuring within the specified interval
   virtual Float64 GetIncrementalSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetIncrementalClosureJointFreeShrinkageStrain(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetIncrementalDeckFreeShrinkageStrain(IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetIncrementalRailingSystemFreeShrinakgeStrain(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const = 0;
   virtual Float64 GetIncrementalLongitudinalJointFreeShrinkageStrain(IntervalIndexType intervalIdx) const = 0;

   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const = 0;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalClosureJointFreeShrinkageStrainDetails(const CSegmentKey& closureKey,IntervalIndexType intervalIdx) const = 0;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalDeckFreeShrinkageStrainDetails(IntervalIndexType intervalIdx) const = 0;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalRailingSystemFreeShrinakgeStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType intervalIdx) const = 0;
   virtual INCREMENTALSHRINKAGEDETAILS GetIncrementalLongitudinalJointFreeShrinkageStrainDetails(IntervalIndexType intervalIdx) const = 0;

   // Returns the creep coefficient at the specified time (timeType) in interval (intervalIdx) for a loading
   // occuring at time (loadingTimeType) in interval (loadingIntervalIdx). 
   virtual Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetClosureJointCreepCoefficient(const CSegmentKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetDeckCreepCoefficient(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual Float64 GetLongitudinalJointCreepCoefficient(IntervalIndexType loadingIntervalIdx, pgsTypes::IntervalTimeType loadingTimeType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType) const = 0;

   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetSegmentCreepCoefficientDetails(const CSegmentKey& segmentKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetClosureJointCreepCoefficientDetails(const CSegmentKey& closureKey,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetDeckCreepCoefficientDetails(IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetRailingSystemCreepCoefficientDetails(pgsTypes::TrafficBarrierOrientation orientation,IntervalIndexType loadingIntervalIdx,pgsTypes::IntervalTimeType loadingTimeType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType timeType) const = 0;
   virtual std::shared_ptr<matConcreteBaseCreepDetails> GetLongitudinalJointCreepCoefficientDetails(IntervalIndexType loadingIntervalIdx, pgsTypes::IntervalTimeType loadingTimeType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType timeType) const = 0;

   // Segment Concrete
   virtual pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey) const = 0;
   virtual bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentEccK1(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentEccK2(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey) const = 0;
   virtual const matConcreteBase* GetSegmentConcrete(const CSegmentKey& segmentKey) const = 0;

   // Closure Joint Concrete
   virtual pgsTypes::ConcreteType GetClosureJointConcreteType(const CClosureKey& closureKey) const = 0;
   virtual bool DoesClosureJointConcreteHaveAggSplittingStrength(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointConcreteAggSplittingStrength(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointStrengthDensity(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointMaxAggrSize(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointEccK1(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointEccK2(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointCreepK1(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointCreepK2(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointShrinkageK1(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointShrinkageK2(const CClosureKey& closureKey) const = 0;
   virtual const matConcreteBase* GetClosureJointConcrete(const CClosureKey& closureKey) const = 0;

   // Deck Concrete
   virtual pgsTypes::ConcreteType GetDeckConcreteType() const = 0;
   virtual bool DoesDeckConcreteHaveAggSplittingStrength() const = 0;
   virtual Float64 GetDeckConcreteAggSplittingStrength() const = 0;
   virtual Float64 GetDeckMaxAggrSize() const = 0;
   virtual Float64 GetDeckStrengthDensity() const = 0;
   virtual Float64 GetDeckEccK1() const = 0;
   virtual Float64 GetDeckEccK2() const = 0;
   virtual Float64 GetDeckCreepK1() const = 0;
   virtual Float64 GetDeckCreepK2() const = 0;
   virtual Float64 GetDeckShrinkageK1() const = 0;
   virtual Float64 GetDeckShrinkageK2() const = 0;
   virtual const matConcreteBase* GetDeckConcrete() const = 0;

   // Longitudinal Joint
   virtual pgsTypes::ConcreteType GetLongitudinalJointConcreteType() const = 0;
   virtual bool DoesLongitudinalJointConcreteHaveAggSplittingStrength() const = 0;
   virtual Float64 GetLongitudinalJointConcreteAggSplittingStrength() const = 0;
   virtual Float64 GetLongitudinalJointStrengthDensity() const = 0;
   virtual Float64 GetLongitudinalJointMaxAggrSize() const = 0;
   virtual Float64 GetLongitudinalJointEccK1() const = 0;
   virtual Float64 GetLongitudinalJointEccK2() const = 0;
   virtual Float64 GetLongitudinalJointCreepK1() const = 0;
   virtual Float64 GetLongitudinalJointCreepK2() const = 0;
   virtual Float64 GetLongitudinalJointShrinkageK1() const = 0;
   virtual Float64 GetLongitudinalJointShrinkageK2() const = 0;
   virtual const matConcreteBase* GetLongitudinalJointConcrete() const = 0;


   // Prestressing Strand
   virtual const matPsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0;
   virtual Float64 GetIncrementalStrandRelaxation(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType) const = 0;
   virtual INCREMENTALRELAXATIONDETAILS GetIncrementalStrandRelaxationDetails(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 fpso,pgsTypes::StrandType strandType) const = 0;

   // PT Tendon
   virtual const matPsStrand* GetTendonMaterial(const CGirderKey& girderKey) const = 0;
   virtual Float64 GetIncrementalTendonRelaxation(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso) const = 0;
   virtual INCREMENTALRELAXATIONDETAILS GetIncrementalTendonRelaxationDetails(const CGirderKey& girderKey,DuctIndexType ductIdx,IntervalIndexType intervalIdx,Float64 fpso) const = 0;

   // Properties of Precast Segment Longitudinal Rebar
   virtual void GetSegmentLongitudinalRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) const = 0;
   virtual std::_tstring GetSegmentLongitudinalRebarName(const CSegmentKey& segmentKey) const = 0;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade) const = 0;

   virtual void GetClosureJointLongitudinalRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) const = 0;
   virtual std::_tstring GetClosureJointLongitudinalRebarName(const CClosureKey& closureKey) const = 0;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade) const = 0;

   // Properties of precast Segment Transverse Rebar
   virtual void GetSegmentTransverseRebarProperties(const CSegmentKey& segmentKey,Float64* pE,Float64 *pFy,Float64* pFu) const = 0;
   virtual std::_tstring GetSegmentTransverseRebarName(const CSegmentKey& segmentKey) const = 0;
   virtual void GetSegmentTransverseRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type* pType,matRebar::Grade* pGrade) const = 0;

   virtual void GetClosureJointTransverseRebarProperties(const CClosureKey& closureKey,Float64* pE,Float64 *pFy,Float64* pFu) const = 0;
   virtual std::_tstring GetClosureJointTransverseRebarName(const CClosureKey& closureKey) const = 0;
   virtual void GetClosureJointTransverseRebarMaterial(const CClosureKey& closureKey,matRebar::Type* pType,matRebar::Grade* pGrade) const = 0;

   // Rebar properties for deck
   virtual void GetDeckRebarProperties(Float64* pE,Float64 *pFy,Float64* pFu) const = 0;
   virtual std::_tstring GetDeckRebarName() const = 0;
   virtual void GetDeckRebarMaterial(matRebar::Type* pType,matRebar::Grade* pGrade) const = 0;

   // Density limits for normal and light weight concrete
   virtual Float64 GetNWCDensityLimit() const = 0; // returns the minimum density for normal weight concrete
   virtual Float64 GetLWCDensityLimit() const = 0; // returns the maximum density for lightweight concrete

   // Material Properties Calcluations
   virtual Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) const = 0;
   virtual Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) const = 0;

   virtual Float64 GetSegmentFlexureFrCoefficient(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentShearFrCoefficient(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetClosureJointFlexureFrCoefficient(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointShearFrCoefficient(const CClosureKey& closureKey) const = 0;

   virtual Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2) const = 0;
};

/*****************************************************************************
INTERFACE
   ILongRebarGeometry

   Interface for getting the geometry of longitudinal rebars in a girder

DESCRIPTION
   Interface for getting the rebar geometry.  

*****************************************************************************/
// {C2EE02C6-1785-11d3-AD6C-00105A9AF985}
DEFINE_GUID(IID_ILongRebarGeometry, 
0xc2ee02c6, 0x1785, 0x11d3, 0xad, 0x6c, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ILongRebarGeometry : IUnknown
{
   virtual void GetRebars(const pgsPointOfInterest& poi,IRebarSection** rebarSection) const = 0;
   virtual Float64 GetAsBottomHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const = 0; // Fig. 5.7.3.4.2-3 (pre2017: 5.8.3.4.2-3)
   virtual Float64 GetAsTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const = 0; // Fig. 5.7.3.4.2-3
   virtual Float64 GetAsGirderTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const = 0; // Fig. 5.7.3.4.2-3
   virtual Float64 GetAsDeckTopHalf(const pgsPointOfInterest& poi,bool bDevAdjust) const = 0; // Fig. 5.7.3.4.2-3
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem) const = 0;
   virtual Float64 GetDevLengthFactor(const pgsPointOfInterest& poi,IRebarSectionItem* rebarItem, pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) const = 0;

   virtual Float64 GetPPRTopHalf(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetPPRBottomHalf(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const = 0;

   // returns the nominal cover to the top mat of deck rebar, measured from
   // the top of the deck (including sacrificial depth)
   virtual Float64 GetCoverTopMat() const = 0;

   // returns the location of the top mat of deck rebar, measured from the bottom of the deck
   virtual Float64 GetTopMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const = 0;

   // returns the area of top mat deck rebar
   virtual Float64 GetAsTopMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const = 0;

   // returns the nominal cover to the bottom mat of deck rebar, measured from
   // the bottom of the deck slab
   virtual Float64 GetCoverBottomMat() const = 0;

   // returns the location of the bottom mat of deck rebar, measured from the bottom of the deck
   virtual Float64 GetBottomMatLocation(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const = 0;

   // returns the area of bottom mat deck rebar based
   virtual Float64 GetAsBottomMat(const pgsPointOfInterest& poi,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory) const = 0;

   virtual void GetDeckReinforcing(const pgsPointOfInterest& poi,pgsTypes::DeckRebarMatType matType,pgsTypes::DeckRebarBarType barType,pgsTypes::DeckRebarCategoryType barCategory,bool bAdjForDevLength,Float64* pAs,Float64* pYb) const = 0;


   virtual void GetRebarLayout(const CSegmentKey& segmentKey, IRebarLayout** rebarLayout) const = 0;

   virtual void GetClosureJointRebarLayout(const CClosureKey& closureKey, IRebarLayout** rebarLayout) const = 0;

   virtual REBARDEVLENGTHDETAILS GetSegmentRebarDevelopmentLengthDetails(const CSegmentKey& segmentKey,IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) const = 0;
   virtual REBARDEVLENGTHDETAILS GetDeckRebarDevelopmentLengthDetails(IRebar* rebar,pgsTypes::ConcreteType type, Float64 fc, bool isFct, Float64 Fct) const = 0;
};

/*****************************************************************************
INTERFACE
   IStirrupGeometry

   Interface for getting the stirrup geometry.

DESCRIPTION
   Interface for getting the stirrup geometry.  The geometry is
   the stirrup spacing and area
*****************************************************************************/
// {1FFE79BE-9545-11d2-AC7B-00105A9AF985}
DEFINE_GUID(IID_IStirrupGeometry, 
0x1ffe79be, 0x9545, 0x11d2, 0xac, 0x7b, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IStirrupGeometry : IUnknown
{
   // Primary bar zones
   virtual bool AreStirrupZonesSymmetrical(const CSegmentKey& segmentKey) const = 0;

   // zone get is zero-based. 
   // zones are sorted left->right across entire girder
   // zones may, or may not, be symmetric about mid-girder
   virtual ZoneIndexType GetPrimaryZoneCount(const CSegmentKey& segmentKey) const = 0;
   virtual void GetPrimaryZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end) const = 0;
   virtual void GetPrimaryVertStirrupBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing) const = 0;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const CSegmentKey& segmentKey,ZoneIndexType zone) const = 0;
   virtual matRebar::Size GetPrimaryConfinementBarSize(const CSegmentKey& segmentKey,ZoneIndexType zone) const = 0;

   // Horizontal Interface additional bar zones
   // zone get is zero-based. 
   // zones are sorted left->right 
   // zones may, or may not, be symmetric about mid-girder
   virtual ZoneIndexType GetHorizInterfaceZoneCount(const CSegmentKey& segmentKey) const = 0;
   virtual void GetHorizInterfaceZoneBounds(const CSegmentKey& segmentKey, ZoneIndexType zone, Float64* start, Float64* end) const = 0;
   virtual void GetHorizInterfaceBarInfo(const CSegmentKey& segmentKey,ZoneIndexType zone, matRebar::Size* pSize, Float64* pCount, Float64* pSpacing) const = 0;

   // Additional splitting and confinement bars
   virtual void GetAddSplittingBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pnBars, Float64* pSpacing) const = 0;
   virtual void GetAddConfinementBarInfo(const CSegmentKey& segmentKey, matRebar::Size* pSize, Float64* pZoneLength, Float64* pSpacing) const = 0;

   // Calculated bar values at poi's
   // Vertical shear
   virtual Float64 GetVertStirrupAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) const = 0;
   virtual Float64 GetVertStirrupBarNominalDiameter(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetAlpha(const pgsPointOfInterest& poi) const = 0; // stirrup angle=90 for vertical

   // Horizontal interface shear
   virtual bool DoStirrupsEngageDeck(const CSegmentKey& segmentKey) const = 0;
   virtual bool DoAllPrimaryStirrupsEngageDeck(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetPrimaryHorizInterfaceBarSpacing(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetPrimaryHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) const = 0;
   virtual Float64 GetPrimaryHorizInterfaceBarCount(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetAdditionalHorizInterfaceBarSpacing(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetAdditionalHorizInterfaceAvs(const pgsPointOfInterest& poi, matRebar::Size* pSize, Float64* pSingleBarArea, Float64* pCount, Float64* pSpacing) const = 0;
   virtual Float64 GetAdditionalHorizInterfaceBarCount(const pgsPointOfInterest& poi) const = 0;

   // Total area of splitting shear steel between two points along the girder
   virtual Float64 GetSplittingAv(const CSegmentKey& segmentKey,Float64 start,Float64 end) const = 0;

   // Processed confinement bar information - returns max bar size/min spacing in required zone length at both ends of girder
   virtual void GetStartConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) const = 0;
   virtual void GetEndConfinementBarInfo(const CSegmentKey& segmentKey, Float64 requiredZoneLength, matRebar::Size* pSize, Float64* pProvidedZoneLength, Float64* pSpacing) const = 0;

   // Returns true if the stirrup layout geometry is ok
   virtual bool AreStirrupZoneLengthsCombatible(const CGirderKey& girderKey) const = 0;
};

/*****************************************************************************
INTERFACE
   IStrandGeometry

   Interface for getting the prestressing strand geometry.

DESCRIPTION
   Interface for getting the prestressing strand geometry.  The geometry is
   the strand slope and eccentricities.
*****************************************************************************/
//
// Options for development length computation. Approximate was added because the vertical shear design algoritm
// was very slow using the accurate method, but it gave much better results. Hence, a speed/accuracy compromise.
enum DevelopmentAdjustmentType {dlaNone, dlaApproximate, dlaAccurate};

// {99B7A322-67A8-11d2-883A-006097C68A9C}
DEFINE_GUID(IID_IStrandGeometry, 
0x99b7a322, 0x67a8, 0x11d2, 0x88, 0x3a, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IStrandGeometry : IUnknown
{
   // Eccentricity values greater than zero indicate the strands are below the cg of the section. 
   // (+ eccentricity, strands in bottom, - eccentricity, strands in top)

   virtual void GetStrandCG(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, Float64* nEffectiveStrands, Float64* pX, Float64* pY) const = 0;
   virtual void GetStrandCG(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, Float64* nEffectiveStrands, Float64* pX, Float64* pY) const = 0;
   virtual void GetStrandCG(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands, Float64* pX, Float64* pY) const = 0;
   virtual void GetStrandCG(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands, Float64* pX, Float64* pY) const = 0;

   virtual void GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;
   virtual void GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands, Float64* pEccX, Float64* pEccY) const = 0;

   // Returns the geometric eccentricity of prestressing strands for the various strand types.
   // Eccentricity is measured with respect to the centroid of the section at the specified interval
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp,Float64* nEffectiveStrands) const = 0;
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,Float64* nEffectiveStrands) const = 0;

   // Returns the geometric eccentricity of prestressing strands for the various strand types.
   // Eccentricity is measured with respect to the centroid of the specified section type at the specified interval
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bIncTemp, Float64* nEffectiveStrands) const = 0;
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, Float64* nEffectiveStrands) const = 0;

   // Returns the geometric eccentricity of prestressing strands for the various strand types for the specified configuration.
   // Eccentricity is measured with respect to the centroid of the section at the specified interval
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) const = 0;
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) const = 0;

   // Returns the geometric eccentricity of prestressing strands for the various strand types.
   // Eccentricity is measured with respect to the centroid of the specified section type at the specified interval
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bIncTemp, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) const = 0;
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig, Float64* nEffectiveStrands) const = 0;

   // Returns the distance from the top of the girder to the geometric CG of the strand in Girder Section Coordinates
   virtual Float64 GetStrandLocation(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx) const = 0;

   // gets a profile view of a strand
   virtual void GetStrandProfile(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, StrandIndexType strandIdx, IPoint2dCollection** ppProfilePoints) const = 0;
   virtual void GetStrandProfile(const CPrecastSegmentData* pSegment, const CStrandData* pStrands, pgsTypes::StrandType strandType, StrandIndexType strandIdx, IPoint2dCollection** ppProfilePoints) const = 0;
   virtual void GetStrandCGProfile(const CSegmentKey& segmentKey, bool bIncTemp, IPoint2dCollection** ppProfilePoints) const = 0;

   // Returns the steepest slope at a point of interest for the harped strands. Slope is in the form 1:n (rise:run). This method returns n.
   // Slopes upward to the right have positive value (Slope is < 0 at left end of girder and > 0 at right end of girder
   // for normal configuration of harped strands)
   virtual Float64 GetMaxStrandSlope(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the steepest slope of the harped strands. Slope is in the form 1:n (rise:run). This method returns n.
   // Slopes upward to the right have positive value (Slope is < 0 at left end of girder and > 0 at right end of girder
   // for normal configuration of harped strands)
   virtual Float64 GetMaxStrandSlope(const CSegmentKey& segmentKey) const = 0;

   // Returns the average slope of the harped strands. Slope is in the form 1:n (rise:run). This method returns n.
   // Slopes upward to the right have positive value (Slope is < 0 at left end of girder and > 0 at right end of girder
   // for normal configuration of harped strands)
   virtual Float64 GetAvgStrandSlope(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr) const = 0;


   virtual Float64 GetApsBottomHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust, const GDRCONFIG* pConfig = nullptr) const = 0; // Fig. 5.8.3.4.2-3
   virtual Float64 GetApsTopHalf(const pgsPointOfInterest& poi,DevelopmentAdjustmentType devAdjust, const GDRCONFIG* pConfig = nullptr) const = 0; // Fig. 5.8.3.4.2-3

   virtual StrandIndexType GetMaxNumPermanentStrands(const CSegmentKey& segmentKey) const = 0;
   virtual StrandIndexType GetMaxNumPermanentStrands(LPCTSTR strGirderName) const = 0;
   // get ratio of harped/straight strands if total permanent strands is used for input. returns false if total doesn't fit
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,const CSegmentKey& segmentKey, StrandIndexType* numStraight, StrandIndexType* numHarped) const = 0;
   virtual bool ComputeNumPermanentStrands(StrandIndexType totalPermanent,LPCTSTR strGirderName, StrandIndexType* numStraight, StrandIndexType* numHarped) const = 0;
   // get next and previous number of strands - return INVALID_INDEX if at end
   virtual StrandIndexType GetNextNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetNextNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetPreviousNumPermanentStrands(const CSegmentKey& segmentKey,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetPreviousNumPermanentStrands(LPCTSTR strGirderName,StrandIndexType curNum) const = 0;
   // Compute strand Indices as in girder library for given filled strands
   virtual bool ComputePermanentStrandIndices(LPCTSTR strGirderName,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType strType, IIndexArray** permIndices) const = 0;

   // Functions to compute ordered strand filling for straight/harped/temporary fill orders
   virtual bool IsValidNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) const = 0;
   virtual bool IsValidNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetNextNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetNextNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetPrevNumStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType curNum) const = 0;
   virtual StrandIndexType GetPrevNumStrands(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType curNum) const = 0;

   // Function to compute a strand fill array using grid index (as used in in PRESTRESSCONFIG) from an ordered fill of Ns strands
   virtual ConfigStrandFillVector ComputeStrandFill(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType Ns) const = 0;
   virtual ConfigStrandFillVector ComputeStrandFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType Ns) const = 0;

   // Conversions from/to sequential fill index to grid fill index (as used in in PRESTRESSCONFIG). A single grid entry can have two strands
   virtual GridIndexType SequentialFillToGridFill(LPCTSTR strGirderName,pgsTypes::StrandType type,StrandIndexType StrandNo) const = 0;
   virtual void GridFillToSequentialFill(LPCTSTR strGirderName,pgsTypes::StrandType type,GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) const = 0;

   virtual void GridPositionToStrandPosition(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, GridIndexType gridIdx, StrandIndexType* pStrandNo1, StrandIndexType* pStrandNo2) const = 0;

   virtual StrandIndexType GetStrandCount(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const GDRCONFIG* pConfig = nullptr) const = 0;
   virtual StrandIndexType GetMaxStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const = 0;
   virtual StrandIndexType GetMaxStrands(LPCTSTR strGirderName,pgsTypes::StrandType type) const = 0;

   // Gets the nominal strand area
   virtual Float64 GetStrandArea(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType) const = 0;

   // Gets the nominal strand area at a poi (reduces the strand area by the area of debonded strands)
   virtual Float64 GetStrandArea(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::StrandType strandType) const = 0;

   // Gets the total nominal strand area
   virtual Float64 GetAreaPrestressStrands(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,bool bIncTemp) const = 0;

   virtual Float64 GetPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual Float64 GetPjack(const CSegmentKey& segmentKey,bool bIncTemp) const = 0;

   virtual void GetStrandPosition(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, IPoint2d** ppPoint) const = 0;
   virtual void GetStrandPositions(const pgsPointOfInterest& poi, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) const = 0;
   virtual void GetStrandPositionEx(const pgsPointOfInterest& poi, StrandIndexType strandIdx,pgsTypes::StrandType type, const PRESTRESSCONFIG& rconfig,IPoint2d** ppPoint) const = 0;
   virtual void GetStrandPositionsEx(const pgsPointOfInterest& poi,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, IPoint2dCollection** ppPoints) const = 0; 
   virtual void GetStrandPositionsEx(LPCTSTR strGirderName, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const PRESTRESSCONFIG& rconfig, pgsTypes::StrandType type, pgsTypes::MemberEndType endType, IPoint2dCollection** ppPoints) const = 0;

   // Harped strands can be forced to be straight along their length
   virtual bool GetAreHarpedStrandsForcedStraight(const CSegmentKey& segmentKey) const = 0;

   // Many of the harped strand geometry methods require the height of the girder segment at
   // the start, end, and harp points. This method gets these values for the current girder
   virtual void GetHarpedStrandControlHeights(const CSegmentKey& segmentKey,Float64* pHgStart,Float64* pHgHp1,Float64* pHgHp2,Float64* pHgEnd) const = 0;

   // harped vertical offsets are measured from original strand locations in strand grid
   virtual void GetHarpStrandOffsets(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* pOffsetEnd,Float64* pOffsetHp) const = 0;
   virtual void GetHarpedEndOffsetBounds(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* DownwardOffset, Float64* UpwardOffset) const = 0;
   virtual void GetHarpedEndOffsetBoundsEx(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset) const = 0;
   virtual void GetHarpedEndOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) const = 0;
   virtual void GetHarpedHpOffsetBounds(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,Float64* DownwardOffset, Float64* UpwardOffset) const = 0;
   virtual void GetHarpedHpOffsetBoundsEx(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,StrandIndexType Nh, Float64* DownwardOffset, Float64* UpwardOffset) const = 0;
   virtual void GetHarpedHpOffsetBoundsEx(LPCTSTR strGirderName, pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd, const ConfigStrandFillVector& rHarpedFillArray, Float64* DownwardOffset, Float64* UpwardOffset) const = 0;

   virtual Float64 GetHarpedEndOffsetIncrement(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetHarpedHpOffsetIncrement(const CSegmentKey& segmentKey) const = 0;

   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* lhp,Float64* rhp) const = 0;
   virtual void GetHarpingPointLocations(const CSegmentKey& segmentKey,Float64* pX1,Float64* pX2,Float64* pX3,Float64* pX4) const = 0;
   virtual void GetHighestHarpedStrandLocationEnds(const CSegmentKey& segmentKey,Float64* pElevation) const = 0;
   virtual void GetHighestHarpedStrandLocationHPs(const CSegmentKey& segmentKey,Float64* pElevation) const = 0;
   virtual Float64 GetHarpedEndOffsetIncrement(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType) const = 0;
   virtual Float64 GetHarpedHpOffsetIncrement(LPCTSTR strGirderName, pgsTypes::AdjustableStrandType adjType) const = 0;

   virtual IndexType GetNumHarpPoints(const CSegmentKey& segmentKey) const = 0;

   virtual StrandIndexType GetNumExtendedStrands(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType standType) const = 0;
   virtual bool IsExtendedStrand(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) const = 0;
   virtual bool IsExtendedStrand(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig=nullptr) const = 0;

   virtual bool IsStrandDebonded(const CSegmentKey& segmentKey,StrandIndexType strandIdx,pgsTypes::StrandType strandType,const GDRCONFIG* pConfig,Float64* pStart,Float64* pEnd) const = 0;
   virtual bool IsStrandDebonded(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) const = 0;
   virtual StrandIndexType GetNumDebondedStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, pgsTypes::DebondMemberEndType dbendType) const = 0;
   virtual RowIndexType GetNumRowsWithStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType ) const = 0;
   virtual StrandIndexType GetNumStrandInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const = 0;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const pgsPointOfInterest& poi, RowIndexType rowIdx, pgsTypes::StrandType strandType ) const = 0;
   virtual StrandIndexType GetNumDebondedStrandsInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const = 0;
   virtual bool IsExteriorStrandDebondedInRow(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const = 0;
   virtual Float64 GetUnadjustedStrandRowElevation(const pgsPointOfInterest& poi,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const = 0;

   virtual bool HasDebonding(const CSegmentKey& segmentKey) const = 0;
   virtual bool IsDebondingSymmetric(const CSegmentKey& segmentKey) const = 0;

   // these functions return the data for the number of strands given (used during design)
   virtual RowIndexType GetNumRowsWithStrand(const pgsPointOfInterest& poi,StrandIndexType nStrands,pgsTypes::StrandType strandType ) const = 0;
   virtual StrandIndexType GetNumStrandInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx,pgsTypes::StrandType strandType ) const = 0;
   virtual std::vector<StrandIndexType> GetStrandsInRow(const pgsPointOfInterest& poi,StrandIndexType nStrands,RowIndexType rowIdx, pgsTypes::StrandType strandType ) const = 0;

   // Section locations measured from left->right end of girder, and are sorted from left end to right
   virtual Float64 GetDebondSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const = 0;
   virtual SectionIndexType GetNumDebondSections(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const = 0;
   virtual StrandIndexType GetNumDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const = 0;
   virtual StrandIndexType GetNumBondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const = 0;
   virtual std::vector<StrandIndexType> GetDebondedStrandsAtSection(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,SectionIndexType sectionIdx,pgsTypes::StrandType strandType) const = 0;

   virtual bool CanDebondStrands(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const = 0; // can debond any of the strands?
   virtual bool CanDebondStrands(LPCTSTR strGirderName,pgsTypes::StrandType strandType) const = 0; // can debond any of the strands?
   // returns long array of the same length as GetStrandPositions. 0==not debondable
   virtual void ListDebondableStrands(const CSegmentKey& segmentKey,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) const = 0;
   virtual void ListDebondableStrands(LPCTSTR strGirderName,const ConfigStrandFillVector& rFillArray,pgsTypes::StrandType strandType, IIndexArray** list) const = 0; 
   virtual Float64 GetDefaultDebondLength(const CSegmentKey& segmentKey) const = 0;

   // Returns the indicies of rows that have debonding
   virtual std::vector<RowIndexType> GetRowsWithDebonding(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the number of different debond configurations in a row
   virtual IndexType GetDebondConfigurationCountByRow(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, RowIndexType rowIdx, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Returns the debond strand configuration. Xstart is location from left end of girder where the strand gets bonded
   // Lstrand is the length of the bonded strands, and nStrands is the number of debonded strands for
   // this configuration
   virtual void GetDebondConfigurationByRow(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, RowIndexType rowIdx, IndexType configIdx, const GDRCONFIG* pConfig, Float64* pXstart, Float64* pLstrand, Float64* pCgX, Float64* pCgY, StrandIndexType* pnStrands) const = 0;

   // Functions to compute harped strand offsets based on available measurement types
   // Absolute offset is distance that raw strand grid locations are to be moved.
   // rHarpedFillArray is same as StrandFill in PRESTRESSCONFIG
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const = 0;
   virtual Float64 ComputeAbsoluteHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 offset) const = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const = 0;
   virtual Float64 ComputeHarpedOffsetFromAbsoluteHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64 absoluteOffset) const = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const = 0;
   virtual void ComputeValidHarpedOffsetForMeasurementTypeHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType measurementType, Float64* lowRange, Float64* highRange) const = 0;
   virtual Float64 ConvertHarpedOffsetEnd(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const = 0;
   virtual Float64 ConvertHarpedOffsetEnd(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const = 0;
   virtual Float64 ConvertHarpedOffsetHp(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const = 0;
   virtual Float64 ConvertHarpedOffsetHp(LPCTSTR strGirderName,pgsTypes::MemberEndType endType,pgsTypes::AdjustableStrandType adjType, Float64 HgStart,Float64 HgHp1,Float64 HgHp2,Float64 HgEnd,const ConfigStrandFillVector& rHarpedFillArray, HarpedStrandOffsetType fromMeasurementType, Float64 offset, HarpedStrandOffsetType toMeasurementType) const = 0;

   virtual pgsTypes::TTSUsage GetTemporaryStrandUsage(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig = nullptr) const = 0;

   // Resolves the harp point location provided in pSegment to harp point locations in Segment Coordinates
   // if pStrands is nullptr, strand data is taken from pSegment, otherwise from pStrand
   virtual void ResolveHarpPointLocations(const CPrecastSegmentData* pSegment, const CStrandData* pStrands, std::array<Float64, 4>& Xhp) const = 0;

   // For row or individual strand based input, resolves the strand elevation values to Girder Section Coordinates
   // if pStrands is nullptr, strand data is taken from pSegment, otherwise from pStrand
   virtual void ResolveStrandRowElevations(const CPrecastSegmentData* pSegment, const CStrandData* pStrands, const CStrandRow& strandRow, std::array<Float64, 4>& Xhp, std::array<Float64, 4>& Y) const = 0;
};

/*****************************************************************************
INTERFACE
   ISectionProperties

   Interface for obtaining section properties.

DESCRIPTION
   Interface for obtaining section properties.
****************************************************************************/
// {28D53414-E8FD-4b53-A9B7-B395EB1E11E7}
DEFINE_GUID(IID_ISectionProperties, 
0x28d53414, 0xe8fd, 0x4b53, 0xa9, 0xb7, 0xb3, 0x95, 0xeb, 0x1e, 0x11, 0xe7);
interface ISectionProperties : IUnknown
{
   // returns the current section properties mode
   virtual pgsTypes::SectionPropertyMode GetSectionPropertiesMode() const = 0;

   // returns how haunch is varried when computing section properties
   virtual pgsTypes::HaunchAnalysisSectionPropertiesType GetHaunchAnalysisSectionPropertiesType() const = 0;

   // Returns the stress points for a section. The controlling stress point indes returned by GetStressCoefficients can be used to access the container
   virtual std::vector<gpPoint2d> GetStressPoints(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location, const GDRCONFIG* pConfig = nullptr) const = 0;

   // Get the stress coefficients at the specifed location
   // f = Ca*Axial + Cbx*Mx + Cby*My
   virtual void GetStressCoefficients(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation location, const GDRCONFIG* pConfig, Float64* pCa, Float64 *pCbx,Float64* pCby,IndexType* pControllingStressPointIndex=nullptr) const = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the section properties model defined in the project criteria
   virtual Float64 GetHg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetIxx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetIyy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetIxy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetXleft(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetXright(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) const = 0;
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) const = 0;
   virtual Float64 GetKt(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetKb(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetEIx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the section properties model defined in the project criteria, except that
   // the segment concrete strength is taken to be fcgdr
   virtual Float64 GetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) const = 0;
   virtual Float64 GetIxx(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fcgdr) const = 0;
   virtual Float64 GetIyy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fcgdr) const = 0;
   virtual Float64 GetIxy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fcgdr) const = 0;
   virtual Float64 GetY(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr) const = 0;
   virtual Float64 GetS(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fcgdr) const = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the specified section property type
   virtual Float64 GetHg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetIxx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetIyy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetIxy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetXleft(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetXright(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) const = 0;
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location) const = 0;
   virtual Float64 GetKt(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetKb(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetEIx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;

   // Returns section properties for the specified interval. Section properties
   // are based on the specified section property type, except that the segment concrete strength is
   // taken to be fcgdr
   virtual Float64 GetAg(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) const = 0;
   virtual Float64 GetIxx(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 fc) const = 0;
   virtual Float64 GetIyy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fc) const = 0;
   virtual Float64 GetIxy(pgsTypes::SectionPropertyType spType, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, Float64 fc) const = 0;
   virtual Float64 GetY(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc) const = 0;
   virtual Float64 GetS(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation location,Float64 fc) const = 0;

   // Net girder properties
   virtual Float64 GetNetAg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetIxx(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetIyy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetIxy(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetYbg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetYtg(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;

   // Net deck properties
   virtual Float64 GetNetAd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetId(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetYbd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetNetYtd(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;


   virtual Float64 GetQSlab(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetQSlab(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64 fc) const = 0;
   virtual Float64 GetAcBottomHalf(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0; // for Fig. 5.7.3.4.2-3
   virtual Float64 GetAcTopHalf(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const = 0; // for Fig. 5.7.3.4.2-3

   virtual Float64 GetEffectiveFlangeWidth(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetTributaryFlangeWidth(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetTributaryFlangeWidthEx(const pgsPointOfInterest& poi, Float64* pLftFw, Float64* pRgtFw) const = 0;

   virtual Float64 GetEffectiveDeckArea(const pgsPointOfInterest& poi) const = 0; // deck area based on effective flange width
   virtual Float64 GetTributaryDeckArea(const pgsPointOfInterest& poi) const = 0; // deck area based on tributary width
   virtual Float64 GetGrossDeckArea(const pgsPointOfInterest& poi) const = 0;     // same as triburary deck area, except gross slab depth is used
   // Depth of haunch for given haunch type. For parabolic case, assumes a parabolic variation and includes roadway effect and assumed excess camber at mid-span
   virtual Float64 GetStructuralHaunchDepth(const pgsPointOfInterest& poi,pgsTypes::HaunchAnalysisSectionPropertiesType haunchAType) const = 0;

   // Reporting
   virtual void ReportEffectiveFlangeWidth(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const = 0;

   // Volume and surface area
   virtual Float64 GetPerimeter(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetSegmentSurfaceArea(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentVolume(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetClosureJointSurfaceArea(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetClosureJointVolume(const CClosureKey& closureKey) const = 0;
   virtual Float64 GetDeckSurfaceArea() const = 0;
   virtual Float64 GetDeckVolume() const = 0;

   // Bending stiffness of entire bridge section - for deflection calculation
   // Crowns, slopes, and slab haunches are ignored.
   // Xb is measured in Bridge Line Coordinates
   virtual void GetBridgeStiffness(Float64 Xb, Float64* pEIxx, Float64* pEIyy, Float64* pEIxy) const = 0;
   virtual Float64 GetBridgeEIxx(Float64 Xb) const = 0;
   virtual Float64 GetBridgeEIyy(Float64 Xb) const = 0;
   virtual Float64 GetBridgeEIxy(Float64 Xb) const = 0;


   virtual Float64 GetSegmentWeightPerLength(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSegmentWeight(const CSegmentKey& segmentKey) const = 0;

   virtual Float64 GetSegmentHeightAtPier(const CSegmentKey& segmentKey,PierIndexType pierIdx) const = 0;
   virtual Float64 GetSegmentHeightAtTemporarySupport(const CSegmentKey& segmentKey,SupportIndexType tsIdx) const = 0;

   // Returns the height of the segment at Xs based on the specified parameters
   virtual Float64 GetSegmentHeight(const CPrecastSegmentData* pSegment, Float64 Xs) const = 0;
};

/*****************************************************************************
INTERFACE
   IShapes

DESCRIPTION
   The method on this interface return a geometric shape object for
   the gross (outline) shape of a bridge component. These shapes are ususally
   used for making images and graphical displays. Shapes are located
   in the Bridge Section Coordinate system.
*****************************************************************************/
// {B0BFEC24-7355-46d7-B552-5A177BB20EEE}
DEFINE_GUID(IID_IShapes, 
0xb0bfec24, 0x7355, 0x46d7, 0xb5, 0x52, 0x5a, 0x17, 0x7b, 0xb2, 0xe, 0xee);
interface IShapes : public IUnknown
{
   // returns the raw shape of the segment
   virtual void GetSegmentShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bOrient,pgsTypes::SectionCoordinateType coordinateType,pgsTypes::HaunchAnalysisSectionPropertiesType haunchAType,IShape** ppShape) const = 0;

   // returns the raw segment shape based on the provided segment data. the shape will be in the girder section coordinate system
   virtual void GetSegmentShape(const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const = 0;

   // returns the shape of the segment with any section removal (such as clipping for sacrifical depth)
   virtual void GetSegmentSectionShape(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bOrient, pgsTypes::SectionCoordinateType csType, IShape** ppShape) const = 0;

   virtual void GetSlabShape(Float64 station,IDirection* pDirection,bool bIncludeHaunch,IShape** ppShape) const = 0;
   virtual void GetLeftTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape) const = 0;
   virtual void GetRightTrafficBarrierShape(Float64 station,IDirection* pDirection,IShape** ppShape) const = 0;
   virtual void GetJointShapes(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bOrient, pgsTypes::SectionCoordinateType coordinateType, IShape** ppLeftJointShape,IShape** ppRightJointShape) const = 0;

   // get the slab shape at the given location including haunch parts
   // if bFollowMatingSurfaceProfile is true, the bottom of the haunch will follow the mating surface profile
   // regardless of hauchDepth
   virtual void GetSlabAnalysisShape(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,Float64 haunchDepth,bool bFollowMatingSurfaceProfile,IShape** ppShape) const = 0;
};

/*****************************************************************************
INTERFACE
   IBarriers

   Interface for obtaining information barriers on the bridge

DESCRIPTION
   Interface for obtaining information barriers on the bridge
*****************************************************************************/
// {8EAC1B80-43B6-450b-B73C-4F55FC8681F6}
DEFINE_GUID(IID_IBarriers, 
0x8eac1b80, 0x43b6, 0x450b, 0xb7, 0x3c, 0x4f, 0x55, 0xfc, 0x86, 0x81, 0xf6);
interface IBarriers : public IUnknown
{
   // Traffic Barrier Properties
   virtual Float64 GetAtb(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetItb(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetYbtb(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetInterfaceWidth(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetExteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetExteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) const = 0;

   virtual bool HasInteriorBarrier(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetInteriorBarrierWeight(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual Float64 GetInteriorBarrierCgToDeckEdge(pgsTypes::TrafficBarrierOrientation orientation) const = 0;


   virtual pgsTypes::TrafficBarrierOrientation GetNearestBarrier(const CSegmentKey& segmentKey) const = 0;

   // Distance from nearest edge of deck to edges of sidewalk for Dead load and Pedestrian load
   virtual void GetSidewalkDeadLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) const = 0;
   virtual void GetSidewalkPedLoadEdges(pgsTypes::TrafficBarrierOrientation orientation, Float64* pintEdge, Float64* pextEdge) const = 0;

   virtual Float64 GetSidewalkWeight(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
   virtual bool HasSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const = 0;
};

/*****************************************************************************
INTERFACE
   IUserDefinedLoads

   Interface to get processed User Defined Loads.

DESCRIPTION
   Interface to get processed User Defined Load information
*****************************************************************************/
// {B5BC8CBA-352D-4660-95AE-C8D43D5176A8}
DEFINE_GUID(IID_IUserDefinedLoads, 
0xb5bc8cba, 0x352d, 0x4660, 0x95, 0xae, 0xc8, 0xd4, 0x3d, 0x51, 0x76, 0xa8);
interface IUserDefinedLoads : IUnknown 
{
   enum UserDefinedLoadCase {userDC, userDW, userLL_IM};

   // structs to define loads. locations are in system units (no fractional)
   struct UserPointLoad
   {
      UserDefinedLoadCase m_LoadCase;
      bool                m_bLoadOnStartCantilever; // if true, the load is on the start cantilever of the span
      bool                m_bLoadOnEndCantilever; // if true, the load is on the end cantilever of the span
      // if neither are true, load is in the main span
      Float64             m_Location; // from the left end of the firs segment in the girder
      Float64             m_Magnitude;
      std::_tstring       m_Description;
   };

   // distributed loads always in general trapezoidal form
   struct UserDistributedLoad  
   {
      UserDefinedLoadCase m_LoadCase;
      Float64             m_StartLocation; // from left support
      Float64             m_EndLocation; // from left support
      Float64             m_WStart;
      Float64             m_WEnd;
      std::_tstring       m_Description;
   };

   // moment and point loads are the same, except for the interpretation of Magnitude
   typedef UserPointLoad UserMomentLoad;

   // returns true if user defined loads exist in any interval
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey) const = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey) const = 0;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,UserDefinedLoadCase loadCase) const = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,UserDefinedLoadCase loadCase) const = 0;

   // returns true if user defined loads exist in the specified interval
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx) const = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const = 0;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType intervalIdx,UserDefinedLoadCase loadCase) const = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType intervalIdx,UserDefinedLoadCase loadCase) const = 0;

   // returns true if user defined loads exist in the specified range of intervals
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx) const = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx) const = 0;
   virtual bool DoUserLoadsExist(const CSpanKey& spanKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,UserDefinedLoadCase loadCase) const = 0;
   virtual bool DoUserLoadsExist(const CGirderKey& girderKey,IntervalIndexType firstIntervalIdx,IntervalIndexType lastIntervalIdx,UserDefinedLoadCase loadCase) const = 0;

   virtual const std::vector<UserPointLoad>* GetPointLoads(IntervalIndexType intervalIdx, const CSpanKey& spanKey) const = 0;
   virtual const std::vector<UserDistributedLoad>* GetDistributedLoads(IntervalIndexType intervalIdx, const CSpanKey& spanKey) const = 0;
   virtual const std::vector<UserMomentLoad>* GetMomentLoads(IntervalIndexType intervalIdx, const CSpanKey& spanKey) const = 0;
};


/*****************************************************************************
INTERFACE
   ITempSupport

   Interface for obtaining information about temporary supports

DESCRIPTION
   Interface for obtaining information about temporary supports
*****************************************************************************/
// {A6722C82-2796-4a4b-999E-E34C6F1A2FEE}
DEFINE_GUID(IID_ITempSupport, 
0xa6722c82, 0x2796, 0x4a4b, 0x99, 0x9e, 0xe3, 0x4c, 0x6f, 0x1a, 0x2f, 0xee);

interface ITempSupport : public IUnknown
{
   virtual void GetControlPoints(SupportIndexType tsIdx,pgsTypes::PlanCoordinateType pcType,IPoint2d** ppLeft,IPoint2d** ppAlignment_pt,IPoint2d** ppBridge_pt,IPoint2d** ppRight) const = 0;
   virtual void GetDirection(SupportIndexType tsIdx,IDirection** ppDirection) const = 0;
   virtual void GetSkew(SupportIndexType tsIdx,IAngle** ppAngle) const = 0;
   virtual std::vector<SupportIndexType> GetTemporarySupports(GroupIndexType grpIdx) const = 0;
   virtual std::vector<TEMPORARYSUPPORTELEVATIONDETAILS> GetElevationDetails(SupportIndexType tsIdx) const = 0;
};

/*****************************************************************************
INTERFACE
   IGirder
DESCRIPTION
   Interface for obtaining information about girder segments
*****************************************************************************/
// {7B03736C-E8AD-49b9-BF5C-D5F6E61B50D5}
DEFINE_GUID(IID_IGirder, 
0x7b03736c, 0xe8ad, 0x49b9, 0xbf, 0x5c, 0xd5, 0xf6, 0xe6, 0x1b, 0x50, 0xd5);
interface IGirder : public IUnknown
{
   // Returns true if the segment is prismatic in the specified interval
   virtual bool    IsPrismatic(IntervalIndexType intervalIdx,const CSegmentKey& segmentKey) const = 0;

   // Returns true if the longitudinal shape of the segment is symmetric about its mid-point
   virtual bool IsSymmetricSegment(const CSegmentKey& segmentKey) const = 0;

   // Returns true if the girder is symmetric in the specified interval. Symmetry is defined
   // as the left end and right end of the girder is the same (same debonding, symmetric harp points, etc)
   virtual bool    IsSymmetric(IntervalIndexType intervalIdx,const CGirderKey& girderKey) const = 0; 

   // Returns teh number of mating surfaces
   virtual MatingSurfaceIndexType  GetNumberOfMatingSurfaces(const CGirderKey& girderKey) const = 0;

   // Location of mating surface, measured from the CL girder. < 0 if left of CL.
   // if bGirderOnly is false, structural longitinal joints are considered as part of the mating surface
   virtual Float64 GetMatingSurfaceLocation(const pgsPointOfInterest& poi,MatingSurfaceIndexType msIdx, bool bGirderOnly=false) const = 0;

   // Returns the width of a mating surface
   // if bGirderOnly is false, structural longitinal joints are considered as part of the mating surface
   virtual Float64 GetMatingSurfaceWidth(const pgsPointOfInterest& poi,MatingSurfaceIndexType msIdx, bool bGirderOnly = false) const = 0;

   // Gets the mating surface profile. Returns true if sucessful. Can return false or a nullptr container if a mating surface profile is not available
   // if bGirderOnly is false, structural longitinal joints are considered as part of the mating surface
   virtual bool GetMatingSurfaceProfile(const pgsPointOfInterest& poi, MatingSurfaceIndexType msIdx, pgsTypes::SectionCoordinateType scType, bool bGirderOnly, IPoint2dCollection** ppPoints) const = 0;

   // Returns the number of top flanges
   virtual FlangeIndexType GetNumberOfTopFlanges(const CGirderKey& girderKey) const = 0;

   // Returns the location of the center of a top flange measured from the CL girder. < 0 if left of CL.
   virtual Float64 GetTopFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the width of a top flange
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the total top flange width by summing with width of all the mating surfaces
   virtual Float64 GetTopFlangeWidth(const pgsPointOfInterest& poi) const = 0;

   // Returns the overall top width of a girder (for U-beam, this would be out-to-out width at top of girder)
   // If pLeft and pRight are not nullptr, the left and right dimensions that make up the overall width are provided
   // This is meaningful if a section is asymmetric. If not asymetric section, left and right is taken to be half the width
   virtual Float64 GetTopWidth(const pgsPointOfInterest& poi,Float64* pLeft=nullptr,Float64* pRight=nullptr) const = 0;

   // Returns the thickness of a top flange
   virtual Float64 GetTopFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the spacing between the centers of top flanges
   virtual Float64 GetTopFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns true if the top flange can be longitudinally thickened
   virtual bool CanTopFlangeBeLongitudinallyThickened(const CSegmentKey& segmentKey) const = 0;

   // Returns the top flange thickening type
   virtual pgsTypes::TopFlangeThickeningType GetTopFlangeThickeningType(const CSegmentKey& segmentKey) const = 0;

   // Returns the top flange thickening parameter
   virtual Float64 GetTopFlangeThickening(const CSegmentKey& segmentKey) const = 0;

   // Returns the thickening of the top flange. Positive values means the top flange is thicker at this
   // poi then it is at the start of the segment.
   virtual Float64 GetTopFlangeThickening(const pgsPointOfInterest& poi) const = 0;
   virtual Float64 GetTopFlangeThickening(const CSegmentKey& segmentKey,Float64 Xpoi) const = 0;

   // Returns the top flange thickening for the provided segment
   virtual Float64 GetTopFlangeThickening(const CPrecastSegmentData* pSegment, Float64 Xs) const = 0;

   // Returns the number of bottom flanges
   virtual FlangeIndexType GetNumberOfBottomFlanges(const CGirderKey& girderKey) const = 0;

   // Returns the location of the center of a bottom flange measured from the CL girder. < 0 if left of CL.
   virtual Float64 GetBottomFlangeLocation(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the width of a bottom flange
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the width of all the bottom flanges summed together
   virtual Float64 GetBottomFlangeWidth(const pgsPointOfInterest& poi) const = 0;

   // Returns the total width of the bottom of a girder
   virtual Float64 GetBottomWidth(const pgsPointOfInterest& poi) const = 0;

   // Returns the thickness of a top flange
   virtual Float64 GetBottomFlangeThickness(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the spacing between the centers of bottom flanges
   virtual Float64 GetBottomFlangeSpacing(const pgsPointOfInterest& poi,FlangeIndexType flangeIdx) const = 0;

   // Returns the minimum web width
   virtual Float64 GetMinWebWidth(const pgsPointOfInterest& poi) const = 0;

   // Returns the web thickness at the level of the specified duct
   virtual Float64 GetWebThicknessAtDuct(const pgsPointOfInterest& poi,DuctIndexType ductIdx) const = 0;

   // Returns the minimum top flange thickness
   virtual Float64 GetMinTopFlangeThickness(const pgsPointOfInterest& poi) const = 0;

   // Returns the minimum bottom flange thickness
   virtual Float64 GetMinBottomFlangeThickness(const pgsPointOfInterest& poi) const = 0;

   // Returns the height of the basic (non-composite) girder at the specified POI.
   // Interval isn't taken into account so you always get a value.
   virtual Float64 GetHeight(const pgsPointOfInterest& poi) const = 0;

   // Returns the shear width (bv for vertical shear calculations)
   virtual Float64 GetShearWidth(const pgsPointOfInterest& poi) const = 0;

   // Returns the width used for horizontal interface shear calculations (acv for horizontal shear)
   virtual Float64 GetShearInterfaceWidth(const pgsPointOfInterest& poi) const = 0;

   // Returns the number of webs
   virtual WebIndexType GetWebCount(const CGirderKey& girderKey) const = 0;

   // Returns the location of the CL web as an offset from the CL beam
	virtual Float64 GetWebLocation(const pgsPointOfInterest& poi,WebIndexType webIdx) const = 0;

   // Returns the spacing between CL webs
	virtual Float64 GetWebSpacing(const pgsPointOfInterest& poi,WebIndexType spaceIdx) const = 0;

   // Returns the thickness of the web (average thickeness for tapered webs... see IBeams)
   virtual Float64 GetWebThickness(const pgsPointOfInterest& poi,WebIndexType webIdx) const = 0;

   // Returns the horizontal distance from the CL Girder to the CL of the exterior web
   virtual Float64 GetCL2ExteriorWebDistance(const pgsPointOfInterest& poi) const = 0;

   // Returns the web width (bw for moment capacity calculations)
   virtual Float64 GetWebWidth(const pgsPointOfInterest& poi) const = 0;

   // Gets key segment points
   // pntPier1/2 - point where the CL segment intersects the CL Pier
   // pntBrg1/2 - point where the CL segment intersects the CL Bearing
   // pntEnd1/2 - location of the ends of the segments, on the CL Segment
   // 1 = start of segment
   // 2 = end of segment
   virtual void GetSegmentEndPoints(const CSegmentKey& segmentKey,pgsTypes::PlanCoordinateType pcType,IPoint2d** pntPier1,IPoint2d** pntEnd1,IPoint2d** pntBrg1,IPoint2d** pntBrg2,IPoint2d** pntEnd2,IPoint2d** pntPier2) const = 0;

   // Returns the four plan view points that define the outline of a segment
   virtual void GetSegmentPlanPoints(const CSegmentKey& segmentKey, pgsTypes::PlanCoordinateType pcType, IPoint2d** ppEnd1Left, IPoint2d** ppEnd1,IPoint2d** ppEnd1Right, IPoint2d** ppEnd2Right, IPoint2d** ppEnd2,IPoint2d** ppEnd2Left) const = 0;

   // Returns the orientation of the girder, as a slope
   // An orientation of 0 means the girder is plumb
   // Positive values means the Y axis of the girder is rotated CW
   virtual Float64 GetOrientation(const CSegmentKey& segmentKey) const = 0;

   // Returns the transverse slope of the girder top flange
   virtual Float64 GetTransverseTopFlangeSlope(const CSegmentKey& segmentKey) const = 0;

   // Top Girder Reference Chord is a straight line that intersections the top of deck at the CL Brg at start and end of girder
   virtual Float64 GetProfileChordElevation(const pgsPointOfInterest& poi) const = 0;

   // Returns the elevation along the top of girder chord (straight line along the top of the girder)
   virtual Float64 GetTopGirderChordElevation(const pgsPointOfInterest& poi) const = 0;

   // Returns the elevation along the top of girder chord defined by the provided "A" dimension values (straight line along the top of the girder)
   virtual Float64 GetTopGirderChordElevation(const pgsPointOfInterest& poi, Float64 Astart, Float64 Aend) const = 0;

   // Returns the top of girder elevation at the centerline of the specified mating surface. If pConfig is nullptr, the slab offset and excess camber from the
   // bridge model are used, otherwise the slab offset from the config is used and the excess camber is computed using the supplied configuration
   virtual Float64 GetTopGirderElevation(const pgsPointOfInterest& poi,MatingSurfaceIndexType matingSurfaceIdx,const GDRCONFIG* pConfig=nullptr) const = 0;

   // Returns the top of girder elevation for the left, center, and right edges of the girder at the specified poi. The elevation takes into
   // account slab offsets and excess camber. Direction defines a tranverse line passing through poi. Left and Right elevations are computed
   // where the transverse line intersects the edges of the girder. If pDirection is nullptr, the transverse line is taken to be normal to the girder
   virtual void GetTopGirderElevation(const pgsPointOfInterest& poi, IDirection* pDirection,Float64* pLeft, Float64* pCenter, Float64* pRight) const = 0;

   // Returns the finished top of girder elevation for the left, center, and right edges of the girder at the specified poi. The elevation takes into
   // account slab offsets and excess camber. Direction defines a tranverse line passing through poi. Left and Right elevations are computed
   // where the transverse line intersects the edges of the girder. If pDirection is nullptr, the transverse line is taken to be normal to the girder.
   // if bIncludeOverlay is true, the depth of the overlay is included (future overlays are not included), otherwise this method is the same
   // as GetTopGirderElevation
   virtual void GetFinishedElevation(const pgsPointOfInterest& poi, IDirection* pDirection, bool bIncludeOverlay, Float64* pLeft, Float64* pCenter, Float64* pRight) const = 0;

   // Returns the height of the splitting zone 
   virtual Float64 GetSplittingZoneHeight(const pgsPointOfInterest& poi) const = 0;

   // Returns the direction fo the splitting zone
   virtual pgsTypes::SplittingDirection GetSplittingDirection(const CGirderKey& girderKey) const = 0;


   // Returns true if the girder has shear keys for the provided spacing type
   virtual bool HasShearKey(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType) const = 0;

   // Area of shear key. uniform portion assumes no joint, section is per joint spacing.
   virtual void GetShearKeyAreas(const CGirderKey& girderKey,pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const = 0;

   // Returns true if the girders are connected with structural longitudinal joints
   virtual bool HasStructuralLongitudinalJoints() const = 0;

   // Returns the width of the structural joints
   virtual Float64 GetStructuralLongitudinalJointWidth(const pgsPointOfInterest& poi) const = 0;
   virtual void GetStructuralLongitudinalJointWidth(const pgsPointOfInterest& poi, Float64* pLeft, Float64* pRight) const = 0;

   // Returns the shape of the segment profile. If bIncludeClosure is true, the segment shape
   // includes its projection into the closure joint. Y=0 is at the top of the segment
   // X values are in Girder Path Coordinates.
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IShape** ppShape) const = 0;

   // Returns the shape of a the segment profile for a segment within the provided girder. If bIncludeClosure is
   // true, the segment shape includes its projection into the closure joint. Y=0 is at the top of the segment
   // X values are in Girder Path Coordinates.
   virtual void GetSegmentProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,bool bIncludeClosure,IShape** ppShape) const = 0;

   // Returns the shape of the closure joint profile
   // X values are in Girder Path Coordinates.
   // Y = 0 is at the top of the closure joint
   virtual void GetClosureJointProfile(const CClosureKey& closureKey, IShape** ppShape) const = 0;

   // Returns the height of the segment at the specified location (Xsp is in Segment Path Coordinates)
   virtual Float64 GetSegmentHeight(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64 Xsp) const = 0;

   // Returns true if the segment can be precambered
   virtual bool CanPrecamber(const CSegmentKey& segmentKey) const = 0;

   // Returns the precamber at the center of the segment
   virtual Float64 GetPrecamber(const CSegmentKey& segmentKey) const = 0;

   // Returns the precamber relative to the ends of the girder at the specified location
   virtual Float64 GetPrecamber(const pgsPointOfInterest& poi) const = 0;

   // Returns the precamber relative to the ends of the provided segment at the specified location
   virtual Float64 GetPrecamber(const CPrecastSegmentData* pSegment, Float64 Xs) const = 0;

   // Returns the precamber relative to the support locations during the specified interval at the specified location.
   virtual Float64 GetPrecamber(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const = 0;

   // Returns the slope of the girder due to precamber at the specified location
   virtual Float64 GetPrecamberSlope(const pgsPointOfInterest& poi) const = 0;

   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,bool bIncludeClosure,IPoint2dCollection** points) const = 0;
   virtual void GetSegmentBottomFlangeProfile(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,bool bIncludeClosure,IPoint2dCollection** points) const = 0;
   virtual void GetSegmentDirection(const CSegmentKey& segmentKey,IDirection** ppDirection) const = 0;

   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,Float64* pStartEndDistance,Float64* pEndEndDistance) const = 0;
   virtual void GetSegmentEndDistance(const CSegmentKey& segmentKey,const CSplicedGirderData* pSplicedGirder,Float64* pStartEndDistance,Float64* pEndEndDistance) const = 0;
   virtual void GetSegmentBearingOffset(const CSegmentKey& segmentKey,Float64* pStartBearingOffset,Float64* pEndBearingOffset) const = 0;
   virtual void GetSegmentStorageSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) const = 0;
   virtual void GetSegmentReleaseSupportLocations(const CSegmentKey& segmentKey,Float64* pDistFromLeftEnd,Float64* pDistFromRightEnd) const = 0;

   virtual const stbGirder* GetSegmentLiftingStabilityModel(const CSegmentKey& segmentKey) const = 0;
   virtual const stbLiftingStabilityProblem* GetSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey) const = 0;
   virtual const stbLiftingStabilityProblem* GetSegmentLiftingStabilityProblem(const CSegmentKey& segmentKey,const HANDLINGCONFIG& handlingConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD) const = 0;

   virtual const stbGirder* GetSegmentHaulingStabilityModel(const CSegmentKey& segmentKey) const = 0;
   virtual const stbHaulingStabilityProblem* GetSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey) const = 0;
   virtual const stbHaulingStabilityProblem* GetSegmentHaulingStabilityProblem(const CSegmentKey& segmentKey,const HANDLINGCONFIG& handlingConfig,ISegmentHaulingDesignPointsOfInterest* pPOId) const = 0;
};

/*****************************************************************************
INTERFACE
   ITendonGeometry

DESCRIPTION
   Interface for obtaining information about the geometry of spliced girder
   ducts and tendons
*****************************************************************************/
// {209DA239-91BD-464c-895F-D1398A48125C}
DEFINE_GUID(IID_ITendonGeometry, 
0x209da239, 0x91bd, 0x464c, 0x89, 0x5f, 0xd1, 0x39, 0x8a, 0x48, 0x12, 0x5c);
interface ITendonGeometry : public IUnknown
{
   // returns the number of ducts in a girder
   virtual DuctIndexType GetDuctCount(const CGirderKey& girderKey) const = 0;

   // returns the geometric centerline of a duct as a series of points. 
   // use this to plot ducts in the UI
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint2dCollection** ppPoints) const = 0;
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,IPoint3dCollection** ppPoints) const = 0;

   // returns the geometric centerline of a duct for the duct configuration given in the girder object.
   // use this to plot ducts in the UI
   virtual void GetDuctCenterline(const CGirderKey& girderKey,DuctIndexType ductIdx,const CSplicedGirderData* pSplicedGirder,IPoint2dCollection** ppPoints) const = 0;

   // returns the location of the centerline of the duct in girder section coordiantes given a location in girder coordinates
   virtual void GetDuctPoint(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IPoint2d** ppPoint) const = 0;

   // returns the location of the centerline of the duct in girder section coordiantes given a POI
   virtual void GetDuctPoint(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IPoint2d** ppPoint) const = 0;

   // returns the diameter of the duct
   virtual Float64 GetOutsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual Float64 GetInsideDiameter(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   virtual Float64 GetInsideDuctArea(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns number of strands in a duct
   virtual StrandIndexType GetTendonStrandCount(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns the area of the tendon
   virtual Float64 GetTendonArea(const CGirderKey& girderKey,IntervalIndexType intervalIdx,DuctIndexType ductIdx) const = 0;

   // get the slope of a tendon. Slope is in the form of a 3D vector. Z (along the length of the girder) 
   // is always 1.0. X is the slope in the plane of girder cross section. Y is the slope along
   // the length of the girder. Slope is rise over run so it is computed as X/Z and Y/Z, however, Z is
   // always 1.0 so X and Y give the direct value for slope
   virtual void GetTendonSlope(const pgsPointOfInterest& poi,DuctIndexType ductIdx,IVector3d** ppSlope) const = 0;

   // get the slope of a tendon
   virtual void GetTendonSlope(const CGirderKey& girderKey,Float64 Xg,DuctIndexType ductIdx,IVector3d** ppSlope) const = 0;

   // returns the minimum radius of curvature of the tendon
   virtual Float64 GetMinimumRadiusOfCurvature(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns the jacking force
   virtual Float64 GetPjack(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;
   virtual Float64 GetFpj(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns the distance from the top of the non-composite girder to the CG of the tendon
   // adjustments are made for the tendon being shifted within the duct
   virtual Float64 GetDuctOffset(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) const = 0;

   // returns the centerline length of the duct curve
   virtual Float64 GetDuctLength(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns the distance from the CG of the girder to the tendon in the specified duct
   // at the specified interval. eccentricity is based on the current section properties mode.
   // adjustments are made for the tendon being shifted within the duct
   virtual Float64 GetEccentricity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) const = 0;

   // returns the distance from the CG of the girder to the tendon in the specified duct
   // at the specified interval. eccentricity is based on the specified section properties type.
   // adjustments are made for the tendon being shifted within the duct
   virtual Float64 GetEccentricity(pgsTypes::SectionPropertyType spType,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,DuctIndexType ductIdx) const = 0;

   // returns the cumulative angular change of the tendon path between one end of the tendon and a poi
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) const = 0;

   // returns the angular change of the tendon path between two POIs
   // if poi1 is to the left of poi2, it is assumed that jacking is from the left end otherwise it is from the right end
   virtual Float64 GetAngularChange(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,DuctIndexType ductIdx) const = 0;

   // returns the end from which the PT tendon is jacked
   virtual pgsTypes::JackingEndType GetJackingEnd(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns the area of tendon on the top half of the girder. See Figure C5.7.3.4.2-3
   virtual Float64 GetAptTopHalf(const pgsPointOfInterest& poi) const = 0;

   // returns the area of tendon on the bottom half of the girder. See Figure C5.7.3.4.2-3
   virtual Float64 GetAptBottomHalf(const pgsPointOfInterest& poi) const = 0;
};

#endif // INCLUDED_IFACE_BRIDGE_H_

