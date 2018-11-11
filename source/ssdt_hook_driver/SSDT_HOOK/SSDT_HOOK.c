/* * * * * * * * * * * * * * * * * * * 

���� PsSetCreateProcessNotifyRoutineEx
ʵ�ֶԽ��̵ļ��(�������ر�)

* * * * * * * * * * * * * * * * * * * */

#include "Regedit_Operation.h"
#include "ProcessForbid.h"
#include "SSDT_HOOK.h"

//SkyEye����SSDT��ǰ����ID������
#define IOCTL_SSDT_ADD \
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x830, METHOD_BUFFERED, \
	FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//SkyEyeɾ��SSDT��ǰ�ܱ�������ID
#define IOCTL_SSDT_DELETE \
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x831, METHOD_BUFFERED, \
	FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//�����û�������û���,����,DBname,DBhost��CP->SSDT
#define IOCTL_SSDT_SEND_UP\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x850, METHOD_BUFFERED, \
	FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//��õ�ǰ�˻��û���,����,DBname,DBhost��Register+CP -> SkyEye
#define IOCTL_SSDT_GET_UP\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x851, METHOD_BUFFERED, \
	FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//����װ��Ϣд��ע���SkyEye->Register
#define IOCTL_SSDT_SET_REG\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
	0x852,METHOD_BUFFERED,\
	FILE_WRITE_ACCESS | FILE_READ_ACCESS)

//��ע��������װ��Ϣ��Register->CP
#define IOCTL_SSDT_GET_REG\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
	0x853,METHOD_BUFFERED,\
	FILE_WRITE_ACCESS | FILE_READ_ACCESS)

//�������������
#define IOCTL_SSDT_CLAER_BLACK_ILST\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x832, METHOD_BUFFERED, \
	FILE_WRITE_ACCESS | FILE_READ_ACCESS)

#define DEVICE_NAME_PROCESS				L"\\Device\\SSDTProcess"
#define SYMBOLINK_NAME_PROCESS			L"\\??\\SSDTProcess"

//������ ntoskrnl.exe �������� SSDT
extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;

//�洢CP�������û���,����
typedef struct _USER_PASSWORD{
	int  userLength;
	int  passwordLength;
	CHAR USER[10];
	CHAR PASSWORD[20];
}USER_PASSWORD, *PUSER_PASSWORD;

//�û���¼����
typedef struct _USER_LIST{
	LIST_ENTRY listEntry;
	USER_PASSWORD UserPassword;
}USER_LIST, *PUSER_LIST;
//��¼�������
KSPIN_LOCK userLock;
//�û�����
LIST_ENTRY userList;
//CP����Ϣ+ע������Ϣ�����ݽṹ����SkyEye
PCP_TO_R3 pc2r = NULL;
//CP��ȡע������Ϣ
PREG_TO_CP pRTC = NULL;
//��־������Ĭ��ΪFALSE
//ΪTRUEʱ����Ϊ��SkyEye�����Ϣ;
//ΪFALSEʱ����Ϊ��CP�����Ϣ.
BOOLEAN SkyEyeGetReg = FALSE;
//��ֹ�򿪵Ľ�����
LIST_ENTRY ForbidProcessList;
//����������
KSPIN_LOCK ForbidProcessListLock;

//Ҫ����ȥ�Ľ�ֹ���������ַ�������
typedef struct _ForbidProcessName{
	int length;
	WCHAR ProcessName[50];
}ForbidProcessName, *PForbidProcessName;


//�����½���û���¼����
NTSTATUS InsertUserList(PUSER_LIST up){
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	KLOCK_QUEUE_HANDLE handle;
	KeAcquireInStackQueuedSpinLock(&userLock, &handle);

	InsertHeadList(&userList, &up->listEntry);
	Status = STATUS_SUCCESS;

	KeReleaseInStackQueuedSpinLock(&handle);
	return Status;
}

//ɾ����½���û���¼
NTSTATUS RemoveUserList(PUSER_LIST ul){
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	KLOCK_QUEUE_HANDLE handle;
	KeAcquireInStackQueuedSpinLock(&userLock, &handle);

	RemoveEntryList(&ul->listEntry);
	Status = STATUS_SUCCESS;

	KeReleaseInStackQueuedSpinLock(&handle);
	return Status;
}

