#include "stdafx.h"


//****************sfilter.cpp*********************

extern NPAGED_LOOKASIDE_LIST looksidelist; 
extern NPAGED_LOOKASIDE_LIST looksidelist_result;
extern HASH_TABLE hashtable; 
extern HASH_TABLE hashtable_result;
extern PThreadEvents myevents[HASH_TABLE_LENGTH];


extern POperationResult resultfortest;
//**************************************************************************************




NTSTATUS FastIO_GetOperationInformation(ULONG iocontrolcode,PVOID inbuffer,PVOID outbuffer,ULONG inlen,ULONG outlen)
{
	NTSTATUS status=STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(outlen);
	ULONG code=iocontrolcode;
	ULONG in_len=inlen;
	ULONG out_len=outlen;
	ULONG operationtype;   //���Ǳ��θ���Ȥ�Ĳ�������
	HANDLE waiteventhandle;
	PKEVENT waitevent;
	POperationRecord record=(POperationRecord)inbuffer;
	POperationRecord out=(POperationRecord) outbuffer;

	ASSERT(code==IOCTL_VERIFY_OPERATION_INFORMATION);
	ASSERT(in_len==sizeof(POperationRecord));
	ASSERT(out_len==sizeof(POperationRecord));
	ASSERT(inbuffer!=NULL);
	ASSERT(outbuffer!=NULL);

	operationtype=record->operationtype;
	waiteventhandle=record->event;
	do
	{

    if(record->event==NULL||record->forback==NULL)
	{
		KdPrint(("event or forback is null \n"));
		record->exception=TRUE;
		record->exceptiontype=EXCEPTION_APPLICATION_ERROR;
		wcsncpy(record->exceptionmessage,
			L"event or forback is null!\r\n",
			sizeof(L"event or forback is null!\r\n\n")
			);
		break ;
	}
	status=ObReferenceObjectByHandle(waiteventhandle,EVENT_MODIFY_STATE,*ExEventObjectType,KernelMode,(PVOID*)&waitevent,NULL);
	if(!NT_SUCCESS(status))
	{
		//KdPrint(("��Ч�ľ����\n"));
		record->exception=TRUE;
		record->exceptiontype=EXCEPTION_APPLICATION_ERROR;
		wcsncpy(record->exceptionmessage,
			L"ObReferenceObjectByHandle failed!\r\n",
			sizeof(L"ObReferenceObjectByHandle failed!\r\n\n")
			);
		break;
	}

	//��ʼ��hashtable������OperationInformation�ṹ
	POperationInformation pnode=NULL;
	GetOperationInformationFromHashTable(operationtype,&pnode);
	if(pnode==NULL)
	{
		record->exception=true;
		record->exceptiontype=EXCEPTION_NONE_RECORD;
		wcsncpy(record->exceptionmessage,
			L"there are no OperationInformation anymore!\r\n",
			sizeof(L"there are no OperationInformation anymore!\r\n\n")
			);

		break;
	}
	 //�ɹ�ȡ��������ṹ����ô��ʱ�����û������ṩ�������������д��Щ��Ϣ
	
	if(out!=NULL)
	{
	wcsncpy(out->filename,pnode->FileName.Buffer,pnode->FileName.Length);
	wcsncpy(out->filepath,pnode->FilePath.Buffer,pnode->FilePath.Length);
	out->OperationID=pnode->OperationID;
	out->operationtype=pnode->OperationType;

	}
	pnode=NULL;                       //�ͷ�ָ��
	KdPrint(("write data over\r\n\n"));
	}while(0);
	::KeSetEvent(waitevent,IO_NO_INCREMENT,false);       //�����¼���֪ͨ�û��Ѿ�ȡ������Ϣ
	::ObfDereferenceObject(waitevent);                   //�����¼�����
	return status;
}

