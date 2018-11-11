#include "stdafx.h"


extern NPAGED_LOOKASIDE_LIST looksidelist; 
extern NPAGED_LOOKASIDE_LIST looksidelist_result;
extern HASH_TABLE hashtable; 
extern HASH_TABLE hashtable_result;
extern ThreadEvents myevents[HASH_TABLE_LENGTH];


extern POperationResult resultfortest;
extern size_t process_name_offset;

/*���������������̣��ṩһ�����ĵ��ýӿ�
   �����������̴�����£��ڵ�ǰ�߳��ռ���Ϣ��
   Ȼ�����������ѯ��ǰ���ļ��Ƿ�Ϊ���ĵ��ļ�
   ����ǣ���д��Ȼ��ȴ�Ӧ�ó���ķ�������
   �ݽ�������Ӧ�Ĵ���������Ĭ�ϵ��߼����� */
NTSTATUS VerifyControl(POperationInformation operationinformation,POperationResult result)
{
	        int flag=0;
	        ULONG ID;
			NTSTATUS statu;
		/*	KIRQL oldirqls;
		KeAcquireSpinLock(&test_lock,&oldirqls);
		KdPrint((" in lock!\n"));
		if((intest!=0))
		{
			KdPrint(("ֱ�ӷ��أ�\n"));
			KeReleaseSpinLock(&test_lock,oldirqls);
			statu=STATUS_ACCESS_DENIED;
			irp->IoStatus.Information=0;
			irp->IoStatus.Status=statu;
			IoCompleteRequest(irp,IO_NO_INCREMENT);
			return statu;
		}
		KdPrint(("leave lock\n"));
		 intest++;
		KeReleaseSpinLock(&test_lock,oldirqls);*/
		  //  InitOperationInformation(operationinformation);
			//operationinformation->finished=event;
			// InitOperationResult(result);
			//::RtlInitUnicodeString(&(operationinformation->FileName),(PCWSTR)directory->Buffer);

			//::RtlInitUnicodeString(&(operationinformation->FilePath),(PCWSTR)directory->Buffer);
			//operationinformation->OperationType=IRP_MJ_SET_INFORMATION;
			//result->operationtype=IRP_MJ_SET_INFORMATION;

			KdPrint(("����OperationInformation�ȴ���֤%d!\n",::KeGetCurrentIrql()));
			
			ID=::FillOperationInformationAndAddToList(IRP_MJ_SET_INFORMATION,operationinformation,NULL);
			result->operationID=ID;
			result->wasFill=false;
			wcsncpy(result->otherinfo,L"flag --\r\n",wcslen(L"flag --\r\n\n"));
			::FillOperationResultAddToHashTable_Result(result);
			KdPrint(("&&&&&&&&&&&&&&&&&&&&&&&&&&&wait for current irql:%d",KeGetCurrentIrql()));
			statu=KeWaitForSingleObject(&(operationinformation->finished), Executive, KernelMode, FALSE, NULL);
			
			KdPrint((" event current irql:%d",::KeGetCurrentIrql()));
			bool permit=false;
			
			if(result!=NULL &&result->wasFill)
			{
			KdPrint(("�ڹ�ϣ����ȡ����֤���!:%d�ͷ��ڴ�\n",result->operation_permit?1:0));
			permit=result->operation_permit;
			::ExFreeToNPagedLookasideList(&looksidelist_result,result);
			}
			else
			KdPrint(("�������⣺ȡ��һ���յĽ��!\n\n"));

			::ExFreeToNPagedLookasideList(&looksidelist,operationinformation);  //��hashtable����Ķ�Ӧ��operationinformation�ṹ�����������ͷų���
			if(permit)
			{
			KdPrint(("�������ܾ�! %wZ\n",&operationinformation->FileName));
			statu=STATUS_ACCESS_DENIED;
			return statu;
			}
			else
			{
				statu=STATUS_SUCCESS;
				KdPrint(("����������%wZ\n",&operationinformation->FileName));
			}
			return statu;
};
VOID     AllocatePool(OUT POperationInformation &operationinformation,OUT POperationResult &result)
{
	KdPrint(("AllocarePool In!\n\n"));
	operationinformation=(POperationInformation)::ExAllocateFromNPagedLookasideList(&looksidelist);
	result=(POperationResult)::ExAllocateFromNPagedLookasideList(&looksidelist_result);

    if(operationinformation==NULL || result==NULL)
	{
		KdPrint(("allocate from lookaside is failed!\n"));
		operationinformation=NULL;
		result=NULL;
	}
	InitOperationInformation(operationinformation);
			
	InitOperationResult(result);

}