//�������̵Ļص�����
VOID CreateProcessNotifyEx(__inout PEPROCESS  Process,__in HANDLE  ProcessId,__in_opt PPS_CREATE_NOTIFY_INFO  CreateInfo){
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(Process);
	KdPrint(("SSDT-------Notify-In------->"));
	PWCHAR pSub = NULL;
	UNICODE_STRING  uniPath = { 0 };
	uniPath.Length = 0;
	uniPath.MaximumLength = 256 * 2;
	uniPath.Buffer = (PWSTR)ExAllocatePool(NonPagedPool, uniPath.MaximumLength);
	ULONG currenPID = (ULONG)ProcessId;
	//ΪNULL��ʾ�����˳�
	if (NULL == CreateInfo){
		/*for (int i = 0; i < MAX_PROCESS_ARRARY_LENGTH; i++){
			if (g_PIDProtectArray[i] == currenPID)
			{
				//��õ�ǰ����ȫ·��
				GetProcessPath(currenPID,&uniPath);
				//�Լ�����һ��PPS_CREATE_NOTIFY_INFO�ṹ��������
				PPS_CREATE_NOTIFY_INFO pPCNI = (PPS_CREATE_NOTIFY_INFO)
					ExAllocatePool(NonPagedPool, sizeof(PS_CREATE_NOTIFY_INFO));
				pPCNI->ImageFileName = &uniPath;
				pPCNI->ParentProcessId = PsGetProcessId(Process);
				PEPROCESS peProcess = NULL;
				if (NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &peProcess))){
					pPCNI->CreatingThreadId = ProcessId
				}
				ExFreePool(pPCNI);
				
			}
		}*/
			//g_PIDProtectArray
		KdPrint(("SSDT------process exits.\n"));
		return;
	}

	KdPrint(("SSDT------process create:(%wZ).\n", CreateInfo->ImageFileName));
	
	LIST_ENTRY *entry = NULL;
	if (!IsListEmpty(&ForbidProcessList)){
		for (entry = ForbidProcessList.Flink;
			entry != &ForbidProcessList;
			entry = entry->Blink){

			PForbidProcess data1 = CONTAINING_RECORD(entry, ForbidProcess, listEntry);
			KdPrint(("SSDT------deleteProcessName in list:%ws\n", data1->ProcessName));
			pSub = wcswcs(CreateInfo->ImageFileName->Buffer, data1->ProcessName);
			if (pSub != NULL)
				break;
		}
	}
	if (NULL != pSub)
	{
		//�޸ķ��ؽ��Ϊ�ܾ����ʣ�ʹ�ô�������ʧ��
		CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
	}

	return;
}

//#pragma INITCODE
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING pRegistryPath)
{
	//�������windows���������,DriverSection��ʾϵͳ����������״�ĵ�ǰ���������еĽڵ�
#ifdef _AMD64_ 
	DbgPrint("64\r\n");
	*((PCHAR)pDriverObject->DriverSection + 0x68) |= 0x20;
#else
	DbgPrint("32\r\n");
	*((PCHAR)pDriverObject->DriverSection + 0x34) |= 0x20;
#endif//_AMD_64


	ULONG i;
	NTSTATUS status;
	UNICODE_STRING strDeviceName;
	UNICODE_STRING strSymbolLinkName;
	PDEVICE_OBJECT pDeviceObject;

	pDeviceObject = NULL;

	KdPrint(("register: %ws\n",pRegistryPath->Buffer));

	RtlInitUnicodeString(&strDeviceName, DEVICE_NAME_PROCESS);
	RtlInitUnicodeString(&strSymbolLinkName, SYMBOLINK_NAME_PROCESS);

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = SSDTGeneralDispatcher;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = SSDTCreateDispatcher;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = SSDTCloseDispatcher;
	pDriverObject->MajorFunction[IRP_MJ_READ] = SSDTReadDispatcher;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = SSDTWriteDispatcher;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SSDTDeviceIoControlDispatcher;

	pDriverObject->DriverUnload = SSDTDriverUnload;

	status = IoCreateDevice(pDriverObject, 0, &strDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create Device Failed!\n"));
		return status;
	}
	if (!pDeviceObject)
	{
		return STATUS_UNEXPECTED_IO_ERROR;
	}

	KdPrint(("DriverEntry===->\n"));

	//ʹ��ֱ�� IO ��д��ʽ
	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
	status = IoCreateSymbolicLink(&strSymbolLinkName, &strDeviceName);

	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	//��ʼ���û���¼����
	InitializeListHead(&userList);
	//��ʼ���û���¼��
	KeInitializeSpinLock(&userLock);
	//��ʼ����ֹ�򿪵Ľ�����
	InitializeListHead(&ForbidProcessList);
	//��ʼ����ֹ�򿪽���������
	KeInitializeSpinLock(&ForbidProcessListLock);

	//��ʼ��Ŀǰ��½���û���Ϣ����
	pc2r = (PCP_TO_R3)ExAllocatePool(NonPagedPool,sizeof(CP_TO_R3));
	if (pc2r != NULL)
		KdPrint(("pc2r Allocate Success!"));
	else
		KdPrint(("pc2r Allocate Failed!"));
	RtlZeroMemory(pc2r,sizeof(CP_TO_R3));
	for (int i = 0; i < 10; i++){
		pc2r->DBNAME[i] = '#';
		pc2r->HOST[i] = '#';
		pc2r->USER[i] = '#';
	}
	for (int j = 0; j < 20; j++)
		pc2r->PASSWORD[j] = '#';
	//��ʼ��Ҫ���͸�CP�����ݿ��ϵͳ��Ϣ
	pRTC = (PREG_TO_CP)ExAllocatePool(NonPagedPool, sizeof(REG_TO_CP));
	if (pRTC != NULL)
		KdPrint(("pRTC Allocate Success!"));
	else
		KdPrint(("pRTC Allocate Failed!"));
	RtlZeroMemory(pRTC, sizeof(REG_TO_CP));
	for (int i = 0; i < 10; i++){
		pRTC->dbHost[i] = '#';
		pRTC->dbName[i] = '#';
		pRTC->sysAdmin[i] = '#';
	}
	for (int i = 0; i < 20; i++)
		pRTC->sysPassword[i] = '#';

	//������Ҫ����ԭ���� SSDT ϵͳ���������������з���ĵ�ַ����Щ��ַ��Ҫ����ʵ�ֽ�� Hook
	BackupSysServicesTable();
	//���ý��̴������رջص�����
	NTSTATUS pn = STATUS_SUCCESS;
	pn = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)CreateProcessNotifyEx,FALSE);
	if (pn != STATUS_SUCCESS)
		KdPrint(("SSDT--------PsSetCreateProcessNotifyRoutineEx Successful!!!!"));
	else
		KdPrint(("SSDT--------Failed PsSetCreateProcessNotifyRoutineEx!!!!!"));

	//��װ Hook:���µ�MyNtQuerySystemInformation/MyNtTerminateProcess�滻ZwQuerySystemInformation/ZwTerminateProcess
	InstallSSDTHook((ULONG)ZwQuerySystemInformation, (ULONG)MyNtQuerySystemInformation);
	InstallSSDTHook((ULONG)ZwTerminateProcess, (ULONG)MyNtTerminateProcess);

	return STATUS_SUCCESS;
}