NTSTATUS FastIO_SetOperationResult(ULONG iocontrolcode,PVOID inbuffer,PVOID outbuffer,ULONG inlen,ULONG outlen)
{
    
	UNREFERENCED_PARAMETER(outlen);
	UNREFERENCED_PARAMETER(outbuffer);
	UNREFERENCED_PARAMETER(inlen);
	UNREFERENCED_PARAMETER(iocontrolcode);
	NTSTATUS status=STATUS_ACCESS_DENIED;
	ASSERT(iocontrolcode==IOCTL_SEND_VERIFY_RESULT);
	ASSERT(inlen>=sizeof(OperationResult));
	ASSERT(inbuffer!=NULL);
	BOOLEAN flag=FALSE;

	PVOID buffer=inbuffer;
	POperationResult result;
	POperationResult newresult;

	result=(POperationResult)buffer;
	if(result==NULL)
		return status;
	newresult=(POperationResult)::ExAllocateFromNPagedLookasideList(&looksidelist_result);
	newresult->operationID=result->operationID;
	newresult->operationtype=result->operationtype;
	newresult->operation_permit=result->operation_permit;
	wcsncpy(newresult->otherinfo,result->otherinfo,sizeof(WCHAR)*512);

	 MyOperationResultAddToHashTable2(newresult);

	PHASH_TABLE_NODE tablenode;
	ULONG number=HashFunction_ForOperation(result->operationtype);
	tablenode=&(hashtable.link[number]);
	 	PLIST_ENTRY p,head;
	POperationInformation results;
	head=&tablenode->entry;
	::HashTableNodeListLock(tablenode);
	for(p=head->Flink;p!=head;p=p->Flink)        
	{
        results=(POperationInformation)p;
		if(result->operationID==results->OperationID)
		{
			::KeSetEvent(&results->finished,IO_NO_INCREMENT,FALSE);

			PLIST_ENTRY flink,blink;
			flink=p->Blink;
			blink=p->Flink;
			flink->Flink=blink;
			blink->Blink=flink;
			//::ExFreeToNPagedLookasideList(&looksidelist,results);  //��hashtable����Ķ�Ӧ��operationinformation�ṹ�����������ͷų���
			flag=TRUE;
			break;
		}
	}
	::HashTableNodeListUnLock(tablenode);
	if(flag)
	{
		KdPrint(("FASTIO_SetOperationResult failed!\n"));
		status=STATUS_SUCCESS;
	}
	
	return status;
}
//**************************************************************************************************
VOID GetOperationInformation(PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpsp=::IoGetCurrentIrpStackLocation(irp);
	ULONG number=-1;
	ULONG operationtype;   //���Ǳ��θ���Ȥ�Ĳ�������
	HANDLE waiteventhandle; 
	PKEVENT waitevent;
	ULONG concode=irpsp->Parameters.DeviceIoControl.IoControlCode;
	PVOID buffer=irp->AssociatedIrp.SystemBuffer;   //
	ULONG inlen=irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outlen=irpsp->Parameters.DeviceIoControl.OutputBufferLength;        
	POperationRecord out;
	bool exception=false;
	WCHAR exceptionmessage[512]=L"";
	ASSERT(inlen==sizeof(POperationRecord));
	ASSERT(buffer!=NULL);
	do
	{
	if(buffer==NULL)
	{		
		break ;
	}
	KdPrint(("buffer is not null\n"));
	waiteventhandle=*(HANDLE*)buffer;

	if(waiteventhandle==NULL)
	{
		KdPrint(("event is null!\n"));
		exception=true;
		wcsncpy(exceptionmessage,
			L"event or forback is null!\r\n",
			sizeof(L"event or forback is null!\r\n\n")
			);
	}	
	//********************������������������������⣬��ô����д��������Ϣֱ�ӷ���;
	KdPrint(("get out buffer:mdladdress!\n"));
	out=(POperationRecord)MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority); 
	if(out!=NULL)
	{
		operationtype=out->operationtype;
		number=HashFunction_ForOperation(operationtype);
		KdPrint(("��ǰ�߳������Ĳ���������:%d",operationtype));
	}
	else
	{
		KdPrint(("out buffer is null!\n"));
		break;
	}
	//***************��ʼ��ѯ�����ĵ������Ƿ��ж�Ӧ�Ĳ�����Ϣ��Ҫ��֤������У���ȡ��������ͼ�¼�¼���Ȼ��ȴ���ֱ���У�Ȼ��֪ͨӦ����ȡ*****************
	status=ObReferenceObjectByHandle(waiteventhandle,EVENT_MODIFY_STATE,*ExEventObjectType,KernelMode,(PVOID*)&waitevent,NULL);
	if(!NT_SUCCESS(status))//���ʧ�ܣ���ô�ͻ��������������event�¼���һ��������������������������
	{
		KdPrint(("event from InputBuffer have occured a error when it is used to get a kernel object by useing ObReferenceObjectByHandle!\r\n\n"));
		exception=true;
		wcsncpy(exceptionmessage,
			L"event from InputBuffer have occured a error when it is used to get a kernel object by useing ObReferenceObjectByHandle!\r\n",
			sizeof(L"event from InputBuffer have occured a error when it is used to get a kernel object by useing ObReferenceObjectByHandle!\r\n\n")
			);
		KdPrint(("start ObReferenceObjectByHandle in POperationRecord\n\n"));
		waiteventhandle=out->event;
		status=ObReferenceObjectByHandle(waiteventhandle,EVENT_MODIFY_STATE,*ExEventObjectType,KernelMode,(PVOID*)&waitevent,NULL);
	    if(!NT_SUCCESS(status))    //�������¼����ں˶���ʧ���ˣ���ôֱ�ӷ���ʧ��
	   {
		 KdPrint(("��Ч�ľ����\n"));
		 exception=true;
		 wcsncpy(exceptionmessage,
			L"ObReferenceObjectByHandle failed!\r\n",
			sizeof(L"ObReferenceObjectByHandle failed !\r\n\n")
			);
		 break;
	    }	 
	}
	//********************************��ȡ�¼�����ɹ�***********************************
	KdPrint(("start GetOperationInformationFromHashTable:operationtype:%x ",operationtype));
	//��ʼ��hashtable������OperationInformation�ṹ
	POperationInformation pnode=NULL;
	GetOperationInformationFromHashTable(operationtype,&pnode);
	//û�л������ṹ���洢�¼����󣬵ȴ�
	if(pnode==NULL)
	{
		KdPrint(("pnode is null and get the kevent,��ÿյĽڵ㣬�����¼�������myevents[%d]!\n",number));
		exception=true;
		out->exceptiontype=EXCEPTION_NONE_RECORD;
		wcsncpy(exceptionmessage,
			L"there are no OperationInformation anymore!\r\n",
			sizeof(L"there are no OperationInformation anymore!\r\n\n")
			);
		KeMyEventsLock(number);
		myevents[number]->event=waitevent;
		myevents[number]->waitstatue=true;
		myevents[number]->OperationType=operationtype;
		KeMyEventsUnLock(number);
	
		break;
	}
	KdPrint(("pnode is not null and start write out\n"));
	
	
	 //�ɹ�ȡ��������ṹ����ô��ʱ�����û������ṩ�������������д��Щ��Ϣ
	
	KdPrint(("filename:%wZ filepath:%wZ",pnode->FileName,pnode->FilePath));

	wcsncpy(out->filename,pnode->FileName.Buffer,pnode->FileName.Length);
	wcsncpy(out->filepath,pnode->FilePath.Buffer,pnode->FilePath.Length);
	out->OperationID=pnode->OperationID;
	if(operationtype!=pnode->OperationType)
		KdPrint(("pnode type:%d operationtype:%d",pnode->OperationType,operationtype));
	out->operationtype=pnode->OperationType;
	pnode->wasdealwith=true;                               //���õ�ǰ�����������Ϣ���ڱ���֤
	::KeSetEvent(waitevent,IO_NO_INCREMENT,false);       //�����¼���֪ͨ�û��Ѿ�ȡ������Ϣ
	::ObfDereferenceObject(waitevent);                   //�����¼�����
	

	pnode=NULL;                       //�ͷ�ָ��
	KdPrint(("write data over\r\n\n"));
	}while(0);
	//**********��������⣬д���������Ϣ**********
	if(exception)
	wcsncpy(out->exceptionmessage,exceptionmessage,wcslen(exceptionmessage));
	out->exception=exception;
	KdPrint(("GetOperationInformation End!\n"));	
}


