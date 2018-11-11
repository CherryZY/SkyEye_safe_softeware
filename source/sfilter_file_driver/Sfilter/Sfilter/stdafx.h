#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifdef __cplusplus
extern "C" 
{

#endif


#include "ntifs.h"
#include "ntdddisk.h"
#include "HashTable_Function.h"
#include "Common_Function.h"
#include "CareFile.h"
#include "Integration_Function.h"
#include "DBAC_ControlLogic.h"

#pragma warning(error:4100)        //ʹ��������������(���βδ���ʱ��������)
#pragma warning(error:4101)        //���ֲ���������ʱ��������

//============================== �������� ============================
#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))          //�﷨: PVOID  Add2Ptr(PUCHAR p,PUCHAR i);  ����:  ���������ַ���
#endif
//================ ��������(��OS�汾��Win2000ʱ�����¶δ���) ===================
#if WINVER == 0x0500

#ifndef FlagOn  //��FlagOnδ��#define�����,������һ��
#define FlagOn(_F,_SF)  ((_F) & (_SF))                  //�﷨: FiagOn(a,b);  ����: a&b(a��b��λ�����)
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF)  ((BOOLEAN)(((F) & (SF)) != 0))  //�﷨: BooleanFlagOn(a,b); ����: a&b !=0
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)  ((_F) |= (_SF))                  //�﷨: SetFlag(a,b);  ����: a |= b(a��b��λ�����,�������a)
#endif

#ifndef ClearFlag  
#define ClearFlag(_F,_SF)  ((_F) &= ~(_SF))              //�﷨: ClearFlag(a,b);  ����: a &= ~b(a��b�ǰ�λ�����,�������a)
#endif

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))                //�﷨: min(a,b); ����: ����a��b�е�С��
#endif

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))                //�﷨: max(a,b); ����: ����a��b�еĴ���
#endif

#ifdef ASSERT
#undef ASSERT
#if DBG
#define ASSERT( exp ) ((!(exp)) ? (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : TRUE)
#else
#define ASSERT( exp ) ((void) 0)
#endif
#endif        

#define ExFreePoolWithTag( a, b )  ExFreePool( (a) )        //�ɿ���,ExFreePoolWithTag��һ���궨��,ʵ���Ͼ���ExFreePool����

#endif


//=================== ��������(WinXP������OSʱ�����¶δ���) ====================
#if WINVER >= 0x0501

typedef  NTSTATUS (*PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS) ( IN PDRIVER_OBJECT DriverObject, IN PFS_FILTER_CALLBACKS Callbacks );
typedef  NTSTATUS (*PSF_ENUMERATE_DEVICE_OBJECT_LIST) ( IN  PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT *DeviceObjectList, IN ULONG DeviceObjectListSize, OUT PULONG ActualNumberDeviceObjects );
typedef  NTSTATUS (*PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE) ( IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, OUT PDEVICE_OBJECT *AttachedToDeviceObject );
typedef  NTSTATUS (*PSF_GET_DISK_DEVICE_OBJECT) ( IN PDEVICE_OBJECT FileSystemDeviceObject, OUT PDEVICE_OBJECT *DiskDeviceObject );
typedef  NTSTATUS (*PSF_GET_VERSION) ( IN OUT PRTL_OSVERSIONINFOW VersionInformation );

typedef  PDEVICE_OBJECT (*PSF_GET_LOWER_DEVICE_OBJECT) ( IN PDEVICE_OBJECT  DeviceObject );
typedef  PDEVICE_OBJECT (*PSF_GET_DEVICE_ATTACHMENT_BASE_REF) ( IN PDEVICE_OBJECT DeviceObject );
typedef  PDEVICE_OBJECT (*PSF_GET_ATTACHED_DEVICE_REFERENCE) ( IN PDEVICE_OBJECT DeviceObject );

