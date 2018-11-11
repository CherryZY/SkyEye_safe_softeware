
#define MAX_FILE_PATH_LENGTH   1024
#define Name_Value_Number 3         //ֵ�Ե��������:filename,filepath,security_level

#define MATCHING_ERROR_TYPE_ONE CAREFILETABLE_LENGTH-1  //ƥ����ֵĴ�������
#define MATCHING_ERROR_TYPE_TWO CAREFILETABLE_LENGTH-2

#define NAME_VALUE_TYPE_FILENAME        0x1        //��������
#define NAME_VALUE_TYPE_FILEPATH        0x2
#define NAME_VALUE_TYPE_OWNER           0x3
#define NAME_VALUE_TYPE_SECURITY_LEVEL  0x4


typedef struct _NameValue{
	UCHAR name[100];
	ULONG type;
	PWCHAR value;
}NameValue,*PNameValue;
typedef  struct _Name
{
	WCHAR name[100];
	ULONG type;
	ULONG length;

}Name,*PName;

#define CAREFILETABLE_LENGTH 30
#define DEBUG  0x1
#define SF_LOG_PRINT( _dbgLevel, _string )  (FlagOn(SfDebug,(_dbgLevel)) ? DbgPrint _string : ((void)0))
#define  CR_DEBUG(flag,_string) ((flag==0x1)? DbgPrint _string:((void)0))


ULONG    CR_HashFunction(ULONG index);
VOID     CR_MyNodeLock(PHASH_TABLE_CAREFILE_NODE node);
VOID     CR_MyNodeUnLock(PHASH_TABLE_CAREFILE_NODE node);
VOID CR_CareFileTable_Insert(PCareFile carefile);
//����ǳ�ʼ�������ļ��Ĺ�ϣ�����ڱȽ����⣬���Ե�����Ϊһ������
VOID CR_CareFileTable_CleanUp();
VOID InitHashTable_CareFile();
NTSTATUS CR_SetCareFileInit(PIRP irp);
NTSTATUS  CR_StartInitCareFileHashTable(PIRP irp);
NTSTATUS CR_InitFinished(PIRP irp);
BOOLEAN  CR_CheckIfCare(PCareFile a);