///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2017  Washington State Department of Transportation
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


// CACIParametersDlg dialog

class CACIParametersDlg : public CDialog
{
	DECLARE_DYNAMIC(CACIParametersDlg)

public:
	CACIParametersDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CACIParametersDlg();

   Float64 m_t1;
   Float64 m_fc1;
   Float64 m_fc2;
   Float64 m_A;
   Float64 m_B;

// Dialog Data
	enum { IDD = IDD_ACI_PARAMETERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void UpdateParameters();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
};
