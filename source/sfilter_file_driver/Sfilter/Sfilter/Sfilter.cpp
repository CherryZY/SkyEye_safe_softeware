#include "stdafx.h"

ULONG intest=0;

typedef struct _SfilterExtension
{
	UNICODE_STRING symbolname;
	UNICODE_STRING devicename;
	PDEVICE_OBJECT sfliterdeviceobject; 
	

}SfilterDeviceExtension,*PSfilterDeviceExtension;
//ȫ�ֱ���*******************************************
PDRIVER_OBJECT gSFilterDriverObject = NULL; //������/O���������ɲ��������������
PDEVICE_OBJECT gSFilterControlDeviceObject = NULL; //�����ɱ������������ɵĿ����豸����
FAST_MUTEX gSfilterAttachLock; //����һ�����ٻ���ṹ����(����)
ULONG SfDebug =1;
LIST_ENTRY listentry;      //��ͷ
ULONG MYDEBUG=0x0010;
HASH_TABLE hashtable; 
HASH_TABLE hashtable_result;
HASH_TABLE_CAREFILE hashtable_carefile;

UserInfo user_info;
LIST_ENTRY operationrecord_listentry;   //������Ϣ����
NPAGED_LOOKASIDE_LIST looksidelist;    //����ṹ��Ϊ�˱���Ƶ�������ڴ浼�µ��ڴ���Ƭ����
NPAGED_LOOKASIDE_LIST looksidelist_result;
NPAGED_LOOKASIDE_LIST  looksidelist_carefile;
NPAGED_LOOKASIDE_LIST carefile_verify;

extern BOOLEAN CareFile_Initilized=FALSE;   //��¼�Ƿ��ʼ���ɹ��˹����ļ�����

size_t process_name_offset;  //�������������PEPRCOESS�ṹ����Ľ������ֵ�ƫ������ÿ��windowsϵͳ������ṹ��������������ȷ����
            
//��DriverEntry�����������system������������������ƫ����

KSPIN_LOCK test_lock; 
POperationResult resultfortest;
KSPIN_LOCK myevents_lock;

PThreadEvents myevents[HASH_TABLE_LENGTH]; //�洢��Ӧ�ó��������¼���������������ʱ�������¼�����֪ͨ�¼�


#if WINVER >= 0x0501
 SF_DYNAMIC_FUNCTION_POINTERS gSfDynamicFunctions = {0}; //����ýṹ��������ʼ��Ϊ0
 ULONG gSfOsMajorVersion = 0;        //����ϵͳ���汾��
ULONG gSfOsMinorVersion = 0;        //����ϵͳ���汾��
#endif
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#if DBG && WINVER >= 0x0501
#pragma alloc_text(PAGE, DriverUnload)
#endif

#pragma alloc_text(INIT, SfReadDriverParameters)
#pragma alloc_text(INIT, ProcessOffsetInit)
#pragma alloc_text(PAGE, SfFsNotification)
#pragma alloc_text(PAGE, SfCreate)
#pragma alloc_text(PAGE, SfCleanupClose)
#pragma alloc_text(PAGE, SfFsControl)
#pragma alloc_text(PAGE, SfFsControlMountVolume)
#pragma alloc_text(PAGE, SfFsControlMountVolumeComplete)
#pragma alloc_text(PAGE, SfFsControlLoadFileSystem)
#pragma alloc_text(PAGE, SfFsControlLoadFileSystemComplete)
#pragma alloc_text(PAGE, SfFastIoCheckIfPossible)
#pragma alloc_text(PAGE, SfFastIoRead)
#pragma alloc_text(PAGE, SfFastIoWrite)
#pragma alloc_text(PAGE, SfFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, SfFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, SfFastIoLock)
#pragma alloc_text(PAGE, SfFastIoUnlockSingle)
#pragma alloc_text(PAGE, SfFastIoUnlockAll)
#pragma alloc_text(PAGE, SfFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, SfFastIoDeviceControl)
#pragma alloc_text(PAGE, SfFastIoDetachDevice)
#pragma alloc_text(PAGE, SfFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, SfFastIoMdlRead)
#pragma alloc_text(PAGE, SfFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, SfFastIoMdlWriteComplete)
#pragma alloc_text(PAGE, SfFastIoReadCompressed)
#pragma alloc_text(PAGE, SfFastIoWriteCompressed)
#pragma alloc_text(PAGE, SfFastIoQueryOpen)
#pragma alloc_text(PAGE, SfAttachDeviceToDeviceStack)
#pragma alloc_text(PAGE, SfAttachToFileSystemDevice)
#pragma alloc_text(PAGE, SfDetachFromFileSystemDevice)
#pragma alloc_text(PAGE, SfAttachToMountedDevice)
#pragma alloc_text(PAGE, SfIsAttachedToDevice)
#pragma alloc_text(PAGE, SfIsAttachedToDeviceW2K)
#pragma alloc_text(PAGE, SfIsShadowCopyVolume)

#if WINVER >= 0x0501
#pragma alloc_text(INIT, SfLoadDynamicFunctions)
#pragma alloc_text(INIT, SfGetCurrentVersion)
#pragma alloc_text(PAGE, SfEnumerateFileSystemVolumes)
#pragma alloc_text(PAGE, SfIsAttachedToDeviceWXPAndLater)
#endif

#endif

bool MY_DEBUG(ULONG a,ULONG b) {if(a&&b==0x0000)return false;else return true;}
//************************************���ڳ�ʼ�����ݵĺ���**********************
VOID ProcessOffsetInit()
 {
	 ULONG i;
	 PEPROCESS current;
	 current=::PsGetCurrentProcess();

	 for(i=0;i<3*4*1024;i++)
	 {
		 if(!strncmp("System",(PCHAR)current+i,strlen("System\n")))
		 {
			 process_name_offset=i;
			 KdPrint(("�ҵ�system�ַ�����ʼ����ϣ�%d %0x",i,i));
		     break;
		 }
	 }
}
VOID InitMyEvents()
{
	KdPrint(("��ʼ��ʼ��ȫ���¼��������!\n\n"));
	for(int i=0;i<HASH_TABLE_LENGTH;i++)
	{
		myevents[i]=(PThreadEvents)::ExAllocatePool(NonPagedPool,sizeof(ThreadEvents));
		myevents[i]->OperationType=-1;
		myevents[i]->event=NULL;
		myevents[i]->waitstatue=false;
		::KeInitializeSpinLock(&myevents[i]->nodelock);
		myevents[i]->oldirql=0;

	}
	KdPrint(("��ʼ����ϣ�\n\n"));
}
VOID InitHashTable()
{
        PHASH_TABLE a=&hashtable;
		PHASH_TABLE b=&hashtable_result;

		
		//a=(PHASH_TABLE)ExAllocatePoolWithTag(NonPagedPool,sizeof(HASH_TABLE),MEM_HASH_TAG);	
	
		a->Length=HASH_TABLE_LENGTH;
		b->Length=HASH_TABLE_LENGTH;
		                    
		for(int i=0;i<a->Length;i++)
		{
			PHASH_TABLE_NODE m=&(a->link[i]);
			m->number=0;
			m->OperationType=-1;     //��ʼ����������Ϊ-1����Ϊ�գ�
			::InitializeListHead(&(m->entry));
			::KeInitializeSpinLock(&(m->node_lock));
			m->initilzed=true;
			KdPrint(("the %d node is init:%d\n",i,m->initilzed?1:0));
		}

		KdPrint(("��ʼ��HASH_TABLE�ɹ�,���ڳ�ʼ��HASH_TABLE RESULT.....\n\n"));
		//KdPrint(("attmpt to access hashtable:%d ,inilized:%d",hashtable.link[10].number,hashtable.link[10].initilzed?1:0));
	   		for(int i=0;i<b->Length;i++)
		{
			PHASH_TABLE_NODE m=&(b->link[i]);
			m->number=0;
			m->OperationType=-1;     //��ʼ����������Ϊ-1����Ϊ�գ�
			::InitializeListHead(&(m->entry));
			::KeInitializeSpinLock(&(m->node_lock));
			m->initilzed=true;
			m->totalnumbers=0;
			m->NodeID=i;
			
			KdPrint(("the %d node is init:%d",i,m->initilzed?1:0));
		}
        KdPrint(("��ʼ��HASH_TABLE RESULT�ɹ�!\n\n"));
}
VOID InitUserInfo()
{
	user_info.Security_Level=ROLE_SYSTEM;
	user_info.UserID=2;
}
#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
	KdPrint(("2.53\n"));
if(!FlagOn(SfDebug,0x0000020))
	KdPrint(("true\n"));
UNICODE_STRING symbolname;
//KdPrint(("sfilter in!\n"));
PFAST_IO_DISPATCH fastIoDispatch; //����FAST_IO_DISPATCH�ṹ����
UNICODE_STRING nameString; //�������ִ��ṹ����
NTSTATUS status; //״̬��
ULONG i;

::ProcessOffsetInit();
::KeInitializeSpinLock(&test_lock);
::InitHashTable();
InitMyEvents();
::InitUserInfo();
_try{
::ExInitializeNPagedLookasideList(&looksidelist
	,NULL,
	NULL,
	0,
	sizeof(OperationInformation),
	LOOKASIDE_LIST_FOR_OPERATION,
	0);
::ExInitializeNPagedLookasideList(&looksidelist_result
	,NULL,
	NULL,
	0,
	sizeof(OperationResult),
	LOOKASIDE_LIST_FOR_RESULT,
	0);
::ExInitializeNPagedLookasideList(&looksidelist_carefile
	,NULL,
	NULL,
	0,
	sizeof(CareFile),
	LOOKASIDE_LIST_FOR_CAREFILE,
	0);
::ExInitializeNPagedLookasideList(&carefile_verify
	,NULL,
	NULL,
	0,
	sizeof(CareFile),
	LOOKASIDE_LIST_FOR_CAREFILE_VERIFY,
	0);
}
_except(STATUS_ACCESS_VIOLATION)
{
	KdPrint(("��ʼ��LookAsidelistʧ�ܣ�\n"));
}

::InitHashTable_CareFile();	    //����Ҫ��looksid_carefile�ɹ���ʼ��֮����ܿ�ʼ��ʼ��

//::InitializeListHead(&listentry);  //��ʼ������
//KdPrint(("��ʼ������,��ϣ�����1!\r\n"));

#if WINVER >= 0x0501 //��������,���OS�汾��WinXP����,����������,���򲻱���
SfLoadDynamicFunctions();
SfGetCurrentVersion();
KdPrint(("the current system version upon Windows XP\n"));
#endif




SfReadDriverParameters( RegistryPath );
gSFilterDriverObject = DriverObject; //��I/O������������������󱣴浽ȫ�ֱ���gSFilterDriverObject��


#if DBG && WINVER >= 0x0501 //��OS�汾��xp����������checked��,������Щ���,���򲻱���
if (NULL != gSfDynamicFunctions.EnumerateDeviceObjectList)
{
gSFilterDriverObject->DriverUnload = DriverUnload; //ע������ж�غ���
}
#endif

ExInitializeFastMutex( &gSfilterAttachLock ); //��ʼ��"FastMutex(���ٻ���)"����,�Ժ���߳�ֻ�ܻ��������



//���������豸����
RtlInitUnicodeString( &nameString, L"\\FileSystem\\Filters\\SFilter" ); //���������ļ�ϵͳ�����豸����

//���������豸����
status = IoCreateDevice( DriverObject,
sizeof(SfilterDeviceExtension), //û�� �豸��չ
&nameString, //�豸��: FileSystem\\Filters\\SFilter
FILE_DEVICE_DISK_FILE_SYSTEM, //�豸����: �����ļ�ϵͳ
FILE_DEVICE_SECURE_OPEN, //�豸����: �Է��͵�CDO�Ĵ�������а�ȫ���
FALSE, //����һ�����û�ģʽ��ʹ�õ��豸
&gSFilterControlDeviceObject ); //�������ɵ�"�����豸����"

if (status == STATUS_OBJECT_PATH_NOT_FOUND) //�ж��Ƿ� δ�ҵ�·��
{
RtlInitUnicodeString( &nameString, L"\\FileSystem\\SFilterCDO" ); //���´��� �����豸����
status = IoCreateDevice( DriverObject, 0,
&nameString, //�豸��: FileSystem\\SFilterCDO
FILE_DEVICE_DISK_FILE_SYSTEM,
FILE_DEVICE_SECURE_OPEN,
FALSE,
&gSFilterControlDeviceObject ); //�������ɵ� �����豸����
}


if (!NT_SUCCESS( status )) //�ж�IoCreateDevice�����Ƿ�ɹ�
{
KdPrint(( "SFilter!DriverEntry: Error creating control device object \"%wZ\", status=%08x\n", &nameString, status ));
return status; //���󷵻�(����CDOʧ��)
}

//������������Ӧ�ó����
	::RtlInitUnicodeString(&symbolname,L"\\DosDevices\\SFilterDevice");
	status=::IoCreateSymbolicLink(&symbolname,&nameString);
	if(!NT_SUCCESS(status))
	{
		::IoDeleteSymbolicLink(&symbolname);
		status=::IoCreateSymbolicLink(&symbolname,&nameString);
		if(!NT_SUCCESS(status))
		{
			::IoDeleteSymbolicLink(&symbolname);
			KdPrint(("create symbollink name failed!\n"));
			return status;
		}
	
	}
	KdPrint(("create symbol link successfully!\n"));
	PSfilterDeviceExtension deviceextension=(PSfilterDeviceExtension)DriverObject->DriverExtension;
	deviceextension->devicename=nameString;
	deviceextension->symbolname=symbolname;
	deviceextension->sfliterdeviceobject=gSFilterControlDeviceObject;

//ע��Ĭ����ǲ����
for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
{
DriverObject->MajorFunction[i] = SfPassThrough;
}
//ע�������ǲ����
DriverObject->MajorFunction[IRP_MJ_CREATE] = SfCreate;
DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = SfCreate;
DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = SfCreate;
DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SfCleanupClose;
DriverObject->MajorFunction[IRP_MJ_CLOSE] = SfCleanupClose;
DriverObject->MajorFunction[IRP_MJ_READ]=SfRead;
DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]=SfDirectoryControl;
DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]=SfSetInformation;
DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=SfDeviceControl;

fastIoDispatch = (PFAST_IO_DISPATCH)ExAllocatePoolWithTag( NonPagedPool, //�ӷǷ�ҳ���з���
sizeof( FAST_IO_DISPATCH ), //Ҫ������ֽ���
SFLT_POOL_TAG); //ָ��һ��4�ֽڵı�ǩ(ǰ���Ѻ궨��:'tlFS')