//����ȥ���ڴ�Ŀ�д���ԣ��Ӷ�ʵ���ڴ�ֻ��
VOID DisableWriteProtect(ULONG oldAttr)
{
	KdPrint(("DisableWriteProtect===->\n"));
	_asm
	{
		mov eax, oldAttr
		mov cr0, eax
		sti;
	}
}

//����ȥ���ڴ��ֻ���������Ӷ�ʵ�ֿ���д�ڴ�
VOID EnableWriteProtect(PULONG pOldAttr)
{
	KdPrint(("EnableWriteProtect===->\n"));
	ULONG uAttr;
	//���ָ�
	_asm
	{
		cli;
		mov  eax, cr0;
		mov  uAttr, eax;
		and  eax, 0FFFEFFFFh; // CR0 16 BIT = 0 
		mov  cr0, eax;
	};

	//����ԭ�е� CRO ���� 
	*pOldAttr = uAttr;
}

//���� SSDT ��ԭ�з���ĵ�ַ����Ϊ�����ڽ�� Hook ʱ��Ҫ��ԭ SSDT ��ԭ�е�ַ
VOID BackupSysServicesTable()
{
	KdPrint(("BackupSysServicesTable===->\n"));
	ULONG i;
	//ѭ���������ӵ�0��������ʼ��ֱ��SSDT��ntoskrnl����ַ��󣬱������еķ��񣬲��洢��һ���ṹ�С�
	for (i = 0; (i < KeServiceDescriptorTable->ntoskrnl.NumberOfService) && (i < MAX_SYSTEM_SERVICE_NUMBER); i++)
	{
		oldSysServiceAddr[i] = KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[i];

		//oldSysServiceAddr[i] = *(PULONG)((ULONG)KeServiceDescriptorTable->ntoskrnl.ServiceTableBase + 4 * i);

		KdPrint(("\nBackupSysServicesTable - Function Information { Number: 0x%04X , Address: %08X}",
			i, oldSysServiceAddr[i]));
	}
}

//
//Mark:	���е�EnableWriteProtect��DisableWriteProtect�൱��һ���������������ͽ���
//����HOOK
NTSTATUS InstallSSDTHook(ULONG oldService, ULONG newService)
{
	KdPrint(("InstallSSDTHook===->\n"));

	ULONG uOldAttr = 0;

	//ʵ�ֿ�д���ƽ�SSDTֻ������
	EnableWriteProtect(&uOldAttr);

	//��oldService���ϵ�ַ������newService
	SYSCALL_FUNCTION(oldService) = newService;
	
	//�ָ�ԭ״
	DisableWriteProtect(uOldAttr);

	return STATUS_SUCCESS;
}

