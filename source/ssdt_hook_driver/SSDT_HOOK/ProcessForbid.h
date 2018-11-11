#ifndef PROCESSFORBID_H
#define PROCESSFORBID_H

#include <ntddk.h>
#include <stdlib.h>

#define PROCESSNAME_LENGTH 50

//�û�������Ҫ��ֹ�򿪵Ľ��̲������ֹ������
#define IOCTL_SSDT_FORBIDPROCESS_ADD\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x854, METHOD_BUFFERED, \
	FILE_WRITE_ACCESS | FILE_READ_ACCESS)

//�û�������Ҫ��ֹ�򿪵Ľ��������Ƴ���ֹ������
#define IOCTL_SSDT_FORBIDPROCESS_DELETE\
	CTL_CODE(\
	FILE_DEVICE_UNKNOWN, \
	0x855, METHOD_BUFFERED, \
	FILE_WRITE_ACCESS | FILE_READ_ACCESS)

//�洢��Ҫ��ֹ�Ľ�������
typedef struct _ForbidProcess{
	LIST_ENTRY listEntry;
	WCHAR		ProcessName[PROCESSNAME_LENGTH];
}ForbidProcess,*PForbidProcess;



BOOLEAN InsertForbidProcessList(PForbidProcess fP);

BOOLEAN RemoveForbidProcessList(PForbidProcess fP);

#endif