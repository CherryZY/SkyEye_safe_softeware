#include "stdafx.h"



//****************sfilter.cpp*********************

extern NPAGED_LOOKASIDE_LIST looksidelist;
extern NPAGED_LOOKASIDE_LIST looksidelist_result;
extern HASH_TABLE hashtable; 
extern HASH_TABLE hashtable_result;
extern PThreadEvents myevents[HASH_TABLE_LENGTH];

extern POperationResult resultfortest;
//**************************************************************************************

BOOLEAN FillOperationResultAddToHashTable_Result(POperationResult operationresult);

PVOID SetOperationResultFromHashTable_Result(ULONG OperationType,ULONG ID,POperationResult result);

VOID GetOperationResult(POperationResult result,ULONG type,ULONG ID);
PLIST_ENTRY GetTheResultNode(PLIST_ENTRY px,ULONG ID);
VOID SetOperationResult(PIRP irp);
VOID MyOperationResultAddToHashTable2(POperationResult result);

BOOLEAN CutOffListNode(ULONG ID,PLIST_ENTRY px);         //ʹOperationResult��������


BOOLEAN CutOffListNode(ULONG ID,PLIST_ENTRY px)
{
	KdPrint(("����ɾ��ָ����OPerationResult�ڵ�!\n\n"));
	PLIST_ENTRY head,temp;
	head=px;
	temp=head->Blink;
	for(temp;temp!=head;temp=temp->Blink)
	{
		POperationResult result=(POperationResult)CONTAINING_RECORD(temp,OperationResult,entry);
		if(result->operationID==ID)
		{
			PLIST_ENTRY flink,blink;
			flink=temp->Blink;
			blink=temp->Flink;
			flink->Flink=blink;
			blink->Blink=flink;
			KdPrint(("�ɹ�ɾ��!\n"));
			return TRUE;
			
		}
	}
	return FALSE;
}

