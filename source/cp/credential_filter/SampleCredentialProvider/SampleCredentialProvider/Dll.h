//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
//


/*//////////////////////////////////////////////

ʵ��֧��COM Server��DLL ����Ļ�׼��
��Ӧ�ñ༭��Щ�ļ�

//////////////////////////////////////////////*/

#pragma once

// global dll hinstance
extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

void DllAddRef();
void DllRelease();