if (!fastIoDispatch) //�ڴ����ʧ��
{
IoDeleteDevice( gSFilterControlDeviceObject ); //ɾ�����洴����CDO
return STATUS_INSUFFICIENT_RESOURCES; //����һ������status��(��Դ����)
}
RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );
DriverObject->FastIoDispatch = fastIoDispatch; //��FastIo���ɱ��浽���������FastIoDispatch��
fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH ); //����FastIo���ɱ�ĳ�����

fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible; //����FastIo���ɺ���,��21��
fastIoDispatch->FastIoRead = SfFastIoRead;
fastIoDispatch->FastIoWrite = SfFastIoWrite;
fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo;
fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo;
fastIoDispatch->FastIoLock = SfFastIoLock;
fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle;
fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll;
fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey;
fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl;
fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo;
fastIoDispatch->MdlRead = SfFastIoMdlRead;
fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete;
fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite;
fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete;
fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed;
fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed;
fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;
fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;
                          


//--------------------------------ע��fsFilter�ص�����-------------------------------

#if WINVER >= 0x0501 //���OS�汾ΪWinXP����,������δ���,���򲻱���
{
FS_FILTER_CALLBACKS fsFilterCallbacks;
if (NULL != gSfDynamicFunctions.RegisterFileSystemFilterCallbacks)
{
fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
fsFilterCallbacks.PreAcquireForSectionSynchronization = SfPreFsFilterPassThrough;
fsFilterCallbacks.PostAcquireForSectionSynchronization = SfPostFsFilterPassThrough;
fsFilterCallbacks.PreReleaseForSectionSynchronization = SfPreFsFilterPassThrough;
fsFilterCallbacks.PostReleaseForSectionSynchronization = SfPostFsFilterPassThrough;
fsFilterCallbacks.PreAcquireForCcFlush = SfPreFsFilterPassThrough;
fsFilterCallbacks.PostAcquireForCcFlush = SfPostFsFilterPassThrough;
fsFilterCallbacks.PreReleaseForCcFlush = SfPreFsFilterPassThrough;
fsFilterCallbacks.PostReleaseForCcFlush = SfPostFsFilterPassThrough;
fsFilterCallbacks.PreAcquireForModifiedPageWriter = SfPreFsFilterPassThrough;
fsFilterCallbacks.PostAcquireForModifiedPageWriter = SfPostFsFilterPassThrough;
fsFilterCallbacks.PreReleaseForModifiedPageWriter = SfPreFsFilterPassThrough;
fsFilterCallbacks.PostReleaseForModifiedPageWriter = SfPostFsFilterPassThrough;

status = (gSfDynamicFunctions.RegisterFileSystemFilterCallbacks)( DriverObject, &fsFilterCallbacks );
if (!NT_SUCCESS( status ))
{
DriverObject->FastIoDispatch = NULL;
ExFreePool( fastIoDispatch );
IoDeleteDevice( gSFilterControlDeviceObject );
return status;
}
}
}
#endif


status = IoRegisterFsRegistrationChange( DriverObject, SfFsNotification );//ע���ļ�ϵͳ�ı�ʱ�����Ļص�����������ɺ�����һЩ���飬Ȼ���ں���󶨶�̬�����ļ�ϵͳ��CDO
if (!NT_SUCCESS( status ))
{
KdPrint(( "SFilter!DriverEntry: Error registering FS change notification, status=%08x\n", status ));

DriverObject->FastIoDispatch = NULL; //ע��ָ��fastIo�������ָ��ΪNULL
ExFreePoolWithTag( fastIoDispatch, SFLT_POOL_TAG ); //�ͷŷ����fastIo��������ڴ�
IoDeleteDevice( gSFilterControlDeviceObject ); //ɾ�����洴����CDO
return status; //���󷵻�
}

{

PDEVICE_OBJECT rawDeviceObject;
PFILE_OBJECT fileObject;
RtlInitUnicodeString( &nameString, L"\\Device\\RawDisk" );//��ͨ�Ĵ���


/*
IoGetDeviceObjectPointer�����Ĺ�����:
�����²���豸��������������²��豸ָ�롣�ú�������˶��²��豸�����Լ��²��豸��������Ӧ���ļ���������á�
�������������ж��֮ǰ���²���豸��������û�û�����������²�������ж�ػᱻֹͣ����˱���Ҫ�������²��豸��������á�
���ǳ���һ�㲻��ֱ�Ӷ��²��豸��������ü��١����ֻҪ���ٶ��ļ���������þͿ��Լ����ļ�������豸����������������á�
��ʵ�ϣ�IoGetDeviceObjectPointer���صĲ������²��豸�����ָ�룬���Ǹ��豸��ջ�ж�����豸�����ָ�롣


IoGetDeviceObjectPointer�����ĵ��ñ����� IRQL=PASSIVE_LEVEL�ļ��������С�
*/

status = IoGetDeviceObjectPointer( &nameString, FILE_READ_ATTRIBUTES, &fileObject, &rawDeviceObject );//
if (NT_SUCCESS( status ))
{
SfFsNotification( rawDeviceObject, TRUE );   //����һ�������豸��FSCDO��
ObDereferenceObject( fileObject ); //������ٶ��ļ����������
}

RtlInitUnicodeString( &nameString, L"\\Device\\RawCdRom" );   //�ҵ�Cd�豸��������
status = IoGetDeviceObjectPointer( &nameString, FILE_READ_ATTRIBUTES, &fileObject, &rawDeviceObject );
if (NT_SUCCESS( status ))
{
SfFsNotification( rawDeviceObject, TRUE );   
ObDereferenceObject( fileObject );//������ٶ��ļ����������
}
}

ClearFlag( gSFilterControlDeviceObject->Flags, DO_DEVICE_INITIALIZING );
return STATUS_SUCCESS;
}


// ����ж�غ���
#if DBG && WINVER >= 0x0501
VOID DriverUnload( IN PDRIVER_OBJECT DriverObject )
{
//KdPrint(("DriverUnload : start display the hashtable\n\n"));

PSFILTER_DEVICE_EXTENSION devExt;
PFAST_IO_DISPATCH fastIoDispatch;
NTSTATUS status;
ULONG numDevices;
ULONG i;
LARGE_INTEGER interval;

PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];
ASSERT(DriverObject == gSFilterDriverObject);
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES, ("SFilter!DriverUnload: Unloading driver (%p)\n", DriverObject) );

for(int i=0;i<hashtable.Length;i++)
{
     while(!IsListEmpty(&hashtable.link[i].entry))
		{
		PLIST_ENTRY entry=RemoveHeadList(&hashtable.link[i].entry);;
		POperationInformation m=CONTAINING_RECORD(entry,OperationInformation,listentry);
		::ExFreeToNPagedLookasideList(&looksidelist,m);
		}	
	//::ExFreeToNPagedLookasideList(&looksidelist,hashtable.link[i].entry);
}
::ExFreePool(&hashtable);
::ExDeleteNPagedLookasideList(&looksidelist);

IoUnregisterFsRegistrationChange( DriverObject, SfFsNotification );
for (;;)
{
ASSERT( NULL != gSfDynamicFunctions.EnumerateDeviceObjectList );
status = (gSfDynamicFunctions.EnumerateDeviceObjectList)( DriverObject, devList, sizeof(devList), &numDevices);
if (numDevices <= 0)
{
break;
}

numDevices = min( numDevices, DEVOBJ_LIST_SIZE );
for (i=0; i < numDevices; i++)
{
devExt =(PSFILTER_DEVICE_EXTENSION) devList[i]->DeviceExtension;
if (NULL != devExt)
{
IoDetachDevice( devExt->AttachedToDeviceObject );
}
}

interval.QuadPart = (5 * DELAY_ONE_SECOND); //delay 5 seconds
KeDelayExecutionThread( KernelMode, FALSE, &interval );
for (i=0; i < numDevices; i++)
{
if (NULL != devList[i]->DeviceExtension)
{
SfCleanupMountedDevice( devList[i] );
}
else
{
ASSERT(devList[i] == gSFilterControlDeviceObject);
gSFilterControlDeviceObject = NULL;
}

IoDeleteDevice( devList[i] );
ObDereferenceObject( devList[i] );
}
}

fastIoDispatch = DriverObject->FastIoDispatch;
DriverObject->FastIoDispatch = NULL;
ExFreePool( fastIoDispatch );
}
#endif



// SfLoadDynamicFunctions����(����WindowsXPϵͳ�±���ú���)
#if WINVER >= 0x0501
VOID SfLoadDynamicFunctions()
{

/*
��̬�����������Щ�ں˺�����ʹ�ö�̬������ν�����ǣ��ڵͰ汾��Windows����ϵͳ�ϣ���γ�����Ȼ���Լ��سɹ���
*/

UNICODE_STRING functionName;
RtlZeroMemory( &gSfDynamicFunctions, sizeof( gSfDynamicFunctions ) ); //��gSfDynamicFunctions�ṹ������0

RtlInitUnicodeString( &functionName, L"FsRtlRegisterFileSystemFilterCallbacks" );
gSfDynamicFunctions.RegisterFileSystemFilterCallbacks =(PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS) MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"IoAttachDeviceToDeviceStackSafe" );
gSfDynamicFunctions.AttachDeviceToDeviceStackSafe =(PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE) MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"IoEnumerateDeviceObjectList" );
gSfDynamicFunctions.EnumerateDeviceObjectList =(PSF_ENUMERATE_DEVICE_OBJECT_LIST) MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"IoGetLowerDeviceObject" );
gSfDynamicFunctions.GetLowerDeviceObject =(PSF_GET_LOWER_DEVICE_OBJECT)MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"IoGetDeviceAttachmentBaseRef" );
gSfDynamicFunctions.GetDeviceAttachmentBaseRef =(PSF_GET_DEVICE_ATTACHMENT_BASE_REF)MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"IoGetDiskDeviceObject" );
gSfDynamicFunctions.GetDiskDeviceObject =(PSF_GET_DISK_DEVICE_OBJECT)MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"IoGetAttachedDeviceReference" );
gSfDynamicFunctions.GetAttachedDeviceReference =(PSF_GET_ATTACHED_DEVICE_REFERENCE) MmGetSystemRoutineAddress( &functionName );

RtlInitUnicodeString( &functionName, L"RtlGetVersion" );
gSfDynamicFunctions.GetVersion =(PSF_GET_VERSION) MmGetSystemRoutineAddress( &functionName );
}
#endif



//SfGetCurrentVersion����(��WinXP�±���ú���)
#if WINVER >= 0x0501
VOID SfGetCurrentVersion()
{
if (NULL != gSfDynamicFunctions.GetVersion)
{
RTL_OSVERSIONINFOW versionInfo;
NTSTATUS status;

versionInfo.dwOSVersionInfoSize = sizeof( RTL_OSVERSIONINFOW );
status = (gSfDynamicFunctions.GetVersion)( &versionInfo );
ASSERT( NT_SUCCESS( status ) );
gSfOsMajorVersion = versionInfo.dwMajorVersion;
gSfOsMinorVersion = versionInfo.dwMinorVersion;
}
else
{
PsGetVersion( &gSfOsMajorVersion, &gSfOsMinorVersion, NULL, NULL );
}
}
#endif




VOID SfFsNotification( IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN FsActive )
{

/*
SfFsNotification������
������һ���豸���󣬲��������ӵ�ָ�����ļ�ϵͳ�����豸����(File System CDO)�Ķ���ջ�ϡ������������豸����������з��͸��ļ�ϵͳ������
���������Ǿ��ܹ����һ�����ؾ�����󣬾Ϳ��Ը��ӵ�����µľ��豸������豸����ջ�ϡ�

��SfFsNotification������������Ժ����ǵĹ��������豸������ܹ����յ����͵��ļ�ϵͳCDO�����󣬼����յ�IRP_MJ_FILE_SYSTEM_CONTROL������˵��
�ļ�ϵͳ�����豸�Ѿ����󶨣����Զ�̬��ؾ�Ĺ����ˡ���ô�Ժ�Ĺ�������Ҫ��ɶԾ�ļ�ذ��ˡ�


����˵��:

DeviceObject: ��ָ���ļ�ϵͳ�Ŀ����豸����(CDO)���� �������������File System CDO
FsActive: ֵΪTRUE����ʾ�ļ�ϵͳ�ļ��ֵΪFALSE����ʾ�ļ�ϵͳ��ж�ء�

*/

UNICODE_STRING name; //����ṹ����
WCHAR nameBuffer[MAX_DEVNAME_LENGTH]; //������ַ�������,����64

PAGED_CODE();

RtlInitEmptyUnicodeString( &name, nameBuffer, sizeof(nameBuffer) ); //��ʼ��name(��ԱBuffer->nameBuffer,Length=0,MaximumLength=64)
SfGetObjectName( DeviceObject, &name );
KdPrint(("SfNotification in!\n"));
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsNotification: %s %p \"%wZ\" (%s)\n",
(FsActive) ? "Activating file system " : "Deactivating file system",
DeviceObject,
&name,
GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType)) );

if (FsActive)
{
SfAttachToFileSystemDevice( DeviceObject, &name ); //������ɶ��ļ�ϵͳ�����豸�İ�
}
else
{
SfDetachFromFileSystemDevice( DeviceObject );
}
}



NTSTATUS SfPassThrough( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT( DeviceObject ));
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
IoSkipCurrentIrpStackLocation( Irp );
return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}



VOID SfDisplayCreateFileName( IN PIRP Irp )
{
PIO_STACK_LOCATION irpSp;
PUNICODE_STRING name;
GET_NAME_CONTROL nameControl;

irpSp = IoGetCurrentIrpStackLocation( Irp );
name = SfGetFileName( irpSp->FileObject, Irp->IoStatus.Status, &nameControl );
//SfDebug=SFDEBUG_DISPLAY_CREATE_NAMES;
if (irpSp->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID)
{
SF_LOG_PRINT( SFDEBUG_DISPLAY_CREATE_NAMES,
("SFilter!SfDisplayCreateFileName: Opened %08x:%08x %wZ ,%wZ(FID) \n",
Irp->IoStatus.Status,
Irp->IoStatus.Information,
name));
}
else
{
SF_LOG_PRINT( SFDEBUG_DISPLAY_CREATE_NAMES,
("SFilter!SfDisplayCreateFileName: Opened %08x:%08x %wZ\n",
Irp->IoStatus.Status,
Irp->IoStatus.Information,
name));
}
//SfDebug=1;
SfGetFileNameCleanup( &nameControl );
}



