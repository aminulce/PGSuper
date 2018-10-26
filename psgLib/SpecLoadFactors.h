///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#if !defined(AFX_SPECLOADFACTORS_H__41DD1D4F_BFEB_4DD3_A3D9_42B1722C359B__INCLUDED_)
#define AFX_SPECLOADFACTORS_H__41DD1D4F_BFEB_4DD3_A3D9_42B1722C359B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpecLoadFactors.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpecLoadFactors dialog

class CSpecLoadFactors : public CPropertyPage
{
	DECLARE_DYNCREATE(CSpecLoadFactors)

// Construction
public:
	CSpecLoadFactors();
	~CSpecLoadFactors();

// Dialog Data
	//{{AFX_DATA(CSpecLoadFactors)
	enum { IDD = IDD_SPEC_LOADFACTORS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSpecLoadFactors)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSpecLoadFactors)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
   afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECLOADFACTORS_H__41DD1D4F_BFEB_4DD3_A3D9_42B1722C359B__INCLUDED_)
