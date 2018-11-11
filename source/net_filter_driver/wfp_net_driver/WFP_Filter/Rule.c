#include "FilterLayers.h"
#include <ntddk.h>
#include <wdm.h>

extern LIST_ENTRY RuleList;

extern KSPIN_LOCK  RuleListLock;

VOID WfpRuleListInit(){
	
}

VOID WfpRuleListUnInit(){
	KdPrint(("Rule List UnInit\n"));
	KLOCK_QUEUE_HANDLE handle;
	KeAcquireInStackQueuedSpinLock(&RuleListLock,&handle);
	if (!IsListEmpty(&RuleList)){
		while (!IsListEmpty(&RuleList)){
			PLIST_ENTRY pEntry = RemoveTailList(&RuleList);
			PRULES_LIST data = CONTAINING_RECORD(pEntry, RULES_LIST, listEntry);
			ExFreePool(data);
		}
	}
	KeReleaseInStackQueuedSpinLock(&handle);
}

//���Rule
NTSTATUS AddRuleFromR3(PRULE_FROM_R3 pRFR){
	NTSTATUS Status = STATUS_SUCCESS;

	PRULES_LIST p = ExAllocatePool(NonPagedPool, sizeof(RULES_LIST));
	RtlZeroMemory(p, sizeof(RULES_LIST));

	int sizeIN = strlen(pRFR->ImageName);
	int sizePP = wcslen(pRFR->ProcessPath);

	for (int i = 0; i < sizeIN; i++){
		p->Rule.ImageName[i] = pRFR->ImageName[i];
	}

	for (int j = 0; j < sizePP; j++){
		p->Rule.ProcessPath[j] = pRFR->ProcessPath[j];
	}

	Status = InsertRuleList(p);
	if (Status == STATUS_UNSUCCESSFUL){
		KdPrint(("Insert Failed!\n"));
	}
	else{
		KdPrint(("Insert Success!\n"));
	}

	return Status;
}

//���뵽RuleList
NTSTATUS InsertRuleList(PRULES_LIST rl){
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	KLOCK_QUEUE_HANDLE LockHandle;

	KeAcquireInStackQueuedSpinLock(&RuleListLock, &LockHandle);

	KdPrint(("Insert Rule List\n"));

	InsertHeadList(&RuleList, &rl->listEntry);
	Status = STATUS_SUCCESS;

	KeReleaseInStackQueuedSpinLock(&LockHandle);
	return Status;
}

//�Ƴ�ָ����Rule��RuleList
NTSTATUS RemoveRule(PRULE_FROM_R3 rl){
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	KdPrint(("Remove Rule List\n"));
	KLOCK_QUEUE_HANDLE lockHandle;
	KeAcquireInStackQueuedSpinLock(&RuleListLock,&lockHandle);

	LIST_ENTRY* entry = NULL;
	PRULES_LIST p = NULL;
	int time = 1;

	if (IsListEmpty(&RuleList)){
		KdPrint(("Rule List is Empty!!!\n"));
	}
	else{
		for (entry = RuleList.Flink;
			entry != &RuleList;
			entry = entry->Flink){
			//�ȱ���������������ȷ�����ĸ��ڵ�
			int Sa = 0;
			int Pp = 0;
			
			PRULES_LIST prl = CONTAINING_RECORD(entry, RULES_LIST, listEntry);
			//ImageNameƥ��
			for (int i = 0; i < strlen(rl->ImageName); i++){
				if (prl->Rule.ImageName[i] == rl->ImageName[i])
					Sa += 1;
			}
			//ProcessPathƥ��
			for (int j = 0; j < wcslen(rl->ProcessPath); j++){
				if (prl->Rule.ProcessPath[j] == rl->ProcessPath[j])
					Pp += 1;
			}
			if (Sa == strlen(rl->ImageName)){
				RemoveEntryList(&prl->listEntry);
				Status = STATUS_SUCCESS;
				break;
			}
			if (Pp == wcslen(rl->ProcessPath)){
				//KdPrint(("ProcessPath ƥ�����ڵ�ɹ�������\n"));
			}
			time += 1;
		}
	}
	
	KeReleaseInStackQueuedSpinLock(&lockHandle);

	return Status;
}

//���ݹ��ˣ�ƥ��ɹ�������STATUS_SUCCESS;ƥ�䲻�ɹ�������STATUS_UNSUCCESSFUL
NTSTATUS Data_Filtering(PBIND_DATA_LIST pBDL){
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	LIST_ENTRY* entry = NULL;

	if (IsListEmpty(&RuleList)) KdPrint(("RuleList is Empty\n"));
	else{
		int t = 1;
		//����RuleList
		for (entry = RuleList.Flink;
			entry != &RuleList;
			entry = entry->Flink){

			//KdPrint(("��%d������ƥ�䣡������",t));

			PRULES_LIST prl = CONTAINING_RECORD(entry, RULES_LIST, listEntry);
			int sizeIN = strlen(prl->Rule.ImageName);
			int sizePP = wcslen(prl->Rule.ProcessPath);
			int PPs = 0;
			int INs = 0;

			//ƥ��Rule�е�ImageName
			for (int i = 0; i < sizeIN; i++){
				if (prl->Rule.ImageName[i] == pBDL->imageName[i]){
					INs += 1;
				}
			}
			//KdPrint(("INs:%d\nsizeIN:%d\n",INs,sizeIN));
			if (INs == sizeIN){
				return STATUS_SUCCESS;
			}

			//ƥ��Rule�е�ProcessPath
			for (int j = 0; j < sizePP; j++){
				if (prl->Rule.ProcessPath[j] == pBDL->wProcessPath[j])
					PPs += 1;
			}
			if (PPs == sizePP){
				return STATUS_SUCCESS;
			}
			t+=1;
		}
	}
	
	return Status;
}