NTSTATUS SfCreate( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
NTSTATUS status;

/*
��sFilter�Ĵ����У���������if���������д�ĺ����:������CreateFile������Ring3�´򿪴˿����豸�������ӵ�ʱ���ʧ��

if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
{
Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
Irp->IoStatus.Information = 0;
IoCompleteRequest( Irp, IO_NO_INCREMENT );
return STATUS_INVALID_DEVICE_REQUEST;
}
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));*/
//KdPrint(("sfCreate 1\n"));
/*
��ˣ���Ҫ����������if������޸ĳ�������ʽ:
*/
if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
{
KdPrint(("Sfiter:My Control Device!\n"));
Irp->IoStatus.Status = STATUS_SUCCESS;
Irp->IoStatus.Information = FILE_OPENED;
IoCompleteRequest( Irp, IO_NO_INCREMENT );
return STATUS_SUCCESS;
}


PIO_STACK_LOCATION irpsp=(PIO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
PFILE_OBJECT fileobject=irpsp->FileObject;
UNICODE_STRING filename={0};
UNICODE_STRING filterfilename={0};
WCHAR *filter=L"\\Device\\HarddiskVolume1\\Windows\\fortest.txt";
WCHAR filenamebuffer[512];
ULONG length;
/*
filename.MaximumLength=1024;
filterfilename.MaximumLength=1024;
*/
do
{
	::RtlInitUnicodeString(&filterfilename,filter);
	::RtlInitEmptyUnicodeString(&filename,filenamebuffer,sizeof(WCHAR)*512);
/*filename.Buffer=(PWSTR)::ExAllocatePool(NonPagedPool,1024);

if(filename.Buffer==NULL)
{
	KdPrint(("filename allocate memory failed\n"));
	break;
}

filterfilename.Buffer=(PWSTR)::ExAllocatePool(NonPagedPool,1024);
if( filterfilename.Buffer==NULL)
{
	ExFreePool(filename.Buffer);
	break;
}

RtlZeroMemory(filename.Buffer,1024);
filename.Length=2*wcslen(string);
ASSERT(filename.MaximumLength>=filename.Length);
::RtlCopyMemory(filename.Buffer,string,filename.Length);


filterfilename.Length=2*wcslen(filter);
ASSERT(filterfilename.MaximumLength>=filterfilename.Length);
::RtlCopyMemory(filterfilename.Buffer,filter,filterfilename.Length);
//CURRENT_IRQL_PRINT();
*/
if((KeGetCurrentIrql()==PASSIVE_LEVEL))
{

length=MyFileFullPathQuery(fileobject,&filename);//����ļ���

if(::IsCreateNewFile(Irp))
{
	
}	

}
else
{
	KdPrint(("�жϼ������\n"));
}

}while(0);



SfDebug=SFDEBUG_GET_CREATE_NAMES;
if (!FlagOn( SfDebug, SFDEBUG_DO_CREATE_COMPLETION | SFDEBUG_GET_CREATE_NAMES| SFDEBUG_DISPLAY_CREATE_NAMES ))//�Ķ���,ԭΪ!
{
	//KdPrint(("sfCreate 2\n"));
IoSkipCurrentIrpStackLocation( Irp );
return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}
else
{
//��ʼ���¼���������������̡�

//	KdPrint(("sfCreate 3\n"));
/*PIO_STACK_LOCATION irpSp;
PUNICODE_STRING name;
UNICODE_STRING nameto;
GET_NAME_CONTROL nameControl;
WCHAR buff[1024]=L"start";
WCHAR *filter=L"\\Device\\HarddiskVolume1\\fortest";
irpSp = IoGetCurrentIrpStackLocation( Irp );
if((KeGetCurrentIrql()==PASSIVE_LEVEL))
{
//KdPrint(("compare in2!\n"));
//name = SfGetFileName( irpSp->FileObject, Irp->IoStatus.Status, &nameControl );
name=::MyFileFullPathQueryW(irpSp->FileObject,1024);
::RtlInitUnicodeString(&nameto,filter);
if(::RtlCompareUnicodeString(name,&nameto,true)==0)
{
	 KdPrint(("��Ŀ¼�µ��ļ��������:%wZ",name));
	 ::ExFreePool(name->Buffer);
     status =STATUS_SUCCESS;
     IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

}
*/
KEVENT waitEvent;
KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );
IoCopyCurrentIrpStackLocationToNext( Irp );
IoSetCompletionRoutine( Irp,
SfCreateCompletion,
&waitEvent,
TRUE,
TRUE,
TRUE );
status = IoCallDriver( ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
if (STATUS_PENDING == status)//��״̬�� ����
{
	KdPrint(("����!\n\n"));
NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
ASSERT(STATUS_SUCCESS == localStatus);
}

ASSERT(KeReadStateEvent(&waitEvent) || !NT_SUCCESS(Irp->IoStatus.Status));


/*if (irpSp->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID)
{
::RtlInitUnicodeString(&nameto,filter);
if(::RtlCompareUnicodeString(name,&nameto,true)==0)
{
	 KdPrint(("��Ŀ¼�µ��ļ��������:%wZ",&nameto));
}
else
{
	//KdPrint(("name:%wZ ,nameto: %wZ",name,&nameto));
	;
}
}
else
{
	//KdPrint(("Non FID:%wZ",name));
}
::SfGetFileNameCleanup(&nameControl);
}*/

if (FlagOn(SfDebug, (SFDEBUG_GET_CREATE_NAMES|SFDEBUG_DISPLAY_CREATE_NAMES)))
{
//SfDisplayCreateFileName( Irp );
}
status = Irp->IoStatus.Status;
IoCompleteRequest( Irp, IO_NO_INCREMENT );
return status;
}

}



NTSTATUS SfCreateCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context )
{
PKEVENT event =(PKEVENT) Context;
KSPIN_LOCK spinlock;
KIRQL irql=::KeGetCurrentIrql();//�������û�г�ʼ��������Ĭ��Ϊ��0�����ں����kerealsespinlock���������dispatch��0�Ĵ��󣬳�����irql_gt_zero_at_system_service�Ĵ���
KeInitializeSpinLock(&spinlock);
UNREFERENCED_PARAMETER( DeviceObject );
//UNREFERENCED_PARAMETER( Irp );

PIO_STACK_LOCATION irpsp=(PIO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
PFILE_OBJECT fileobject=irpsp->FileObject;
//WCHAR buff[FILE_NAME_LENGTH_MAX];
//GET_NAME_CONTROL nameControl;
if(NT_SUCCESS(Irp->IoStatus.Status))
{
	if(fileobject!=NULL&&(irpsp->Parameters.Create.Options&FILE_DIRECTORY_FILE)!=0)
	{
	//	PUNICODE_STRING filenames=SfGetFileName(fileobject,Irp->IoStatus.Status,&nameControl);

		//MyAddToHashTable(fileobject,&spinlock,filenames);

	//	::SfGetFileNameCleanup(&nameControl);
	}
}
KeReleaseSpinLock(&spinlock,irql);
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

KeSetEvent(event, IO_NO_INCREMENT, FALSE);

return STATUS_MORE_PROCESSING_REQUIRED;
}


//VOID MyDeleteToLinkSet(PFILE_OBJECT fileobject,PKSPIN_LOCK spn)
//{

//}
NTSTATUS SfCleanupClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
PAGED_CODE();
ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT( DeviceObject ));

if(DeviceObject==gSFilterControlDeviceObject)     //����ǶԿ������ý���������ֱ�ӷ���
{
	Irp->IoStatus.Information=0;
	Irp->IoStatus.Status=STATUS_SUCCESS;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
IoSkipCurrentIrpStackLocation( Irp );
return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}



NTSTATUS SfFsControl( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
/*
����˵��:

DeviceObject: ���Ǵ������豸�������Ǳ��󶨵��ļ�ϵͳ�����豸����ջ�ϡ�

*/

PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
PAGED_CODE();
ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT( DeviceObject ));
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
switch (irpSp->MinorFunction) {

case IRP_MN_MOUNT_VOLUME:         //�����豸�Ĺ��ش�����

return SfFsControlMountVolume( DeviceObject, Irp );

case IRP_MN_LOAD_FILE_SYSTEM:     //�ļ�ϵͳ���״μ���ʱ

return SfFsControlLoadFileSystem( DeviceObject, Irp );

case IRP_MN_USER_FS_REQUEST:
{
switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

case FSCTL_DISMOUNT_VOLUME://�����豸���Ƴ������Ǻ��Ѳ�׽��   
{
PSFILTER_DEVICE_EXTENSION devExt =(PSFILTER_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);

SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsControl: Dismounting volume %p \"%wZ\"\n",
devExt->AttachedToDeviceObject,
&devExt->DeviceName) );
break;
}
}
break;
}
}

IoSkipCurrentIrpStackLocation( Irp );
return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}



NTSTATUS SfFsControlCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context )
{
UNREFERENCED_PARAMETER( DeviceObject );
UNREFERENCED_PARAMETER( Irp );

ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
ASSERT(Context != NULL);

#if WINVER >= 0x0501
if (IS_WINDOWSXP_OR_LATER())
{
KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
}
else
{
#endif
if (KeGetCurrentIrql() > PASSIVE_LEVEL)
{
ExQueueWorkItem( (PWORK_QUEUE_ITEM) Context, DelayedWorkQueue );
}
else
{
PWORK_QUEUE_ITEM workItem = (PWORK_QUEUE_ITEM)Context;
(workItem->WorkerRoutine)(workItem->Parameter);
}

#if WINVER >= 0x0501
}
#endif

return STATUS_MORE_PROCESSING_REQUIRED;
}




NTSTATUS SfFsControlMountVolume( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
/*
����˵��:

DeviceObject: �������Ǵ������豸���������󶨵��ļ�ϵͳCDO���豸ջ�ϡ�
Irp: ���Ƿ��͸��ļ�ϵͳCDO�Ĺ�����������һ���¾�Ĺ�������

*/

PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

PDEVICE_OBJECT storageStackDeviceObject;


// newDeviceObject�ǽ�Ҫ�󶨵��ļ�ϵͳ�ľ��豸�����ϡ�����˵�����newDeviceObjectҪ���󶨵��¹��ؾ���豸���ϡ�
PDEVICE_OBJECT newDeviceObject;

PSFILTER_DEVICE_EXTENSION newDevExt;
NTSTATUS status;
BOOLEAN isShadowCopyVolume;
PFSCTRL_COMPLETION_CONTEXT completionContext;


PAGED_CODE();

ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));


/*
�ڰ�IRP���͵��ļ�ϵͳ֮ǰ��������������ʱ��Vpb->RealDevice������ǣ���Ҫ�����صĴ����豸����
storageStackDeviceObject���ȱ�����VPB��ֵ��������Ϊ����IRP�·����ײ������󣬿��ܻ�ı䡣
*/
storageStackDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;


status = SfIsShadowCopyVolume ( storageStackDeviceObject, &isShadowCopyVolume );

if (NT_SUCCESS(status) &&
isShadowCopyVolume &&
!FlagOn(SfDebug,SFDEBUG_ATTACH_TO_SHADOW_COPIES))
{

UNICODE_STRING shadowDeviceName;
WCHAR shadowNameBuffer[MAX_DEVNAME_LENGTH];

RtlInitEmptyUnicodeString( &shadowDeviceName, shadowNameBuffer, sizeof(shadowNameBuffer) );
SfGetObjectName( storageStackDeviceObject, &shadowDeviceName );
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsControlMountVolume Not attaching to Volume %p \"%wZ\", shadow copy volume\n",
storageStackDeviceObject,
&shadowDeviceName) );

//���������󶨾�Ӱ��������һ������
IoSkipCurrentIrpStackLocation( Irp );
return IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}

status = IoCreateDevice( gSFilterDriverObject,
sizeof( SFILTER_DEVICE_EXTENSION ),
NULL,
DeviceObject->DeviceType,
0,
FALSE,
&newDeviceObject );



/*�������IRP���͵��ļ�ϵͳ�У���ô�ļ�ϵͳ�Ͳ����յ������Ĺ�������*/
if (!NT_SUCCESS( status ))
{
KdPrint(( "SFilter!SfFsControlMountVolume: Error creating volume device object, status=%08x\n", status ));

Irp->IoStatus.Information = 0;
Irp->IoStatus.Status = status;
IoCompleteRequest( Irp, IO_NO_INCREMENT );

return status;
}


//��д�豸��չ������Ŀ���ǣ���������ɺ��������׵�storageStackDeviceObject
newDevExt =(PSFILTER_DEVICE_EXTENSION) newDeviceObject->DeviceExtension;
newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
RtlInitEmptyUnicodeString( &newDevExt->DeviceName, newDevExt->DeviceNameBuffer, sizeof(newDevExt->DeviceNameBuffer) );
SfGetObjectName( storageStackDeviceObject, &newDevExt->DeviceName );

#if WINVER >= 0x0501

if (IS_WINDOWSXP_OR_LATER())
{

//�������������¼����󣬰���������������С���������Ŀ���ǣ�֪ͨ��ǰ���̣��ļ�ϵͳ�Ѿ�����˵�ǰ��Ĺ��ء�
KEVENT waitEvent;
KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );
IoCopyCurrentIrpStackLocationToNext ( Irp );
IoSetCompletionRoutine( Irp,
SfFsControlCompletion,
&waitEvent, //context parameter
TRUE,
TRUE,
TRUE );
status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
if (STATUS_PENDING == status)
{

//�ȴ����ײ��������ɣ�Ȼ��ͻ����������̡�
status = KeWaitForSingleObject( &waitEvent,
Executive,
KernelMode,
FALSE,
NULL );
ASSERT( STATUS_SUCCESS == status );
}
ASSERT(KeReadStateEvent(&waitEvent) ||
!NT_SUCCESS(Irp->IoStatus.Status));


//ִ�е������˵����Ĺ����Ѿ���ɣ�Ҫ��ʼ�󶨾��ˡ��ȵ���ɺ����������¼�֮�������󶨾�
status = SfFsControlMountVolumeComplete( DeviceObject, Irp, newDeviceObject );

}
else
{
#endif
completionContext = (PFSCTRL_COMPLETION_CONTEXT)ExAllocatePoolWithTag( NonPagedPool, sizeof( FSCTRL_COMPLETION_CONTEXT ), SFLT_POOL_TAG );
if (completionContext == NULL)
{
IoSkipCurrentIrpStackLocation( Irp );
status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}
else
{
//��ʼ��һ����������ָ��һ���д�ִ�еĺ���SfFsControlMountVolumeCompleteWorker��������������뵽ĳ���߳���ȥִ�С�
ExInitializeWorkItem( &completionContext->WorkItem, (PWORKER_THREAD_ROUTINE)SfFsControlMountVolumeCompleteWorker, completionContext );
completionContext->DeviceObject = DeviceObject;
completionContext->Irp = Irp;
completionContext->NewDeviceObject = newDeviceObject;
IoCopyCurrentIrpStackLocationToNext( Irp );
IoSetCompletionRoutine( Irp,
SfFsControlCompletion,
&completionContext->WorkItem, //context parameter
TRUE,
TRUE,
TRUE );
status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}
#if WINVER >= 0x0501
}
#endif

