///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_GIRDERLIBRARYENTRY_H_
#define INCLUDED_GIRDERLIBRARYENTRY_H_

// SYSTEM INCLUDES
//
#include <map>
#include <WBFLTools.h>
#include <WBFLGeometry.h>

// PROJECT INCLUDES
//
#include <PGSuperTypes.h>

#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

#include <GeometricPrimitives\GeometricPrimitives.h>

#include <System\SubjectT.h>

#include <MathEx.h>

#include <Material\Rebar.h>

#include <psgLib\ShearData.h>
// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

class CGirderMainSheet;
class GirderLibraryEntry;
class GirderLibraryEntryObserver;

PSGLIBTPL sysSubjectT<GirderLibraryEntryObserver, GirderLibraryEntry>;

interface IStrandGrid;
interface IBeamFactory;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   GirderLibraryEntryObserver

   A pure virtual entry class for observing Girder material entries.


DESCRIPTION
   This class may be used to describe observe Girder  materials in a library.


COPYRIGHT
   Copyright � 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS GirderLibraryEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(GirderLibraryEntry* pSubject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   GirderLibraryEntry

   A library entry class for girder templates


DESCRIPTION
   This class may be used to describe girder templates


COPYRIGHT
   Copyright � 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class CClassFactoryHolder
{
public:
   CClassFactoryHolder(IClassFactory* factory)
   {
      m_ClassFactory = factory;
      m_ClassFactory->LockServer(TRUE);
   }

   CClassFactoryHolder(const CClassFactoryHolder& rother) 
   {
      CComPtr<IClassFactory> pholder(m_ClassFactory);

      m_ClassFactory = rother.m_ClassFactory;
      m_ClassFactory->LockServer(TRUE);

      if (pholder)
         pholder->LockServer(FALSE);
   }


   ~CClassFactoryHolder()
   {
      //if ( m_ClassFactory )
      //   m_ClassFactory->LockServer(FALSE);

      //m_ClassFactory.Release();
   }

   HRESULT CreateInstance(IUnknown* pUnkOuter,REFIID riid,void** ppvObject)
   {
      return m_ClassFactory->CreateInstance(pUnkOuter,riid,ppvObject);
   }

private:
   CComPtr<IClassFactory> m_ClassFactory;
};

class PSGLIBCLASS GirderLibraryEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<GirderLibraryEntryObserver, GirderLibraryEntry>
{
public:
   typedef std::map<std::_tstring,CClassFactoryHolder> ClassFactoryCollection;
   static ClassFactoryCollection ms_ClassFactories;

   // the dialog is our friend.
   friend CGirderMainSheet;

   enum psStrandType { stStraight, stHarped };

   // describes from where something is location is measured
   enum MeasurementLocation { mlEndOfGirder        = 0, 
                              mlBearing            = 1,
                              mlCenterlineOfGirder = 2
   };

   // describes how a measurement is made
   enum MeasurementType {  mtFractionOfSpanLength   = 0,
                           mtFractionOfGirderLength = 1,
                           mtAbsoluteDistance       = 2
   };

   typedef std::pair<std::_tstring,Float64> Dimension;
   typedef std::vector<Dimension> Dimensions;

   //------------------------------------------------------------------------
   // information about rows of longitudinal steel
   struct LongSteelInfo
   {
      pgsTypes::RebarLayoutType BarLayout;
      Float64 DistFromEnd; // Only applicable to blFromLeft, blFromRight
      Float64 BarLength; //   Applicable to blFromLeft, blFromRight, blMidGirder

      pgsTypes::GirderFace  Face;
      matRebar::Size BarSize;
      CollectionIndexType NumberOfBars;
      Float64     Cover;
      Float64     BarSpacing;
      bool operator==(const LongSteelInfo& rOther) const
      {return BarLayout    == rOther.BarLayout   &&
              DistFromEnd  == rOther.DistFromEnd &&
              BarLength    == rOther.BarLength   &&
              Face         == rOther.Face        &&
              BarSize      == rOther.BarSize     &&
              Cover        == rOther.Cover       &&
              BarSpacing   == rOther.BarSpacing  &&
              NumberOfBars == rOther.NumberOfBars;} 

   };
   typedef std::vector<LongSteelInfo> LongSteelInfoVec;

   //------------------------------------------------------------------------
   // diaphragm layout rule definition
   enum DiaphragmType { dtExternal, dtInternal };
   enum ConstructionType { ctCastingYard, ctBridgeSite };
   enum DiaphragmWeightMethod { dwmCompute, dwmInput };
   struct DiaphragmLayoutRule
   {
      std::_tstring Description;
      Float64 Weight;
      DiaphragmWeightMethod Method;
      Float64 MinSpan;
      Float64 MaxSpan;
      Float64 Height;
      Float64 Thickness;
      DiaphragmType Type;
      ConstructionType Construction;
      MeasurementType MeasureType;
      MeasurementLocation MeasureLocation;
      Float64 Location;

      DiaphragmLayoutRule() :
      Description(_T("New Rule"))
      {
         MinSpan = 0;
         MaxSpan = 0;
         Height = 0;
         Thickness = 0;
         Type = dtExternal;
         Construction = ctBridgeSite;
         MeasureType = mtFractionOfSpanLength;
         MeasureLocation = mlBearing;
         Location = 0.5;
         Weight = 0;
         Method = dwmCompute;
      };

      bool operator==(const DiaphragmLayoutRule& rOther) const
      {
         if ( Description != rOther.Description )
            return false;

         if ( !::IsEqual(MinSpan,rOther.MinSpan) )
            return false;

         if ( !::IsEqual(MaxSpan,rOther.MaxSpan) )
            return false;

         if ( Type != rOther.Type )
            return false;

         if ( Construction != rOther.Construction )
            return false;

         if ( MeasureType != rOther.MeasureType )
            return false;

         if ( MeasureLocation != rOther.MeasureLocation )
            return false;

         if ( !::IsEqual(Location,rOther.Location) )
            return false;

         if ( Method != rOther.Method )
            return false;

         if ( Method == dwmCompute )
         {
            if ( !::IsEqual(Height,rOther.Height) )
               return false;

            if ( !::IsEqual(Thickness,rOther.Thickness) )
               return false;
         }
         else
         {
            if ( !::IsEqual(Weight,rOther.Weight) )
               return false;
         }

         return true;
      }
   };
   typedef std::vector<DiaphragmLayoutRule> DiaphragmLayoutRules;

   //------------------------------------------------------------------------
   // a base class for types of errors that can occur when the entry data
   // is being verified.
   enum ErrorType { GirderHeightIsZero,
                    BottomFlangeWidthIsZero,
                    TopFlangeWidthIsZero,
                    StraightStrandOutsideOfGirder,
                    HarpedStrandOutsideOfGirder,
                    TemporaryStrandOutsideOfGirder,
                    NumberOfHarpedStrands,
                    BundleAdjustmentGtIncrement,
                    BundleAdjustmentTooBig,
                    //NumberOfStrandsInBottomBundleIsZero,
                    EndAdjustmentGtIncrement,
                    EndAdjustmentTooBig,
                    //TopBundleBelowBottomBundle,
                    //BundleOutsideOfGirder,
                    ZoneLengthIsZero,
                    StirrupSpacingIsZero,
                    ConfinementZoneIsInvalid,
                    LiftingLoopRangeLengthTooLong,
                    BarCoverIsZero,
                    NumberOfBarsIsZero,
                    BarSpacingIsZero,
                    InteriorDiaphragmTooWide,
                    LongitudinalRebarOutsideOfGirder,
                    TopFlangeBarSpacingIsZero
   };

   class GirderEntryDataError
   {
   public:
      GirderEntryDataError(ErrorType type, const std::_tstring& msg, IndexType index=0)
         : m_Type(type), m_ErrorInfo(index), m_Msg(msg) {}
      ErrorType GetErrorType() const                     {return m_Type;}
      IndexType GetErrorInfo() const                         {return m_ErrorInfo;}
      std::_tstring GetErrorMsg() const                    {return m_Msg;}
   private:
      ErrorType   m_Type;
      IndexType       m_ErrorInfo;  // index of offending thing (if applicable).
      std::_tstring m_Msg;      // a default error message
   };

   typedef std::vector<GirderEntryDataError> GirderEntryDataErrorVec;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   GirderLibraryEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   GirderLibraryEntry(const GirderLibraryEntry& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~GirderLibraryEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   GirderLibraryEntry& operator = (const GirderLibraryEntry& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(bool allowEditing);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(sysIStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(sysIStructuredLoad* pLoad);

   //------------------------------------------------------------------------
   // Check entry for errors and return a vector of errors
   void ValidateData(GirderEntryDataErrorVec* pvec);

   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

    // GROUP: ACCESS
   //------------------------------------------------------------------------
   void SetBeamFactory(IBeamFactory* pFactory);
   void GetBeamFactory(IBeamFactory** ppFactory) const;

   std::_tstring GetGirderFamilyName() const;

   //------------------------------------------------------------------------
   const Dimensions& GetDimensions() const;
   Float64 GetDimension(const std::_tstring& name) const;
   void SetDimension(const std::_tstring& name,Float64 value,bool bAdjustStrands);

   //------------------------------------------------------------------------
   // Remove all strands
   void ClearAllStrands();

   //------------------------------------------------------------------------
   enum PermanentStrandType {ptNone, ptHarped, ptStraight};
   std::vector<PermanentStrandType> GetPermanentStrands() const;

   //------------------------------------------------------------------------
   // Get number of Coordinates for Straight strands 
   // This sets the upper bound for GetStraightStrandCoordinates
   StrandIndexType GetNumStraightStrandCoordinates() const;

   //------------------------------------------------------------------------
   // Locations for Straight strands
   // Note that if the X location is greater than zero at either location,
   // two strands are located at +/- X. 
   // If X is zero at both locations, only one strand is located.
   // Y is measured from the bottom of the girder
   void GetStraightStrandCoordinates(GridIndexType ssGridIdx, Float64* Xstart, Float64* Ystart, Float64* Xend, Float64* Yend, bool* canDebond) const;

   //------------------------------------------------------------------------
   // Adds a strand position. Note that if the X location is greater than zero then two
   // strands at +/- X are created. Y is measured from the bottom of the girder
   // Returns the strand grid index for the new strand position
   GridIndexType AddStraightStrandCoordinates(Float64 Xstart, Float64 Ystart, Float64 Xend, Float64 Yend, bool canDebond);

   //------------------------------------------------------------------------
   // Returns the maximum number of straight strands that can be used
   // in this girder type.
   StrandIndexType GetMaxStraightStrands() const;

   //------------------------------------------------------------------------
   // Allow non-paralell harped strand pattern
   // e.g., Use Different Harped Grid At Ends
   void UseDifferentHarpedGridAtEnds(bool d);
   bool IsDifferentHarpedGridAtEndsUsed() const;

   //------------------------------------------------------------------------
   // Get number of Coordinates for Harped strands 
   // This sets the upper bound for GetHarpedStrandCoordinates
   StrandIndexType GetNumHarpedStrandCoordinates() const;

   //------------------------------------------------------------------------
   // Get locations for Harped strands at girder end and harping point
   // Note that if the X location is greater than zero at either location,
   // two strands are located at +/- X. 
   // If X is zero at both locations, only one strand is located.
   // Y is measured from the bottom of the girder
   void GetHarpedStrandCoordinates(GridIndexType hsGridIdx, Float64* Xstart,Float64* Ystart, Float64* Xhp, Float64* Yhp,Float64* Xend, Float64* Yend) const;

   //------------------------------------------------------------------------
   // Adds a strand position. Note that if the X location is greater than zero then two
   // strands at +/- X are created. Y is measured from the bottom of the girder
   // Returns the strand grid index for the new strand position
   GridIndexType AddHarpedStrandCoordinates(Float64 Xstart,Float64 Ystart, Float64 Xhp, Float64 Yhp,Float64 Xend, Float64 Yend);

   //------------------------------------------------------------------------
   // Returns the maximum number of harped strands that can be used
   // in this girder type.
   // This value is computed taking account for +/- X values
   StrandIndexType GetMaxHarpedStrands() const;

   //------------------------------------------------------------------------
   // Get number of Coordinates for Temporary strands 
   // This sets the upper bound for GetTemporaryStrandCoordinates
   StrandIndexType GetNumTemporaryStrandCoordinates() const;

   //------------------------------------------------------------------------
   // Get locations for Temporary strands
   // Note that if the X location is greater than zero at either location,
   // two strands are located at +/- X. 
   // If X is zero at both locations, only one strand is located.
   // Y is measured from the bottom of the girder
   void GetTemporaryStrandCoordinates(GridIndexType tsGridIdx, Float64* Xstart, Float64* Ystart, Float64* Xend, Float64* Yend) const;

   //------------------------------------------------------------------------
   // Adds a strand position. Note that if the X location is greater than zero then two
   // strands at +/- X are created. Y is measured from the bottom of the girder
   // Returns the strand grid index for the new strand position
   GridIndexType AddTemporaryStrandCoordinates(Float64 Xstart, Float64 Ystart, Float64 Xend, Float64 Yend);

   //------------------------------------------------------------------------
   // Returns the maximum number of Temporary strands that can be used
   // in this girder type.
   StrandIndexType GetMaxTemporaryStrands() const;

   //------------------------------------------------------------------------
   // Permanent straight strands and harped strands can be considered to be
   // in a single permanent strand grid. This grid defines the permanent
   // strand fill order so users can input "total number of strands".
   // This method returns the size of the permanent strand grid.
   GridIndexType GetPermanentStrandGridSize() const;

   //------------------------------------------------------------------------
   // Given a position in the permanent strand grid, returns the equivalent
   // strand type (straight/harped) and position in the local strand grid.
   void GetGridPositionFromPermStrandGrid(GridIndexType permStrandGridIdx, psStrandType* type, GridIndexType* gridIdx) const;

   //------------------------------------------------------------------------
   // Adds a strand to the permanent strand grid. The strand is defined by its type and position
   // in a straight or hapred strand grid. The position in the permanent strand grid is returned.
   // Use this method when you are programatically creating a strand grid (See TxDOT TOGA for example)
   GridIndexType AddStrandToPermStrandGrid(psStrandType type,  GridIndexType gridIdx);

   //------------------------------------------------------------------------
   // Given a total number of permanent strands, the number of straight and harped strands 
   // is determined. Returns true if the total number of strands can be distributed into
   // straight and harped strans based on the definition of the straight and harped strand
   // grids, otherwise returns false.
   bool GetPermStrandDistribution(StrandIndexType totalNumStrands, StrandIndexType* numStraight, StrandIndexType* numHarped) const;

   //------------------------------------------------------------------------
   // Check if the number of strands for each type are valid
   bool IsValidNumberOfStraightStrands(StrandIndexType ns) const;
   bool IsValidNumberOfHarpedStrands(StrandIndexType ns) const;
   bool IsValidNumberOfTemporaryStrands(StrandIndexType ns) const;

   //------------------------------------------------------------------------
   // Allow adjustment of harped strands at girder end
   void AllowVerticalAdjustmentEnd(bool d);
   bool IsVerticalAdjustmentAllowedEnd() const;

   //------------------------------------------------------------------------
   // Allow adjustment of harped strands at harping points
   void AllowVerticalAdjustmentHP(bool d);
   bool IsVerticalAdjustmentAllowedHP() const;

   //------------------------------------------------------------------------
   // Set/get the harped strand adjustment limits at ends and harping points
   void SetHPAdjustmentLimits(pgsTypes::GirderFace  topFace, Float64  topLimit, pgsTypes::GirderFace  bottomFace, Float64  bottomLimit);
   void GetHPAdjustmentLimits(pgsTypes::GirderFace* topFace, Float64* topLimit, pgsTypes::GirderFace* bottomFace, Float64* bottomLimit) const;

   void SetEndAdjustmentLimits(pgsTypes::GirderFace  topFace, Float64  topLimit, pgsTypes::GirderFace  bottomFace, Float64  bottomLimit);
   void GetEndAdjustmentLimits(pgsTypes::GirderFace* topFace, Float64* topLimit, pgsTypes::GirderFace* bottomFace, Float64* bottomLimit) const;

   //------------------------------------------------------------------------
   // Set the max downward strand increment for design at girder end
   void SetEndStrandIncrement(Float64 d);

   //------------------------------------------------------------------------
   // Get the max downward strand increment at girder end
   Float64 GetEndStrandIncrement() const;

   //------------------------------------------------------------------------
   // Set the max Upward strand increment for design at harping point
   void SetHPStrandIncrement(Float64 d);

   //------------------------------------------------------------------------
   // Get the max Upward strand increment at harping point
   Float64 GetHPStrandIncrement() const;

   //------------------------------------------------------------------------
   // Set/Get shear data struct
   void SetShearData(const CShearData2& cdata);
   const CShearData2& GetShearData() const;

   //------------------------------------------------------------------------
   // Set vector of longitidinal steel information.
   void SetLongSteelInfo(const LongSteelInfoVec& vec);

   //------------------------------------------------------------------------
   // Get vector of longitidinal steel information.
   LongSteelInfoVec GetLongSteelInfo() const;

   //------------------------------------------------------------------------
   // Set material for Long steel
   void SetLongSteelMaterial(matRebar::Type type,matRebar::Grade grade);

   //------------------------------------------------------------------------
   // Get material name for Long steel
   void GetLongSteelMaterial(matRebar::Type& type,matRebar::Grade& grade) const;

   //------------------------------------------------------------------------
   // Set the location of the harping point as a ratio of span length
   void SetHarpingPointLocation(Float64 d);

   //------------------------------------------------------------------------
   // Get the location of the harping point as a ratio of span length
   Float64 GetHarpingPointLocation() const;

   void SetMinHarpingPointLocation(bool bUseMin,Float64 min);
   bool IsMinHarpingPointLocationUsed() const;
   Float64 GetMinHarpingPointLocation() const;

   void SetHarpingPointReference(MeasurementLocation reference);
   MeasurementLocation GetHarpingPointReference() const;

   void SetHarpingPointMeasure(MeasurementType measure);
   MeasurementType GetHarpingPointMeasure() const;

   Float64 GetBeamHeight(pgsTypes::MemberEndType endType) const;
   Float64 GetBeamWidth(pgsTypes::MemberEndType endType) const;

   bool OddNumberOfHarpedStrands() const;
   void EnableOddNumberOfHarpedStrands(bool bEnable);

   bool IsForceHarpedStrandsStraight() const;
   void ForceHarpedStrandsStraight(bool bEnable);

   void ConfigureStraightStrandGrid(Float64 HgStart,Float64 HgEnd,IStrandGrid* pStartGrid,IStrandGrid* pEndGrid) const;
   void ConfigureHarpedStrandGrids(Float64 HgStart,Float64 HgHP1,Float64 HgHP2,Float64 HgEnd,IStrandGrid* pEndGridAtStart, IStrandGrid* pHPGridAtStart, IStrandGrid* pHPGridAtEnd, IStrandGrid* pEndGridAtEnd) const;
   void ConfigureTemporaryStrandGrid(Float64 HgStart,Float64 HgEnd,IStrandGrid* pStartGrid,IStrandGrid* pEndGrid) const;

   bool CanDebondStraightStrands() const;
   void CanDebondStraightStrands(bool bCanDebond);

   // Limits and criteria for debonding
   // Most of these were taken from the SpecLibraryEntry for Version 13
   void SetMaxTotalFractionDebondedStrands(Float64 fraction);
   Float64 GetMaxTotalFractionDebondedStrands() const;

   void SetMaxFractionDebondedStrandsPerRow(Float64 fraction);
   Float64 GetMaxFractionDebondedStrandsPerRow() const;

   void SetMaxDebondedStrandsPerSection(StrandIndexType number, Float64 fraction);
   void GetMaxDebondedStrandsPerSection(StrandIndexType* pNumber, Float64* pFraction) const;

   void  SetMaxDebondedLength(bool useSpanFraction, Float64 spanFraction, bool useHardDistance, Float64 hardDistance);
   void  GetMaxDebondedLength(bool* pUseSpanFraction, Float64* pSpanFraction, bool* pUseHardDistance, Float64* pHardDistance)const;

   void SetMinDebondSectionLength(Float64 fraction);
   Float64 GetMinDebondSectionLength() const;

   Float64 GetDefaultDebondSectionLength() const;
   void SetDefaultDebondSectionLength(Float64 l);

   //------------------------------------------------------------------------
   void SetDiaphragmLayoutRules(const DiaphragmLayoutRules& rules);
   const DiaphragmLayoutRules& GetDiaphragmLayoutRules() const;

   //------------------------------------------------------------------------
   // Equality - test if two entries are equal. Ignore names by default
   bool IsEqual(const GirderLibraryEntry& rOther, bool considerName=false) const;

   //------------------------------------------------------------------------
   // Get name of section from factory
   std::_tstring GetSectionName() const;

   //------------------------------------------------------------------------
   // Data for Shear Design Algorithm
   //------------------------------------------------------------------------
   // Available bars for design
   IndexType GetNumStirrupSizeBarCombos() const;
   void ClearStirrupSizeBarCombos();
   void GetStirrupSizeBarCombo(IndexType index, matRebar::Size* pSize, Float64* pNLegs) const;
   void AddStirrupSizeBarCombo(matRebar::Size Size, Float64 NLegs);

   // Available bar spacings for design
   IndexType GetNumAvailableBarSpacings() const;
   void ClearAvailableBarSpacings();
   Float64 GetAvailableBarSpacing(IndexType index) const;
   void AddAvailableBarSpacing(Float64 Spacing);

   // Max change in spacing between zones
   Float64 GetMaxSpacingChangeInZone() const;
   void SetMaxSpacingChangeInZone(Float64 Change);

   // Max change in shear capacity between zones (% fraction)
   Float64 GetMaxShearCapacityChangeInZone() const;
   void SetMaxShearCapacityChangeInZone(Float64 Change);

   void GetMinZoneLength(Uint32* pSpacings, Float64* pLength) const;
   void SetMinZoneLength(Uint32 Spacings, Float64 Length);

   bool GetExtendBarsIntoDeck() const;
   void SetExtendBarsIntoDeck(bool isTrue);

   bool GetBarsActAsConfinement() const;
   void SetBarsActAsConfinement(bool isTrue);

   enum LongShearCapacityIncreaseMethod { isAddingRebar, isAddingStrands };

   LongShearCapacityIncreaseMethod GetLongShearCapacityIncreaseMethod() const;
   void SetLongShearCapacityIncreaseMethod(LongShearCapacityIncreaseMethod method);

protected:
   void MakeCopy(const GirderLibraryEntry& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const GirderLibraryEntry& rOther);
  // GROUP: ACCESS
  // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   CComPtr<IBeamFactory> m_pBeamFactory;
   Dimensions m_Dimensions;

   bool m_bUseDifferentHarpedGridAtEnds;

   // grade and type for extra top flange bars
   matRebar::Type m_LongitudinalBarType;
   matRebar::Grade m_LongitudinalBarGrade;
   Float64 m_HarpingPointLocation;
   Float64 m_MinHarpingPointLocation;
   bool m_bMinHarpingPointLocation;
   MeasurementLocation m_HarpPointReference;
   MeasurementType   m_HarpPointMeasure;
   bool m_bOddNumberOfHarpedStrands;
   bool m_bForceHarpedStrandsStraight;

   // version 13
   // debond limits
   Float64 m_MaxDebondStrands;
   Float64 m_MaxDebondStrandsPerRow;
   StrandIndexType  m_MaxNumDebondedStrandsPerSection;
   Float64 m_MaxDebondedStrandsPerSection;
   Float64 m_MinDebondLength;
   Float64 m_DefaultDebondLength;

   Float64 m_MaxDebondLengthBySpanFraction; // if negative, value not used
   Float64 m_MaxDebondLengthByHardDistance; // if negative, value not used

   // version 12 
   // Added individual debondablity to straight strands
   // can be used for straight or temporary strands (temporary strands are straight!)
   struct StraightStrandLocation
   {
      Float64 m_Xstart;  // measured from CL of girder
      Float64 m_Ystart;  // measured from bottom of girder
      Float64 m_Xend;  // measured from CL of girder
      Float64 m_Yend;  // measured from bottom of girder

      bool    m_bCanDebond; // ignored for temporary strands

      StraightStrandLocation():
         m_Xstart(-1), m_Ystart(-1), m_Xend(-1), m_Yend(-1), m_bCanDebond(false)
         {;}

      StraightStrandLocation(Float64 X, Float64 Y, bool canDebond):
         m_Xstart(X), m_Ystart(Y), m_Xend(X), m_Yend(Y), m_bCanDebond(canDebond)
         {;}

      bool operator==(const StraightStrandLocation& rOther) const
      {return m_Xstart==rOther.m_Xstart && m_Ystart==rOther.m_Ystart && m_Xend==rOther.m_Xend && m_Yend==rOther.m_Yend && m_bCanDebond==rOther.m_bCanDebond;}
   };

   typedef std::vector<StraightStrandLocation>       StraightStrandCollection;
   typedef StraightStrandCollection::iterator        StraightStrandIterator;
   typedef StraightStrandCollection::const_iterator  ConstStraightStrandIterator;

   StraightStrandCollection m_StraightStrands;
   StraightStrandCollection m_TemporaryStrands;

   LongSteelInfoVec m_LongSteelInfo;

   // version 4.0 
   // Got rid of two separate collections of harped strands and consolidate into one.
   struct HarpedStrandLocation
   {
      Float64 m_Xstart; // measured from CL girder
      Float64 m_Ystart; // measured from bottom of girder
      Float64 m_Xhp;    // measured from CL girder
      Float64 m_Yhp;    // measured from bottom of girder
      Float64 m_Xend;   // measured from CL girder
      Float64 m_Yend;   // measured from bottom of girder

      HarpedStrandLocation():
         m_Xstart(-1), m_Ystart(-1), m_Xhp(-1), m_Yhp(-1), m_Xend(-1), m_Yend(-1)
         {;}

      HarpedStrandLocation(Float64 startX,Float64 startY,Float64 hpX, Float64 hpY, Float64 endX, Float64 endY):
         m_Xhp(hpX), m_Yhp(hpY), m_Xstart(startX), m_Ystart(startY), m_Xend(endX), m_Yend(endY)
         {;}

      bool operator==(const HarpedStrandLocation& rOther) const
      {
         return m_Xhp    == rOther.m_Xhp    && 
                m_Yhp    == rOther.m_Yhp    && 
                m_Xstart == rOther.m_Xstart && 
                m_Ystart == rOther.m_Ystart &&
                m_Xend   == rOther.m_Xend   && 
                m_Yend   == rOther.m_Yend;
      }
   };

   typedef std::vector<HarpedStrandLocation>       HarpedStrandCollection;
   typedef HarpedStrandCollection::iterator        HarpedStrandIterator;
   typedef HarpedStrandCollection::const_iterator  ConstHarpedStrandIterator;

   HarpedStrandCollection m_HarpedStrands;

   // Added permanent strand ordering in Vers 4.0 for TxDOT
   struct PermanentStrand
   {
      PermanentStrand(): m_StrandType(stStraight), m_GridIdx(INVALID_INDEX)
      {;}
      PermanentStrand( psStrandType type, GridIndexType gridIdx): m_StrandType(type), m_GridIdx(gridIdx)
      {;}
      bool operator==(const PermanentStrand& rOther) const
      {return m_GridIdx==rOther.m_GridIdx && m_StrandType==rOther.m_StrandType;} 

      GridIndexType m_GridIdx; // index in the straight or hapred strand grid
      psStrandType m_StrandType; // defines the strand grid type (straight or harped)
   };

   typedef std::vector<PermanentStrand> PermanentStrandCollection;
   PermanentStrandCollection m_PermanentStrands;

   // Diaphragms
   DiaphragmLayoutRules m_DiaphragmLayoutRules;

	// Harped strand vertical adjustment
   struct HarpedStrandAdjustment
   {
      bool       m_AllowVertAdjustment;
      Float64    m_StrandIncrement;
      pgsTypes::GirderFace m_TopFace;
      Float64    m_TopLimit;
      pgsTypes::GirderFace m_BottomFace;
      Float64    m_BottomLimit;

      HarpedStrandAdjustment();

      bool operator==(const HarpedStrandAdjustment& rOther) const
      {  
         if (m_AllowVertAdjustment==rOther.m_AllowVertAdjustment)
         {
            // only test adjustment values if adjustment is allowed
            if (m_AllowVertAdjustment)
            {
               return m_StrandIncrement==rOther.m_StrandIncrement &&
                      m_TopFace==rOther.m_TopFace &&
                      m_TopLimit==rOther.m_TopLimit &&
                      m_BottomFace==rOther.m_BottomFace &&
                      m_BottomLimit==rOther.m_BottomLimit;
            }
            else
               return true;
         }
         else
         {
            return false;
         }
      } 
   };

	HarpedStrandAdjustment m_HPAdjustment;
	HarpedStrandAdjustment m_EndAdjustment;

   //------------------------------------------------------------------------
   // Our main shear reinforcement data
   CShearData2 m_ShearData;

   //------------------------------------------------------------------------
   // LegacyShearData
   // This class contains shear data from prior to version 19.0
   // CShearData is now used for all shear information and is used both in the 
   // library classes and in PGSuper's GirderData struct.
   class LegacyShearData
   {
   public:
      LegacyShearData():
         m_StirrupBarType(matRebar::A615),
         m_StirrupBarGrade(matRebar::Grade60),
         m_ConfinementBarSize(matRebar::bsNone),
         m_LastConfinementZone(0),
         m_TopFlangeShearBarSize(matRebar::bsNone),
         m_TopFlangeShearBarSpacing(0.0),
         m_bStirrupsEngageDeck(true),
         m_bIsRoughenedSurface(true)
         {;}

      // Conversion function to ShearData
      CShearData2 ConvertToShearData() const;

      // grade and type for all stirrups, confinement, and extra top flange bars
      matRebar::Type  m_StirrupBarType;
      matRebar::Grade m_StirrupBarGrade;
      matRebar::Size  m_ConfinementBarSize; 
      matRebar::Size  m_TopFlangeShearBarSize;
      Uint16          m_LastConfinementZone;
      bool            m_bStirrupsEngageDeck;
      bool            m_bIsRoughenedSurface;
      Float64         m_TopFlangeShearBarSpacing;

      // information about shear zones
      struct ShearZoneInfo
      {
         Float64     ZoneLength;
         matRebar::Size VertBarSize, HorzBarSize;
         Float64     StirrupSpacing;
         Uint32      nVertBars, nHorzBars;
         bool operator==(const ShearZoneInfo& rOther) const
         {return ZoneLength     == rOther.ZoneLength     &&
                 VertBarSize    == rOther.VertBarSize    &&
                 HorzBarSize    == rOther.HorzBarSize    &&
                 StirrupSpacing == rOther.StirrupSpacing &&
                 nVertBars      == rOther.nVertBars      &&
                 nHorzBars      == rOther.nHorzBars;
         } 
      };

      typedef std::vector<ShearZoneInfo> ShearZoneInfoVec;
      ShearZoneInfoVec m_ShearZoneInfo;
   };

   // Data Members for Shear Design Algorithm
   struct StirrupSizeBarCombo
   {
      matRebar::Size Size;
      Float64 NLegs;

      bool operator==(const StirrupSizeBarCombo& rOther) const
      {
         if (!::IsEqual(NLegs, rOther.NLegs))
            return false;

         return Size==rOther.Size;
      }
   };

   typedef std::vector<StirrupSizeBarCombo> StirrupSizeBarComboColl;
   typedef StirrupSizeBarComboColl::iterator StirrupSizeBarComboIter;

   StirrupSizeBarComboColl m_StirrupSizeBarComboColl;
   std::vector<Float64> m_AvailableBarSpacings;
   Float64 m_MaxSpacingChangeInZone;
   Float64 m_MaxShearCapacityChangeInZone;
   Uint32 m_MinZoneLengthSpacings;
   Float64 m_MinZoneLengthLength;
   bool m_DoExtendBarsIntoDeck;
   bool m_DoBarsActAsConfinement;
   LongShearCapacityIncreaseMethod m_LongShearCapacityIncreaseMethod;


   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   bool IsEqual(IPoint2d* p1,IPoint2d* p2) const;
   bool IsEqual(IPoint2dCollection* points1,IPoint2dCollection* points2) const;

   void AddDimension(const std::_tstring& name,Float64 value);

   HRESULT CreateBeamFactory(const std::_tstring& strCLSID);
   void LoadIBeamDimensions(sysIStructuredLoad* pLoad);

   // GROUP: ACCESS
   // GROUP: INQUIRY

   std::map<std::_tstring,std::_tstring> m_CLSIDMap; // maps old PGSuper CLSIDs to new BridgeLink CLSIDs
   void InitCLSIDMap();
   std::_tstring TranslateCLSID(const std::_tstring& strCLSID);
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_GIRDERLIBRARYENTRY_H_
