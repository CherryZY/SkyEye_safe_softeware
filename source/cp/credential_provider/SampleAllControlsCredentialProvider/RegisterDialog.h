#pragma once
#include "resource.h"
#include "stdafx.h"

// RegisterDialog �Ի���
// CDialogEx��CDialog������:
//		CDialogEx �����ԶԻ������ñ���ͼƬ���߱�����ɫ

class RegisterDialog : public CDialogEx
{
	DECLARE_DYNAMIC(RegisterDialog)

public:
	RegisterDialog(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~RegisterDialog();

// �Ի�������
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnEnChangeRichedit21();
};