#define IS_WINDOWS2000() ((gSfOsMajorVersion == 5) && (gSfOsMinorVersion == 0))
#define IS_WINDOWSXP() ((gSfOsMajorVersion == 5) && (gSfOsMinorVersion == 1))
#define IS_WINDOWSXP_OR_LATER() (((gSfOsMajorVersion == 5) && (gSfOsMinorVersion >= 1)) || (gSfOsMajorVersion > 5))
#define IS_WINDOWSSRV2003_OR_LATER() (((gSfOsMajorVersion == 5) && (gSfOsMinorVersion >= 2)) || (gSfOsMajorVersion > 5))

typedef struct _SF_DYNAMIC_FUNCTION_POINTERS {
    PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS RegisterFileSystemFilterCallbacks;  
    PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE AttachDeviceToDeviceStackSafe;      
    PSF_ENUMERATE_DEVICE_OBJECT_LIST EnumerateDeviceObjectList;              
    PSF_GET_LOWER_DEVICE_OBJECT GetLowerDeviceObject;                    
    PSF_GET_DEVICE_ATTACHMENT_BASE_REF GetDeviceAttachmentBaseRef;          
    PSF_GET_DISK_DEVICE_OBJECT GetDiskDeviceObject;                        
    PSF_GET_ATTACHED_DEVICE_REFERENCE GetAttachedDeviceReference;            
    PSF_GET_VERSION GetVersion;                                        
} SF_DYNAMIC_FUNCTION_POINTERS, *PSF_DYNAMIC_FUNCTION_POINTERS;



#endif


#define MAX_DEVNAME_LENGTH 64        //���峣��ֵ
#define DEVOBJ_LIST_SIZE 64
#define SFLT_POOL_TAG   'tlFS'        //�ڴ���ǩ


#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND*1000)


// �궨��: IS_MY_DEVICE_OBJECT(ָ���豸�����ָ��); ����: (ָ���NULL)��(�豸�����DriverObject��gSFilterDriverObject)��(�豸�����DeviceExtension��NULL)
#define IS_MY_DEVICE_OBJECT(_devObj)  (((_devObj) != NULL) && ((_devObj)->DriverObject == gSFilterDriverObject) && ((_devObj)->DeviceExtension != NULL))
                                              

// �궨��: �����ǲ����ҵ�"�����豸����"
#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj)  (((_devObj) == gSFilterControlDeviceObject) ? (ASSERT(((_devObj)->DriverObject == gSFilterDriverObject) &&((_devObj)->DeviceExtension == NULL)), TRUE): FALSE)


// �궨��: �����ǲ�������Ҫ��"�豸����"
#define IS_DESIRED_DEVICE_TYPE(_type)  (((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || ((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || ((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

// �궨��: ����FAST_IO_DISPATCH�еĴ������Ϸ�
#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName)  (((_FastIoDispatchPtr) != NULL) && (((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= (FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && ((_FastIoDispatchPtr)->_FieldName != NULL))
#define  CURRENT_IRQL_PRINT() {if(KeGetCurrentIrql()==PASSIVE_LEVEL) {KdPrint(("PASSIVE\n"));}else if(KeGetCurrentIrql()==APC_LEVEL) {KdPrint(("APC\n"));}else if(KeGetCurrentIrql()==DISPATCH_LEVEL) {KdPrint(("DISPATCH\n"));}else {KdPrint(("Higher than DisPatch\n"));}}
// �궨��: 
#define SF_LOG_PRINT( _dbgLevel, _string )  (FlagOn(SfDebug,(_dbgLevel)) ? DbgPrint _string : ((void)0))

#define SFDEBUG_DISPLAY_ATTACHMENT_NAMES  0x00000001    //display names of device objects we attach to
#define SFDEBUG_DISPLAY_CREATE_NAMES  0x00000002        //get and display names during create
#define SFDEBUG_GET_CREATE_NAMES  0x00000004          //get name (don't display) during create
#define SFDEBUG_DO_CREATE_COMPLETION  0x00000008      //do create completion routine, don't get names
#define SFDEBUG_ATTACH_TO_FSRECOGNIZER  0x00000010      //do attach to FSRecognizer device objects
#define SFDEBUG_ATTACH_TO_SHADOW_COPIES  0x00000020    //do attach to ShadowCopy Volume device objects -- they are only around on Windows XP and later