//
//Mark:	���е�EnableWriteProtect��DisableWriteProtect�൱��һ���������������ͽ���
//�ر�HOOK
NTSTATUS UnInstallSSDTHook(ULONG oldService)
{
	KdPrint(("UnInstallSSDTHook===->\n"));
	ULONG uOldAttr = 0;

	//ʵ�ֿ�д
	EnableWriteProtect(&uOldAttr);

	//��ԭ��ַ���´��oldService
	SYSCALL_FUNCTION(oldService) = oldSysServiceAddr[SYSCALL_INDEX(oldService)];
	//ʵ�ֿɶ�
	DisableWriteProtect(uOldAttr);

	return STATUS_SUCCESS;
}


NTSTATUS SSDTGeneralDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return pIrp->IoStatus.Status;
}


NTSTATUS SSDTCreateDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	KdPrint(("Create"));
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS SSDTCloseDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS SSDTReadDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS rtStatus;

	rtStatus = STATUS_NOT_SUPPORTED;

	return rtStatus;
}


NTSTATUS SSDTWriteDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS rtStatus;

	rtStatus = STATUS_NOT_SUPPORTED;

	return rtStatus;
}

//�������
VOID clear(){
	KLOCK_QUEUE_HANDLE userhandle;
	KeAcquireInStackQueuedSpinLock(&userLock,&userhandle);
	//����û���¼����
	if (!IsListEmpty(&userList)){
		while (!IsListEmpty(&userList)){
			PLIST_ENTRY pEntry = RemoveTailList(&userList);
			PUSER_LIST data = CONTAINING_RECORD(pEntry, USER_LIST, listEntry);
			ExFreePool(data);
		}
	}
	KeReleaseInStackQueuedSpinLock(&userhandle);

	KLOCK_QUEUE_HANDLE handle;
	KeAcquireInStackQueuedSpinLock(&ForbidProcessListLock, &handle);
	//��ռ�¼��ֹ���̵�����
	if (!IsListEmpty(&ForbidProcessList)){
		while (!IsListEmpty(&ForbidProcessList)){
			PLIST_ENTRY pEntry = RemoveTailList(&ForbidProcessList);
			PForbidProcess data = CONTAINING_RECORD(pEntry, ForbidProcess, listEntry);
			ExFreePool(data);
		}
	}
	KeReleaseInStackQueuedSpinLock(&handle);
}