VOID  InitOperationResult(POperationResult re)
{
	if(re==NULL)
		return;
	re->wasFill=false;
	re->operation_permit=0;
	re->operationID=0;
	re->operationtype=0;
	RtlZeroMemory(re->otherinfo,sizeof(WCHAR)*512);
}
VOID SetOperationResult(PIRP irp)  //�Ѵ�app��õ���֤������뵽hashtable_result�в���֪ͨ��ǰ�ȴ����¼�
{
	KIRQL oldirql=::KeGetCurrentIrql();
	KIRQL now=oldirql;
	KdPrint(("SetOperationResult:current irql:%d",oldirql));
	PIO_STACK_LOCATION irpsp=::IoGetCurrentIrpStackLocation(irp);
	PVOID buffer=irp->AssociatedIrp.SystemBuffer;
	POperationResult result;
	POperationResult newresult;




	result=(POperationResult)buffer;
	if(result==NULL)
		return;
	KdPrint(("SetOperationResult:current irql:%d",now));

	KdPrint(("���Ҷ�Ӧ��OperationResult�ṹ��\n\n"));
	newresult=(POperationResult)SetOperationResultFromHashTable_Result(result->operationtype,result->operationID,result);
	KdPrint(("SetOperationResult:current irql:%d\n",now));
	if(newresult==NULL)
	{
		KdPrint(("�յ�OperationResult�ṹ��\n\n"));
		return ;
	}
	KeRaiseIrql(DISPATCH_LEVEL,&oldirql);

	KdPrint(("��ֵǰ��ֵ:\n------ID:%x\n------ operationtype:%x(%d)(%d)\n------permit:%d otherinfo %ws\n",
		newresult->operationID,newresult->operationtype,newresult->operationtype,result->operationtype,newresult->operation_permit?1:0,newresult->otherinfo));
	KdPrint(("��Ӧ�û����֤��Ϣ����:\n------ID:%x\n------ operationtype:%x(%d)(%d) \n------permit:%d\n",
		result->operationID,result->operationtype,result->operationtype,result->operationtype,result->operation_permit?1:0));

	KdPrint(("��ֵ����Ϣ����:\n------ID:%x\n------ operationtype:%x(%d) \n------permit:%d\n wasfill:%d\n",
		newresult->operationID,newresult->operationtype,newresult->operationtype,newresult->operation_permit?1:0,newresult->wasFill?1:0));
	
	//�ѻ���µĽ�����뵽��ϣ�ȱ�Ȼ�����Ӧ��OperationInformation���Ұ����ͷŵ�
	 MyOperationResultAddToHashTable2(newresult);

	PHASH_TABLE_NODE tablenode;
	ULONG number=HashFunction_ForOperation(result->operationtype);
	tablenode=&(hashtable.link[number]);
	 	PLIST_ENTRY p,head;
	POperationInformation results;
	head=&tablenode->entry;
	::HashTableNodeListLock(tablenode);
		 	now=::KeGetCurrentIrql();
	KdPrint(("SetOperationResult:current irql:%d\n",now));
	for(p=head->Flink;p!=head;p=p->Flink)        
	{
        results=(POperationInformation)p;
		if(newresult->operationID==results->OperationID)
		{
			KdPrint(("\n�ɹ����ҵ���Ӧ��OperationInformation�ṹ\n\n"));
			//DbgBreakPoint();
			PLIST_ENTRY flink,blink;
			flink=p->Blink;
			blink=p->Flink;
			flink->Flink=blink;
			blink->Blink=flink;   //ʹ��ǰ�Ĳ�����Ϣ�ṹ����hashtable,�������ֱ���ͷ�����ڴ�
			::KeSetEvent(&(results->finished),IO_NO_INCREMENT,FALSE);//֪ͨ��Ӧ���̸߳ò����Ѿ�����֤
			KdPrint(("kesetevent\n\n"));	
			break;
		}
	}

	::HashTableNodeListUnLock(tablenode);
	now=::KeGetCurrentIrql();
	KdPrint(("current irql:%d\n",now));
	if(oldirql!=now)
	{
		KdPrint(("irql change!\n\n"));
	            //�ָ���ԭ����IRQL�ȼ�����������IRQL_GT_ZERO_AT_SYSTEM_SERVICE����������
		
	}	
	KeLowerIrql(oldirql);
	ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL);
	KdPrint(("leave from SetOperaitonResult\n\n"));
}
VOID GetOperationResult(POperationResult result,ULONG type,ULONG ID)//
{
	ULONG number=HashFunction_ForOperation(type);
	PHASH_TABLE_NODE tablenode;
	KdPrint(("GetOperationResult!\n\n"));
	tablenode=&(hashtable_result.link[number]);
	if(tablenode->OperationType!=type)
	{
		KdPrint(("���������ڱȶ�IRP_TYPEʱ%d --%d(GetOperationResult)",tablenode->OperationType,type));
		return;
	}
	PLIST_ENTRY p,head;
	POperationResult results;
	head=&tablenode->entry;
	::HashTableNodeListLock(tablenode);
	for(p=head->Flink;p!=head;p=p->Flink)        
	{
        results=(POperationResult)p;
		if(results->operationID==ID)
		{
			result=results;
			break;
		}
	}
	::HashTableNodeListUnLock(tablenode);
	
}
BOOLEAN FillOperationResultAddToHashTable_Result(POperationResult operationresult)
{
	if(operationresult==NULL)
		return FALSE;
	if(operationresult->operationID==0 || operationresult->operationtype==0)
		return FALSE;
	KdPrint(("����յĽ�������ȴ�������д:%d!\n",::KeGetCurrentIrql()));
	 MyOperationResultAddToHashTable2(operationresult);
	 KdPrint(("�������!%d\n",::KeGetCurrentIrql()));
	 return true;
}
PVOID SetOperationResultFromHashTable_Result(ULONG OperationType,ULONG ID,POperationResult result)
{
	KdPrint(("��ʼ���Ҳ�������Ϊ%d(%x) IDΪ��%d ��OperationResult �ṹ��",OperationType,OperationType,OperationType,ID));
	CHAR message[124];
	unsigned short number;//��ǰ�����ĵ�irp�洢���ĸ��ڵ�����
	PHASH_TABLE_NODE tablenode;
	ULONG allnodenumber;
	number=HashFunction_ForOperation(OperationType);
	if(result==NULL)
	{
		strcpy(message,"POperationResult is NULL in function SetOperationResultFromHashTable_Result!\n\n");
		goto ERROR;
		
		return NULL;
	}
	if(number<0 ||number>=HASH_TABLE_LENGTH)//
	{
		strcpy(message,"number is illegal!\n");
		goto ERROR;
		
		return NULL;
	}
	tablenode=&(hashtable_result.link[number]);
    HashTableNodeListLock(tablenode);	

	if(tablenode->OperationType!=OperationType) //�����ǰ�Ľڵ�洢��irp���Ͳ�������ô����
	{
		KdPrint(("tablenode->OperationType:%d %x OperationType:%d %x number:%d",tablenode->OperationType,tablenode->OperationType,OperationType,OperationType,number));
		strcpy(message,"OperationType mismatch!\n");
		goto ERROR;
		return NULL;
	}
	
	allnodenumber=tablenode->number;
	PLIST_ENTRY px=&tablenode->entry;
	if(allnodenumber==0 )                         //�����ǰ�ڵ�����û�йҽ�OperationInformation
	{
		if(!IsListEmpty(px))
		{
			tablenode->number=QueryListNodeCount(px);   //������ǰ��number
		}
		else
		{
		strcpy(message,"list is empty!\n");
		goto ERROR;
			return NULL;
		}
	}
	PLIST_ENTRY node;
	node=GetTheResultNode(px,ID);       //
	if(node==NULL)
	{
		
		strcpy(message,"node is NULL!\n");
		goto ERROR;
		return NULL;
	}
		  ;
	POperationResult toreturn=CONTAINING_RECORD(node,OperationResult,entry);
    if(toreturn!=NULL)
	{
		KdPrint(("�ɹ��ҵ��ýڵ�successfully ,:wasfill:%d\n otherinfo:%ws",toreturn->wasFill?1:0,toreturn->otherinfo));
		toreturn->wasFill=true;
		toreturn->operation_permit=result->operation_permit;
		wcsncpy(toreturn->otherinfo,result->otherinfo,sizeof(WCHAR)*wcslen(result->otherinfo));
		KdPrint(("��д���: wasfill:%d\n otherinfo:%ws ",toreturn->wasFill?1:0,toreturn->otherinfo));
	}
	CutOffListNode(ID,px);
	tablenode->number--;
	allnodenumber=tablenode->number;

	
	HashTableNodeListUnLock(tablenode);
	
	return toreturn;

ERROR:
	KdPrint(("errormessage:%s",message));
	HashTableNodeListUnLock(tablenode);
	return 0;
}
VOID MyOperationResultAddToHashTable2(POperationResult result)
{
	KdPrint(("MyOperationResultAddToHashTable2:\n\n"));
	if(result==NULL)
	{
		KdPrint(("�����һ���յ�OperationResult��\n"));
		return;
	}
	ULONG id=result->operationID;//��Ҫ������֤���ID
	if(id==0 ||id <0)
	{
		KdPrint(("���Ϸ���ID��\n\n"));
	}
	
	
	ULONG operationtype=result->operationtype;
	ULONG number=HashFunction_ForOperation(operationtype);
	if(operationtype>30 ||operationtype <0 ||operationtype ==30)
	{
		KdPrint(("HashFunction error:%d",operationtype));
		return ;
	}

	PHASH_TABLE_NODE hashnode=&(hashtable_result.link[number]);
	PLIST_ENTRY listentryofhashtable=&(hashnode->entry);

	//POperationInformation operationinformation=(POperationInformation)::ExAllocatePool(NonPagedPool,sizeof(OperationInformation));
	    KIRQL irql=::KeGetCurrentIrql();
		if(irql>=DISPATCH_LEVEL)
		KdPrint(("current irql in addtohashtable:%c",irql));

	//    KdPrint(("operation filename:%wZ\n",&operation->FileName));
	//	KdPrint(("attemp to access node->number:%d init:%din function  MyOperationInformationAddToHashTabl!",hashnode->number,hashnode->initilzed?1:0));
		HashTableNodeListLock(hashnode);
		::InsertHeadList(&hashnode->entry,(PLIST_ENTRY)result);

		
		if(hashnode->number==0)
		{
			hashnode->OperationType=result->operationtype;
		}
		hashnode->number++;
		HashTableNodeListUnLock(hashnode);
		KdPrint(("MyOperationResultAddToHashTable2 end!\n\n"));
}
PLIST_ENTRY GetTheResultNode(PLIST_ENTRY px,ULONG ID)
{
	KdPrint(("���ڲ���ָ����ID\n\n"));
	PLIST_ENTRY head,temp;
	head=px;
	temp=head->Blink;
	for(temp;temp!=head;temp=temp->Blink)
	{
		POperationResult result=(POperationResult)CONTAINING_RECORD(temp,OperationResult,entry);
		if(result->operationID==ID)
		{
		//	KdPrint(("�ҵ���ָ����id %d!",result->operationID));
			//KdPrint(("���Ÿı���ֵ:wasfill:%d",result->wasFill?1:0));
			//result->wasFill=true;
			//KdPrint(("�ı�֮��:wasfill:%d",result->wasFill?1:0));
			return temp;
			
		}
	}
	return NULL;
}