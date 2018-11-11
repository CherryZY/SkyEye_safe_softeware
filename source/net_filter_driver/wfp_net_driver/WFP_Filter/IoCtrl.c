#include "FilterLayers.h"
#include "IoCtrl.h"
#include <ndis.h>
#include <wdm.h>
#define INITGUID

extern KSPIN_LOCK DataListLock;
extern LIST_ENTRY DataList;

//---------------------------------------

//ͬ���¼�����  
extern PRKEVENT g_pEventObject;
//�����Ϣ  
extern OBJECT_HANDLE_INFORMATION g_ObjectHandleInfo;
//�Ƿ�ʼ����
extern BOOLEAN StartMonitor;

//---------------------------------------

//Device Io Control
NTSTATUS TransferControl(PDEVICE_OBJECT pDO, PIRP pIrp,int *Info){
	KdPrint(("Transfer Data Control\n"));
	NTSTATUS nStatus = STATUS_SUCCESS;

	PIO_STACK_LOCATION	IrpStack = NULL;
	PTRANSFER_R3 pSystemBuffer = NULL;
	PRULE_FROM_R3 prfr = NULL;
	ULONG uInLen = 0;
	
	IrpStack = IoGetCurrentIrpStackLocation(pIrp);

	if (IrpStack != NULL)
	{
		uInLen = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		if (IrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
		{
			// ��ʼ����DeivceIoControl�����
			switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
			{
			case IOCTL_WFP_MONITOR://�û����ñ���ص���Ϣ
				pSystemBuffer = (PTRANSFER_R3)pIrp->AssociatedIrp.SystemBuffer;
				KdPrint(("-----------Monitor-----------\n"));
				if (IsListEmpty(&DataList))
					KdPrint(("DataList is Empty!\n"));
				else{
					MonitorData(pSystemBuffer);
				}
					
				KdPrint(("-------END----Monitor--------\n"));
				*Info = sizeof(TRANSFER_R3);
				break;
			case IOCTL_WFP_FORBIDDEN://���������ӹ����
				prfr = (PRULE_FROM_R3)pIrp->AssociatedIrp.SystemBuffer;
				KdPrint(("-----------Forbidden-----------\n"));
				KdPrint(("imageName From App :%s\n", prfr->ImageName));
				nStatus = AddRuleFromR3(prfr);
				KdPrint(("-------END----Forbidden--------\n"));
				*Info = 0;
				break;
			case IOCTL_WFP_REMOVE_RULE://�������Ƴ�ָ������
				prfr = (PRULE_FROM_R3)pIrp->AssociatedIrp.SystemBuffer;
				KdPrint(("----------RemoveRule-----------\n"));
				if (STATUS_SUCCESS == RemoveRule(prfr))
					KdPrint(("RemoveListSuccess!\n"));
				else
					KdPrint(("RemoveListFailed!\n"));
				KdPrint(("-------END----RemoveRule-------\n"));
				*Info = 0;
				break;
			case IOCTL_WFP_START_EVENT://��ʼ�¼�
				pSystemBuffer = (PTRANSFER_R3)pIrp->AssociatedIrp.SystemBuffer;
				KdPrint(("-------start EVENT-------\n"));
				//����ͬ���¼�  
				if (pSystemBuffer == NULL || uInLen < sizeof(HANDLE)){
					KdPrint(("Set Event Error~!\n"));
					break;
				}
				//ȡ�þ������  
				HANDLE hEvent = *(HANDLE*)pSystemBuffer;
				nStatus = ObReferenceObjectByHandle(hEvent, GENERIC_ALL, NULL, KernelMode, (PVOID*)&g_pEventObject, &g_ObjectHandleInfo);
				KdPrint(("g_pEventObject = 0x%X\n", g_pEventObject));
				//����û�п�ʼ����,����Ϊ��ʼ����
				if (!StartMonitor){
					StartMonitor = TRUE;
				}
				KdPrint(("-------end  start EVENT-------\n"));
				break;
			case IOCTL_WFP_STOP_EVENT://ֹͣ�¼�
				KdPrint(("-------stop EVENT-------\n"));
				if (StartMonitor){
					StartMonitor = FALSE;
				}
				//�ͷŶ�������  
				if (g_pEventObject != NULL){
					ObDereferenceObject(g_pEventObject);
					g_pEventObject = NULL;
				}
				KdPrint(("-------end stop EVENT-------\n"));
				break;
			case IOCTL_WFP_CLEAR_BLACK_LIST:
				WfpRuleListUnInit();
				break;
			default:
				nStatus = STATUS_UNSUCCESSFUL;
			}
		}
	}
	return nStatus;
}

//�������Ҳ����Ӧ�ò��ȡ�ں˲㵱ǰ��Ϣ
VOID MonitorData(PTRANSFER_R3 pSystemBuffer){
	KdPrint(("Monitor Data\n"));

	KLOCK_QUEUE_HANDLE handle;
	KeAcquireInStackQueuedSpinLock(&DataListLock, &handle);

	//KdPrint(("DataList is Not Empty!\n"));
	LIST_ENTRY* entry = RemoveHeadList(&DataList);
	PBIND_DATA_LIST data = CONTAINING_RECORD(entry, BIND_DATA_LIST, listEntry);
	PEPROCESS epro = NULL;
	UCHAR *pImageName = NULL;

	//���PEPROCESS����
	epro = LookupProcess((HANDLE)data->uProcessID);
	if (epro)
		pImageName = PsGetProcessImageFileName(epro);

	memcpy(data->imageName, pImageName, MAX_NAME);

	//KdPrint(("data->imageName:%s\n", data->imageName));

	//IsSend
	pSystemBuffer->bIsSend = data->bIsSend;
	//���ݳ���
	pSystemBuffer->uDataLength = data->uDataLength;
	//KdPrint(("pSystemBuffer->uDataLength:%d\n", pSystemBuffer->uDataLength));
	//���ض˿�
	pSystemBuffer->uLocalPort = data->uLocalPort;
	//Զ�̶˿�
	pSystemBuffer->uRemotePort = data->uRemotePort;
	//����ID
	pSystemBuffer->uProcessID = data->uProcessID;
	//����IP
	pSystemBuffer->SourceIp = data->localAddressV4;
	//Զ��IP
	pSystemBuffer->RemoteIp = data->remoteAddressV4;

	//imageName����
	for (int i = 0; i < 64; i++){
		pSystemBuffer->imageName[i] = data->imageName[i];
	}

	//processPath����
	for (int j = 0; j < 1024; j++){
		pSystemBuffer->wProcessPath[j] = data->wProcessPath[j];
	}

	//KdPrint(("ProcessID: %d\n", pSystemBuffer->uProcessID));
	//KdPrint(("ProcessPath: %ws\n", pSystemBuffer->wProcessPath));
	//KdPrint(("ImageName: %s\n", pSystemBuffer->imageName));

	ExFreePoolWithTag(data, 'ttr');

	KeReleaseInStackQueuedSpinLock(&handle);
}

//���ݽ���ID���ؽ���EPROCESS��ʧ�ܷ���NULL
PEPROCESS LookupProcess(HANDLE Pid)
{
	PEPROCESS eprocess = NULL;
	if (NT_SUCCESS(PsLookupProcessByProcessId(Pid, &eprocess)))
		return eprocess;
	else
		return NULL;
}