return status;
}


VOID SfFsControlMountVolumeCompleteWorker( IN PFSCTRL_COMPLETION_CONTEXT Context )
{
ASSERT( Context != NULL );

SfFsControlMountVolumeComplete( Context->DeviceObject,
Context->Irp,
Context->NewDeviceObject );

ExFreePoolWithTag( Context, SFLT_POOL_TAG );
}



NTSTATUS SfFsControlMountVolumeComplete( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_OBJECT NewDeviceObject )
{
/*
����˵��:

DeviceObject: ���ǰ󶨵��ļ�ϵͳ�����豸������豸ջ�ϣ�����һ�������豸����
Irp: ���Ƿ��͸��ļ�ϵͳCDO�Ĺ�����������һ���¾�Ĺ�������
NewDeviceObject: �����´����Ĺ����豸�������ڰ󶨵��ļ�ϵͳ�ľ��豸������豸ջ�ϡ�

*/

PVPB vpb;
PSFILTER_DEVICE_EXTENSION newDevExt;
PIO_STACK_LOCATION irpSp;
PDEVICE_OBJECT attachedDeviceObject;
NTSTATUS status;

PAGED_CODE();

newDevExt =(PSFILTER_DEVICE_EXTENSION)NewDeviceObject->DeviceExtension;
irpSp = IoGetCurrentIrpStackLocation( Irp );

/*
* ��ȡ���Ǳ����VPB�����ʱ��Ϳ���ͨ�����豸����õ�VPB
* VPB->DeviceObject�� �ļ�ϵͳ�����ľ��豸����
* VPB->RealDevice�� �������������������豸����
*/
vpb = newDevExt->StorageStackDeviceObject->Vpb;
if (vpb != irpSp->Parameters.MountVolume.Vpb)
{
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsControlMountVolume: VPB in IRP stack changed %p IRPVPB=%p VPB=%p\n",
vpb->DeviceObject,
irpSp->Parameters.MountVolume.Vpb,
vpb) );
}

if (NT_SUCCESS( Irp->IoStatus.Status ))
{
ExAcquireFastMutex( &gSfilterAttachLock );
if (!SfIsAttachedToDevice( vpb->DeviceObject, &attachedDeviceObject ))
{

/*
* SfAttachToMountedDevice�����壺�����Ǵ����Ĺ����豸����NewDeviceObject�󶨵��ļ�ϵͳ������VPB->DeviceObject���豸����ջ�ϡ�
*/
status = SfAttachToMountedDevice( vpb->DeviceObject, NewDeviceObject );

if (!NT_SUCCESS( status ))
{
SfCleanupMountedDevice( NewDeviceObject );
IoDeleteDevice( NewDeviceObject );
}

ASSERT( NULL == attachedDeviceObject );
}
else
{
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsControlMountVolume Mount volume failure for %p \"%wZ\", already attached\n",
((PSFILTER_DEVICE_EXTENSION)attachedDeviceObject->DeviceExtension)->AttachedToDeviceObject,
&newDevExt->DeviceName) );

SfCleanupMountedDevice( NewDeviceObject );
IoDeleteDevice( NewDeviceObject );
ObDereferenceObject( attachedDeviceObject );
}

ExReleaseFastMutex( &gSfilterAttachLock );
}
else
{
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsControlMountVolume: Mount volume failure for %p \"%wZ\", status=%08x\n",
DeviceObject,
&newDevExt->DeviceName,
Irp->IoStatus.Status) );
SfCleanupMountedDevice( NewDeviceObject );
IoDeleteDevice( NewDeviceObject );
}

status = Irp->IoStatus.Status;
IoCompleteRequest( Irp, IO_NO_INCREMENT );
return status;
}



NTSTATUS SfFsControlLoadFileSystem( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
PSFILTER_DEVICE_EXTENSION devExt =(PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
NTSTATUS status;
PFSCTRL_COMPLETION_CONTEXT completionContext;

PAGED_CODE();

SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFscontrolLoadFileSystem: Loading File System, Detaching from \"%wZ\"\n",
&devExt->DeviceName) );

#if WINVER >= 0x0501
if (IS_WINDOWSXP_OR_LATER())
{

KEVENT waitEvent;

KeInitializeEvent( &waitEvent,
NotificationEvent,
FALSE );

IoCopyCurrentIrpStackLocationToNext( Irp );

IoSetCompletionRoutine( Irp,
SfFsControlCompletion,
&waitEvent, //context parameter
TRUE,
TRUE,
TRUE );

status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

if (STATUS_PENDING == status)
{

status = KeWaitForSingleObject( &waitEvent,
Executive,
KernelMode,
FALSE,
NULL );
ASSERT( STATUS_SUCCESS == status );
}

ASSERT(KeReadStateEvent(&waitEvent) ||
!NT_SUCCESS(Irp->IoStatus.Status));

status = SfFsControlLoadFileSystemComplete( DeviceObject, Irp );

} else {
#endif

completionContext =(PFSCTRL_COMPLETION_CONTEXT) ExAllocatePoolWithTag( NonPagedPool,
sizeof( FSCTRL_COMPLETION_CONTEXT ),
SFLT_POOL_TAG );

if (completionContext == NULL) {

IoSkipCurrentIrpStackLocation( Irp );
status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

} else {

ExInitializeWorkItem( &completionContext->WorkItem, (PWORKER_THREAD_ROUTINE)SfFsControlMountVolumeCompleteWorker, completionContext );

completionContext->DeviceObject = DeviceObject;
completionContext->Irp = Irp;
completionContext->NewDeviceObject = NULL;

IoCopyCurrentIrpStackLocationToNext( Irp );

IoSetCompletionRoutine(
Irp,
SfFsControlCompletion,
completionContext,
TRUE,
TRUE,
TRUE );

IoDetachDevice( devExt->AttachedToDeviceObject );
status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}
#if WINVER >= 0x0501
}
#endif

return status;
}



VOID SfFsControlLoadFileSystemCompleteWorker( IN PFSCTRL_COMPLETION_CONTEXT Context )
{
ASSERT( NULL != Context );
SfFsControlLoadFileSystemComplete( Context->DeviceObject, Context->Irp );
ExFreePoolWithTag( Context, SFLT_POOL_TAG );
}


NTSTATUS SfFsControlLoadFileSystemComplete ( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
PSFILTER_DEVICE_EXTENSION devExt;
NTSTATUS status;

PAGED_CODE();

devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFsControlLoadFileSystem: Detaching from recognizer %p \"%wZ\", status=%08x\n",
DeviceObject,
&devExt->DeviceName,
Irp->IoStatus.Status) );
if (!NT_SUCCESS( Irp->IoStatus.Status ) && (Irp->IoStatus.Status != STATUS_IMAGE_ALREADY_LOADED))
{
SfAttachDeviceToDeviceStack( DeviceObject, devExt->AttachedToDeviceObject, &devExt->AttachedToDeviceObject );
ASSERT(devExt->AttachedToDeviceObject != NULL);
}
else
{
SfCleanupMountedDevice( DeviceObject );
IoDeleteDevice( DeviceObject );
}

status = Irp->IoStatus.Status;
IoCompleteRequest( Irp, IO_NO_INCREMENT );
return status;
}



/*****************************************************************************************************************************************************
���µĴ�����FastIO �Ĵ�����:

�������ǵ�������Ҫ�󶨵��ļ�ϵͳ�������ϱߣ��ļ�ϵͳ���˴���������IRP֮�⣬��Ҫ������ν��FastIo
FastIo��Cache Manager������������һ��û��irp�����󡣳���������Dispatch Functions֮�⣬�㻹��ΪDriverObject ��д��һ��Fast Io Functions��
�ļ�ϵͳ����ͨ�ַ����̺�fastio���̶���ʱ�п��ܱ����á����õĹ���������ȻӦ��ͬʱ���������׽ӿڡ�

�ڽ��л���IRPΪ�����Ľӿڵ���ǰ, IO�������᳢��ʹ��FAST IO �ӿ������ٸ���IO������


FastIO �����������������ٵġ�ͬ���ġ����ҡ�on cached files����IO������������FastIO����ʱ�����账���������ֱ�����û�buffer��ϵͳ�����н��д���ģ�
������ͨ���ļ�ϵͳ�ʹ洢������ջ(storage driver stack)��
��ʵ�ϴ洢����������ʹ��FastIO���ơ�����Ҫ����������Ѿ�������ϵͳ���棬�����FastIO���ƵĶ�д�������̾Ϳ�����ɡ�����ϵͳ�����һ��ȱҳ�жϣ�
��ᵼ��ϵͳ������Ӧ��IRP��������û�����Ķ�д������ͨ���������������ָ����������ݲ���ϵͳ����������ʱ��FastIO�����᷵��FALSE��
����һֱ�ȵ�ȱҳ�ж���Ӧ��������������ݶ����ص�ϵͳ�����С�(���FastIO����������FALSE����ô�����߱����Լ�������ӦIrp���������Ĵ���)


�ļ�ϵͳ������������Ŀ����豸����CDO������һ����Ҫ����IO�������������豸����(FiDO)����Ҫ�����в���ʶ���Irp�������ݵ��Լ��²����������
������һ����Ҫע�⣬���ڵ����ϵĹ������豸��������ṩ FastIoDetachDevice ������

******************************************************************************************************************************************************/
BOOLEAN SfFastIoCheckIfPossible( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, IN ULONG LockKey, IN BOOLEAN CheckForReadOperation, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoCheckIfPossible ))
{
return (fastIoDispatch->FastIoCheckIfPossible)(
FileObject,
FileOffset,
Length,
Wait,
LockKey,
CheckForReadOperation,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoRead( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, IN ULONG LockKey, OUT PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension) {

ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoRead ))
{
return (fastIoDispatch->FastIoRead)(
FileObject,
FileOffset,
Length,
Wait,
LockKey,
Buffer,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoWrite( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, IN ULONG LockKey, IN PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWrite ))
{
return (fastIoDispatch->FastIoWrite)(
FileObject,
FileOffset,
Length,
Wait,
LockKey,
Buffer,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoQueryBasicInfo( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_BASIC_INFORMATION Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryBasicInfo ))
{
return (fastIoDispatch->FastIoQueryBasicInfo)(
FileObject,
Wait,
Buffer,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoQueryStandardInfo( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_STANDARD_INFORMATION Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryStandardInfo ))
{
return (fastIoDispatch->FastIoQueryStandardInfo)(
FileObject,
Wait,
Buffer,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoLock( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, BOOLEAN FailImmediately, BOOLEAN ExclusiveLock, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoLock ))
{
return (fastIoDispatch->FastIoLock)(
FileObject,
FileOffset,
Length,
ProcessId,
Key,
FailImmediately,
ExclusiveLock,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoUnlockSingle( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockSingle ))
{
return (fastIoDispatch->FastIoUnlockSingle)(
FileObject,
FileOffset,
Length,
ProcessId,
Key,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}
BOOLEAN SfFastIoUnlockAll( IN PFILE_OBJECT FileObject, PEPROCESS ProcessId, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
if (nextDeviceObject)
{
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAll ))
{
return (fastIoDispatch->FastIoUnlockAll)(
FileObject,
ProcessId,
IoStatus,
nextDeviceObject );
}
}
}
return FALSE;
}
BOOLEAN SfFastIoUnlockAllByKey( IN PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAllByKey ))
{
return (fastIoDispatch->FastIoUnlockAllByKey)(
FileObject,
ProcessId,
Key,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}


BOOLEAN SfFastIoDeviceControl( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;
PAGED_CODE();

if (DeviceObject!=gSFilterControlDeviceObject)//������ǿ����豸��ô�ͼ򵥵ķ���false��ϵͳ�����·���irp
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryNetworkOpenInfo ))
{
	return (fastIoDispatch->FastIoDeviceControl)(
FileObject,
Wait,
InputBuffer,
InputBufferLength,
OutputBuffer,
OutputBufferLength,
IoControlCode,
IoStatus,
DeviceObject);
}
return FALSE;
}
KdPrint(("fastio:device io control!\n"));
return FALSE;

//PAGED_CODE();
UNREFERENCED_PARAMETER(DeviceObject);
UNREFERENCED_PARAMETER(Wait);
UNREFERENCED_PARAMETER(FileObject);
NTSTATUS status=STATUS_SUCCESS;


switch(IoControlCode)
{
  	case(IOCTL_DEVICE_READ):
		break;
	case(IOCTL_DEVICE_WRITE):

		break;
	case(IOCTL_VERIFY_OPERATION_INFORMATION)://
	    KdPrint(("�յ�������Ӧ�õ�ȡ������Ϣ֪ͨ��\n"));
		ASSERT(InputBufferLength==sizeof(POperationRecord));
        ASSERT(OutputBufferLength==sizeof(POperationRecord));
		status=FastIO_GetOperationInformation(IOCTL_VERIFY_OPERATION_INFORMATION,InputBuffer,OutputBuffer,InputBufferLength,OutputBufferLength);
		break;
	case(IOCTL_SEND_VERIFY_RESULT):
		KdPrint(("�յ�������Ӧ�ó���Ĳ�����֤���֪ͨ!\n"));		
		status=FastIO_SetOperationResult(IOCTL_SEND_VERIFY_RESULT,InputBuffer,OutputBuffer,InputBufferLength,OutputBufferLength);
		break;
	default:
		KdPrint(("UnKown Device Control Code:%d!",IoControlCode));
		break;
}
/*if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoDeviceControl ))
{
return (fastIoDispatch->FastIoDeviceControl)(
FileObject,
Wait,
InputBuffer,
InputBufferLength,
OutputBuffer,
OutputBufferLength,
IoControlCode,
IoStatus,
nextDeviceObject );
}
}*/
IoStatus->Status=status;
IoStatus->Information=0;

return TRUE;
}


VOID SfFastIoDetachDevice( IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice )
{
PSFILTER_DEVICE_EXTENSION devExt;

PAGED_CODE();

ASSERT(IS_MY_DEVICE_OBJECT( SourceDevice ));
devExt =(PSFILTER_DEVICE_EXTENSION)SourceDevice->DeviceExtension;
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfFastIoDetachDevice: Detaching from volume %p \"%wZ\"\n",
TargetDevice, &devExt->DeviceName) );
SfCleanupMountedDevice( SourceDevice );
IoDetachDevice( TargetDevice );
IoDeleteDevice( SourceDevice );
}


