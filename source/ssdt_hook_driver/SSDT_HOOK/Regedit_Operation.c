#include "Regedit_Operation.h"
#include <tchar.h>

#define  MY_REG_SOFTWARE_KEY_NAME		L"\\Registry\\Machine\\Software\\Owl"
#define  MY_REG_SOFTWARE_KEY_DB			L"\\Registry\\Machine\\Software\\Owl\\db"
#define	 MY_REG_SOFTWARE_KEY_SYS		L"\\Registry\\Machine\\Software\\Owl\\sys"

extern PCP_TO_R3 pc2r;

extern PREG_TO_CP pRTC;

extern BOOLEAN SkyEyeGetReg;

//������
NTSTATUS CreateKey(){
	NTSTATUS Status = STATUS_SUCCESS;
	//----------------����---------------
	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	ULONG  ulResult;
	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString,MY_REG_SOFTWARE_KEY_NAME);
	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes, &RegUnicodeString, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	//���������ע�����Ŀ
	NTSTATUS ntStatus = ZwCreateKey(&hRegister,KEY_ALL_ACCESS,&objectAttributes,0,NULL,REG_OPTION_NON_VOLATILE,&ulResult);
	if (NT_SUCCESS(ntStatus)){
		//�ж��Ǳ��´����������Ѿ�������
		if (ulResult == REG_CREATED_NEW_KEY){
			KdPrint(("The register item is created\n"));
		}else if (ulResult == REG_OPENED_EXISTING_KEY){
			KdPrint(("The register item has been created,and now is opened\n"));
		}
	}

	//------------------����-1----------------
	UNICODE_STRING subRegUnicodeString;
	HANDLE hSubRegister;
	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&subRegUnicodeString,L"db");
	OBJECT_ATTRIBUTES subObjectAttributes;
	//��ʼ��subObjectAttributes
	InitializeObjectAttributes(&subObjectAttributes,&subRegUnicodeString,OBJ_CASE_INSENSITIVE, hRegister,NULL);
	//�������ע�����Ŀ
	ntStatus = ZwCreateKey(&hSubRegister,KEY_ALL_ACCESS,&subObjectAttributes,0,NULL,REG_OPTION_NON_VOLATILE,&ulResult);
	if (NT_SUCCESS(ntStatus)){
		//�ж��Ǳ��´����������Ѿ�������
		if (ulResult == REG_CREATED_NEW_KEY){
			KdPrint(("The sub register db is created\n"));
		}else if (ulResult == REG_OPENED_EXISTING_KEY){
			KdPrint(("The sub register db has been created,and now is opened\n"));
		}
	}

	//------------------����-2----------------
	UNICODE_STRING subRegisterString;
	HANDLE hSubRegister0;
	//��ʼ��String
	RtlInitUnicodeString(&subRegisterString,L"sys");
	OBJECT_ATTRIBUTES subObjectAttribute0;
	//��ʼ��subObjectAttribute
	InitializeObjectAttributes(&subObjectAttribute0,&subRegisterString,OBJ_CASE_INSENSITIVE,hRegister,NULL);
	//�������ע�����Ŀ
	ntStatus = ZwCreateKey(&hSubRegister0,KEY_ALL_ACCESS,&subObjectAttribute0,0,NULL,REG_OPTION_NON_VOLATILE,&ulResult);
	if (NT_SUCCESS(ntStatus)){
		//�ж��Ǳ��´����������Ѿ�������
		if (ulResult == REG_CREATED_NEW_KEY){
			KdPrint(("The sub register sys is created\n"));
		}
		else if (ulResult == REG_OPENED_EXISTING_KEY){
			KdPrint(("The sub register sys has been created,and now is opened\n"));
		}
	}

	//�ر�ע�����
	ZwClose(hRegister);
	ZwClose(hSubRegister);
	ZwClose(hSubRegister0);
	return Status;
}

//����ע�����DB��ֵ
NTSTATUS SetDBKey(WCHAR *dbname,WCHAR *dbhost){
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	UNICODE_STRING ValueName;
	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString, MY_REG_SOFTWARE_KEY_DB);
	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes,&RegUnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);

	do{
		//��ע���
		NTSTATUS ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
		if (NT_SUCCESS(ntStatus)){
			KdPrint(("Open register db successfully\n"));
		}
		else{ 
			KdPrint(("Open register db failed\n")); 
			break;
		}

		//REG_SZ
		//-----------------------------DBNAME-------------------------------------
		RtlInitUnicodeString(&ValueName, L"dbname");
		//WCHAR* dbname = L"mysql";
		Status = ZwSetValueKey(hRegister, &ValueName, 0, REG_SZ, dbname, wcslen(dbname) * 2 + 2);
		//------------------------------------------------------------------------

		//-----------------------------DBHOST-------------------------------------
		RtlInitUnicodeString(&ValueName,L"dbhost");
		//WCHAR* dbhost = L"localhost";
		Status = ZwSetValueKey(hRegister, &ValueName, 0, REG_SZ, dbhost, wcslen(dbhost) * 2 + 2);
		//------------------------------------------------------------------------

		//�ر�ע�����
		ZwClose(hRegister);
	} while (FALSE);
	
	return Status;
}