//���Ʒַ�����
NTSTATUS SSDTDeviceIoControlDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	KdPrint(("IOCtrl Dispatch"));
	NTSTATUS rtStatus = STATUS_SUCCESS;
	ULONG uPID = 0;
	ULONG uInLen;
	ULONG uOutLen;
	ULONG uCtrlCode;
	PIO_STACK_LOCATION pStack;
	//SkyEye => SSDT IOCTL_SSDT_ADD/IOCTL_SSDT_DELETE
	PULONG pInBuffer = NULL;
	//CP  =>  SSDT���洢 IOCTL_SSDT_SEND_UP
	PUSER_PASSWORD pUP = NULL;
	PUSER_LIST userList = NULL;
	//ע���&SSDT  => SkyEye IOCTL_SSDT_GET_UP
	PCP_TO_R3 pcp2r3 = NULL;
	//SkyEye  => ע���  IOCTL_SSDT_SET_REG
	PR3_TO_REG pRTR = NULL;
	//ע��� => CP  IOCTL_SSDT_GET_REG
	PREG_TO_CP pRTCC = NULL;
	//ɾ���Ľ�ֹ������
	PForbidProcessName addProcesName = NULL;
	//ɾ���Ľ�ֹ������
	PForbidProcessName deleteProcesName = NULL;

	pStack = IoGetCurrentIrpStackLocation(pIrp);//�õ���ǰIO��ջ
	uInLen = pStack->Parameters.DeviceIoControl.InputBufferLength;//�õ����뻺������С
	uOutLen = pStack->Parameters.DeviceIoControl.OutputBufferLength;//�õ������������С
	uCtrlCode = pStack->Parameters.DeviceIoControl.IoControlCode;//�õ�IOCTL��

	switch (uCtrlCode){
	case IOCTL_SSDT_ADD://--------------SkyEye����ǰ���̼ӱ���
		KdPrint(("----------Add----------"));
		pInBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
		//���뱻��������
		InsertProtectProcess(*pInBuffer);
		//���뱻���ؽ���
		InsertHideProcess(*pInBuffer);
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_DELETE://--------------SKyEyeȥ����ǰ���̱���
		KdPrint(("----------Delete----------"));
		pInBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
		//�Ƴ�����������
		RemoveProtectProcess(*pInBuffer);
		//�Ƴ������ؽ���
		RemoveHideProcess(*pInBuffer);
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_SEND_UP://--------------CP���͵�ǰ�û��˻�������
		KdPrint(("----------send-UP---------"));
		pUP = (PUSER_PASSWORD)pIrp->AssociatedIrp.SystemBuffer;
		//�ͷ�ԭʼ��pUL���ڴ�����ݣ��������·���
		ExFreePool(pc2r);
		//���䲢��ʼ��pc2r
		pc2r = (PCP_TO_R3)ExAllocatePool(NonPagedPool,sizeof(CP_TO_R3));
		RtlZeroMemory(pc2r,sizeof(CP_TO_R3));
		for (int i = 0; i < 10; i++){
			pc2r->DBNAME[i] = '#';
			pc2r->HOST[i] = '#';
		}
		//��USER��ֵ
		for (int i = 0; i < pUP->userLength; i++){
			if (pUP->USER[i] <= 'z' && pUP->USER[i] >= '0'){
				pc2r->USER[i] = pUP->USER[i];
			}
			else break;
		}
		pc2r->userLen = pUP->userLength;
		//��PASSWORD��ֵ
		for (int i = 0; i < pUP->passwordLength; i++){
			if (pUP->PASSWORD[i] <= 'z' && pUP->PASSWORD[i] >= '0'){
				pc2r->PASSWORD[i] = pUP->PASSWORD[i];
			}
			else break;
		}
		pc2r->passLen = pUP->passwordLength;
		//-------------------------��������----------------------------------
		userList = (PUSER_LIST)ExAllocatePool(NonPagedPool,sizeof(USER_LIST));
		RtlZeroMemory(userList,sizeof(USER_LIST));
		for (int i = 0; i < strlen(pc2r->USER); i++)
			userList->UserPassword.USER[i] = pc2r->USER[i];
		for (int i = 0; i < strlen(pc2r->PASSWORD); i++)
			userList->UserPassword.PASSWORD[i] = pc2r->PASSWORD[i];
		if (InsertUserList(userList) == STATUS_UNSUCCESSFUL)
			rtStatus = STATUS_UNSUCCESSFUL;
		userList->UserPassword.passwordLength = pUP->passwordLength;;
		userList->UserPassword.userLength = pUP->userLength;

		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_GET_UP://----------------------SkyEye��õ�ǰ�û��˻��������DBhost,DBname
		KdPrint(("----------R3-get-UP---------"));
		pcp2r3 = (PCP_TO_R3)pIrp->AssociatedIrp.SystemBuffer;
		//���ñ�־Ϊ��
		SkyEyeGetReg = TRUE;
		//��USERֵ��pcp2r3
		for (int i = 0; i < pc2r->userLen; i++){
			if (pc2r->USER[i] <= 'z' && pc2r->USER[i] >= '0'){
				pcp2r3->USER[i] = pc2r->USER[i];
				KdPrint(("User:%c\n", pcp2r3->USER[i]));
			}
			else break;
		}
		pcp2r3->userLen = pc2r->userLen;
		//��PASSWORDֵ��pcp2r3
		for (int i = 0; i < strlen(pc2r->PASSWORD); i++){
			if (pc2r->PASSWORD[i] <= 'z' && pc2r->PASSWORD[i] >= '0'){
				pcp2r3->PASSWORD[i] = pc2r->PASSWORD[i];
				KdPrint(("Password:%c\n", pcp2r3->PASSWORD[i]));
			}
			else break;
		}
		pcp2r3->passLen = pc2r->passLen;
		//��ȡע���DB���ֵ������pc2r
		QueryDBKey();

		for (int i = 0; i < strlen(pc2r->HOST); i++)
			pcp2r3->HOST[i] = pc2r->HOST[i];
		for (int i = 0; i < strlen(pc2r->DBNAME); i++)
			pcp2r3->DBNAME[i] = pc2r->DBNAME[i];
		//���û�ԭֵ
		SkyEyeGetReg = FALSE;
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = sizeof(CP_TO_R3);
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_SET_REG://----------------------SKyEye����ע����ֵ
		pRTR = (PR3_TO_REG)pIrp->AssociatedIrp.SystemBuffer;
		if (NT_SUCCESS(CreateKey())){
			rtStatus = SetDBKey(pRTR->dbName, pRTR->dbHost);
			rtStatus = SetSYSKey(pRTR->sysAdmin, pRTR->sysPassword);
			if (rtStatus == STATUS_UNSUCCESSFUL)
				KdPrint(("Set Register UnSuccessful!"));
			else
				KdPrint(("Set Register Successful!"));
		}
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_GET_REG://----------------------CP��Registry����װ��Ϣ
		pRTCC = (PREG_TO_CP)pIrp->AssociatedIrp.SystemBuffer;
		QueryDBKey();
		QuerySYSKey();
		//dbHost
		for (int i = 0; i < strlen(pRTC->dbHost); i++){
			if (pRTC->dbHost[i] <= 'z' && pRTC->dbHost[i] >= '0')
				pRTCC->dbHost[i] = pRTC->dbHost[i];
			else break;
		}
		//dbName
		for (int i = 0; i < strlen(pRTC->dbName); i++){
			if (pRTC->dbName[i] <= 'z' && pRTC->dbName[i] >= '0')
				pRTCC->dbName[i] = pRTC->dbName[i];
			else break;
		}
		//sysAdmin
		for (int i = 0; i < strlen(pRTC->sysAdmin); i++){
			if (pRTC->sysAdmin[i] <= 'z' && pRTC->sysAdmin[i] >= '0')
				pRTCC->sysAdmin[i] = pRTC->sysAdmin[i];
			else
				break;
		}
		//sysPassword
		for (int i = 0; i < strlen(pRTC->sysPassword); i++){
			if (pRTC->sysPassword[i] <= 'z' && pRTC->sysPassword[i] >= '0')
				pRTCC->sysPassword[i] = pRTC->sysPassword[i];
			else break;
		}
			
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = sizeof(REG_TO_CP);
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_FORBIDPROCESS_ADD://-----------------------------���ӽ�ֹ������
		addProcesName = (PForbidProcessName)pIrp->AssociatedIrp.SystemBuffer;
		PForbidProcess insertFP = (PForbidProcess)ExAllocatePool(NonPagedPool,sizeof(ForbidProcess));
		RtlZeroMemory(insertFP,sizeof(ForbidProcess));
		KdPrint(("add ProcessName: %ws\n",addProcesName->ProcessName));
		KdPrint(("add process Name:%d\n",addProcesName->length));
		for (int i = 0; i < addProcesName->length; i++)
		{
			KdPrint(("ProcessName %c\n",addProcesName->ProcessName[i]));
			insertFP->ProcessName[i] = addProcesName->ProcessName[i];
		}
		if (InsertForbidProcessList(insertFP)){
			KdPrint(("----SSDT---Insert--ForbidProcessName--Success!\n"));
		}
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	case IOCTL_SSDT_FORBIDPROCESS_DELETE://----------------------------ɾ����ֹ������
		deleteProcesName = (PForbidProcessName)pIrp->AssociatedIrp.SystemBuffer;
		KdPrint(("delete ProcessName: %ws\n", deleteProcesName->ProcessName));
		PLIST_ENTRY entry = NULL;
		if (IsListEmpty(&ForbidProcessList)){
			KdPrint(("---SSDT---ForbidProcessList is Empty!"));
		}
		else{
			for (entry = ForbidProcessList.Flink;
				entry != &ForbidProcessList;
				entry = entry->Flink){

				KdPrint(("--SSDT----Entry IN delete"));
				PForbidProcess fP = CONTAINING_RECORD(entry, ForbidProcess, listEntry);
				int count = 0;//��ȵ���Ŀ
				for (int i = 0; i < deleteProcesName->length; i++)
				{
					if (deleteProcesName->ProcessName[i] == fP->ProcessName[i]){
						count++;
					}else break;
				}
				KdPrint(("count : %d",count));
				KdPrint(("SSDT-----Now---ProcessName:%ws\n",fP->ProcessName));

				if (deleteProcesName->length == count){
					if (RemoveForbidProcessList(fP))
						KdPrint(("----SSDT---Delete--ForbidProcessName--Success!\n"));
					else
						KdPrint(("----SSDT---Delete--ForbidProcessName--Failed!!"));
				}
				KdPrint(("SSDT--------------------------------"));
			}
		}
		
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;

	case IOCTL_SSDT_CLAER_BLACK_ILST://�������
		clear();
		pIrp->IoStatus.Status = rtStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		break;
	default:
		break;
	}
	return rtStatus;
}