//********************************�ѽ���ṹ�������ϣ��***********************************



//************************************************************************************
/*BOOLEAN GetOperationMessage(PIRP irp)  //METHOD_OUT_DIRECT
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpsp=::IoGetCurrentIrpStackLocation(irp);
	ULONG inlen=irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outlen=irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	
	HANDLE event=*(HANDLE*)irp->AssociatedIrp.SystemBuffer;
	POperationRecord out=0;

	if(event!=NULL)
	{
		
		PKEVENT kevent;
	
		
		status=ObReferenceObjectByHandle(event,EVENT_MODIFY_STATE,*ExEventObjectType,KernelMode,(PVOID*)&kevent,NULL);
		
		if(NT_SUCCESS(status))
		{
			KdPrint(("get event from application !\n\n"));
			out=(POperationRecord)MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);
			if(out!=NULL)
			{
				

		        ::KeSetEvent(kevent,IO_NO_INCREMENT,false);
				::ObfDereferenceObject(kevent);
				KdPrint(("write data over!\n\n"));
				return true;
			}
		}
		return false;
	}
	else
	{
		KdPrint(("systembuffer is null in GetOperationMessage!\n\n"));
		return false;
	}
}*/
ULONG GetOperationInformationFromHashTable(ULONG OperationType,OUT POperationInformation* operinfos)
{
	KdPrint(("��ʼ���Ҳ�������Ϊ%d(%x)�Ĳ�����Ϣ�ṹ��",OperationType,OperationType));
	CHAR message[124];
	unsigned short number;//��ǰ�����ĵ�irp�洢���ĸ��ڵ�����
	PHASH_TABLE_NODE tablenode;
	ULONG allnodenumber;
	number=HashFunction_ForOperation(OperationType);
	if(number<0 ||number>=HASH_TABLE_LENGTH)//
	{
		strcpy(message,"number is illegal!\n");
		goto ERROR;
		
		return -1;
	}
	KdPrint(("1\n\n"));
	tablenode=&(hashtable.link[number]);
     HashTableNodeListLock(tablenode);	
	 KdPrint(("2\n\n"));
	if(tablenode->OperationType!=OperationType) //�����ǰ�Ľڵ�洢��irp���Ͳ�������ô����
	{
		KdPrint(("3\n\n"));
		KdPrint(("tablenode->OperationType:%d %x OperationType:%d %x number:%d",tablenode->OperationType,tablenode->OperationType,OperationType,OperationType,number));
		strcpy(message,"OperationType mismatch!\n");
		KdPrint(("4\n\n"));
		goto ERROR;
		return -2;
	}
	KdPrint(("5\n\n"));
	allnodenumber=tablenode->number;
	PLIST_ENTRY px=&tablenode->entry;
	KdPrint(("6\n\n"));
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
			return 0;
		}
	}
	PLIST_ENTRY node;
	node=GetListTailNode(px);               //���ڲ����ʱ���Ǵ�ͷ���룬�������Ҫ��βȡ��(������û�б������)��������һ�����е���ʽ
	if(node==NULL)
	{

		operinfos=NULL;
		strcpy(message,"node is NULL!\n");
		goto ERROR;
		return -3;
	}
		  ;
	POperationInformation toreturn=CONTAINING_RECORD(node,OperationInformation,listentry);
    if(toreturn!=NULL)
	{
		KdPrint(("successfully\n"));
		*operinfos=toreturn;
		
		
	}
	tablenode->number--;
	allnodenumber=tablenode->number;

	
	HashTableNodeListUnLock(tablenode);
	
	return allnodenumber;

