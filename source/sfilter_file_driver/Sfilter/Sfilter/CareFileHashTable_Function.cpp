#include "stdafx.h"

#define FILE_PATH_LENGTH   150   //���󲿷ֵ�·�����Ȳ��ᳬ�����ֵ,����ϵͳ����û�ж�·��������Ҫ��

#define START_CHAR   L'*'
#define END_CHAR     L'?'
#define SEG_CHAR     L'|'
#define DEF_CHAR	 L':'

extern NPAGED_LOOKASIDE_LIST looksidelist; 
extern NPAGED_LOOKASIDE_LIST looksidelist_result;
extern HASH_TABLE hashtable; 
extern HASH_TABLE hashtable_result;
extern PThreadEvents myevents[HASH_TABLE_LENGTH];


extern POperationResult resultfortest;
extern size_t process_name_offset;

extern NPAGED_LOOKASIDE_LIST  looksidelist_carefile;
extern HASH_TABLE_CAREFILE hashtable_carefile;

extern BOOLEAN CareFile_Initilized;

KSPIN_LOCK lock;
KIRQL lock_irql;

Name myname[Name_Value_Number];
//***************************�������ں������·����Ӧ�ó���Щ��ͬ�����ԣ���Ҫ��ע�����ȡ��һЩ��Ϣ���洢����*****************


