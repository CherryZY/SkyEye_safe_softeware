#ifndef REGDIT_OPERATION_H
#define REGDIT_OPERATION_H

#include <ntddk.h>
#include <stdlib.h>


//CP��SSDT��ע����������
typedef struct _REG_TO_CP{
	CHAR dbName[10];
	CHAR dbHost[10];
	CHAR sysAdmin[10];
	CHAR sysPassword[20];
}REG_TO_CP,*PREG_TO_CP;

//R3���͸�SSDT�����������ע����е�����
typedef struct _R3_TO_REG{
	//���ݿ�Host��,���ݿ���
	WCHAR dbHost[10];
	WCHAR dbName[10];
	//ϵͳ����Ա�˻�
	WCHAR sysAdmin[10];
	WCHAR sysPassword[20];
}R3_TO_REG,*PR3_TO_REG;

//�����ĸ��������͸�R3
//USER,PASSWORD	 ��CP���;
//HOST,DBNAME	 ��ע������.
typedef struct _CP_TO_R3{
	int  userLen;
	int  passLen;
	CHAR USER[10];
	CHAR PASSWORD[20];
	CHAR HOST[10];
	CHAR DBNAME[10];
}CP_TO_R3, *PCP_TO_R3;

NTSTATUS CreateKey();

NTSTATUS SetDBKey(WCHAR *dbname,WCHAR *dbhost);

NTSTATUS SetSYSKey(WCHAR *sysAdmin,WCHAR *sysPassword);

VOID QueryDBKey();

VOID QuerySYSKey();

NTSTATUS DeleteDBKey();

NTSTATUS DeleteSYSKey();


#endif
