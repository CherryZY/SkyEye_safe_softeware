// RegisterDialog.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RegisterDialog.h"
#include "afxdialogex.h"


// RegisterDialog �Ի���

IMPLEMENT_DYNAMIC(RegisterDialog, CDialogEx)

RegisterDialog::RegisterDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(RegisterDialog::IDD, pParent)
{

}

RegisterDialog::~RegisterDialog()
{
}

void RegisterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(RegisterDialog, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO1, &RegisterDialog::OnCbnSelchangeCombo1)
	ON_EN_CHANGE(IDC_EDIT2, &RegisterDialog::OnEnChangeEdit2)
	ON_BN_CLICKED(IDC_RADIO1, &RegisterDialog::OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, &RegisterDialog::OnBnClickedRadio2)
	ON_EN_CHANGE(IDC_EDIT3, &RegisterDialog::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT4, &RegisterDialog::OnEnChangeEdit4)
	ON_EN_CHANGE(IDC_RICHEDIT21, &RegisterDialog::OnEnChangeRichedit21)
END_MESSAGE_MAP()


// RegisterDialog ��Ϣ�������


void RegisterDialog::OnCbnSelchangeCombo1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void RegisterDialog::OnEnChangeEdit2()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void RegisterDialog::OnBnClickedRadio1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void RegisterDialog::OnBnClickedRadio2()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void RegisterDialog::OnEnChangeEdit3()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void RegisterDialog::OnEnChangeEdit4()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void RegisterDialog::OnEnChangeRichedit21()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}
