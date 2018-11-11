




#define FILE_NAME_LENGTH_MAX 512
#define FILE_PATH_LENGTH_MAX 1024

#define FILE_UNKOWN FILE_DEVICE_DISK_FILE_SYSTEM
#define IOCTL_DEVICE_WRITE CTL_CODE(FILE_UNKOWN,0x808,METHOD_IN_DIRECT,FILE_ANY_ACCESS)
#define IOCTL_DEVICE_READ CTL_CODE(FILE_UNKOWN,0x807,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)
#define IOCTL_DEVICE_PRINT CTL_CODE(FILE_UNKOWN,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
//#define IOCTL_GET_OPERATION_INFORMATION CTL_CODE(FILE_UNKOWN,0x810,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)
#define IOCTL_QUERY_REGISTRY_KEY CTL_CODE(FILE_UNKOWN,0x811,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)

#define IOCTL_VERIFY_OPERATION_INFORMATION CTL_CODE(FILE_UNKOWN,0x812,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)//���ڴ��ں���ȡ����Ҫ��֤�Ĳ�����Ϣ
#define IOCTL_SEND_VERIFY_RESULT CTL_CODE(FILE_UNKOWN,0x813,METHOD_IN_DIRECT,FILE_ANY_ACCESS) //���ڷ�����֤����Ϣ
#define IOCTL_EXCEPTION_BUFFER_LENGTH_NOT_ENOUGH CTL_CODE(FILE_UNKOWN,0X814,METHOD_IN_DIRECT,FILE_ANY_ACCESS)//�����������������ĳ��Ȳ�����������������ڴ�

#define IOCTL_CAREFILE_INIT  CTL_CODE(FILE_UNKOWN,0x815,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)//���ڳ�ʼ�������ļ�����
#define IOCTL_CAREFILE_START_INIT  CTL_CODE(FILE_UNKOWN,0x816,METHOD_IN_DIRECT,FILE_ANY_ACCESS)//��ʼ��ʼ��
#define IOCTL_CAREFILE_INIT_SHUTDOWN   CTL_CODE(FILE_UNKOWN,0x817,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)//�ں��Ѿ���ʼ�����

#define FILE_READ 0x1
#define FILE_MODIFY 0x2
#define FILE_REMOVE 0x3                                                                                                       
#define FILE_CREATE 0x4
#define FILE_WRITE 0x5

#define EXCEPTION_NONE_RECORD 0x001           //û�м�¼
#define EXCEPTION_APPLICATION_ERROR 0x002     //�ڲ����������Ϣ��¼��message��
#define EACH_NUMBER 5                       //CareFileTransfer���ͽṹ�ж���CareFileNode�ṹ

typedef struct _operationinrecord   //���ݸ��û������ҵȴ��䷵��
{
	ULONG OperationID;    
	BOOLEAN lock;              //�ļ��Ƿ����
//	int length;             //�ļ��ĳ���
	HANDLE event;           //event object�������¼����󣬳��������ʱ��ʹ��
	HANDLE forback;         //�����������ں�����ȴ��û�����Ĵ��������¼�
	ULONG  operationtype;
	BOOLEAN operation_access;  //�����Ƿ�����
	WCHAR result[512];       //�������ϸ����Ϣ
	WCHAR filename[FILE_NAME_LENGTH_MAX];     //�ļ���
	WCHAR filepath[FILE_PATH_LENGTH_MAX];     //�ļ���·��
	WCHAR otherinfo[512];    //������Ϣ
	BOOLEAN   exception;        //�����Ƿ��������
	ULONG  exceptiontype;
	WCHAR  exceptionmessage[128]; //�������Ϣ
}OperationRecord,*POperationRecord;  //������¼�ṹ���������ڴ��ݸ�App�Ľṹ��Ҳ����Ӧ�ó����½����жϵ���Ҫ����

typedef struct _CareFile_T
{
	WCHAR filename[FILE_NAME_LENGTH_MAX];
	WCHAR filepath[FILE_PATH_LENGTH_MAX];
    INT            secutiry_level;
	ULONG          Owner;
}CareFile_T,*PCareFile_T;
typedef struct _CareFileTransfer
{
	ULONG    number;   //��¼��ǰ�ṹ�ж���CareFile�ṹ
	CareFile_T filenode[EACH_NUMBER]; //������׵�ַ
}CareFileTransfer,*PCareFileTransfer;
typedef struct _operationresult  //��¼��������������ظ��ں�
{
	LIST_ENTRY entry;
	ULONG operationID;
	bool operation_permit;
	WCHAR otherinfo[512];
	ULONG operationtype; 
	bool  wasFill;   //�Ƿ���д��
}OperationResult,*POperationResult;



// TODO: reference additional headers your program requires here