BOOLEAN SfFastIoQueryNetworkOpenInfo( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_NETWORK_OPEN_INFORMATION Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryNetworkOpenInfo ))
{
return (fastIoDispatch->FastIoQueryNetworkOpenInfo)(
FileObject,
Wait,
Buffer,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}


BOOLEAN SfFastIoMdlRead( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlRead ))
{
return (fastIoDispatch->MdlRead)(
FileObject,
FileOffset,
Length,
LockKey,
MdlChain,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}


BOOLEAN SfFastIoMdlReadComplete( IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadComplete ))
{
return (fastIoDispatch->MdlReadComplete)( FileObject, MdlChain, nextDeviceObject );
}
}
return FALSE;
}


BOOLEAN SfFastIoPrepareMdlWrite( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, PrepareMdlWrite ))
{
return (fastIoDispatch->PrepareMdlWrite)(
FileObject,
FileOffset,
Length,
LockKey,
MdlChain,
IoStatus,
nextDeviceObject );
}
}
return FALSE;
}


BOOLEAN SfFastIoMdlWriteComplete( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteComplete ))
{
return (fastIoDispatch->MdlWriteComplete)(
FileObject,
FileOffset,
MdlChain,
nextDeviceObject );
}
}
return FALSE;
}


BOOLEAN SfFastIoReadCompressed( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PVOID Buffer, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo, IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoReadCompressed ))
{
return (fastIoDispatch->FastIoReadCompressed)(
FileObject,
FileOffset,
Length,
LockKey,
Buffer,
MdlChain,
IoStatus,
CompressedDataInfo,
CompressedDataInfoLength,
nextDeviceObject );
}
}
return FALSE;
}



BOOLEAN SfFastIoWriteCompressed( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, IN PVOID Buffer, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo, IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWriteCompressed ))
{
return (fastIoDispatch->FastIoWriteCompressed)(
FileObject,
FileOffset,
Length,
LockKey,
Buffer,
MdlChain,
IoStatus,
CompressedDataInfo,
CompressedDataInfoLength,
nextDeviceObject );
}
}
return FALSE;
}



BOOLEAN SfFastIoMdlReadCompleteCompressed( IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadCompleteCompressed ))
{
return (fastIoDispatch->MdlReadCompleteCompressed)(
FileObject,
MdlChain,
nextDeviceObject );
}
}
return FALSE;
}



BOOLEAN SfFastIoMdlWriteCompleteCompressed( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;

if (DeviceObject->DeviceExtension) {

ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);

fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteCompleteCompressed )) {

return (fastIoDispatch->MdlWriteCompleteCompressed)(
FileObject,
FileOffset,
MdlChain,
nextDeviceObject );
}
}
return FALSE;
}



BOOLEAN SfFastIoQueryOpen( IN PIRP Irp, OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation, IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT nextDeviceObject;
PFAST_IO_DISPATCH fastIoDispatch;
BOOLEAN result;

PAGED_CODE();

if (DeviceObject->DeviceExtension)
{
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
nextDeviceObject = ((PSFILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
ASSERT(nextDeviceObject);
fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;
if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryOpen ))
{
PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
irpSp->DeviceObject = nextDeviceObject;
result = (fastIoDispatch->FastIoQueryOpen)(
Irp,
NetworkInformation,
nextDeviceObject );

irpSp->DeviceObject = DeviceObject;
return result;
}
}
return FALSE;
}



#if WINVER >= 0x0501
//========================== FSFilter �ص����� ===========================
NTSTATUS SfPreFsFilterPassThrough( IN PFS_FILTER_CALLBACK_DATA Data, OUT PVOID *CompletionContext )
{
UNREFERENCED_PARAMETER( Data );
UNREFERENCED_PARAMETER( CompletionContext );

ASSERT( IS_MY_DEVICE_OBJECT( Data->DeviceObject ) );

return STATUS_SUCCESS;
}


VOID SfPostFsFilterPassThrough ( IN PFS_FILTER_CALLBACK_DATA Data, IN NTSTATUS OperationStatus, IN PVOID CompletionContext )
{
UNREFERENCED_PARAMETER( Data );
UNREFERENCED_PARAMETER( OperationStatus );
UNREFERENCED_PARAMETER( CompletionContext );

ASSERT( IS_MY_DEVICE_OBJECT( Data->DeviceObject ) );
}
#endif



NTSTATUS SfAttachDeviceToDeviceStack( IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT *AttachedToDeviceObject )
{
/*
����˵��:

SourceDevice: ���ǵ����ߴ������豸���󡣱��������ʹ��IoCreateDevice�����������豸����
TargetDevice: ��ָ����������������豸����
AttachedToDeviceObject: �������洢IoAttachDeviceToDeviceStack�����ķ���ֵ��

*/

PAGED_CODE();

#if WINVER >= 0x0501

if (IS_WINDOWSXP_OR_LATER())
{
ASSERT( NULL != gSfDynamicFunctions.AttachDeviceToDeviceStackSafe );
return (gSfDynamicFunctions.AttachDeviceToDeviceStackSafe)( SourceDevice, TargetDevice, AttachedToDeviceObject );
}
else
{
ASSERT( NULL == gSfDynamicFunctions.AttachDeviceToDeviceStackSafe );
#endif

*AttachedToDeviceObject = TargetDevice;
*AttachedToDeviceObject = IoAttachDeviceToDeviceStack( SourceDevice, TargetDevice );

if (*AttachedToDeviceObject == NULL)
{
return STATUS_NO_SUCH_DEVICE;
}

return STATUS_SUCCESS;

#if WINVER >= 0x0501
}
#endif
}



NTSTATUS SfAttachToFileSystemDevice( IN PDEVICE_OBJECT DeviceObject, IN PUNICODE_STRING DeviceName )
{
/*
SfAttachToFileSystemDevice����������ɶ��ļ�ϵͳ�����豸�İ󶨡�

����˵��:
DeviceObject: ��ָ���ļ�ϵͳ�Ŀ����豸����(CDO)���� �������������File System CDO

*/

PDEVICE_OBJECT newDeviceObject; //���豸����
PSFILTER_DEVICE_EXTENSION devExt; //�ļ�ϵͳ��������������豸��չ
NTSTATUS status; //״̬��
UNICODE_STRING fsrecName;
UNICODE_STRING fsName; //�ļ�ϵͳ��
WCHAR tempNameBuffer[MAX_DEVNAME_LENGTH];//��ʱ������(������ִ�)

PAGED_CODE();

if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)) //���Ը����豸�ǲ�������Ҫ���ĵ��豸
{
return STATUS_SUCCESS;
}


/*
* Windows�ı�׼�ļ�ϵͳʶ���������϶��������� \FileSystem\Fs_Rec ���ɵġ�����ֱ���ж����������ֿ��Խ��һ�������⡣
* Ҳ��һ���ǵ�Ҫ���ļ�ϵͳʶ��������������\FileSystem\Fs_Rec���档ֻ��˵��һ�����������\FileSystem\Fs_Rec���档
*/
RtlInitEmptyUnicodeString( &fsName, tempNameBuffer, sizeof(tempNameBuffer) );

if (!FlagOn(SfDebug,SFDEBUG_ATTACH_TO_FSRECOGNIZER))
{
RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );
SfGetObjectName( DeviceObject->DriverObject, &fsName );

if (RtlCompareUnicodeString( &fsName, &fsrecName, TRUE ) == 0)
{
//ͨ���������������ֱ������Windows�ı�׼�ļ�ϵͳʶ����������ǣ���ô���سɹ���Ҳ���Ƿ������ˡ�
//������д�����ļ�ϵͳʶ����û�б��жϵ����ļ�ϵͳ��������Ĺ����������ж�Ӧ�Ĵ���
return STATUS_SUCCESS;
}
}



//�����ǹ��ĵ��ļ�ϵͳ���Ҳ���΢����ļ�ϵͳʶ�������豸������һ���豸������豸����
status = IoCreateDevice( gSFilterDriverObject,
sizeof( SFILTER_DEVICE_EXTENSION ),


NULL,
DeviceObject->DeviceType,
0,
FALSE,
&newDeviceObject );

if (!NT_SUCCESS( status ))
{
return status;
}

if ( FlagOn( DeviceObject->Flags, DO_BUFFERED_IO ))
{
SetFlag( newDeviceObject->Flags, DO_BUFFERED_IO );
}

if ( FlagOn( DeviceObject->Flags, DO_DIRECT_IO ))
{
SetFlag( newDeviceObject->Flags, DO_DIRECT_IO );
}

if ( FlagOn( DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN ) )
{
SetFlag( newDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN );
}

devExt =(PSFILTER_DEVICE_EXTENSION) newDeviceObject->DeviceExtension;


/*
����SfAttachDeviceToDeviceStack�����������豸����󶨵�File System CDO���豸ջ���档���������ǵ�newDeviceObject�Ϳ��Խ��յ����͵�
File System CDO��IRP_MJ_FILE_SYSTEM_CONTROL�����ˡ� �Ժ󣬳���Ϳ���ȥ�󶨾��ˡ�
ʹ��SfAttachDeviceToDeviceStack���������а󶨡�����1�󶨵�����2���󶨺������ص��豸�洢�ڲ���3�С�
*/
status = SfAttachDeviceToDeviceStack( newDeviceObject, DeviceObject, &devExt->AttachedToDeviceObject );
if (!NT_SUCCESS( status ))
{
goto ErrorCleanupDevice;
}

RtlInitEmptyUnicodeString( &devExt->DeviceName, devExt->DeviceNameBuffer, sizeof(devExt->DeviceNameBuffer) );
RtlCopyUnicodeString( &devExt->DeviceName, DeviceName ); //Save Name
ClearFlag( newDeviceObject->Flags, DO_DEVICE_INITIALIZING );
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfAttachToFileSystemDevice: Attaching to file system %p \"%wZ\" (%s)\n",
DeviceObject,
&devExt->DeviceName,
GET_DEVICE_TYPE_NAME(newDeviceObject->DeviceType)) );

#if WINVER >= 0x0501

if (IS_WINDOWSXP_OR_LATER())
{
ASSERT( NULL != gSfDynamicFunctions.EnumerateDeviceObjectList &&
NULL != gSfDynamicFunctions.GetDiskDeviceObject &&
NULL != gSfDynamicFunctions.GetDeviceAttachmentBaseRef &&
NULL != gSfDynamicFunctions.GetLowerDeviceObject );


/*
����SpyEnumerateFileSystemVolumesö�ٸ������ļ�ϵͳ�µĵ�ǰ���ڵ����й����˵��豸�����Ұ����ǡ�
��������Ŀ�ģ�����Ϊ��������������ʱ�����أ����Ǽ��ع���������ʱ���ļ�ϵͳ�Ѿ������˾��豸��
���ǣ��ù��������Ӻ��أ���ʱ���ܰ��Ѿ����ڻ�ոչ����������ļ�ϵͳ���豸��
*/
status = SfEnumerateFileSystemVolumes( DeviceObject, &fsName );
if (!NT_SUCCESS( status ))
{
IoDetachDevice( devExt->AttachedToDeviceObject );
goto ErrorCleanupDevice;
}
}

#endif

return STATUS_SUCCESS;

ErrorCleanupDevice:
SfCleanupMountedDevice( newDeviceObject );
IoDeleteDevice( newDeviceObject );

return status;
}



VOID SfDetachFromFileSystemDevice( IN PDEVICE_OBJECT DeviceObject )
{
PDEVICE_OBJECT ourAttachedDevice;
PSFILTER_DEVICE_EXTENSION devExt;

PAGED_CODE();

ourAttachedDevice = DeviceObject->AttachedDevice;
while (NULL != ourAttachedDevice)
{
if (IS_MY_DEVICE_OBJECT( ourAttachedDevice ))
{
devExt =(PSFILTER_DEVICE_EXTENSION)ourAttachedDevice->DeviceExtension;
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfDetachFromFileSystemDevice: Detaching from file system %p \"%wZ\" (%s)\n",
devExt->AttachedToDeviceObject,
&devExt->DeviceName,
GET_DEVICE_TYPE_NAME(ourAttachedDevice->DeviceType)) );

SfCleanupMountedDevice( ourAttachedDevice );
IoDetachDevice( DeviceObject );
IoDeleteDevice( ourAttachedDevice );

return;
}

DeviceObject = ourAttachedDevice;
ourAttachedDevice = ourAttachedDevice->AttachedDevice;
}
}