VOID SSDTDriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING strSymbolLinkName;

	DbgPrint("In SSDTDriverUnload !");

	RtlInitUnicodeString(&strSymbolLinkName, SYMBOLINK_NAME_PROCESS);
	IoDeleteSymbolicLink(&strSymbolLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	//��� Hook
	UnInstallSSDTHook((ULONG)ZwQuerySystemInformation);
	UnInstallSSDTHook((ULONG)ZwTerminateProcess);
	//���ý��̻ص�Ϊȥ��״̬
	PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyEx, TRUE);

	//���������Դ
	clear();

	if (pc2r != NULL)
		ExFreePool(pc2r);
	if (pRTC != NULL)
		ExFreePool(pRTC);

	DbgPrint("Out SSDT01DriverUnload !");
}

//���� uPID �����������б��е�����������ý����������б��в����ڣ��򷵻� -1
ULONG ValidateProcessNeedHide(ULONG uPID)
{
	ULONG i = 0;
	if (uPID == 0)
	{
		return -1;
	}

	for (i = 0; i<g_currHideArrayLen && i<MAX_PROCESS_ARRARY_LENGTH; i++)
	{
		if (g_PIDHideArray[i] == uPID)
		{
			return i;
		}
	}
	return -1;
}

//���� uPID �����ڱ����б��е�����������ý����ڱ����б��в����ڣ��򷵻� -1
ULONG ValidateProcessNeedProtect(ULONG uPID)
{
	ULONG i = 0;
	if (uPID == 0)
	{
		return -1;
	}

	for (i = 0; i<g_currProtectArrayLen && i<MAX_PROCESS_ARRARY_LENGTH; i++)
	{
		if (g_PIDProtectArray[i] == uPID)
		{
			return i;
		}
	}
	return -1;
}

