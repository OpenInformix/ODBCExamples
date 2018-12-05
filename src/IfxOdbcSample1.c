
/////////////////////////////////////////////////////////////
// Informix ODBC Applicatin Examples
// Copyright (c) 2017 OpenInformix. All rights reserved.
// Licensed under the Apache License, Version 2.0
//
// Authors:
//      Sathyanesh Krishnan

// IfxOdbcSample1.exe "DSN=odbc1"
// IfxOdbcSample1.exe  "DRIVER={IBM INFORMIX ODBC DRIVER (64-bit)};SERVER=ids5;DATABASE=db1;HOST=x.x.x.x;PROTOCOL=onsoctcp;SERVICE=5555;UID=user1;PWD=xyz;";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define strdup _strdup
#endif

#ifdef DRIVER_MANAGER 
#include "sql.h"
#include "sqlext.h"
#else
#include <infxcli.h>
#endif

void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag);
int ReadResult(SQLHDBC hdbc, char *SqlSelect);
void  MyServerSetup(SQLHDBC hdbc);


int main(int argc, char *argv[])
{
    SQLCHAR     ConnStrIn[1024] = "DSN=odbc_demo";
    SQLHANDLE   henv = NULL;
    SQLHANDLE   hdbc = NULL;
    int         rc = 0;

    char   *MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER};SERVER=srv1;DATABASE=xb1;HOST=xyz.abc.com;PROTOCOL=onsoctcp;SERVICE=5550;UID=user1;PWD=xyz;";

    if (argc == 1)
    {
        if (sizeof(int *) == 8)  // 64bit application 
        {
            // With SSL 
            // DB2CLI WITH SSL   = "DATABASE=db1;UID=user1;PWD=xyz;HOSTNAME=x.x.63.222;port=9091;SECURITY=SSL;"  
            // MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER (64-bit)};HOST=x.x.x.222;SERVER=informix;SERVICE=9089;PROTOCOL=olsocssl;DATABASE=db1;UID=user1;PWD=xyz;CLIENT_LOCALE=en_us.8859-1;DB_LOCALE=en_us.utf8";
            MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER (64-bit)};SERVER=ids5;DATABASE=db1;HOST=x.x.x.x;PROTOCOL=onsoctcp;SERVICE=5555;UID=user1;PWD=xyz;";
        }
        strcpy((char *)ConnStrIn, MyLocalConnStr);

    }
    else if (argc == 2)
    {
        strcpy( (char *)ConnStrIn,  argv[1] );
    }
    else
    {
        strcpy((char *)ConnStrIn, MyLocalConnStr);

        if (0)
        {
            printf("\n Usage option is :");
            printf("\n %s    <Connection String>", argv[0]);
            printf("\n Example :");
            printf("\n %s   \"DSN=MyOdbcDsnName; uid=MyUserName; pwd=MyPassword;\" ", argv[0]);
            printf("\n OR ");
            printf("\n %s  \"%s\" ", argv[0], MyLocalConnStr);
            printf("\n\n");
            exit(0);
        }
    }


    rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle");

    printf("\n***************************************************\n");
    printf("\n Connecting with : \n [%s] \n", (char *)ConnStrIn);
    printf("\n***************************************************\n");

    rc = SQLDriverConnect(hdbc, NULL, ConnStrIn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (rc != 0)
    {
        printf("\n Connection Error (:- \n", rc);
        GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto Exit;
    }
    else
    {
        printf("\n Connection Success! \n", rc);
    }

    if (1)
    {
        // Try Basic Setup 
        MyServerSetup(hdbc);
        ReadResult(hdbc, "SELECT * FROM t1");
    }


Exit:
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);

    return(0);
}


void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag)
{
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode = 0;
    SQLSMALLINT length = 0;

    if (szMsgTag == NULL)
    {
        szMsgTag = "---";
    }

    printf("\n %s: %d : ", szMsgTag, rc);
    if (rc >= 0)
    {
        printf(" OK [rc=%d] \n", rc);
    }
    else
    {
        int i = 1;
        printf(" FAILED : %i", rc);
        while (SQLGetDiagRec(htype,
            hndl,
            i,
            sqlstate,
            &sqlcode,
            message,
            SQL_MAX_MESSAGE_LENGTH + 1,
            &length) == SQL_SUCCESS)
        {
            printf("\n SQLSTATE          = %s", sqlstate);
            printf("\n Native Error Code = %ld", sqlcode);
            printf("\n %s", message);
            i++;
        }
        printf("\n-------------------------\n");
    }
}




void  MyServerSetup(SQLHDBC hdbc)
{
    char        buff[1024];
    SQLRETURN   rc = 0;
    SQLHSTMT    hstmt = NULL;    int         i = 0;

    static unsigned char *SetupSqls[] =
    {
        "DROP TABLE t1;",
        "CREATE TABLE t1 ( c1 INT, c2  char(15),  c3 FLOAT, c4 char(10) )",
        "INSERT INTO  t1 VALUES ( 1, 'aaa-1', 11.55, 'bbbb-1' );",
        "INSERT INTO  t1 VALUES ( 2, 'aaa-2', 12.55, 'bbbb-2' );",
        NULL,
    };

    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "MyServerSetup::SQLAllocHandle::SQL_HANDLE_STMT");

    for (i = 0; SetupSqls[i] != NULL; ++i)
    {
        rc = SQLExecDirect(hstmt, SetupSqls[i], SQL_NTS);
        printf("\n[%d] %s", rc, SetupSqls[i]);
    }


    if (hstmt)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
}


int ReadResult(SQLHDBC hdbc, char *SqlSelect)
{
    SQLRETURN       rc = 0;
    SQLHSTMT        hstmt = NULL;
    SQLCHAR         ReadBuffer[1024];
    int             ReadBufferSize = sizeof(ReadBuffer) - 2;

    printf("\n\n ----ReadResult ----");


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");


    rc = SQLExecDirect(hstmt, SqlSelect, SQL_NTS);
    if ((rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
    {
        SQLSMALLINT     ColumnCount = 0;
        int             RowNum = 0;

        rc = SQLNumResultCols(hstmt, &ColumnCount);
        printf("\nNumber of colum in the result is %d ---\n", ColumnCount);


        while ((rc = SQLFetch(hstmt)) != SQL_NO_DATA)
        {
            SQLLEN StrLen_or_IndPtr = 0;
            int NumBytes = 0;
            int col = 0;

            ++RowNum;
            printf("\n\n -Fetching Row# %d-", RowNum);

            for (col = 1; col <= ColumnCount; ++col)
            {

                memset(ReadBuffer, 0, sizeof(ReadBuffer));
                StrLen_or_IndPtr = 0;

                rc = SQLGetData(hstmt, col, SQL_C_CHAR, ReadBuffer, ReadBufferSize, &StrLen_or_IndPtr);
                if (rc == SQL_NO_DATA)
                {
                    break;
                }
                if (rc < 0)
                {
                    GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "SQLGetData");
                    break;
                }

                NumBytes = (int)((StrLen_or_IndPtr > ReadBufferSize) || (StrLen_or_IndPtr == SQL_NO_TOTAL) ? ReadBufferSize : StrLen_or_IndPtr);

                ReadBuffer[NumBytes] = 0;
                printf("\nColum_%d = %s", col, ReadBuffer);
            }
        }

    }
    else
    {
        GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, SqlSelect);
    }

    if (hstmt)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        rc = SQLFreeStmt(hstmt, SQL_UNBIND);
        rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    printf("\n");
    return (0);
}