//����ע�����SYS��ֵ
NTSTATUS SetSYSKey(WCHAR *sysAdmin, WCHAR *sysPassword){
	NTSTATUS Status = STATUS_SUCCESS;

	UNICODE_STRING ValueName;
	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString, MY_REG_SOFTWARE_KEY_SYS);
	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes, &RegUnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

	do{
		//��ע���
		NTSTATUS ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
		if (NT_SUCCESS(ntStatus)){
			KdPrint(("Open register sys successfully\n"));
		}
		else{
			KdPrint(("Open register sys failed\n"));
			break;
		}

		//REG_SZ
		//-----------------------------SYSADMIN-------------------------------------
		RtlInitUnicodeString(&ValueName, L"sysAdmin");
		//WCHAR* sysAdmin = L"root";
		Status = ZwSetValueKey(hRegister, &ValueName, 0, REG_SZ, sysAdmin, wcslen(sysAdmin) * 2 + 2);
		//------------------------------------------------------------------------

		//-----------------------------SYSPASSWORD-------------------------------------
		RtlInitUnicodeString(&ValueName, L"sysPassword");
		//WCHAR* sysPassword = L"123";
		Status = ZwSetValueKey(hRegister, &ValueName, 0, REG_SZ, sysPassword, wcslen(sysPassword) * 2 + 2);
		//------------------------------------------------------------------------

		//�ر�ע�����
		ZwClose(hRegister);
	} while (FALSE);

	return Status;
}

//��ѯDB��ֵ
VOID QueryDBKey(){

	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	NTSTATUS ntStatus;
	UNICODE_STRING ValueName;		//��ֵ��
	ULONG ulSize;					//���ص�ֵ����
	PKEY_VALUE_PARTIAL_INFORMATION pvpi;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString,MY_REG_SOFTWARE_KEY_DB);

	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes,&RegUnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);
	do{
		//��ע���
		ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
		if (NT_SUCCESS(ntStatus)){
			KdPrint(("Open register successfully\n"));
		}
		else break;

		//REG_SZ
		//--------------------------------------------DBHOST-------------------------------------------------
		//��ʼ��ValueName
		RtlInitUnicodeString(&ValueName, L"dbhost");
		//��ȡREG_SZ�Ӽ�
		ntStatus = ZwQueryValueKey(hRegister,&ValueName,KeyValuePartialInformation,NULL,0,&ulSize);
		if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0){
			ZwClose(hRegister);
			KdPrint(("The item is not exist\n"));
			return ;
		}
		pvpi =(PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ntStatus = ZwQueryValueKey(hRegister,&ValueName,KeyValuePartialInformation,pvpi,ulSize,&ulSize);
		if (!NT_SUCCESS(ntStatus)){
			ZwClose(hRegister);
			KdPrint(("Read regsiter error\n"));
			return ;
		}
		//�ж��Ƿ�ΪREG_SZ����
		if (pvpi->Type == REG_SZ){
			KdPrint(("The value:%S\n", pvpi->Data));
			int i = 0;
			//����������ֵ����rgr�ṹ
			KdPrint(("dbhost length:%d\n",pvpi->DataLength));
			//-------------------------------------------------------------------
			ANSI_STRING asci;
			UNICODE_STRING unicode;
			unicode.Length = unicode.MaximumLength = (USHORT)pvpi->DataLength;
			unicode.Buffer = (PWSTR)ExAllocatePool(PagedPool, unicode.Length);
			//����ѯ��������pvpi�������½���UNICODE_STRING��
			RtlCopyMemory(unicode.Buffer, pvpi->Data, unicode.Length);

			KdPrint(("unicode buffer:%ws\n", unicode.Buffer));
			//��unicodeת��Asci,����Asci�����ڴ�
			RtlUnicodeStringToAnsiString(&asci, &unicode, TRUE);

			KdPrint(("asci buffer:%s\n", asci.Buffer));

			if (SkyEyeGetReg){
				for (; i < strlen(asci.Buffer); i++){
					KdPrint(("asci.Buffer: %c\n", asci.Buffer[i]));
					pc2r->HOST[i] = asci.Buffer[i];
				}
			}
			else{
				for (; i < strlen(asci.Buffer); i++){
					pRTC->dbHost[i] = asci.Buffer[i];
				}
			}
			
			//-------------------------------------------------------------------
		}
		ExFreePool(pvpi);
		//---------------------------------------------------------------------------------------------------

		//--------------------------------------------DBNAME-------------------------------------------------
		//��ʼ��ValueName
		RtlInitUnicodeString(&ValueName, L"dbname");
		//�ж��Ӽ�����
		ntStatus = ZwQueryValueKey(hRegister, &ValueName, KeyValuePartialInformation, NULL, 0, &ulSize);
		if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0){
			ZwClose(hRegister);
			KdPrint(("The item is not exist\n"));
			return ;
		}
		pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ntStatus = ZwQueryValueKey(hRegister, &ValueName, KeyValuePartialInformation, pvpi, ulSize, &ulSize);
		if (!NT_SUCCESS(ntStatus)){
			ZwClose(hRegister);
			KdPrint(("Read regsiter error\n"));
			return ;
		}
		//�ж��Ƿ�ΪREG_SZ����
		if (pvpi->Type == REG_SZ){
			KdPrint(("The value:%S\n", pvpi->Data));
			int i = 0;
			KdPrint(("dbname length:%d\n", pvpi->DataLength));

			//-------------------------------------------------------------------
			//PWCHAR namedata = (PWCHAR)pvpi->Data;
			ANSI_STRING asci;
			UNICODE_STRING unicode;
			unicode.Length = unicode.MaximumLength = (USHORT)pvpi->DataLength;
			unicode.Buffer = (PWSTR)ExAllocatePool(PagedPool,unicode.Length);

			RtlCopyMemory(unicode.Buffer,pvpi->Data,unicode.Length);

			KdPrint(("unicode buffer:%ws\n",unicode.Buffer));

			RtlUnicodeStringToAnsiString(&asci,&unicode,TRUE);

			KdPrint(("asci buffer:%s\n", asci.Buffer));
			if (SkyEyeGetReg){
				for (; i < strlen(asci.Buffer); i++){
					KdPrint(("asci.Buffer: %c\n", asci.Buffer[i]));
					pc2r->DBNAME[i] = asci.Buffer[i];
				}
			}
			else{
				for (; i < strlen(asci.Buffer); i++){
					pRTC->dbName[i] = asci.Buffer[i];
				}
			}
			
			//-------------------------------------------------------------------
		}
		ExFreePool(pvpi);
		//---------------------------------------------------------------------------------------------------

		ZwClose(hRegister);
	} while (FALSE);
}

