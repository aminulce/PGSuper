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

// BoxBeamFactory2.cpp : Implementation of CBoxBeamFactory2
#include "stdafx.h"
#include <Plugins\Beams.h>

#include <Plugins\BeamFamilyCLSID.h>

#include "BoxBeamFactory2.h"
#include "BoxBeamDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactory2
HRESULT CBoxBeamFactory2::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("H1"));
   m_DimNames.push_back(_T("H2"));
   m_DimNames.push_back(_T("H3"));
   m_DimNames.push_back(_T("H4"));
   m_DimNames.push_back(_T("H5"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("W3"));
   m_DimNames.push_back(_T("W4"));
   m_DimNames.push_back(_T("F1"));
   m_DimNames.push_back(_T("F2"));
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("Jmax"));
   m_DimNames.push_back(_T("EndBlockLength"));

   m_DefaultDims.push_back(::ConvertToSysUnits( 5.5,unitMeasure::Inch)); // H1
   m_DefaultDims.push_back(::ConvertToSysUnits(16.0,unitMeasure::Inch)); // H2
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.5,unitMeasure::Inch)); // H3
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.0,unitMeasure::Inch)); // H4
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.0,unitMeasure::Inch)); // H5
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(26.0,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.375,unitMeasure::Inch));// W3
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.75,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // F1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // F2
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.0,unitMeasure::Inch)); // Jmax
   m_DefaultDims.push_back(::ConvertToSysUnits( 18.0,unitMeasure::Inch)); // end block

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Jmax
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // end block

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Jmax
   m_DimUnits[1].push_back(&unitMeasure::Inch); // end block

   return S_OK;
}

void CBoxBeamFactory2::CreateGirderSection(IBroker* pBroker,StatusItemIDType statusID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IBoxBeamSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_BoxBeamSection);
   CComPtr<IBoxBeam> beam;
   gdrsection->get_Beam(&beam);

   Float64 H1, H2, H3, H4, H5, W1, W2, W3, W4, F1, F2, C1, J, endBlockLength;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, W1, W2, W3, W4, F1, F2, C1, J, endBlockLength);

   Float64 ht = H1+H2+H3;

   beam->put_H1(H1);
   beam->put_H2(H2);
   beam->put_H3(H3);
   beam->put_H4(H4);

   if (H4>0.0)
      beam->put_H5(W4-W3); // 45 deg chamfer
   else
      beam->put_H5(0.0); // 45 deg chamfer

   beam->put_H6(W4);
   beam->put_H7(ht-H5-H4);
   beam->put_W1(W4-W3);
   beam->put_W2(W1-W4);
   beam->put_W3(W2);
   beam->put_W4(W4);
   beam->put_F1(F1);
   beam->put_F2(F2);
   beam->put_C1(C1);

   gdrsection.QueryInterface(ppSection);
}