ERROR:
	KdPrint(("errormessage:%s",message));
	HashTableNodeListUnLock(tablenode);
	return 0;
}
//**********************�������Ҿ��������������Ե�OperationResult������************************

PLIST_ENTRY GetListTailNode(PLIST_ENTRY px)
{
	if(IsListEmpty(px))
	{
		KdPrint(("empty\n"));
		return 0;
	}
	PLIST_ENTRY p,head;
	head=px;
	
	ULONG i=0;
	
	int m=0;
	p=head->Blink;  //β�ڵ�
    while(p!=head)
	{
		POperationInformation po=(POperationInformation)p;
			if(po->wasdealwith!=true)             //�������ṹû�б�����
				return p;
	    p=p->Blink;
		m++;
	}
	KdPrint(("not found :%d",m));
	return NULL;
}
ULONG QueryListNodeCount(PLIST_ENTRY px)
{
	if(IsListEmpty(px))
		return 0;
	PLIST_ENTRY p,head;
	head=px;
	ULONG i=0;
	
	for(p=head->Flink;p!=head;p=p->Flink)
	{
		i++;
	}
	
	return i;
	
}
/*ULONG QueryHashTable(IN ULONG type,IN ULONG operationID,OUT OperationInformation operinfos)
{
	UNREFERENCED_PARAMETER(operinfos);
	UNREFERENCED_PARAMETER(operationID);
	unsigned short number;//��ǰ�����ĵ�irp�洢���ĸ��ڵ�����
	PHASH_TABLE_NODE tablenode;
	ULONG allnodenumber;
	number=HashFunction_ForOperation(type);
	if(number<0 ||number>=HASH_TABLE_LENGTH)//
		return -1;
	tablenode=&(hashtable.link[number]);
	if(tablenode->OperationType!=type) //�����ǰ�Ľڵ�洢��irp���Ͳ�������ô����
		return -2;
	allnodenumber=tablenode->number;
	if(allnodenumber==0)                         //�����ǰ�ڵ�����û�йҽ�OperationInformation
	{
		return 0;
	}
	PLIST_ENTRY p,head;
	head=&tablenode->entry;
	POperationInformation node;
	
	for(p=head->Flink;p!=head;p=p->Flink)
	{
	    node=(POperationInformation)p; //����OperationInformation�ṹ��Ƴ��˵�һ����Ա����PLIST_ENTRY����PLIST_ENTRY��ָ����൱�ڽṹ���׵�ַָ��
	    
	}

}*/

