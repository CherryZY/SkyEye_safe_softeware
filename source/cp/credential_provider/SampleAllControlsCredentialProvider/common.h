
#pragma once
#include <credentialprovider.h>
#include <ntsecapi.h>
#define SECURITY_WIN32
#include <security.h>
#include <intsafe.h>

#define MAX_ULONG  ((ULONG)(-1))

//��õĹ���Ա�û���������/������û���������
typedef struct _USER_PASSWORD{
	int  userLength;
	int  passwordLength;
	CHAR USER[10];
	CHAR PASSWORD[20];
}USER_PASSWORD, *PUSER_PASSWORD;

//��CP��tile���������
enum SAMPLE_FIELD_ID 
{
    SFI_TILEIMAGE       = 0,
    SFI_LARGE_TEXT      = 1,
    SFI_SMALL_TEXT      = 2,
    SFI_EDIT_TEXT       = 3,
    SFI_PASSWORD        = 4,
    SFI_SUBMIT_BUTTON   = 5,
    SFI_COMMAND_LINK    = 6,
    SFI_NUM_FIELDS      = 7,  // ������
};

//��״̬�ͽ���״̬
struct FIELD_STATE_PAIR
{
    CREDENTIAL_PROVIDER_FIELD_STATE cpfs;
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE cpfis;
};

//��ʾ����
static const FIELD_STATE_PAIR s_rgFieldStatePairs[] = 
{
    { CPFS_DISPLAY_IN_BOTH, CPFIS_NONE },                   // SFI_TILEIMAGE
    { CPFS_DISPLAY_IN_BOTH, CPFIS_NONE },                   // SFI_LARGE_TEXT
    { CPFS_DISPLAY_IN_DESELECTED_TILE, CPFIS_NONE    },     // SFI_SMALL_TEXT   
    { CPFS_DISPLAY_IN_SELECTED_TILE, CPFIS_NONE    },       // SFI_EDIT_TEXT   
    { CPFS_DISPLAY_IN_SELECTED_TILE, CPFIS_FOCUSED },       // SFI_PASSWORD
    { CPFS_DISPLAY_IN_SELECTED_TILE, CPFIS_NONE    },       // SFI_SUBMIT_BUTTON   
    { CPFS_DISPLAY_IN_SELECTED_TILE, CPFIS_NONE    },       // SFI_COMMAND_LINK   
};

//CP����ʾ�������������������������ͣ�������
static const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgCredProvFieldDescriptors[] =
{
    { SFI_TILEIMAGE, CPFT_TILE_IMAGE, L"Image" },
    { SFI_LARGE_TEXT, CPFT_LARGE_TEXT, L"LargeText" },
    { SFI_SMALL_TEXT, CPFT_SMALL_TEXT, L"SmallText" },
    { SFI_EDIT_TEXT, CPFT_EDIT_TEXT, L"Username (ascii '0'-> 'z') " },
    { SFI_PASSWORD, CPFT_PASSWORD_TEXT, L"Password (ascii '0'-> 'z')" },
    { SFI_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON, L"Submit" },
    { SFI_COMMAND_LINK, CPFT_COMMAND_LINK, L"Register" },
};