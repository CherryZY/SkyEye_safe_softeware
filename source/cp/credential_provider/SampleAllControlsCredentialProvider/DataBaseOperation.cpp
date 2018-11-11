#include "DriverOperation.h"
#include "DataBaseOperation.h"
#include "resource.h"
#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include "winsock.h"
#include <mysql.h>



BOOL DataBaseOperation::OpenDB(CHAR *chu, CHAR *chp,CHAR *dbhost,CHAR *dbname){
	BOOL Opened = FALSE;
	MYSQL * con;
	con = mysql_init((MYSQL*)0);
	if (con != NULL &&
		mysql_real_connect(con, dbhost, chu, chp, dbname, 3306/*TCP IP�˿�*/, NULL/*Unix Socket ��������*/, 0/*���г�ODBC���ݿ��־*/))
	{
		Opened = TRUE;
	}
	mysql_close(con);
	return Opened;
}

//��ʼ�½��������󣬲����������д�������û����������
BOOL DataBaseOperation::OperateDriver(){
	return FALSE;
}