VOID InitOperationInformation(POperationInformation infor)
{
	if(infor==NULL)
		return;
	::KeInitializeEvent(&(infor->finished),NotificationEvent,FALSE);
	infor->OperationID=0;
	infor->OperationType=-1;
	infor->wasdealwith=false; 
}

/****************����֮����Ҫ���myevents������Ӧ�ĵ�ǰ��irp���͵��¼��Ƿ�Ϊ�գ�
                      ��Ϊ�գ���ô����Ҫ���ø��¼���֪ͨ���ó���*****************************/
ULONG FillOperationInformationAndAddToList(ULONG IrpType,POperationInformation operationinformation,PVOID pContext)
{
	
	
	UNREFERENCED_PARAMETER(pContext);
	KIRQL irql;   	
	ULONG ID;
    
	irql=::KeGetCurrentIrql();
	if(irql<=DISPATCH_LEVEL)
	{
	switch(IrpType)
	{
	case(IRP_MJ_CREATE):
		
		break;
    case(IRP_MJ_SET_INFORMATION):
		                        
		KdPrint(("��ʼ��Ӳ�����Ϣ��\n"));		  //��ӽ�֮��͵ȴ�Ӧ�ó�����ȡ������жϷ���֮���������
		ID=MyOperationInformationAddToHashTable(operationinformation);
		
		break;
	default:
		break;
	}
	}
	return ID;
}