//�ڽ��������б��в����µĽ��� ID
ULONG InsertHideProcess(ULONG uPID)
{
	KdPrint(("Insert Hide Process====>\n"));
	if (ValidateProcessNeedHide(uPID) == -1 &&
		g_currHideArrayLen < MAX_PROCESS_ARRARY_LENGTH)
	{
		g_PIDHideArray[g_currHideArrayLen++] = uPID;
		return TRUE;
	}
	return FALSE;
}

//�ӽ��������б����Ƴ����� ID
ULONG RemoveHideProcess(ULONG uPID)
{
	KdPrint(("Remove Hide Process====>\n"));
	ULONG uIndex = ValidateProcessNeedHide(uPID);
	if (uIndex != -1)
	{
		g_PIDHideArray[uIndex] = g_PIDHideArray[g_currHideArrayLen--];
		return TRUE;
	}
	return FALSE;
}

//�ڽ��̱����б��в����µĽ��� ID
ULONG InsertProtectProcess(ULONG uPID)
{
	KdPrint(("Insert Protect Process====>\n"));

	if (ValidateProcessNeedProtect(uPID) == -1 && g_currProtectArrayLen < MAX_PROCESS_ARRARY_LENGTH)
	{
		g_PIDProtectArray[g_currProtectArrayLen++] = uPID;

		return TRUE;
	}
	return FALSE;
}

//�ڽ��̱����б����Ƴ�һ������ ID
ULONG RemoveProtectProcess(ULONG uPID)
{
	KdPrint(("Remove Protect Process====>\n"));
	ULONG uIndex = ValidateProcessNeedProtect(uPID);
	if (uIndex != -1)
	{
		g_PIDProtectArray[uIndex] = g_PIDProtectArray[g_currProtectArrayLen--];

		return TRUE;
	}
	return FALSE;
}

//NEW��NtQuerySystemInformation
NTSTATUS MyNtQuerySystemInformation(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength
	)
{
	//KdPrint(("MyNtQuerySystemInformation===>\n"));
	NTSTATUS rtStatus;
	pOldNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)oldSysServiceAddr[SYSCALL_INDEX(ZwQuerySystemInformation)];

	rtStatus = pOldNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

	if (NT_SUCCESS(rtStatus)){
		/*
			1.ȷ����ǰQuery���ǽ�����Ϣ
			2.��ȡ��ǰSystemInformation��PSYSTEM_PROCESS_INFORMATION
			3.��ʼѭ����������PSYSTEM_PROCESS_INFORMATION��
				3-1.��õ�ǰPROCESS_INFORMATION��UniqueProcessId
				3-2.�жϵ�ǰCurrProcessId�Ƿ���Hide������
				3-3.��Hide�����ڣ�
					�ж�PrevProcessInfo�Ƿ�Ϊ�գ�Ҳ����˵���Ƿ��ǵ�һ��ProcessInfo
					����ǣ��ͽ��˽�����Ϣ����Ϣ����Ĩ��
					������ǣ���ȷ���˸տ�ʼ�����ͷ�����Ҫ���ص�ProcessInfo,Ĩ����
					
					����أ������ǲ��ǵ�һ��ProcessInfo��Ҫ�ж��Ƿ�����һ��ProcessInfo��
					��Ϊ�в�ͬ�Ĵ���ʽ��1)���� 2)��ǰֱ������/��
				3-4.����Hide�����ڣ�
					������ǰ������ִ��NextEntry
			
		*/

		if (SystemProcessInformation == SystemInformationClass)//ȷ����ǰQuery���ǽ�����Ϣ
		{
			PSYSTEM_PROCESS_INFORMATION pPrevProcessInfo = NULL;
			PSYSTEM_PROCESS_INFORMATION pCurrProcessInfo = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;

			while (pCurrProcessInfo != NULL)//ȷ����ǰ���ʵĽ�����Ϣ��Ϊ��
			{
				//��ȡ��ǰ������ SYSTEM_PROCESS_INFORMATION �ڵ�Ľ������ƺͽ��� ID
				ULONG uPID = (ULONG)pCurrProcessInfo->UniqueProcessId;
				UNICODE_STRING strTmpProcessName = pCurrProcessInfo->ImageName;//��ִ���ļ���

				//�жϵ�ǰ��������������Ƿ�Ϊ��Ҫ���صĽ���
				if (ValidateProcessNeedHide(uPID) != -1)
				{
					if (pPrevProcessInfo)
					{
						if (pCurrProcessInfo->NextEntryOffset)
						{
							//����ǰ�������(��Ҫ���صĽ���)�� SystemInformation ��ժ��(��������ƫ��ָ��ʵ��)
							pPrevProcessInfo->NextEntryOffset += pCurrProcessInfo->NextEntryOffset;
						}
						else
						{
							//˵����ǰҪ���ص���������ǽ��������е����һ��
							pPrevProcessInfo->NextEntryOffset = 0;
						}
					}
					else
					{
						//��һ���������ý��̾�����Ҫ���صĽ���
						if (pCurrProcessInfo->NextEntryOffset)
						{
							(PCHAR)SystemInformation += pCurrProcessInfo->NextEntryOffset;
						}
						else
						{
							SystemInformation = NULL;
						}
					}
				}

				//������һ�� SYSTEM_PROCESS_INFORMATION �ڵ�
				pPrevProcessInfo = pCurrProcessInfo;

				//��������
				if (pCurrProcessInfo->NextEntryOffset)
				{
					pCurrProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(((PCHAR)pCurrProcessInfo) + pCurrProcessInfo->NextEntryOffset);
				}
				else
				{
					pCurrProcessInfo = NULL;
				}
			}
		}
	}

	return rtStatus;
}