#if WINVER >= 0x0501
NTSTATUS SfEnumerateFileSystemVolumes( IN PDEVICE_OBJECT FSDeviceObject, IN PUNICODE_STRING Name )
{
/*
����˵��:

FSDeviceObject: ��ָ���ļ�ϵͳ�Ŀ����豸����(CDO)���� �������������File System CDO
Name: �����ļ�ϵͳ������

*/

PDEVICE_OBJECT newDeviceObject;
PSFILTER_DEVICE_EXTENSION newDevExt;
PDEVICE_OBJECT *devList;
PDEVICE_OBJECT storageStackDeviceObject;
NTSTATUS status;
ULONG numDevices;
ULONG i;
BOOLEAN isShadowCopyVolume;

PAGED_CODE();

/*
* IoEnumerateDeviceObjectList����ö����������µ��豸�����б����������������2�Ρ�
* ��1�ε��ã� ��ȡ�豸�б��е��豸�����������
* ��2�ε���: ���ݵ�1�εĽ��numDevicesֵ�������豸����Ĵ�ſռ䣬�Ӷ��õ��豸��devList��
*/
status = (gSfDynamicFunctions.EnumerateDeviceObjectList)(
FSDeviceObject->DriverObject,
NULL,
0,
&numDevices);

if (!NT_SUCCESS( status ))
{
ASSERT(STATUS_BUFFER_TOO_SMALL == status);
numDevices += 8; //Ϊ��֪���豸�����ڴ�ռ���д洢����������8�ֽڡ�


devList =(PDEVICE_OBJECT *)ExAllocatePoolWithTag( NonPagedPool, (numDevices * sizeof(PDEVICE_OBJECT)), SFLT_POOL_TAG );
if (NULL == devList)
{
return STATUS_INSUFFICIENT_RESOURCES;
}

ASSERT( NULL != gSfDynamicFunctions.EnumerateDeviceObjectList );
status = (gSfDynamicFunctions.EnumerateDeviceObjectList)(
FSDeviceObject->DriverObject,
devList,
(numDevices * sizeof(PDEVICE_OBJECT)),
&numDevices);

if (!NT_SUCCESS( status ))
{
ExFreePool( devList );
return status;
}

//���α��������豸����
for (i=0; i < numDevices; i++)
{
storageStackDeviceObject = NULL;
_try {

//����豸�������ļ�ϵͳCDO�������ǲ��������͵ģ��������Ѿ��󶨵�
if ((devList[i] == FSDeviceObject) || (devList[i]->DeviceType != FSDeviceObject->DeviceType) || SfIsAttachedToDevice( devList[i], NULL ))
{
_leave;
}

SfGetBaseDeviceObjectName( devList[i], Name );
if (Name->Length > 0)
{
_leave;
}


/*
����IoGetDiskDeviceObject��������ȡһ�����ļ�ϵͳ�豸�����йصĴ����豸����ֻ���Ѿ�ӵ��һ�������豸������ļ�ϵͳ�豸����
*/
ASSERT( NULL != gSfDynamicFunctions.GetDiskDeviceObject );
status = (gSfDynamicFunctions.GetDiskDeviceObject)( devList[i], &storageStackDeviceObject );

if (!NT_SUCCESS( status ))
{
_leave;
}

status = SfIsShadowCopyVolume ( storageStackDeviceObject, &isShadowCopyVolume );

if (NT_SUCCESS(status) &&
isShadowCopyVolume &&
!FlagOn(SfDebug,SFDEBUG_ATTACH_TO_SHADOW_COPIES))
{

UNICODE_STRING shadowDeviceName;
WCHAR shadowNameBuffer[MAX_DEVNAME_LENGTH];

RtlInitEmptyUnicodeString( &shadowDeviceName, shadowNameBuffer, sizeof(shadowNameBuffer) );
SfGetObjectName( storageStackDeviceObject, &shadowDeviceName );

SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfEnumerateFileSystemVolumes Not attaching to Volume %p \"%wZ\", shadow copy volume\n",
storageStackDeviceObject,
&shadowDeviceName) );

_leave;
}


// ��һ�������豸���󣬴����µ��豸����׼���󶨡�
status = IoCreateDevice( gSFilterDriverObject,
sizeof( SFILTER_DEVICE_EXTENSION ),
NULL,
devList[i]->DeviceType,
0,
FALSE,
&newDeviceObject );
if (!NT_SUCCESS( status ))
{
_leave;
}

newDevExt =(PSFILTER_DEVICE_EXTENSION)newDeviceObject->DeviceExtension;
newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
RtlInitEmptyUnicodeString( &newDevExt->DeviceName, newDevExt->DeviceNameBuffer,sizeof(newDevExt->DeviceNameBuffer) );
SfGetObjectName( storageStackDeviceObject, &newDevExt->DeviceName );


/*
�ڰ�����ʱ���ٲ����£����豸�Ƿ񱻰󶨹����������һ���������û���󶨣���ִ������İ󶨹��̣�����ֱ�ӷ��ء�
*/
ExAcquireFastMutex( &gSfilterAttachLock );
if (!SfIsAttachedToDevice( devList[i], NULL ))
{
status = SfAttachToMountedDevice( devList[i], newDeviceObject );
if (!NT_SUCCESS( status ))
{
SfCleanupMountedDevice( newDeviceObject );
IoDeleteDevice( newDeviceObject );
}
}
else
{
SfCleanupMountedDevice( newDeviceObject );
IoDeleteDevice( newDeviceObject );
}

ExReleaseFastMutex( &gSfilterAttachLock );
}

_finally {

/*
�����豸����ļ���������������ɺ���IoGetDiskDeviceObject���ӵġ��ɹ��󶨺󣬾ͼ��ٸ��豸����ļ�����
һ���ɹ��󶨵�devList[i]��I/O��������ȷ���豸ջ���²��豸����һֱ���ڣ�һֱ������ļ�ϵͳջ��ж����
*/
if (storageStackDeviceObject != NULL)
{
ObDereferenceObject( storageStackDeviceObject );
}


//�����豸����ļ���������������ɺ���IoEnumerateDeviceObjectList���ӵġ�
ObDereferenceObject( devList[i] );
}
}

status = STATUS_SUCCESS;
ExFreePool( devList );
}

return status;
}
#endif


#if WINVER >= 0x0501
NTSTATUS SfEnumerateFileSystemVolumes( IN PDEVICE_OBJECT FSDeviceObject, IN PUNICODE_STRING Name )
{
/*
����˵��:

FSDeviceObject: ��ָ���ļ�ϵͳ�Ŀ����豸����(CDO)���� �������������File System CDO
Name: �����ļ�ϵͳ������



*/

PDEVICE_OBJECT newDeviceObject;
PSFILTER_DEVICE_EXTENSION newDevExt;
PDEVICE_OBJECT *devList;
PDEVICE_OBJECT storageStackDeviceObject;
NTSTATUS status;
ULONG numDevices;
ULONG i;
BOOLEAN isShadowCopyVolume;

PAGED_CODE();

/*
* IoEnumerateDeviceObjectList����ö����������µ��豸�����б����������������2�Ρ�
* ��1�ε��ã� ��ȡ�豸�б��е��豸�����������
* ��2�ε���: ���ݵ�1�εĽ��numDevicesֵ�������豸����Ĵ�ſռ䣬�Ӷ��õ��豸��devList��
*/
status = (gSfDynamicFunctions.EnumerateDeviceObjectList)(
FSDeviceObject->DriverObject,
NULL,
0,
&numDevices);

if (!NT_SUCCESS( status ))
{
ASSERT(STATUS_BUFFER_TOO_SMALL == status);
numDevices += 8; //Ϊ��֪���豸�����ڴ�ռ���д洢����������8�ֽڡ�


devList =(PDEVICE_OBJECT *)ExAllocatePoolWithTag( NonPagedPool, (numDevices * sizeof(PDEVICE_OBJECT)), SFLT_POOL_TAG );
if (NULL == devList)
{
return STATUS_INSUFFICIENT_RESOURCES;
}

ASSERT( NULL != gSfDynamicFunctions.EnumerateDeviceObjectList );
status = (gSfDynamicFunctions.EnumerateDeviceObjectList)(
FSDeviceObject->DriverObject,
devList,
(numDevices * sizeof(PDEVICE_OBJECT)),
&numDevices);

if (!NT_SUCCESS( status ))
{
ExFreePool( devList );
return status;
}

//���α��������豸����
for (i=0; i < numDevices; i++)
{
storageStackDeviceObject = NULL;
_try {

//����豸�������ļ�ϵͳCDO�������ǲ��������͵ģ��������Ѿ��󶨵�
if ((devList[i] == FSDeviceObject) || (devList[i]->DeviceType != FSDeviceObject->DeviceType) || SfIsAttachedToDevice( devList[i], NULL ))
{
_leave;
}

SfGetBaseDeviceObjectName( devList[i], Name );
if (Name->Length > 0)
{
_leave;
}


/*
����IoGetDiskDeviceObject��������ȡһ�����ļ�ϵͳ�豸�����йصĴ����豸����ֻ���Ѿ�ӵ��һ�������豸������ļ�ϵͳ�豸����
*/
ASSERT( NULL != gSfDynamicFunctions.GetDiskDeviceObject );
status = (gSfDynamicFunctions.GetDiskDeviceObject)( devList[i], &storageStackDeviceObject );

if (!NT_SUCCESS( status ))
{
_leave;
}

status = SfIsShadowCopyVolume ( storageStackDeviceObject, &isShadowCopyVolume );

if (NT_SUCCESS(status) &&
isShadowCopyVolume &&
!FlagOn(SfDebug,SFDEBUG_ATTACH_TO_SHADOW_COPIES))
{

UNICODE_STRING shadowDeviceName;
WCHAR shadowNameBuffer[MAX_DEVNAME_LENGTH];

RtlInitEmptyUnicodeString( &shadowDeviceName, shadowNameBuffer, sizeof(shadowNameBuffer) );
SfGetObjectName( storageStackDeviceObject, &shadowDeviceName );

SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfEnumerateFileSystemVolumes Not attaching to Volume %p \"%wZ\", shadow copy volume\n",
storageStackDeviceObject,
&shadowDeviceName) );

_leave;
}


// ��һ�������豸���󣬴����µ��豸����׼���󶨡�
status = IoCreateDevice( gSFilterDriverObject,
sizeof( SFILTER_DEVICE_EXTENSION ),
NULL,
devList[i]->DeviceType,
0,
FALSE,
&newDeviceObject );
if (!NT_SUCCESS( status ))
{
_leave;
}

newDevExt =(PSFILTER_DEVICE_EXTENSION)newDeviceObject->DeviceExtension;
newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
RtlInitEmptyUnicodeString( &newDevExt->DeviceName, newDevExt->DeviceNameBuffer,sizeof(newDevExt->DeviceNameBuffer) );
SfGetObjectName( storageStackDeviceObject, &newDevExt->DeviceName );


/*
�ڰ�����ʱ���ٲ����£����豸�Ƿ񱻰󶨹����������һ���������û���󶨣���ִ������İ󶨹��̣�����ֱ�ӷ��ء�
*/
ExAcquireFastMutex( &gSfilterAttachLock );
if (!SfIsAttachedToDevice( devList[i], NULL ))
{
status = SfAttachToMountedDevice( devList[i], newDeviceObject );
if (!NT_SUCCESS( status ))
{
SfCleanupMountedDevice( newDeviceObject );
IoDeleteDevice( newDeviceObject );
}
}
else
{
SfCleanupMountedDevice( newDeviceObject );
IoDeleteDevice( newDeviceObject );
}

ExReleaseFastMutex( &gSfilterAttachLock );
}

_finally {

/*
�����豸����ļ���������������ɺ���IoGetDiskDeviceObject���ӵġ��ɹ��󶨺󣬾ͼ��ٸ��豸����ļ�����
һ���ɹ��󶨵�devList[i]��I/O��������ȷ���豸ջ���²��豸����һֱ���ڣ�һֱ������ļ�ϵͳջ��ж����
*/
if (storageStackDeviceObject != NULL)
{
ObDereferenceObject( storageStackDeviceObject );
}


//�����豸����ļ���������������ɺ���IoEnumerateDeviceObjectList���ӵġ�
ObDereferenceObject( devList[i] );
}
}

status = STATUS_SUCCESS;
ExFreePool( devList );
}

return status;
}
#endif


NTSTATUS SfAttachToMountedDevice( IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_OBJECT SFilterDeviceObject )
{
/*
SfAttachToMountedDevice�����Ĺ���: ��ɰ�һ���ļ�ϵͳ���豸�Ĳ�����

����˵��:
SFilterDeviceObject: ��������ʹ��IoCreateDevice�������������豸����

*/

PSFILTER_DEVICE_EXTENSION newDevExt =(PSFILTER_DEVICE_EXTENSION) SFilterDeviceObject->DeviceExtension;
NTSTATUS status;
ULONG i;

PAGED_CODE();
ASSERT(IS_MY_DEVICE_OBJECT( SFilterDeviceObject ));
#if WINVER >= 0x0501
ASSERT(!SfIsAttachedToDevice ( DeviceObject, NULL ));
#endif

if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO ))
{
SetFlag( SFilterDeviceObject->Flags, DO_BUFFERED_IO );
}

if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO ))
{
SetFlag( SFilterDeviceObject->Flags, DO_DIRECT_IO );
}

for (i=0; i < 8; i++)
{
LARGE_INTEGER interval;

//����SfAttachDeviceToDeviceStack�������� ��İ�
status = SfAttachDeviceToDeviceStack( SFilterDeviceObject, DeviceObject, &newDevExt->AttachedToDeviceObject );
if (NT_SUCCESS(status))
{
ClearFlag( SFilterDeviceObject->Flags, DO_DEVICE_INITIALIZING );
SF_LOG_PRINT( SFDEBUG_DISPLAY_ATTACHMENT_NAMES,
("SFilter!SfAttachToMountedDevice: Attaching to volume %p \"%wZ\"\n",
newDevExt->AttachedToDeviceObject,
&newDevExt->DeviceName) );
return STATUS_SUCCESS;
}

interval.QuadPart = (500 * DELAY_ONE_MILLISECOND); //delay 1/2 second
KeDelayExecutionThread( KernelMode, FALSE, &interval );
}

return status;
}



VOID SfCleanupMountedDevice( IN PDEVICE_OBJECT DeviceObject )
{

UNREFERENCED_PARAMETER( DeviceObject );
ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
}



VOID SfGetObjectName( IN PVOID Object, IN OUT PUNICODE_STRING Name )
{
NTSTATUS status;
CHAR nibuf[512]; //���շ��صĶ�����
POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf; //nameInfo��ŵ���ָ��UNICODE_STRING�ṹ������ָ��
ULONG retLength; //����"ʵ�ʷ��صĶ���������"

status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength); //���ض�����
Name->Length = 0;
if (NT_SUCCESS( status ))
{
RtlCopyUnicodeString( Name, &nameInfo->Name );
}
}



#if WINVER >= 0x0501

VOID SfGetBaseDeviceObjectName( IN PDEVICE_OBJECT DeviceObject, IN OUT PUNICODE_STRING Name )
{
ASSERT( NULL != gSfDynamicFunctions.GetDeviceAttachmentBaseRef );
DeviceObject = (gSfDynamicFunctions.GetDeviceAttachmentBaseRef)( DeviceObject );
SfGetObjectName( DeviceObject, Name );
ObDereferenceObject( DeviceObject );
}
#endif