//***************************����hashtable_result�Ĳ�������********************


//��Ӳ����ṹ��hashtable_result���ȴ�ȡ��

//��Ӳ�����Ϣ����ϣ�����棬Ȼ��ȴ��û������ȡ
ULONG MyOperationInformationAddToHashTable(POperationInformation operation)
{
	ULONG re=0;
	
	if(operation==NULL)
	{
		KdPrint(("OperationInformation is Valid!\n"));
		return re;
	}
	
		
	ULONG operationtype=operation->OperationType;
	if(operationtype>30 ||operationtype <0 ||operationtype ==30)
	{
		KdPrint(("HashFunction error:%d",operationtype));
		return re;
	}
	ULONG number=HashFunction_ForOperation(operationtype);
	PHASH_TABLE_NODE hashnode=&(hashtable.link[number]);
	PLIST_ENTRY listentryofhashtable=&(hashnode->entry);

	//POperationInformation operationinformation=(POperationInformation)::ExAllocatePool(NonPagedPool,sizeof(OperationInformation));
	    KIRQL irql=::KeGetCurrentIrql();
		if(irql>=DISPATCH_LEVEL)
		KdPrint(("current irql in addtohashtable:%c",irql));

	    KdPrint(("operation filename:%wZ\n",&operation->FileName));
		KdPrint(("attemp to access node->number:%d init:%din function  MyOperationInformationAddToHashTabl!",hashnode->number,hashnode->initilzed?1:0));
		HashTableNodeListLock(hashnode);
		::InsertHeadList(&hashnode->entry,(PLIST_ENTRY)operation);
		if(hashnode->number==0 &&hashnode->totalnumbers==0)
		{
			hashnode->OperationType=operation->OperationType;
		}
		hashnode->number++;
		hashnode->totalnumbers++;
		re=operation->OperationType+hashnode->totalnumbers;
		operation->OperationID=re;
		KdPrint(("������:�½ڵ���link[%d],OperationType:%x ID:%d \n��ʼ��ѯ�Ƿ����߳��ڵȴ�\n",number,hashnode->OperationType,re));
		KeMyEventsLock(number);
		
		if(myevents[number]->OperationType==operation->OperationType &&myevents[number]->waitstatue && myevents[number]->event!=NULL)
		{
			
			LONG singled;
			
			singled=::KeSetEvent(myevents[number]->event,IO_NO_INCREMENT,FALSE);
		    ObfDereferenceObject(myevents[number]->event);
			if(singled==0)          //����Ѿ�������״̬�᷵�ط���ֵ
			{	
			KdPrint(("֪ͨӦ�ó���\n\n"));
			myevents[number]->waitstatue=false;
			}
			else
			{
				KdPrint(("KeSetEvent:event �Ѿ�������״̬��\n\n"));//����ǵ�ǰ������Ч�ģ�����
				                                     //�������Ǵ��������,Ӧ�ñ����ں˶���
			}
			myevents[number]->event=NULL;
		}
		else
		{

			KdPrint(("��ǰ�ڵ�û���¼��ڵȴ�:myevents[%d].operationtype:%d operationtype:%d waitstatu: %d!\n",number,myevents[number]->OperationType,operationtype,myevents[number]->waitstatue?1:0));
		}
		 KeMyEventsUnLock(number);
		HashTableNodeListUnLock(hashnode);
		
		//::ExInterlockedInsertHeadList(listentryofhashtable,(PLIST_ENTRY)&operation->listentry,spn); 
		return re;
}
//*************************//
