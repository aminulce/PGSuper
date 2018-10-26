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

// ReporterBase.h : Declaration of the CReporterBase

#pragma once

class CReporterBase
{
public:
   void SetBroker(IBroker* pBroker);
   HRESULT InitCommonReportBuilders();

   HRESULT CReporterBase::OnSpecificationChanged();

protected:
   IBroker* m_pBroker; // weak reference

   virtual CTitlePageBuilder* CreateTitlePageBuilder(LPCTSTR strReportName,bool bFullVersion=true) = 0;
   void CreateBridgeGeometryReport();
   void CreateDetailsReport();
   void CreateLoadRatingReport();
   void CreateBearingDesignReport();

   void CreateBridgeAnalysisReport();
   void CreateHaulingReport();
   void CreateLiftingReport();
   void CreateMultiGirderSpecCheckReport();
   void CreateSpecChecReport();
   void CreateDistributionFactorSummaryReport();

#if defined _DEBUG || defined _BETA_VERSION
   void CreateDistributionFactorsReport();
#endif

   void CreateStageByStageDetailsReport();
   void CreateTimeStepDetailsReport();
   void CreatePointOfInterestReport();
};