#define GET_DEVICE_TYPE_NAME( _type ) ((((_type) > 0) && ((_type) < (sizeof(DeviceTypeNames) / sizeof(PCHAR)))) ? DeviceTypeNames[ (_type) ] : "[Unknown]\n")



typedef struct _SFILTER_DEVICE_EXTENSION {
  PDEVICE_OBJECT AttachedToDeviceObject;          //�󶨵��ļ�ϵͳ�豸
  PDEVICE_OBJECT StorageStackDeviceObject;        //���ļ�ϵͳ�豸��ص���ʵ�豸(����) 
  UNICODE_STRING DeviceName;                //�������һ����,����������̾���������,Ϊ�󶨵Ŀ����豸����
  WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];        //�����������ִ��Ļ�����
} SFILTER_DEVICE_EXTENSION, *PSFILTER_DEVICE_EXTENSION;

typedef struct _FSCTRL_COMPLETION_CONTEXT {
  WORK_QUEUE_ITEM WorkItem;                        
  PDEVICE_OBJECT DeviceObject;                      
  PIRP Irp;                                    
  PDEVICE_OBJECT NewDeviceObject;                    
} FSCTRL_COMPLETION_CONTEXT, *PFSCTRL_COMPLETION_CONTEXT;

typedef struct _GET_NAME_CONTROL {
  PCHAR allocatedBuffer;                            
  CHAR smallBuffer[256];                            
} GET_NAME_CONTROL, *PGET_NAME_CONTROL;



//�������� 

NTSTATUS  DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath );

#if DBG && WINVER >= 0x0501  //���������԰汾��OS��WinXPʱ����ú���,���򲻱���
VOID  DriverUnload( IN PDRIVER_OBJECT DriverObject );                  //����ж�غ���
#endif

#if WINVER >= 0x0501        //�������ʱ��OS��WinXPʱ,��������������,���򲻱���
VOID  SfLoadDynamicFunctions ();                  
VOID  SfGetCurrentVersion ();                                                  
#endif

