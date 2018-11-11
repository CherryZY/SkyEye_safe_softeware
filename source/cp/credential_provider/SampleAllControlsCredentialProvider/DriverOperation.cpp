
#include "DriverOperation.h"

#define SERVICE_NAME "WFP_Filter"

//��Ϊwfp�����������ó�����Ҳ�����������Ծ������ַ�ʽ��
//ÿ�ο�����һ�η���
BOOL loadWFPService(){
	BOOL yes = false;
	SC_HANDLE hServiceMgr;
	SC_HANDLE hServiceDDK;
	do{
		//��SCM
		hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hServiceMgr == NULL){
			yes = false;
			break;
		}
		//�򿪷���
		hServiceDDK = OpenServiceA(hServiceMgr, SERVICE_NAME, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL){
			yes = false;
			break;
		}
		//��������
		BOOL s = StartServiceA(hServiceDDK, NULL, NULL);
		if (!s){
			yes = false;
			break;
		}
		yes = true;
	} while (FALSE);
	return yes;
}

//��SSDT����
bool DriverOperation::openSSDT(){
	SSDT = CreateFile(SSDT_NAME,
		GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (SSDT == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	return TRUE;
}

//��NDIS
bool DriverOperation::openNDIS(){
	NDIS = CreateFile(NDIS_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM, 0);
	if (NDIS == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	return TRUE;
}

//��WFP
bool DriverOperation::openWFP(){
	do{
		//�տ�ʼ���򿪳ɹ�����Ҫ�ٴμ���Driver
		WFP = CreateFile(WFP_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0, 0, OPEN_EXISTING,
			FILE_ATTRIBUTE_SYSTEM, 0);
		if (WFP != INVALID_HANDLE_VALUE){
			return TRUE;
		}
		//��ʧ��,˵���տ���,����loadDriver
		if (loadWFPService()){
			WFP = CreateFile(WFP_NAME,
				GENERIC_READ | GENERIC_WRITE,
				0, 0, OPEN_EXISTING,
				FILE_ATTRIBUTE_SYSTEM, 0);
			if (WFP == INVALID_HANDLE_VALUE){
				return FALSE;
			}
			return TRUE;
		}
		else
			return FALSE;
	} while (FALSE);
}

//SSDT�����û�������û���������
bool DriverOperation::sendSSDT_userAndpass(USER_PASSWORD up){
	int ret_len = 0;
	if (&up != NULL){
		if (!DeviceIoControl(SSDT, IOCTL_SSDT_SEND_UP, &up, sizeof(USER_PASSWORD), NULL, 0, (DWORD*)&ret_len, 0)){
			CloseHandle(SSDT);
			return FALSE;
		}
	}
	CloseHandle(SSDT);
	return TRUE;
}

//SSDT��������ù���Ա�û���������
REG_TO_CP DriverOperation::getSSDT_userAndpass(){
	REG_TO_CP up;
	int ret_len = 0;
	//ZeroMemory(&up, sizeof(REG_TO_CP));
	ZeroMemory(&up.dbHost,sizeof(10));
	ZeroMemory(&up.dbName,sizeof(10));
	ZeroMemory(&up.sysAdmin,sizeof(10));
	ZeroMemory(&up.sysPassword,sizeof(20));

	if (openSSDT()){
		if (DeviceIoControl(SSDT, IOCTL_SSDT_GET_REG, NULL, 0, &up, sizeof(REG_TO_CP), (DWORD*)&ret_len, 0)){
			CloseHandle(SSDT);
			return up;
		}
		else
			MessageBoxA(NULL, "Info", "Get Data Failed!", 0);
		CloseHandle(SSDT);
	}
	else
		MessageBoxA(NULL,"Open Driver failed!","Info", 0);
	return up;
}

//��������������е������ڹ�����Ϣ
bool DriverOperation::clearAll(){
	DWORD ret_len = 0;
	//���SSDT
	if (openSSDT()){
		if (!DeviceIoControl(SSDT, IOCTL_SSDT_CLAER_BLACK_ILST, NULL, 0, NULL, 0, (DWORD*)&ret_len, 0))
			MessageBoxA(NULL,"Clear SSDT Failed!","warning",0);
		CloseHandle(SSDT);
	}
	//���WFP
	if (openWFP()){
		if (!DeviceIoControl(WFP, IOCTL_WFP_CLEAR_BLACK_LIST, NULL, 0, NULL, 0, (DWORD *)&ret_len, 0))
			MessageBoxA(NULL, "Clear WFP Failed!", "warning", 0);
		CloseHandle(WFP);
	}
	else{
		MessageBoxA(NULL, "Open WFP Failed!", "warning", 0);
	}
	//���NDIS
	if (openNDIS()){
		if (!DeviceIoControl(NDIS, IOCTL_CLEAR_BLACK_LIST, NULL, 0, NULL, 0, (DWORD*)&ret_len, 0)){
			MessageBoxA(NULL, "Clear NDIS Failed!", "warning", 0);
		}
		CloseHandle(NDIS);
	}
	return true;
}