//��ѯSYS��ֵ
VOID QuerySYSKey(){

	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	NTSTATUS ntStatus;
	UNICODE_STRING ValueName;		//��ֵ��
	ULONG ulSize;					//���ص�ֵ����
	PKEY_VALUE_PARTIAL_INFORMATION pvpi;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString, MY_REG_SOFTWARE_KEY_SYS);

	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes, &RegUnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);
	do{
		//��ע���
		ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
		if (NT_SUCCESS(ntStatus)){
			KdPrint(("Open register successfully\n"));
		}

		//REG_SZ
		//--------------------------------------------SYSAdmin-------------------------------------------------
		//��ʼ��ValueName
		RtlInitUnicodeString(&ValueName, L"sysAdmin");
		//��ȡREG_SZ�Ӽ�
		ntStatus = ZwQueryValueKey(hRegister, &ValueName, KeyValuePartialInformation, NULL, 0, &ulSize);
		if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0){
			ZwClose(hRegister);
			KdPrint(("The item is not exist\n"));
			return;
		}
		pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ntStatus = ZwQueryValueKey(hRegister, &ValueName, KeyValuePartialInformation, pvpi, ulSize, &ulSize);
		if (!NT_SUCCESS(ntStatus)){
			ZwClose(hRegister);
			KdPrint(("Read regsiter error\n"));
			return;
		}
		//�ж��Ƿ�ΪREG_SZ����
		if (pvpi->Type == REG_SZ){
			KdPrint(("The value:%S\n", pvpi->Data));
			int i = 0;
			//-------------------------------------------------------------------
			ANSI_STRING asci;
			UNICODE_STRING unicode;
			unicode.Length = unicode.MaximumLength = (USHORT)pvpi->DataLength;
			unicode.Buffer = (PWSTR)ExAllocatePool(PagedPool, unicode.Length);
			//����ѯ��������pvpi�������½���UNICODE_STRING��
			RtlCopyMemory(unicode.Buffer, pvpi->Data, unicode.Length);

			KdPrint(("unicode buffer:%ws\n", unicode.Buffer));
			//��unicodeת��Asci,����Asci�����ڴ�
			RtlUnicodeStringToAnsiString(&asci, &unicode, TRUE);

			KdPrint(("asci buffer:%s\n", asci.Buffer));

			for (; i < strlen(asci.Buffer); i++){
				pRTC->sysAdmin[i] = asci.Buffer[i];
			}
			//-------------------------------------------------------------------
		}
		ExFreePool(pvpi);
		//---------------------------------------------------------------------------------------------------

		//--------------------------------------------SysPassword-------------------------------------------------
		//��ʼ��ValueName
		RtlInitUnicodeString(&ValueName, L"sysPassword");
		//�ж��Ӽ�����
		ntStatus = ZwQueryValueKey(hRegister, &ValueName, KeyValuePartialInformation, NULL, 0, &ulSize);
		if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ulSize == 0){
			ZwClose(hRegister);
			KdPrint(("The item is not exist\n"));
			return;
		}
		pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ntStatus = ZwQueryValueKey(hRegister, &ValueName, KeyValuePartialInformation, pvpi, ulSize, &ulSize);
		if (!NT_SUCCESS(ntStatus)){
			ZwClose(hRegister);
			KdPrint(("Read regsiter error\n"));
			return;
		}
		//�ж��Ƿ�ΪREG_SZ����
		if (pvpi->Type == REG_SZ){
			KdPrint(("The value:%S\n", pvpi->Data));
			int i = 0;
			//-------------------------------------------------------------------
			//PWCHAR namedata = (PWCHAR)pvpi->Data;
			ANSI_STRING asci;
			UNICODE_STRING unicode;
			unicode.Length = unicode.MaximumLength = (USHORT)pvpi->DataLength;
			unicode.Buffer = (PWSTR)ExAllocatePool(PagedPool, unicode.Length);
			//����ѯ��������pvpi�������½���UNICODE_STRING��
			RtlCopyMemory(unicode.Buffer, pvpi->Data, unicode.Length);

			KdPrint(("unicode buffer:%ws\n", unicode.Buffer));
			//��unicodeת��Asci,����Asci�����ڴ�
			RtlUnicodeStringToAnsiString(&asci, &unicode, TRUE);

			KdPrint(("asci buffer:%s\n", asci.Buffer));

			for (; i < strlen(asci.Buffer); i++){
				pRTC->sysPassword[i] = asci.Buffer[i];
			}
			//-------------------------------------------------------------------
		}
		ExFreePool(pvpi);
		//---------------------------------------------------------------------------------------------------

		ZwClose(hRegister);
	} while (FALSE);

	//return STATUS_SUCCESS;
}