PUNICODE_STRING  SfGetFileName( IN PFILE_OBJECT FileObject, IN NTSTATUS CreateStatus, IN OUT PGET_NAME_CONTROL NameControl );
VOID  SfGetFileNameCleanup( IN OUT PGET_NAME_CONTROL NameControl );
NTSTATUS  SfPassThrough( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS  SfCreate( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp  );
NTSTATUS  SfCreateCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context );
NTSTATUS  SfCleanupClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS  SfFsControl( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS  SfFsControlMountVolume( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS  SfFsControlMountVolumeComplete( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_OBJECT NewDeviceObject );
NTSTATUS  SfFsControlLoadFileSystem( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS  SfFsControlLoadFileSystemComplete( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS  SfFsControlCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context );
BOOLEAN  SfFastIoCheckIfPossible( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, IN ULONG LockKey, IN BOOLEAN CheckForReadOperation, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoRead(IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, IN ULONG LockKey, OUT PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoWrite( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN BOOLEAN Wait, IN ULONG LockKey, IN PVOID Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoQueryBasicInfo( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_BASIC_INFORMATION Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoQueryStandardInfo( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_STANDARD_INFORMATION Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoLock( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, BOOLEAN FailImmediately, BOOLEAN ExclusiveLock, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoUnlockSingle( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoUnlockAll( IN PFILE_OBJECT FileObject, PEPROCESS ProcessId, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoUnlockAllByKey( IN PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoDeviceControl( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN ULONG IoControlCode, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoQueryNetworkOpenInfo( IN PFILE_OBJECT FileObject, IN BOOLEAN Wait, OUT PFILE_NETWORK_OPEN_INFORMATION Buffer, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoMdlRead( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoMdlReadComplete( IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoPrepareMdlWrite( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoMdlWriteComplete( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoReadCompressed( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, OUT PVOID Buffer, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo, IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoWriteCompressed( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN ULONG Length, IN ULONG LockKey, IN PVOID Buffer, OUT PMDL *MdlChain, OUT PIO_STATUS_BLOCK IoStatus, IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo, IN ULONG CompressedDataInfoLength, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoMdlReadCompleteCompressed( IN PFILE_OBJECT FileObject, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoMdlWriteCompleteCompressed( IN PFILE_OBJECT FileObject, IN PLARGE_INTEGER FileOffset, IN PMDL MdlChain, IN PDEVICE_OBJECT DeviceObject );
BOOLEAN  SfFastIoQueryOpen( IN PIRP Irp, OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation, IN PDEVICE_OBJECT DeviceObject );
VOID  SfFsControlLoadFileSystemCompleteWorker( IN PFSCTRL_COMPLETION_CONTEXT Context );
VOID  SfFsControlMountVolumeCompleteWorker( IN PFSCTRL_COMPLETION_CONTEXT Context );
VOID  SfFastIoDetachDevice( IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice );

#if WINVER >= 0x0501
NTSTATUS  SfPreFsFilterPassThrough( IN PFS_FILTER_CALLBACK_DATA Data, OUT PVOID *CompletionContext );
VOID  SfPostFsFilterPassThrough( IN PFS_FILTER_CALLBACK_DATA Data, IN NTSTATUS OperationStatus, IN PVOID CompletionContext );
#endif

NTSTATUS  SfAttachDeviceToDeviceStack( IN PDEVICE_OBJECT SourceDevice, IN PDEVICE_OBJECT TargetDevice, IN OUT PDEVICE_OBJECT *AttachedToDeviceObject );
NTSTATUS  SfAttachToFileSystemDevice( IN PDEVICE_OBJECT DeviceObject, IN PUNICODE_STRING DeviceName );
NTSTATUS  SfAttachToMountedDevice( IN PDEVICE_OBJECT DeviceObject, IN PDEVICE_OBJECT SFilterDeviceObject );
extern VOID  SfFsNotification( IN PDEVICE_OBJECT DeviceObject, IN BOOLEAN FsActive );
VOID  SfDetachFromFileSystemDevice( IN PDEVICE_OBJECT DeviceObject );
VOID  SfCleanupMountedDevice( IN PDEVICE_OBJECT DeviceObject );

#if WINVER >= 0x0501
NTSTATUS  SfEnumerateFileSystemVolumes( IN PDEVICE_OBJECT FSDeviceObject, IN PUNICODE_STRING FSName );
#endif

VOID  SfGetObjectName( IN PVOID Object, IN OUT PUNICODE_STRING Name );
VOID  SfGetBaseDeviceObjectName( IN PDEVICE_OBJECT DeviceObject, IN OUT PUNICODE_STRING DeviceName );
VOID  SfReadDriverParameters( IN PUNICODE_STRING RegistryPath );
BOOLEAN  SfIsAttachedToDevice( PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL );
BOOLEAN  SfIsAttachedToDeviceW2K( PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL );
BOOLEAN  SfIsAttachedToDeviceWXPAndLater( PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL );
NTSTATUS  SfIsShadowCopyVolume( IN PDEVICE_OBJECT StorageStackDeviceObject, OUT PBOOLEAN IsShadowCopy );


//**********************************************
//********************************the new irp toa add******************************
NTSTATUS SfDirectoryControl(IN PDEVICE_OBJECT device,IN PIRP irp);
NTSTATUS SfSetInformation(IN PDEVICE_OBJECT device,IN PIRP irp);
NTSTATUS SfDeviceControl(IN PDEVICE_OBJECT device,IN PIRP irp);
NTSTATUS SfRead(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
VOID SfCreateComplete(IN DEVICE_OBJECT *DeviceObject,IN PIRP *irp,IN PVOID context);
NTSTATUS SfReadCompleteion(PDEVICE_OBJECT pdevice,PIRP irp,PVOID Context);
#ifdef __cplusplus
}
#endif