ULONG    CR_HashFunction(ULONG index)//���ݸ����ļ�ֵ�����±�
{
	if(index<0)       //�������Ӣ�ĵĿ�ͷ�ģ��ļ������������Ŀ�ͷ����ʱ���ȡģ
	{
		return MATCHING_ERROR_TYPE_ONE ;
	}
	else
		if(index>26)
			return MATCHING_ERROR_TYPE_TWO;
		else
		{
			return index;
		}

}
VOID     CR_MyNodeLock(PHASH_TABLE_CAREFILE_NODE node)
{
	if(node==NULL)
		return ;
	KeAcquireSpinLock(&node->node_lock,&node->node_irql);
}
VOID     CR_MyNodeUnLock(PHASH_TABLE_CAREFILE_NODE node)
{
	if(node==NULL)
		return ;
	KeReleaseSpinLock(&node->node_lock,node->node_irql);
}
VOID CR_CareFileTable_Insert(PCareFile carefile)
{
  CR_DEBUG(DEBUG,("CareFileTable_Insert!\n\n"));
  UCHAR chars=carefile->filepath.Buffer[0];
  WCHAR test='c';
  
  ULONG index=carefile->filepath.Buffer[0]>=L'a'?(carefile->filepath.Buffer[0]-L'a'):carefile->filepath.Buffer[0]-L'A';
  CR_DEBUG(DEBUG,("CareFileTable_Insert:index(%d=%d(%c&%wc)-%d) -%d -%d-%c!\n",index,carefile->filepath.Buffer[0],carefile->filepath.Buffer[0],carefile->filepath.Buffer[0],L'A','A',test,test));
 
  ULONG index_number=::CR_HashFunction(index);
  
  PHASH_TABLE_CAREFILE_NODE pnode =&hashtable_carefile.link[index_number];//���ƥ�䲻�����������һ���ڵ�
  CR_MyNodeLock(pnode);
	 
  pnode->totalnumbers++;
  pnode->number++;

  ::InsertHeadList(&pnode->entry,&carefile->entry);

  ::CR_MyNodeUnLock(pnode);

  CR_DEBUG(DEBUG,("leave CareFileTable_Insert !\n\n"));



}
//����ǳ�ʼ�������ļ��Ĺ�ϣ�����ڱȽ����⣬���Ե�����Ϊһ������
VOID CR_CareFileTable_CleanUp()
{
	PHASH_TABLE_CAREFILE_NODE filenode;
	for(int j=0;j<HASH_TABLE_LENGTH;j++)
	{
		
		filenode=&(hashtable_carefile.link[j]);
		PLIST_ENTRY entry=&(filenode->entry);
		::CR_MyNodeLock(filenode);
		PCareFile carefile;
		while(!IsListEmpty(entry))
		{
			
			PLIST_ENTRY temp=::RemoveTailList(entry);
			carefile=CONTAINING_RECORD(temp,CareFile,entry);
			::ExFreePool(carefile->filename.Buffer);
			::ExFreePool(carefile->filepath.Buffer);
			::ExFreeToNPagedLookasideList(&looksidelist_carefile,carefile);
		}
		::CR_MyNodeUnLock(filenode);
	}
}
VOID InitHashTable_CareFile()
{
	//�տ�ʼ��ʼ����������û�п�ʼ����������ʹ��zwcreatefile������ļ������������Ҫ�Լ�����irp����ײ��豸������ļ�������
	//������������������DriverEntry����.
	CR_DEBUG(DEBUG,("��ʼ��ʼ�������ļ���ϣ����ر���  r4��\n\n"));
	HANDLE file;
	::KeInitializeSpinLock(&lock);

	OBJECT_ATTRIBUTES objectattr;
	UNICODE_STRING filename;
	IO_STATUS_BLOCK statusblock;
	NTSTATUS status;
	ULONG length;
	ULONG filenumber=0;
	FILE_STANDARD_INFORMATION fsi;

	hashtable_carefile.Length=CAREFILETABLE_LENGTH;
	for(int i=0;i<hashtable_carefile.Length;i++)
	{
		PHASH_TABLE_CAREFILE_NODE pnode=&(hashtable_carefile.link[i]);
		::InitializeListHead(&pnode->entry);
		::KeInitializeSpinLock(&pnode->node_lock);
		pnode->firstchar=L'a'+i;
		pnode->NodeID=i;
		pnode->number=0;
		pnode->totalnumbers=0;
		pnode->initilzed=true;
	}
	CR_DEBUG(DEBUG,("HASHTABLE_CAREFILE�ṹ��ʼ�����,��ʼ��ȡ����!\n\n"));

	for(int j=0;j<Name_Value_Number;j++)
	{
		RtlZeroMemory(&myname[j],sizeof(Name));
	    switch(j)
		{
		case(0):
			memcpy(&myname[j].name,L"filename",100);
			myname[j].type=NAME_VALUE_TYPE_FILENAME;
			myname[j].length=8;
			break;
		case(1):
			memcpy(&myname[j].name,L"filepath",100);
			myname[j].type=NAME_VALUE_TYPE_FILEPATH;
			myname[j].length=8;
			break;
		case(2):
			memcpy(&myname[j].name,L"security_level",100);
			myname[j].type=NAME_VALUE_TYPE_SECURITY_LEVEL;
			myname[j].length=14;
			break;
		default:
			break;
		}
	}
	
	CareFile_Initilized=false;
	::RtlInitUnicodeString(&filename,L"\\??\\C:\\Windows\\carefile.dat");
	InitializeObjectAttributes(&objectattr,
		                        &filename,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL);

	status=::ZwOpenFile(&file,
		FILE_ANY_ACCESS,
		&objectattr,
		&statusblock,
		0,
		FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

	if(NT_SUCCESS(status))
	{
		CR_DEBUG(DEBUG,("�ɹ����ļ�!\n"));

	}
	else
	{
		CR_DEBUG(DEBUG,("���ļ�ʧ��!\n"));
		::ZwClose(file);
	
		return ;
	}

	status=::ZwQueryInformationFile(file,
		&statusblock,
		&fsi,
		sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation);
	if(!NT_SUCCESS(status))
	{
		CR_DEBUG(DEBUG,("��ѯ��Ϣʧ��\n"));
		::ZwClose(file);

		return;
	}
	PWCHAR pbuffer=(PWCHAR)::ExAllocatePool(NonPagedPool,(ULONG)fsi.EndOfFile.QuadPart);

	status=::ZwReadFile(file,
		NULL,
		NULL,
		NULL,
		&statusblock,
		pbuffer,
		(LONG)fsi.EndOfFile.QuadPart,
		NULL,
		NULL);

	if(!NT_SUCCESS(status))
	{
		CR_DEBUG(DEBUG,("��ȡ�ļ���Ϣʧ��!\n"));
		::ZwClose(file);
	   ::ExFreePool(pbuffer);
		return;
	}
	CR_DEBUG(DEBUG,("�ļ����ݣ�type:%c \n%ws",*pbuffer,pbuffer));
	//***��ȡ��ϣ���ʼ��ȡ��Ϣ*********
	length=(ULONG)fsi.EndOfFile.QuadPart;
	PCareFile filenode;

	
	UCHAR name[100]={0};
	
	PWCHAR temp=pbuffer;
	Query_SymbolLinkInfo();
	//WCHAR filenamebuffer[1024];
	CR_DEBUG(DEBUG,("��ʼ��ȡ��Ϣ!\n\n"));
	ULONG len=0; //��¼pbufferָ��ĵ�ǰλ��
	PWCHAR start=pbuffer;  //�������ͷָ�룬�����ͷŸ��ڴ�

	NameValue namevalue[Name_Value_Number];   //����Ƿ����path��filename��buffer��
	for(int i=0;i<=length;i=len)
	{

		if(*pbuffer==START_CHAR)
		{
			//filenumber++;
			//node=(PCareFile)::ExAllocateFromNPagedLookasideList(&looksidelist_carefile);
			temp=pbuffer;
			len++;
			if(*(++pbuffer)=START_CHAR)       //�ҵ�����ʼ����
			{
			   CR_DEBUG(DEBUG,("�ҵ�һ����ʼ��:%d\n",filenumber));
			   filenumber++;
			   	ULONG namenumber=0;  //��¼���ǵڼ���name
				

			   while(!(*pbuffer==END_CHAR &&*(--pbuffer)==END_CHAR) && len<=length &&namenumber< Name_Value_Number)             //ֱ���ҵ���������Ϊֹ,���������length���߳���������ֵ������Ҳ�Ƿ���
			   {
				   CR_DEBUG(DEBUG,("�ҵ�һ����:%ws\n",*pbuffer));
				  if(CompareWCharString(++pbuffer,myname[namenumber].name,myname[namenumber].length))//�ҵ�name
			      {
				   CR_DEBUG(DEBUG,("�ҵ�һ��Name:%ws \n",myname[namenumber].name));
				   temp=pbuffer;
				   pbuffer+=myname[namenumber].length;//pbuffer�ƶ���:֮��Ҳ����ֵ�õ�һ���ַ�
				   len+=myname[namenumber].length;
				   if(*pbuffer!=DEF_CHAR)         //û���ҵ�ֵ�Եķָ���:��������ѭ��
				   {
					   	CR_DEBUG(DEBUG,("û���ҵ�ֵ�Եķָ���:(*pbuffer==%c)\n",*pbuffer));
					   goto error;
				   }
				   int m=0;
				   BOOLEAN flag=TRUE; //�����־û�г�����FILE_PATH_LENGTH����
				   WCHAR value[MAX_FILE_PATH_LENGTH]={0};
				   pbuffer++;
				   len++;
				   //��̬���䱣�����ļ�·�������������Ԥ���ĳ��ȣ���ô�����·���
				   KdPrint(("������nonpagedpool �ڴ�!\n"));
				   namevalue[namenumber].value=(PWCHAR)::ExAllocatePool(NonPagedPool,sizeof(WCHAR)*FILE_PATH_LENGTH);

				   while((*pbuffer!=SEG_CHAR&& *pbuffer!=END_CHAR)&& len<=length &&namenumber< Name_Value_Number )   //��¼ֵ������ֱ��������ǰֵ�ԵĽ����� %
				   {
					   value[m]=*pbuffer;
					   m++;
					   pbuffer++;
					   len++;
					   if(m>=FILE_PATH_LENGTH-1 && flag)//�������Ѿ�����
					   {
						   PWCHAR tempx=namevalue[namenumber].value;
						   namevalue[namenumber].value=(PWCHAR)::ExAllocatePool(NonPagedPool,sizeof(WCHAR)*FILE_PATH_LENGTH_MAX);						
						   ::ExFreePool(tempx);
						   flag=FALSE;

					   }
					   if(m>=FILE_PATH_LENGTH_MAX-1)
						   break;
				    }
				   value[m]=0;
				   namevalue[namenumber].type=myname[namenumber].type;
				   wcsncpy(namevalue[namenumber].value,value,flag?FILE_PATH_LENGTH:MAX_FILE_PATH_LENGTH);
				   
				   CR_DEBUG(DEBUG,("�ҵ���ǰ��Ӧ��Value:%ws\n %ws\n",value,namevalue[namenumber].value));
				   namenumber++;
			    }

			   else
			     {
				   CR_DEBUG(DEBUG,("ƥ��Name����!\n\n"));
				   goto error;
				  }
			 }
			   //****************************�������ҵ���һ���ļ���¼*************
			   CR_DEBUG(DEBUG,("��ʼд���ļ���¼:%d\n",filenumber));
			   filenode=(PCareFile)::ExAllocateFromNPagedLookasideList(&looksidelist_carefile);
			   
			   for(int k=0;k<Name_Value_Number;k++)
			   {
				   CR_DEBUG(DEBUG,("namevalue[%d]:%ws  %d\n",k,namevalue[k].value,namevalue[k].type));
				   switch(namevalue[k].type)
				   {
				   case(NAME_VALUE_TYPE_FILENAME):				
					   RtlInitUnicodeString(&filenode->filename,namevalue[k].value);//������е�ǿ��ת�������ܻ��������
					   KdPrint(("filename:%ws --%d--\n--%ws--\n",filenode->filename.Buffer,k,namevalue[k].value));
					   break;
				   case(NAME_VALUE_TYPE_FILEPATH):
					   ::RtlInitUnicodeString(&filenode->filepath,(PCWSTR)namevalue[k].value);
					   CR_DEBUG(DEBUG,("filepath:%ws %d\n %ws\n",filenode->filepath.Buffer,k,namevalue[k].value));					  
					   break;

				   case(NAME_VALUE_TYPE_SECURITY_LEVEL):
					   filenode->secutiry_level=namevalue[k].value[0]-'0';  
					   CR_DEBUG(DEBUG,("level:%d\n",k,filenode->secutiry_level));
					   break;
				   default:
					   CR_DEBUG(DEBUG,("DEFAUKLT\n"));
					   break;
				   }
			   }
			   ::CR_CareFileTable_Insert(filenode);

			}
			else
			{
				CR_DEBUG(DEBUG,("û���ҵ��ڶ�����ʼ��:# \n\n"));
				goto error;
			}
		}
		len++;
		
		temp=pbuffer;      
		pbuffer++;
		i=len;
		
	}
	/*CR_DEBUG(DEBUG,("THIS IS A TEST :\n"));
	PWCHAR a=(PWCHAR)::ExAllocatePool(NonPagedPool,sizeof(WCHAR)*FILE_PATH_LENGTH);
	wcsncpy(a,L"sdawdacsa",wcslen(L"sdawdacsa"));
	KdPrint(("sdad:%ws\n",a));
	UNICODE_STRING  my;
	UNICODE_STRING mys;
	my.Buffer=a;
	my.Length=wcslen(L"sdawdacsa");
	my.MaximumLength=FILE_PATH_LENGTH;
	::RtlInitUnicodeString(&mys,a);
	KdPrint(("sdadsa:%ws\n",a));
	KdPrint(("string :%ws   %d  %d\n\n",my.Buffer,my.Length,my.MaximumLength));
	KdPrint(("string :%ws   %d  %d\n\n",mys.Buffer,mys.Length,mys.MaximumLength));
	
	KIRQL old=::KeGetCurrentIrql();
	KdPrint(("current level :%d",old));
	KeRaiseIrql(DISPATCH_LEVEL,&old);
	KdPrint(("string :%ws   %d  %d\n\n",mys.Buffer,mys.Length,mys.MaximumLength));
	KdPrint(("string :%wZ ",mys));
	KeLowerIrql(old);
	*/
	::DisPlayHashTable(3);
	CareFile_Initilized=true;
	::ZwClose(file);
	::ExFreePool(start);
	return ;
	error:
	CareFile_Initilized=false;
	CR_DEBUG(DEBUG,("��ʼ��CareFileʱ�������� %s\n\n"));	
	ZwClose(file);
	ExFreePool(start);
	::CR_CareFileTable_CleanUp();                //���ִ��������Ѿ�����������ڴ�
}
NTSTATUS CR_SetCareFileInit(PIRP irp)
{
	KeAcquireSpinLock(&lock,&lock_irql);
	
	PIO_STACK_LOCATION irpsp=::IoGetCurrentIrpStackLocation(irp);
	PKEVENT waitevent;
	HANDLE waiteventhandle;
	waiteventhandle=*(HANDLE*)irp->AssociatedIrp.SystemBuffer;
	NTSTATUS status;

	status=ObReferenceObjectByHandle(waiteventhandle,
		EVENT_MODIFY_STATE,
		*ExEventObjectType
		,KernelMode,
		(PVOID*)&waitevent,
		NULL);
	if(!NT_SUCCESS(status))
	{

		CR_DEBUG(DEBUG,("��ȡ�ں˶���ʧ��(CR_SetCareFileInit)!\n\n"));
		return NULL;
	}
	BOOLEAN init;
	init=(BOOLEAN)MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);

	init=CareFile_Initilized;
	CR_DEBUG(DEBUG,("����:�ںͳ�ʼ��״̬:INIT=%d Return= %d",CareFile_Initilized?1:0,init?1:0));
	::KeSetEvent(waitevent,IO_NO_INCREMENT,FALSE);

	KeReleaseSpinLock(&lock,lock_irql);
	
	CR_DEBUG(DEBUG,("leave from CR_SetCareFileInit irq��%d !\n",::KeGetCurrentIrql()));
	return status;
}
NTSTATUS  CR_StartInitCareFileHashTable(PIRP irp)//��ʼ��ʼ�������ļ�����
{
	
	CR_DEBUG(DEBUG,("CR_StartInitCareFileHashTable! irql:%d\n",KeGetCurrentIrql()));

	PCareFile node;          //�ڵ�
	HANDLE waitevent;
	PCareFileTransfer in=0;
	NTSTATUS status=STATUS_ACCESS_DENIED;
	KIRQL      oldirql;                   //����������ᱻ������ring0�Ļ�������������к�����IRQL����ˣ���ô�����IRQL������
	oldirql=::KeGetCurrentIrql();
	KIRQL now=oldirql;

	//KeRaiseIrql(DISPATCH_LEVEL,&oldirql);
	waitevent=*(HANDLE*)irp->AssociatedIrp.SystemBuffer;
	if(waitevent==NULL)
		return status;
	status=ObReferenceObjectByHandle(waitevent,
		EVENT_MODIFY_STATE,
		*ExEventObjectType
		,KernelMode,
		(PVOID*)&waitevent,
		NULL);
	if(!NT_SUCCESS(status))
		return status;
	
	in=(PCareFileTransfer)MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);

	if(in==NULL)
		return status;
	CR_DEBUG(DEBUG,("��ʼ��������!:%d\n",in->number));
	PCareFile_T innode=0;
	for(int m=0;m<in->number;m++)
	{
		CR_DEBUG(DEBUG,("-----The %d current irql:%d\n",m,::KeGetCurrentIrql()));

		node=(PCareFile)::ExAllocateFromNPagedLookasideList(&looksidelist_carefile);
		if(node==NULL)
			break;
		if(in->filenode!=NULL)
		{
			innode=&in->filenode[m];
			CR_DEBUG(DEBUG,("filenode %d: %ws",m,innode->filename));
			::RtlInitUnicodeString(&node->filename,innode->filename);
			::RtlInitUnicodeString(&node->filepath,innode->filepath);
			node->Owner=innode->Owner;
			node->secutiry_level=innode->secutiry_level;			
		}
		else
			break;
	}