bool CBoxBeamFactory2::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg)
{
   Float64 H1, H2, H3, H4, H5, W1, W2, W3, W4, F1, F2, C1, J, endBlockLength;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, W1, W2, W3, W4, F1, F2, C1, J, endBlockLength);

   if ( H1 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H3 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("H4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H5 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("H5 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W1 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W3 > W4 )
   {
      std::_tostringstream os;
      os << _T("W3 must be lesser or equal to W4") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("F1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("F2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("C1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C1 >= H3 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than H3") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H1+H2+H3+TOLERANCE <= H4+H5 )
   {
      std::_tostringstream os;
      os << _T("H1+H2+H3 must be greater than or equal to H4+H5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 > W2/2 )
   {
      std::_tostringstream os;
      os << _T("F1 must be less than W2/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 > H2/2 )
   {
      std::_tostringstream os;
      os << _T("F1 must be less than H2/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 > W2/2 )
   {
      std::_tostringstream os;
      os << _T("F2 must be less than W2/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 > H2/2 )
   {
      std::_tostringstream os;
      os << _T("F2 must be less than H2/2") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( J < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Maximum joint size must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CBoxBeamFactory2::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("AASHTOBoxBeamDimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CBoxBeamFactory2::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
      parent_version = pLoad->GetParentVersion();
   else
      parent_version = pLoad->GetVersion();


   IBeamFactory::Dimensions dimensions;
   std::vector<std::_tstring>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("AASHTOBoxBeamDimensions")) )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      dimensions.push_back( std::make_pair(name,value) );
   }

   if( dimVersion < 2 )
   {
      // A bug in the WBFL had F1 and F2 swapped. Version 2 fixes this
      IBeamFactory::Dimensions::iterator itdF1(dimensions.end()), itdF2(dimensions.end());
      IBeamFactory::Dimensions::iterator itd(dimensions.begin());
      IBeamFactory::Dimensions::iterator itdend(dimensions.end());
      while(itd!=itdend)
      {
         IBeamFactory::Dimension& dims = *itd;
         std::_tstring name = dims.first;

         if( name == _T("F1") )
         {
            itdF1 = itd;
         }
         else if ( name == _T("F2") )
         {
            itdF2 = itd;
         }

         itd++;
      }

      if (itdF1==dimensions.end() || itdF2==dimensions.end())
      {
         ATLASSERT(false); // should never happen F1 and F2 must be in list
      }
      else
      {
         // swap values
         Float64 F1val = itdF1->second;
         Float64 F2val = itdF2->second;

         itdF1->second = F2val;
         itdF2->second = F1val;
      }
   }


   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

Float64 CBoxBeamFactory2::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   Float64 perimeter = pSectProp->GetPerimeter(pgsPointOfInterest(segmentKey,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   Float64 solid_box_surface_area = perimeter*Lg;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 F1 = GetDimension(dimensions,_T("F1"));
   Float64 F2 = GetDimension(dimensions,_T("F2"));

   Float64 void_surface_area = Lg*( 2*(H2 - F1 - F2) + 2*(W2 - F1 - F2) + 2*sqrt(2*F1*F1) + 2*sqrt(2*F2*F2) );

   if ( bReduceForPoorlyVentilatedVoids )
      void_surface_area *= 0.50;

   Float64 surface_area = solid_box_surface_area + void_surface_area;

   return surface_area;
}

void CBoxBeamFactory2::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));

   Float64 width = W1-W4;
   Float64 depth = H1+H2+H3;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   Float64 hook_offset = (W2 + width)/2.0;

   CComPtr<IPoint2d> lft_hook, rgt_hook;
   lft_hook.CoCreateInstance(CLSID_Point2d);
   rgt_hook.CoCreateInstance(CLSID_Point2d);

   lft_hook->Move(-hook_offset, -depth/2.0);
   rgt_hook->Move( hook_offset, -depth/2.0);

   lft_harp_rect->putref_HookPoint(lft_hook);
   rgt_harp_rect->putref_HookPoint(rgt_hook);

   CComPtr<IShape> lft_shape, rgt_shape;
   lft_harp_rect->get_Shape(&lft_shape);
   rgt_harp_rect->get_Shape(&rgt_shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(lft_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

std::_tstring CBoxBeamFactory2::GetImage()
{
   return std::_tstring(_T("BoxBeam2.gif"));
}


CLSID CBoxBeamFactory2::GetCLSID()
{
   return CLSID_BoxBeamFactory2;
}

LPCTSTR CBoxBeamFactory2::GetImageResourceName()
{
   return _T("BOXBEAM2");
}

HICON  CBoxBeamFactory2::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BOXBEAM2) );
}

void CBoxBeamFactory2::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                    Float64& H1, 
                                    Float64& H2, 
                                    Float64& H3, 
                                    Float64& H4, 
                                    Float64& H5,
                                    Float64& W1, 
                                    Float64& W2, 
                                    Float64& W3, 
                                    Float64& W4, 
                                    Float64& F1, 
                                    Float64& F2, 
                                    Float64& C1,
                                    Float64& J,
                                    Float64& endBlockLength)
{
   H1 = GetDimension(dimensions,_T("H1"));
   H2 = GetDimension(dimensions,_T("H2"));
   H3 = GetDimension(dimensions,_T("H3"));
   H4 = GetDimension(dimensions,_T("H4"));
   H5 = GetDimension(dimensions,_T("H5"));
   W1 = GetDimension(dimensions,_T("W1"));
   W2 = GetDimension(dimensions,_T("W2"));
   W3 = GetDimension(dimensions,_T("W3"));
   W4 = GetDimension(dimensions,_T("W4"));
   F1 = GetDimension(dimensions,_T("F1"));
   F2 = GetDimension(dimensions,_T("F2"));
   C1 = GetDimension(dimensions,_T("C1"));
   J  = GetDimension(dimensions,_T("Jmax"));
   endBlockLength  = GetDimension(dimensions,_T("EndBlockLength"));
}



void CBoxBeamFactory2::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 gw = GetBeamWidth(dimensions, pgsTypes::metStart);

   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
      {
         *minSpacing = gw;
         *maxSpacing = MAX_GIRDER_SPACING;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
   else
   {
      if (sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         Float64 J  = GetDimension(dimensions,_T("Jmax"));

         *minSpacing = gw;
         *maxSpacing = gw+J;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
}

bool CBoxBeamFactory2::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CBoxBeamFactory2::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

Float64 CBoxBeamFactory2::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));

   return W2 + 2*W1; 
}