//NEW��NtTerminateProcess
NTSTATUS MyNtTerminateProcess(
	__in_opt HANDLE ProcessHandle,
	__in NTSTATUS ExitStatus
	)
{
	//KdPrint(("MyNtTerminateProcess===>\n"));

	ULONG uPID;
	NTSTATUS rtStatus;
	PCHAR pStrProcName;
	PEPROCESS pEProcess;
	ANSI_STRING strProcName;

	/*
		1.ͨ��ObReferenceObjectByHandle��õ�ǰ���̾����EPROCESS;
		2.����packUp����ZwTerminateProcess����ָ��;
		3.��EPROCESS��ý��̵�PID;
		4.��鵱ǰPID�Ƿ���Terminate���У�
		4-1.�ڵĻ����ж��Ƿ�Ϊϵͳ���̣���ΪPsGetCurrentProcess��õ���ϵͳ����
		�������Ǿ��÷�����Ȩ�ޣ�
		4-2.���ڵĻ������þɵ�ZwTerminateProcess����
		*/

	//ͨ�����̾������øý�������Ӧ�� FileObject �������������ǽ��̶�����Ȼ��õ��� EPROCESS ����
	rtStatus = ObReferenceObjectByHandle(ProcessHandle, FILE_READ_DATA, NULL, KernelMode, &pEProcess, NULL);

	if (!NT_SUCCESS(rtStatus))
	{
		return rtStatus;
	}

	//���� SSDT ��ԭ���� NtTerminateProcess ��ַ
	pOldNtTerminateProcess = (NTTERMINATEPROCESS)oldSysServiceAddr[SYSCALL_INDEX(ZwTerminateProcess)];

	//ͨ���ú������Ի�ȡ���������ƺͽ��� ID���ú������ں���ʵ���ǵ�����(�� WRK �п��Կ���)
	//���� ntddk.h �в�û�е�����������Ҫ�Լ���������ʹ��

	uPID = (ULONG)PsGetProcessId(pEProcess);
	pStrProcName = (PCHAR)PsGetProcessImageFileName(pEProcess);

	KdPrint(("-----SSDT------ImageFile Name: %s\n", pStrProcName));

	//ͨ������������ʼ��һ�� ASCII �ַ���
	RtlInitAnsiString(&strProcName, pStrProcName);

	//���Ҫ�����Ľ������ܱ������б��У�
	if (ValidateProcessNeedProtect(uPID) != -1)
	{
		KdPrint(("ValidateProcessNeedProtect Exist------>\n"));
		//ȷ�������߽����ܹ�����(������Ҫ��ָ taskmgr.exe)
		if (uPID != (ULONG)PsGetProcessId(PsGetCurrentProcess()))
		{
			//����ý������������ĵĽ��̵Ļ����򷵻�Ȩ�޲������쳣����
			return STATUS_ACCESS_DENIED;
		}
	}

	//���ڷǱ����Ľ��̿���ֱ�ӵ���ԭ�� SSDT �е� NtTerminateProcess ����������
	rtStatus = pOldNtTerminateProcess(ProcessHandle, ExitStatus);

	return rtStatus;
}