//	KeLowerIrql(oldirql);
	CR_DEBUG(DEBUG,("Leave from CR_StartInitCareFileHashTable!\n\n"));
	::CareFile_Initilized=true;
	return status;;
}
NTSTATUS CR_InitFinished(PIRP irp)
{
	KIRQL level;
	CR_DEBUG(DEBUG,("current irql:%d",::KeGetCurrentIrql()));
	KeRaiseIrql(DISPATCH_LEVEL,&level);

	KeAcquireSpinLock(&lock,&lock_irql);
	PIO_STACK_LOCATION irpsp=::IoGetCurrentIrpStackLocation(irp);
	PKEVENT waitevent;
	HANDLE waiteventhandle;
	waiteventhandle=*(HANDLE*)irp->AssociatedIrp.SystemBuffer;
	NTSTATUS status;

	status=ObReferenceObjectByHandle(waiteventhandle,
		EVENT_MODIFY_STATE,
		*ExEventObjectType
		,KernelMode,
		(PVOID*)&waitevent,
		NULL);
	if(!NT_SUCCESS(status))
	{

		CR_DEBUG(DEBUG,("��ȡ�ں˶���ʧ��(CR_InitFinished)!\n\n"));
		return NULL;
	}

	KeSetEvent(waitevent,IO_NO_INCREMENT,FALSE);
	
	CR_DEBUG(DEBUG,("Leave from CR_InitFinished!:%d\n",::KeGetCurrentIrql()));
	
	KeLowerIrql(level);
	CR_DEBUG(DEBUG,("current irql:%d level:%d",::KeGetCurrentIrql(),level));
	return status;
}
BOOLEAN  CR_CheckIfCare(PCareFile a)
{
	KdPrint(("CR_CheckIfCare In:%ws!\n",a->filepath.Buffer));
	BOOLEAN flag=FALSE;
	if(a->filepath.Length==0)
		return FALSE;
	
   ULONG index=a->filepath.Buffer[0]>L'a'?a->filepath.Buffer[0]-L'a':a->filepath.Buffer[0]-L'A';
	
   ULONG index_number;

   index_number=::CR_HashFunction(index);

    PHASH_TABLE_CAREFILE_NODE pnode =&hashtable_carefile.link[index_number];//���ƥ�䲻�����������һ���ڵ�
	//CR_MyNodeLock(pnode);
	 
	PLIST_ENTRY phead,temp;
	phead=&pnode->entry;    
	
	for(temp=phead->Flink;temp!=phead;temp=temp->Flink)
	{
		PCareFile carefile=CONTAINING_RECORD(temp,CareFile,entry);

		
		if(::CompareWCharString(carefile->filepath.Buffer,a->filepath.Buffer,carefile->filepath.MaximumLength>a->filepath.MaximumLength?a->filepath.MaximumLength:carefile->filepath.MaximumLength))
			{
				
				a->Owner=carefile->Owner;
				a->secutiry_level=carefile->secutiry_level;
				//CR_MyNodeUnLock(pnode);
				return TRUE;
			}
			else
				CR_DEBUG(DEBUG,("mismatch ,filepath:%ws != %ws",a->filepath.Buffer,carefile->filepath.Buffer));
		
	}
//	CR_DEBUG(DEBUG,("mismatch ,filename:%wZ",a->filename));

   // CR_MyNodeUnLock(pnode);
   return FALSE;
}