//ɾ��DB��
NTSTATUS DeleteDBKey(){

	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	NTSTATUS ntStatus;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString,MY_REG_SOFTWARE_KEY_DB);

	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes,&RegUnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);
	//��ע���
	ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);

	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("Open register DB successfully\n"));
	}
	//ɾ��ָ�����Ӽ�
	ntStatus = ZwDeleteKey(hRegister);
	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("Delete the DB successfully\n"));
	}
	else if (ntStatus == STATUS_ACCESS_DENIED)
	{
		KdPrint(("STATUS_ACCESS_DENIED\n"));

	}
	else if (ntStatus == STATUS_INVALID_HANDLE)
	{
		KdPrint(("STATUS_INVALID_HANDLE\n"));
	}
	else
	{
		KdPrint(("Maybe the item has sub item to delete\n"));
	}

	ZwClose(hRegister);

	return STATUS_SUCCESS;
}

//ɾ��SYS��
NTSTATUS DeleteSYSKey(){

	UNICODE_STRING RegUnicodeString;
	HANDLE hRegister;
	NTSTATUS ntStatus;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString,MY_REG_SOFTWARE_KEY_SYS);

	OBJECT_ATTRIBUTES objectAttributes;
	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes,&RegUnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);
	//��ע���
	ntStatus = ZwOpenKey(&hRegister,KEY_ALL_ACCESS,&objectAttributes);
	if (NT_SUCCESS(ntStatus)){
		KdPrint(("Open register SYS successfully\n"));
	}
	//ɾ���������
	ntStatus = ZwDeleteKey(hRegister);
	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("Delete the SYS successfully\n"));
	}
	else if (ntStatus == STATUS_ACCESS_DENIED)
	{
		KdPrint(("STATUS_ACCESS_DENIED\n"));
	}
	else if (ntStatus == STATUS_INVALID_HANDLE)
	{
		KdPrint(("STATUS_INVALID_HANDLE\n"));
	}
	else
	{
		KdPrint(("Maybe the item has sub item to delete\n"));
	}

	ZwClose(hRegister);
	return STATUS_SUCCESS;
}

