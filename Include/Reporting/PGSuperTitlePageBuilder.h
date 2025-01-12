///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2023  Washington State Department of Transportation
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

#pragma once

#include <Reporting\ReportingExp.h>
#include <ReportManager\TitlePageBuilder.h>
#include <WBFLCore.h>

class REPORTINGCLASS CPGSuperTitlePageBuilder :
   public CTitlePageBuilder
{
public:
   CPGSuperTitlePageBuilder(IBroker* pBroker,LPCTSTR strTitle,bool bFullVersion=true, bool bPageBreakAfter=true);
   CPGSuperTitlePageBuilder(const CPGSuperTitlePageBuilder& other);
   ~CPGSuperTitlePageBuilder(void);

   virtual rptChapter* Build(std::shared_ptr<CReportSpecification>& pRptSpec);
   virtual bool NeedsUpdate(CReportHint* pHint,std::shared_ptr<CReportSpecification>& pRptSpec);

   virtual CTitlePageBuilder* Clone() const;

protected:
   CComPtr<IBroker> m_pBroker;
   bool m_bFullVersion;
   bool m_bPageBreakAfter;
};
