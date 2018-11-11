#ifndef SSDT_HOOK_H
#define SSDT_HOOK_H


#include <ntddk.h>
#include <stdlib.h>

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#define MAX_PROCESS_ARRARY_LENGTH		1024

//���� SSDT(ϵͳ����������) �з�������������Ŀ
//���ﶨ��Ϊ 1024 ����ʵ������ XP SP3 �� 0x0128 ��
#define MAX_SYSTEM_SERVICE_NUMBER 1024

//���� Zw_ServiceFunction ��ȡ Zw_ServiceFunction �� SSDT ������Ӧ�ķ����������
//ͬ�� mov eax (������)   eax
#define SYSCALL_INDEX(ServiceFunction) \
	(*(PULONG)((PUCHAR)ServiceFunction + 1))

//����NTKERNELAPI
NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(HANDLE Id, PEPROCESS *Process);


//���� Zw_ServiceFunction ����÷����� SSDT �е������ţ�
//Ȼ����ͨ��������������ȡ Nt_ServiceFunction�ĵ�ַ
//
#define SYSCALL_FUNCTION(ServiceFunction) \
	KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[SYSCALL_INDEX(ServiceFunction)]

//����SSDT�ṹ
typedef struct _KSYSTEM_SERVICE_TABLE
{
	PULONG  ServiceTableBase;					// SSDT (System Service Dispatch Table)�Ļ���ַ
	PULONG  ServiceCounterTableBase;			// ���� checked builds, ���� SSDT ��ÿ�����񱻵��õĴ���
	ULONG   NumberOfService;					// �������ĸ���, NumberOfService * 4 ����������ַ��Ĵ�С
	ULONG   ParamTableBase;						// SSPT(System Service Parameter Table)�Ļ���ַ

} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;


typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	KSYSTEM_SERVICE_TABLE   ntoskrnl;			// ntoskrnl.exe �ķ�����
	KSYSTEM_SERVICE_TABLE   win32k;				// win32k.sys �ķ�����(GDI32.dll/User32.dll ���ں�֧��)
	KSYSTEM_SERVICE_TABLE   notUsed1;
	KSYSTEM_SERVICE_TABLE   notUsed2;

} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

//һЩ���ýṹ�������
typedef struct _SYSTEM_THREAD_INFORMATION
{
	LARGE_INTEGER	KernelTime;		//�߳����ں�ִ��ʱ��
	LARGE_INTEGER	UserTime;		//�߳����û�ģʽִ��ʱ��
	LARGE_INTEGER	CreateTime;		//�̴߳���ʱ��
	ULONG			WaitTime;		//�߳��ܵĵȴ�ʱ��
	PVOID			StartAddress;	//�߳���ʼ��ַ
	CLIENT_ID		ClientId;		//���̺��̼߳�����
	KPRIORITY		Priority;		//�߳����ȼ�
	LONG			BasePriority;	//�̻߳������ȼ�
	ULONG			ContextSwitches;//���߳�ִ�е������Ľ�������
	ULONG			ThreadState;	//��ǰ�߳�״̬
	ULONG			WaitReason;		//�ȴ�ԭ��

} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION //win8��size = 184
{
	ULONG			NextEntryOffset;			//�����������ȥ��һ��������ڵ�ƫ��
	ULONG			NumberOfThreads;			//���̵��߳�����
	LARGE_INTEGER	SpareLi1;					//Reserved[0]
	LARGE_INTEGER	SpareLi2;					//Reserved[1]
	LARGE_INTEGER	SpareLi3;					//Reserved[2]
	LARGE_INTEGER	CreateTime;					//���̴���ʱ��,<=100ns
	LARGE_INTEGER	UserTime;					//�û�ģʽ����Чִ��ʱ��
	LARGE_INTEGER	KernelTime;					//�ں�ģʽ����Чִ��ʱ��
	UNICODE_STRING	ImageName;					//�����������ڿ�ִ���ļ���
	KPRIORITY		BasePriority;				//������ʼ����
	HANDLE			UniqueProcessId;			//Ψһ�Ľ��̱�ʶ
	HANDLE			InheritedFromUniqueProcessId;	//�����ߵı�ʶ
	ULONG			HandleCount;				//�򿪵ľ��������
	ULONG			SessionId;
	ULONG_PTR		PageDirectoryBase;
	SIZE_T			PeakVirtualSize;
	SIZE_T			VirtualSize;
	ULONG			PageFaultCount;
	SIZE_T			PeakWorkingSetSize;
	SIZE_T			WorkingSetSize;
	SIZE_T			QuotaPeakPagedPoolUsage;
	SIZE_T			QuotaPagedPoolUsage;
	SIZE_T			QuotaPeakNonPagedPoolUsage;
	SIZE_T			QuotaNonPagedPoolUsage;
	SIZE_T			PagefileUsage;
	SIZE_T			PeakPagefileUsage;
	SIZE_T			PrivatePageCount;
	LARGE_INTEGER	ReadOperationCount;
	LARGE_INTEGER	WriteOperationCount;
	LARGE_INTEGER	OtherOperationCount;
	LARGE_INTEGER	ReadTransferCount;
	LARGE_INTEGER	WriteTransferCount;
	LARGE_INTEGER	OtherTransferCount;

} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemMirrorMemoryInformation,
	SystemPerformanceTraceInformation,
	SystemObsolete0,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemVerifierAddDriverInformation,
	SystemVerifierRemoveDriverInformation,
	SystemProcessorIdleInformation,
	SystemLegacyDriverInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation,
	SystemTimeSlipNotification,
	SystemSessionCreate,
	SystemSessionDetach,
	SystemSessionInformation,
	SystemRangeStartInformation,
	SystemVerifierInformation,
	SystemVerifierThunkExtend,
	SystemSessionProcessInformation,
	SystemLoadGdiDriverInSystemSpace,
	SystemNumaProcessorMap,
	SystemPrefetcherInformation,
	SystemExtendedProcessInformation,
	SystemRecommendedSharedDataAlignment,
	SystemComPlusPackage,
	SystemNumaAvailableMemory,
	SystemProcessorPowerInformation,
	SystemEmulationBasicInformation,
	SystemEmulationProcessorInformation,
	SystemExtendedHandleInformation,
	SystemLostDelayedWriteInformation,
	SystemBigPoolInformation,
	SystemSessionPoolTagInformation,
	SystemSessionMappedViewInformation,
	SystemHotpatchInformation,
	SystemObjectSecurityMode,
	SystemWatchdogTimerHandler,
	SystemWatchdogTimerInformation,
	SystemLogicalProcessorInformation,
	SystemWow64SharedInformation,
	SystemRegisterFirmwareTableInformationHandler,
	SystemFirmwareTableInformation,
	SystemModuleInformationEx,
	SystemVerifierTriageInformation,
	SystemSuperfetchInformation,
	SystemMemoryListInformation,
	SystemFileCacheInformationEx,
	MaxSystemInfoClass

} SYSTEM_INFORMATION_CLASS;


NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength);

/*
��ʾNTTERMINATEPROCESSΪһ��ָ������ָ�����͵����֡�
Ҫʹ���䣬ֻ��ֱ��ʹ��NTTERMINATEPROCESS����
*/
typedef NTSTATUS (*NTQUERYSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS,
	__out_bcount_opt(SystemInformationLength) PVOID,ULONG,__out_opt PULONG);

typedef NTSTATUS(*NTTERMINATEPROCESS)(HANDLE, NTSTATUS);

NTSTATUS  MyNtQuerySystemInformation(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength);

NTSTATUS MyNtTerminateProcess(
	__in_opt HANDLE ProcessHandle,
	__in NTSTATUS ExitStatus
	);

//����һ��ָ��̶�������ָ�����,���ڱ����ϵĺ�����ַ
NTTERMINATEPROCESS pOldNtTerminateProcess;

NTQUERYSYSTEMINFORMATION pOldNtQuerySystemInformation;


//��ý�����(���ڿ�ִ���ļ������� xxx.exe)
PUCHAR PsGetProcessImageFileName(__in PEPROCESS Process);


ULONG g_PIDHideArray[MAX_PROCESS_ARRARY_LENGTH];//���ؽ���ID����
ULONG g_PIDProtectArray[MAX_PROCESS_ARRARY_LENGTH];//��������ID����

//���صĺ��ܱ���������
ULONG g_currHideArrayLen = 0;
ULONG g_currProtectArrayLen = 0;

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING pRegistryPath);

NTSTATUS SSDTGeneralDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);//�ַ���������
NTSTATUS SSDTCreateDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS SSDTCloseDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS SSDTReadDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS SSDTWriteDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS SSDTDeviceIoControlDispatcher(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

VOID SSDTDriverUnload(IN PDRIVER_OBJECT pDriverObject);

//�������� SSDT �����еľɵķ������ĵ�ַ
ULONG oldSysServiceAddr[MAX_SYSTEM_SERVICE_NUMBER];

//��ֹд�뱣����Ҳ���ǻָ���ֻ��
VOID DisableWriteProtect(ULONG oldAttr);

//����д�뱣����Ҳ��������Ϊ��д
VOID EnableWriteProtect(PULONG pOldAttr);

//���� SSDT ������ϵͳ����ĵ�ַ
VOID BackupSysServicesTable();

//��װ Hook
NTSTATUS InstallSSDTHook(ULONG oldService, ULONG newService);

//��� Hook
NTSTATUS UnInstallSSDTHook(ULONG oldService);

//��֤ uPID ������Ľ����Ƿ���������ؽ����б��У����ж� uPID ��������Ƿ���Ҫ����
ULONG ValidateProcessNeedHide(ULONG uPID);

//��֤ uPID ������Ľ����Ƿ�����ڱ��������б��У����ж� uPID ��������Ƿ���Ҫ����
ULONG ValidateProcessNeedProtect(ULONG uPID);

//�����ؽ����б��в��� uPID
ULONG InsertHideProcess(ULONG uPID);

//�����ؽ����б����Ƴ� uPID
ULONG RemoveHideProcess(ULONG uPID);

//�����������б��в��� uPID
ULONG InsertProtectProcess(ULONG uPID);

//�����ؽ����б����Ƴ� uPID
ULONG RemoveProtectProcess(ULONG uPID);

//ע����̻ص�����
VOID CreateProcessNotifyEx(
__inout PEPROCESS  Process,
__in HANDLE  ProcessId,
__in_opt PPS_CREATE_NOTIFY_INFO  CreateInfo
);

#endif