PUNICODE_STRING SfGetFileName( IN PFILE_OBJECT FileObject, IN NTSTATUS CreateStatus, IN OUT PGET_NAME_CONTROL NameControl )
{
POBJECT_NAME_INFORMATION nameInfo;
NTSTATUS status;
ULONG size;
ULONG bufferSize;

NameControl->allocatedBuffer = NULL;
nameInfo = (POBJECT_NAME_INFORMATION)NameControl->smallBuffer;
bufferSize = sizeof(NameControl->smallBuffer);
status = ObQueryNameString( (NT_SUCCESS( CreateStatus ) ? (PVOID)FileObject : (PVOID)FileObject->DeviceObject), nameInfo, bufferSize, &size );
if (status == STATUS_BUFFER_OVERFLOW)
{
bufferSize = size + sizeof(WCHAR);
NameControl->allocatedBuffer =(PCHAR) ExAllocatePoolWithTag( NonPagedPool, bufferSize, SFLT_POOL_TAG );
if (NULL == NameControl->allocatedBuffer)
{
RtlInitEmptyUnicodeString(
(PUNICODE_STRING)&NameControl->smallBuffer,
(PWCHAR)(NameControl->smallBuffer + sizeof(UNICODE_STRING)),
(USHORT)(sizeof(NameControl->smallBuffer) - sizeof(UNICODE_STRING)) );

return (PUNICODE_STRING)&NameControl->smallBuffer;
}

nameInfo = (POBJECT_NAME_INFORMATION)NameControl->allocatedBuffer;
status = ObQueryNameString(
FileObject,
nameInfo,
bufferSize,
&size );
}

if (NT_SUCCESS( status ) && !NT_SUCCESS( CreateStatus ))
{
ULONG newSize;
PCHAR newBuffer;
POBJECT_NAME_INFORMATION newNameInfo;
newSize = size + FileObject->FileName.Length;
if (NULL != FileObject->RelatedFileObject)
{
newSize += FileObject->RelatedFileObject->FileName.Length + sizeof(WCHAR);
}

if (newSize > bufferSize)
{
newBuffer =(PCHAR) ExAllocatePoolWithTag( NonPagedPool, newSize, SFLT_POOL_TAG );
if (NULL == newBuffer)
{
RtlInitEmptyUnicodeString(
(PUNICODE_STRING)&NameControl->smallBuffer,
(PWCHAR)(NameControl->smallBuffer + sizeof(UNICODE_STRING)),
(USHORT)(sizeof(NameControl->smallBuffer) - sizeof(UNICODE_STRING)) );

return (PUNICODE_STRING)&NameControl->smallBuffer;
}

newNameInfo = (POBJECT_NAME_INFORMATION)newBuffer;

RtlInitEmptyUnicodeString(
&newNameInfo->Name,
(PWCHAR)(newBuffer + sizeof(OBJECT_NAME_INFORMATION)),
(USHORT)(newSize - sizeof(OBJECT_NAME_INFORMATION)) );

RtlCopyUnicodeString( &newNameInfo->Name, &nameInfo->Name );
if (NULL != NameControl->allocatedBuffer)
{
ExFreePool( NameControl->allocatedBuffer );
}

NameControl->allocatedBuffer = newBuffer;
bufferSize = newSize;
nameInfo = newNameInfo;
}
else
{
nameInfo->Name.MaximumLength = (USHORT)(bufferSize - sizeof(OBJECT_NAME_INFORMATION));
}

if (NULL != FileObject->RelatedFileObject)
{
RtlAppendUnicodeStringToString( &nameInfo->Name, &FileObject->RelatedFileObject->FileName );
RtlAppendUnicodeToString( &nameInfo->Name, L"\\" );
}

RtlAppendUnicodeStringToString( &nameInfo->Name, &FileObject->FileName );
ASSERT(nameInfo->Name.Length <= nameInfo->Name.MaximumLength);
}

return &nameInfo->Name;
}



VOID SfGetFileNameCleanup( IN OUT PGET_NAME_CONTROL NameControl )
{

if (NULL != NameControl->allocatedBuffer) {

ExFreePool( NameControl->allocatedBuffer);
NameControl->allocatedBuffer = NULL;
}
}



BOOLEAN SfIsAttachedToDevice( PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL )
{

PAGED_CODE();

#if WINVER >= 0x0501
if (IS_WINDOWSXP_OR_LATER())
{
ASSERT( NULL != gSfDynamicFunctions.GetLowerDeviceObject && NULL != gSfDynamicFunctions.GetDeviceAttachmentBaseRef );
return SfIsAttachedToDeviceWXPAndLater( DeviceObject, AttachedDeviceObject );
}
else
{
#endif
return SfIsAttachedToDeviceW2K( DeviceObject, AttachedDeviceObject );
#if WINVER >= 0x0501
}
#endif
}



BOOLEAN SfIsAttachedToDeviceW2K( PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL )
{
PDEVICE_OBJECT currentDevice;
PAGED_CODE();

for (currentDevice = DeviceObject; currentDevice != NULL; currentDevice = currentDevice->AttachedDevice)
{
if (IS_MY_DEVICE_OBJECT( currentDevice ))
{
if (ARGUMENT_PRESENT(AttachedDeviceObject))
{
ObReferenceObject( currentDevice );
*AttachedDeviceObject = currentDevice;
}
return TRUE;
}
}

if (ARGUMENT_PRESENT(AttachedDeviceObject))
{
*AttachedDeviceObject = NULL;
}
return FALSE;
}



#if WINVER >= 0x0501
BOOLEAN SfIsAttachedToDeviceWXPAndLater( PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL )
{
PDEVICE_OBJECT currentDevObj;
PDEVICE_OBJECT nextDevObj;

PAGED_CODE();

ASSERT( NULL != gSfDynamicFunctions.GetAttachedDeviceReference );
currentDevObj = (gSfDynamicFunctions.GetAttachedDeviceReference)( DeviceObject );

do {

if (IS_MY_DEVICE_OBJECT( currentDevObj ))
{
if (ARGUMENT_PRESENT(AttachedDeviceObject))
{
*AttachedDeviceObject = currentDevObj;
}
else
{
ObDereferenceObject( currentDevObj );
}
return TRUE;
}

ASSERT( NULL != gSfDynamicFunctions.GetLowerDeviceObject );
nextDevObj = (gSfDynamicFunctions.GetLowerDeviceObject)( currentDevObj );
ObDereferenceObject( currentDevObj );
currentDevObj = nextDevObj;
} while (NULL != currentDevObj);

if (ARGUMENT_PRESENT(AttachedDeviceObject))
{
*AttachedDeviceObject = NULL;
}

return FALSE;
}
#endif



VOID SfReadDriverParameters( IN PUNICODE_STRING RegistryPath )
{
KdPrint(("RegistryPath:%s",RegistryPath));
OBJECT_ATTRIBUTES attributes;
HANDLE driverRegKey;
NTSTATUS status;
ULONG resultLength;
UNICODE_STRING valueName;
UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

PAGED_CODE();

if (0 == SfDebug)
{
InitializeObjectAttributes( &attributes,
RegistryPath,
OBJ_CASE_INSENSITIVE,
NULL,
NULL );

status = ZwOpenKey( &driverRegKey, KEY_READ, &attributes );
if (!NT_SUCCESS( status ))
{
	KdPrint(("read driver failed\n"));
return;
}

RtlInitUnicodeString( &valueName, L"DebugFlags" );
status = ZwQueryValueKey( driverRegKey,
&valueName,       
KeyValuePartialInformation,
buffer,
sizeof(buffer),
&resultLength );
if (NT_SUCCESS( status ))
{
SfDebug = *((PLONG) &(((PKEY_VALUE_PARTIAL_INFORMATION) buffer)->Data));
}

ZwClose(driverRegKey);
}
}




NTSTATUS SfIsShadowCopyVolume( IN PDEVICE_OBJECT StorageStackDeviceObject, OUT PBOOLEAN IsShadowCopy )
{

/*******************************************************************************************************************************************
SfIsShadowCopyVolume( )��������Ҫ�����ǣ��漰�� ��Ӱ��������ġ�

��Ӱ��������(Volume Shadow Copy Service��VSS)��һ�ֱ��ݺͻָ��ļ���������һ�ֻ���ʱ����������ļ������ļ�����
ͨ��ʹ�þ�Ӱ�����������ǿ������ض����Ͻ������ݿ���ʱ��㣬���ڽ�����ĳһʱ�̰����ݻָ����κ�һ������������ʱ����״̬��
���������һ���ǻָ���Ϊԭ����ɵ����ݶ�ʧ���û�������ش洢���д�����Ϣ���ļ��������ǲ�С��ɾ���ļ��������������������⡣
����VSS���ղ����ɾ������ݾ����Լ��ָ�ʱ��㿽�������������Ǽȿ��Իָ��������գ�Ҳ����ȡ���裬���߻�����ʹ��VSS���ݹ������ָ��������ļ����ļ��С�

VSS���Ƕ�Ӧ�ó�����б��ݵģ�VSS���Զ��߼���(Ӳ�̷���)���п��ա�
VSS��Windows�µĿ��ռ�������Requestor, Write,��Provider��ɣ���Ҫ����������Volsnap.sysʵ�֣����ӵ�������������ļ�ϵͳ����֮�䣬ͬʱ������COM������
��ˣ��������ǵ����ھ��ϵ�block���д������Ǻ͸���ϵͳӦ�������������SQL��EXCHANGE��AD�ȵȡ��Ӷ�ʹ���ڲ��ػ���Ҳ��ֹͣӦ�õ�����£������ա�
VSS���㷺��Ӧ�õ�Windows�ı��ݴ����С�

VSS �������ķ����ǣ�ͨ���ṩ����������Ҫʵ��֮���ͨѶ����֤���ݵĸ߶���ʵ�ͻָ����̵ļ�㡣
(1)�������������������ʱ������ݸ������Ӱ������Ӧ�ó��򣬱��籸�ݻ�洢����Ӧ�ó���
(2)д��������Ǹ�������֪ͨ�����ݱ�����д�������VSS������������Ӱ��������ս�������ĵط�����VSS�ľ�Ӱ���ƹ����л��漰һЩӦ�ó���
(3)�ṩ���������ڱ�¶����Ӳ��������ľ�Ӱ�����Ļ��ơ����洢Ӳ����Ӧ�̶���Ϊ���ǵĴ洢���б�д�ṩ����


VSS����Ψһ��ȱ���ǣ�
������ҪΪÿһ����Ӱ��������Ĵ��̿ռ䣬���Ǳ�����ĳ���洢��Щ��������ΪVSSʹ��ָ�����ݣ���Щ����ռ�õĿռ�Ҫ�������С�ö࣬���ǿ�����Ч�ش洢��Щ������


�й�VSS�ĸ���˵��������ȥ���Microsoft��������վ

http://technet.microsoft.com/en-us/library/ee923636.aspx

*********************************************************************************************************************************************/
PAGED_CODE();

*IsShadowCopy = FALSE;

#if WINVER >= 0x0501
if (IS_WINDOWS2000())
{
#endif
UNREFERENCED_PARAMETER( StorageStackDeviceObject );
return STATUS_SUCCESS;
#if WINVER >= 0x0501
}

if (IS_WINDOWSXP())
{
UNICODE_STRING volSnapDriverName;
WCHAR buffer[MAX_DEVNAME_LENGTH];
PUNICODE_STRING storageDriverName;
ULONG returnedLength;
NTSTATUS status;

if (FILE_DEVICE_DISK != StorageStackDeviceObject->DeviceType)
{
return STATUS_SUCCESS;
}

storageDriverName = (PUNICODE_STRING) buffer;
RtlInitEmptyUnicodeString( storageDriverName, (PWCHAR)Add2Ptr(storageDriverName, sizeof( UNICODE_STRING ) ), sizeof( buffer ) - sizeof( UNICODE_STRING ) );
status = ObQueryNameString( StorageStackDeviceObject, (POBJECT_NAME_INFORMATION)storageDriverName, storageDriverName->MaximumLength, &returnedLength );
if (!NT_SUCCESS( status ))
{
return status;
}

RtlInitUnicodeString( &volSnapDriverName, L"\\Driver\\VolSnap" );
if (RtlEqualUnicodeString( storageDriverName, &volSnapDriverName, TRUE ))
{
*IsShadowCopy = TRUE;
}
else
{
NOTHING;
}

return STATUS_SUCCESS;
}
else
{
PIRP irp;
KEVENT event;
IO_STATUS_BLOCK iosb;
NTSTATUS status;
if (FILE_DEVICE_VIRTUAL_DISK != StorageStackDeviceObject->DeviceType)
{
return STATUS_SUCCESS;
}

KeInitializeEvent( &event, NotificationEvent, FALSE );

/*
*Microsoft WDK�ٷ��ĵ��� IOCTL_DISK_IS_WRITABLE���������͵ģ�
*Determines whether a disk is writable.
*The Status field can be set to STATUS_SUCCESS, or possibly to STATUS_INSUFFICIENT_RESOURCES, STATUS_IO_DEVICE_ERROR, or STATUS_MEDIA_WRITE_PROTECTED.
*
*IOCTL_DISK_IS_WRITABLE��û������Ҳû������ġ�
*/
irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
StorageStackDeviceObject,
NULL,
0,
NULL,
0,
FALSE,
&event,
&iosb );
if (irp == NULL)
{
return STATUS_INSUFFICIENT_RESOURCES;
}

status = IoCallDriver( StorageStackDeviceObject, irp );
if (status == STATUS_PENDING)
{
(VOID)KeWaitForSingleObject( &event,
Executive,
KernelMode,
FALSE,
NULL );
status = iosb.Status;
}

if (STATUS_MEDIA_WRITE_PROTECTED == status)
{
*IsShadowCopy = TRUE;
status = STATUS_SUCCESS;
}

return status;
}
#endif
}
//���ڲ���������ķַ�����
NTSTATUS SfRead(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
//	KdPrint(("SfReaad in2!\n"));
	bool MyDebug=MY_DEBUG(MYDEBUG,READ_DEBUG);
	
    NTSTATUS statue;
	PIO_STACK_LOCATION irpsp=IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file_object=irpsp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt=(PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if(IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject))
	{
		KdPrint(("1\n"));
		Irp->IoStatus.Status=STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information=0;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}
	if(devExt->StorageStackDeviceObject!=NULL)//���ǶԾ��豸�Ĳ����������ļ�
	{
		
		//return SfPassThrough(DeviceObject,Irp);
	    
		//KdPrint(("2.17\n"));
	    KEVENT event;
	    PUCHAR buf;
		ULONG length;
		LARGE_INTEGER offset;
		UCHAR mini_code=irpsp->MinorFunction;
		::KeInitializeEvent(&event,NotificationEvent,FALSE);
		::IoCopyCurrentIrpStackLocationToNext(Irp);
		::IoSetCompletionRoutine(Irp,
			                     SfReadCompleteion,
								 &event,
								 true,
								 true,
								 true);
	//	if(MyDebug)
	//	KdPrint(("IRP_MJ_READ����\n"));
		statue=::IoCallDriver(devExt->AttachedToDeviceObject,Irp);
		if(statue==STATUS_PENDING)
		{
			statue=KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
		//	ASSERT(statue!=STATUS_SUCCESS);
		}
	if(Irp->IoStatus.Status==STATUS_SUCCESS)
	{
		irpsp=IoGetCurrentIrpStackLocation(Irp);
		offset.QuadPart=irpsp->Parameters.Read.ByteOffset.QuadPart;

	   length=irpsp->Parameters.Read.Length;
	    UNICODE_STRING tocompare;
		
		::RtlInitUnicodeString(&tocompare,(PCWSTR)L"\\fortest\\90\\devicetree.exe");
	//	KdPrint(("filename:%wZ ",&(irpsp->FileObject->FileName)));
	   if(RtlCompareUnicodeString(&(irpsp->FileObject->FileName),&tocompare,true)!=0)
	   {
		  // if(MyDebug)
		 //  KdPrint(("passthrough,filename:%wZ ",&(irpsp->FileObject->FileName)));
		   return SfPassThrough(DeviceObject,Irp);
	   }
	   else
		   KdPrint(("����filename:%wZ ",&(irpsp->FileObject->FileName)));

		switch(mini_code)
			{
		case(IRP_MN_NORMAL):
		{
			if(Irp->MdlAddress!=NULL)
			{
				KdPrint(("MDLAddress \n"));
				buf=(PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);
				if(buf==NULL)
				{
					KdPrint(("there are a error occured in MmGetSystemAddressForMdl!\n"));
					buf=(PUCHAR)Irp->UserBuffer;
				}
			}
			else
				buf=(PUCHAR)Irp->UserBuffer;
			for(int i=0;i<length;++i)
				buf[i]^=0x77;
			//KdPrint(("NORMAL:��ȡ������1:%wZ",buf));
			if(MyDebug)
			KdPrint(("content:%c%c%c%c",buf[0],buf[1],buf[2],buf[3]));
			//RtlFillMemory(buf,length,'U');
			Irp->IoStatus.Information=length;
			Irp->IoStatus.Status=STATUS_SUCCESS;			
			irpsp->FileObject->CurrentByteOffset.QuadPart=offset.QuadPart+length;
			
		    IoCompleteRequest(Irp,IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}
		case(IRP_MN_MDL):
			//PAGE_SIZE * (65535 - sizeof(MDL)) / sizeof(ULONG_PTR)����winodows7��ǰ�����Ŀɷ����mdl���������ڴ��size;
		{
			PMDL mdl=::MyMdlMemoryAllocate(length);
			if(mdl==NULL)
			{
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			Irp->MdlAddress=mdl;
		    buf=(PUCHAR)MmGetSystemAddressForMdlSafe(mdl,NormalPagePriority);
			//KdPrint(("MN_MDL��ȡ������2:%wZ",buf));
			Irp->IoStatus.Information=length;
			Irp->IoStatus.Status=STATUS_SUCCESS;
			irpsp->FileObject->CurrentByteOffset.QuadPart=offset.QuadPart+length;
			IoCompleteRequest(Irp,IO_NO_INCREMENT);
			return STATUS_SUCCESS;

		}
		case(IRP_MN_COMPLETE_MDL):
		{
			::MyFreeMdl(Irp->MdlAddress);
			Irp->IoStatus.Information=length;
			Irp->IoStatus.Status=STATUS_SUCCESS;
			irpsp->FileObject->CurrentByteOffset.QuadPart=offset.QuadPart+length;
			IoCompleteRequest(Irp,IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}
		default:
			{
				KdPrint(("�������͵�IRP_MJ_READ\n"));
				return STATUS_INVALID_DEVICE_REQUEST;
			}
	}
		


   }
	}
	
  // KdPrint(("4\n"));
   statue=STATUS_SUCCESS;
	return statue;
}
_inline PMDL MyMdlAllocate(PVOID buff,ULONG length)
{
	PMDL mdl=::IoAllocateMdl(buff,length,false,false,NULL);
	if(mdl==NULL)
		return NULL;
	::MmBuildMdlForNonPagedPool(mdl);
	return mdl;
}
_inline PMDL MyMdlMemoryAllocate(ULONG length)
{
	PVOID buf=::ExAllocatePool(NonPagedPool,length);
	if(buf==NULL)
		return NULL;
	PMDL mdl=::MyMdlAllocate(buf,length);
	if(mdl==NULL)
	{
		::ExFreePool(buf);
		return NULL;
	}
	return mdl;
}
_inline VOID MyFreeMdl(PMDL mdl)
{
	void *buf=MmGetSystemAddressForMdlSafe(mdl,NormalPagePriority);
	::IoFreeMdl(mdl);
	::ExFreePool(buf);
}
NTSTATUS SfReadCompleteion(PDEVICE_OBJECT pdevice,PIRP irp,PVOID Context)
{
	PKEVENT waitevent=(PKEVENT)Context;
	
	UNREFERENCED_PARAMETER( pdevice );

	if(irp->IoStatus.Status==STATUS_SUCCESS&&(MY_DEBUG(MYDEBUG,READ_DEBUG)))
	{
	//	KdPrint(("SfReadCompleteion finised 1.1!\n"));
	}
	KeSetEvent(waitevent,IO_NO_INCREMENT,FALSE);
	ASSERT(IS_MY_DEVICE_OBJECT(pdevice));

	return STATUS_MORE_PROCESSING_REQUIRED;
}
BOOLEAN LookForDirectory(PFILE_OBJECT fileobject)
{
	PLIST_ENTRY p=NULL;
	for(p=listentry.Flink;p!=(PLIST_ENTRY)&listentry;p=p->Blink)
	{
		
		PAll_DIRECTORS pmd=CONTAINING_RECORD(p,All_DIRECTORS,link);
	//	KdPrint(("number:%ld",pmd->directors_number));
		if(fileobject==pmd->FileObj)
		{
		//	KdPrint(("��Ŀ¼�������ҵ���Ŀ¼:%l",pmd->directors_number));
			return true;
		}
	}
	return false;
}

// <=DISPATCH

/*PUNICODE_STRING MyFileFullPathQueryW(IN PFILE_OBJECT file,ULONG size)
{
	NTSTATUS status;
	POBJECT_NAME_INFORMATION nameinfo=NULL;
	UNICODE_STRING path;
	path.Buffer=(PWCHAR)::ExAllocatePool(NonPagedPool,size);
	path.MaximumLength=size;
	path.Length=0;
	RtlZeroMemory(&path,size);

	WCHAR buf[FILE_NAME_LENGTH_MAX]={0};
    void *obj_ptr;
	ULONG length=0;
	BOOLEAN need_split=FALSE;
	ASSERT(file!=NULL);
	if(file==NULL)
	{
		return 0;
	}
	if(file->FileName.Buffer==NULL)
		return 0;
	nameinfo=(POBJECT_NAME_INFORMATION)buf;
	if(file->RelatedFileObject!=NULL)
		obj_ptr=(void*)file->RelatedFileObject;
	else
		obj_ptr=(void*)file->DeviceObject;
	status=ObQueryNameString(obj_ptr,nameinfo,64*sizeof(WCHAR),&length);
	do
	{
	
	if(status==STATUS_INFO_LENGTH_MISMATCH)
	{
		nameinfo=(POBJECT_NAME_INFORMATION )ExAllocatePoolWithTag(NonPagedPool,length,MEM_TAG);
		if(nameinfo==NULL)
			return NULL;
		RtlZeroMemory(nameinfo,length);
		status=ObQueryNameString(obj_ptr,nameinfo,length,&length);
	}
	if(!NT_SUCCESS(status))
	{
		KdPrint(("ObQueryNameSreing failed\n"));
		break;
	}
	if(file->FileName.Length>2&&file->FileName.Buffer[0]!=L'\\'&&nameinfo->Name.Buffer[nameinfo->Name.Length/sizeof(WCHAR)-1]!=L'\\')
		need_split=TRUE;
	length=nameinfo->Name.Length+file->FileName.Length;
	if(need_split)
		length+=sizeof(WCHAR);
	if(path.MaximumLength<length)
	{
		KdPrint(("length is not enough :%d",path.MaximumLength));
		
		break;
	}
	::RtlCopyUnicodeString(&path,&nameinfo->Name);
	//KdPrint(("devicenameinfo:%wZ",nameinfo->Name));
	if(need_split)
		RtlAppendUnicodeStringToString(&path,&file->FileName);
	//KdPrint(("filename:%wZ",file->FileName));
	}while(0);
	if((void*)nameinfo!=(void*)buf)
		  ExFreePool(nameinfo);
	return &path;
}*/

NTSTATUS SfDirectoryControl(IN PDEVICE_OBJECT device,IN PIRP irp)
{
	NTSTATUS statu;
	UNREFERENCED_PARAMETER(device);
	PIO_STACK_LOCATION  irpsp=::IoGetCurrentIrpStackLocation(irp);
	KIRQL oldirql=::KeGetCurrentIrql();

	POperationInformation operationinformation;
	POperationResult result;

	
	
	PUNICODE_STRING directory=NULL;
	KdPrint(("Ŀ¼���ʿ���\n"));
	statu=DBAC_ControlLogicEx(directory,IRP_MJ_SET_INFORMATION,irpsp->FileObject);
	if(statu!=0x1000)  //��Ĭ�Ͽ����߼�������״̬�����������Ҫapp��֤,��ʼΪ����ͨ��
	{	   
		KdPrint(("statu:%x",statu));
		   goto FREE;
	}

	
	if(oldirql>PASSIVE_LEVEL)
	{
		KdPrint(("High Irql level in SfDirectoryControl!\n"));
	}
	else
	{
	
		    KdPrint(("statu==NULL: %d",statu==NULL?1:0));
		    ::AllocatePool(operationinformation,result);
			::RtlInitUnicodeString(&(operationinformation->FileName),(PCWSTR)directory->Buffer);

			::RtlInitUnicodeString(&(operationinformation->FilePath),(PCWSTR)directory->Buffer);
			operationinformation->OperationType=IRP_MJ_SET_INFORMATION;
			result->operationtype=IRP_MJ_SET_INFORMATION;
			statu=::VerifyControl(operationinformation,result);
			
			goto FREE;
		

	}
    goto FREE;

	FREE:
	if(NT_SUCCESS(statu))//���ͨ�����·������������
	{
	//::ExFreePool(filename);
	::IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION)device->DeviceExtension)->AttachedToDeviceObject, irp );

	}
//	::ExFreePool(filename);
	irp->IoStatus.Information=0;
	irp->IoStatus.Status=statu;
	IoCompleteRequest(irp,IO_NO_INCREMENT);
	return statu;
}

NTSTATUS SfSetInformation(IN PDEVICE_OBJECT device,IN PIRP irp)
{


	NTSTATUS statu;
	UNREFERENCED_PARAMETER(device);
	PIO_STACK_LOCATION  irpsp=::IoGetCurrentIrpStackLocation(irp);
	POperationInformation operationinformation;
	POperationResult result;
	
	
	PUNICODE_STRING directory=NULL;

	statu=DBAC_ControlLogicEx(directory,IRP_MJ_SET_INFORMATION,irpsp->FileObject);
	if(statu!=0x1000)  //��Ĭ�Ͽ����߼�������״̬�����������Ҫapp��֤,��ʼΪ����ͨ��
	{	   
		KdPrint(("statu:%x",statu));
		   goto FREE;
	}
	

	if(irpsp->Parameters.SetFile.FileInformationClass==FileDispositionInformation ||irpsp->Parameters.SetFile.FileInformationClass==FileRenameInformation )
	{			
		    KdPrint(("statu==NULL: %d",statu==NULL?1:0));
		    ::AllocatePool(operationinformation,result);
			::RtlInitUnicodeString(&(operationinformation->FileName),(PCWSTR)directory->Buffer);

			::RtlInitUnicodeString(&(operationinformation->FilePath),(PCWSTR)directory->Buffer);
			operationinformation->OperationType=IRP_MJ_SET_INFORMATION;
			result->operationtype=IRP_MJ_SET_INFORMATION;
			statu=::VerifyControl(operationinformation,result);
			
			goto FREE;

	}

   goto FREE;

FREE:
	if(NT_SUCCESS(statu))//���ͨ�����·������������
	{
	
	::IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION)device->DeviceExtension)->AttachedToDeviceObject, irp );

	}
	
	irp->IoStatus.Information=0;
	irp->IoStatus.Status=statu;
	IoCompleteRequest(irp,IO_NO_INCREMENT);
	return statu;
}


//��Ӧ��ͨ�ŵ�IRP��ǲ����
NTSTATUS SfDeviceControl(IN PDEVICE_OBJECT device,IN PIRP irp)
{
	
	if(device==gSFilterControlDeviceObject)
	{
	KdPrint(("device io control IN!\n"));
	
	NTSTATUS status=STATUS_SUCCESS;
	PIO_STACK_LOCATION irpsp=::IoGetCurrentIrpStackLocation(irp);
	ULONG iocode=irpsp->Parameters.DeviceIoControl.IoControlCode;
	
	switch(iocode)
	{
	case(IOCTL_DEVICE_READ):
		break;
	case(IOCTL_DEVICE_WRITE):

		break;
	case(IOCTL_CAREFILE_INIT):
		KdPrint(("�յ�ѯ��:�Ƿ���Ҫ��ʼ��!\n"));
		CR_SetCareFileInit(irp);
		break;
	case(IOCTL_CAREFILE_START_INIT):
		KdPrint(("��ʼ�����ļ����ݣ����ҳ�ʼ�������ļ�����!\n\n"));
		CR_StartInitCareFileHashTable(irp);
		break;
	case(IOCTL_CAREFILE_INIT_SHUTDOWN):
		KdPrint(("�յ�ѯ��:��ʼ���Ƿ���ȷ���!\n"));
		CR_InitFinished(irp);
		break;
	case(IOCTL_VERIFY_OPERATION_INFORMATION)://
	    KdPrint(("�յ�������Ӧ�õ�ȡ������Ϣ֪ͨ��call GetOperationInformation(irp)\n"));
		GetOperationInformation(irp);
		//DisPlayHashTable(1);
		break;
	case(IOCTL_SEND_VERIFY_RESULT):
		KdPrint(("�յ�������Ӧ�ó���Ĳ�����֤���֪ͨ!\n"));		
		::SetOperationResult(irp);
	//	DisPlayHashTable(2);
		break;
	default:
		KdPrint(("UnKown Device Control Code:%d!",iocode));
		break;
	}
	
	KdPrint(("SfDeviceControl  finished!\n\n"));
	irp->IoStatus.Information=0;
	irp->IoStatus.Status=STATUS_SUCCESS;
	IoCompleteRequest(irp,IO_NO_INCREMENT);	
	return STATUS_SUCCESS;
	}
	else
	{
		KdPrint(("device io control: call nextdeivce\n"));
		::IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver( ((PSFILTER_DEVICE_EXTENSION)device->DeviceExtension)->AttachedToDeviceObject, irp );
	}
	    

}



