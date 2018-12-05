/////////////////////////////////////////////////////////////
// Informix ODBC Applicatin Examples
// Copyright (c) 2017 OpenInformix. All rights reserved.
// Licensed under the Apache License, Version 2.0
//
// Authors:
//      Sathyanesh Krishnan
//
// ODBC SELECT with Row and Column wise Bind. 
// Then each case retrieving values by using SQLFetchScroll and SQLExtendedFetch.

// Row-Wise Binding
// https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/row-wise-binding
//
// Column-Wise Binding
// https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/column-wise-binding
//
// ODBC Asynchronous Execution
// https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/asynchronous-execution

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

#define MY_NULL_VALUE  " NULL      "

#define TOTAL_NUM_ROWS 100
#define ROW_READ_CHUNK 25
#define C2_CharX_SIZE  15
#define C4_CharX_SIZE  18


enum AppFetchType
{
    ExtendedFetch, FetchScroll
};


SQLRETURN  MyServerSetup(SQLHDBC hdbc);

SQLRETURN  RowWiseBinding(SQLHDBC hdbc, enum AppFetchType FetchType);
SQLRETURN  ColumnWiseBinding(SQLHDBC hdbc, enum AppFetchType FetchType);
void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag);


int main(long argc, char* argv[])
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
    GetDiagRec(rc, SQL_HANDLE_ENV, henv, "SQL_ATTR_ODBC_VERSION TO SQL_OV_ODBC3");

    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    GetDiagRec(rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle: SQL_HANDLE_DBC");


    printf("\n\n\n***********************************************************");
    printf("\n Connection String passed to SQLDriverConnect is : \n");
    printf((char *)ConnStrIn);
    printf("\n***********************************************************\n\n");

    rc = SQLDriverConnect(hdbc, NULL, ConnStrIn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");

    if (rc != SQL_SUCCESS)
    {
        printf("\n SQLDriverConnect Failed  \n");
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return(-1);
    }


    printf("Connected!\n");

    printf("\n DbSetup ...");
    //////// Server side setup ///////
    MyServerSetup(hdbc);

    /////// RowWiseBinding //////////
    printf("\n RowWiseBinding with FetchScroll...");
    rc = RowWiseBinding(hdbc, FetchScroll);
    printf("  %s ", (rc == 0) ? "Success!" : "**Failed**");

    printf("\n RowWiseBinding with ExtendedFetch...");
    rc = RowWiseBinding(hdbc, ExtendedFetch);
    printf("  %s ", (rc == 0) ? "Success!" : "**Failed**");

    /////// ColumnWiseBinding //////////
    printf("\n ColumnWiseBinding with FetchScroll...");
    rc = ColumnWiseBinding(hdbc, FetchScroll);
    printf("  %s ", (rc == 0) ? "Success!" : "**Failed**");

    printf("\n ColumnWiseBinding with ExtendedFetch...");
    rc = ColumnWiseBinding(hdbc, ExtendedFetch);
    printf("  %s ", (rc == 0) ? "Success!" : "**Failed**");
    /////////////////////////

    rc = SQLDisconnect(hdbc);

    rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    rc = SQLFreeHandle(SQL_HANDLE_ENV, henv);


    printf("\n\n End!");

    return (0);
}


SQLRETURN  MyServerSetup(SQLHDBC hdbc)
{
    SQLRETURN   ErrorLevel = 0;
    char        buff[1024];
    SQLRETURN   rc = 0;
    SQLHSTMT    hstmt;
    int         i = 0;


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    //////// Server side setup SQLs ///////
    rc = SQLExecDirect(hstmt, (SQLCHAR *)"DROP TABLE mytab1", SQL_NTS);


    sprintf(buff,
        "CREATE TABLE mytab1 ( C1_Int INT, C2_CharX char(%d),  C3_Flt FLOAT, C4_CharX char(%d) )",
        C2_CharX_SIZE, C4_CharX_SIZE);

    rc = SQLExecDirect(hstmt, (SQLCHAR *)buff, SQL_NTS);
    printf("\n%s", buff);
    rc ? ++ErrorLevel : 0;


    for (i = 1; i <= TOTAL_NUM_ROWS; ++i)
    {
        sprintf(buff,
            "INSERT INTO mytab1 VALUES ( %d, 'C2_Val_%07d',  %f, 'C4_Val_%09d' )",
            i, i, (0.205 + i), i);

        rc = SQLExecDirect(hstmt, (SQLCHAR *)buff, SQL_NTS);
        printf("\n[%d] %s", rc, buff);
        rc ? ++ErrorLevel : 0;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    printf("\n\n");

    return(ErrorLevel);
}



SQLRETURN  RowWiseBinding(SQLHDBC hdbc, enum AppFetchType FetchType)
{
    char        *MyBindType = "RowWiseBinding";
    char        *ApiUsed = NULL;
    SQLRETURN   rc = 0;
    SQLHSTMT    hstmt;
    int         i = 0;

    // Define the MyRowInfo_t struct and allocate an array of TOTAL_NUM_ROWS element
    typedef struct
    {
        SQLUINTEGER   C1_Int;
        SQLLEN        C1_Int_Indicator;

        SQLCHAR       C2_CharX[C2_CharX_SIZE + 1];
        SQLLEN        C2_CharX_Indicator;

        SQLDOUBLE     C3_Flt;
        SQLLEN        C3_Flt_Indicator;

        SQLCHAR       C4_CharX[C4_CharX_SIZE + 1];
        SQLLEN        C4_CharX_Indicator;
    } MyRowInfo_t;

    MyRowInfo_t     MyRowInfo_Array[ROW_READ_CHUNK];
    SQLUSMALLINT    RowStatusArray[ROW_READ_CHUNK];

    SQLULEN         NumRowsFetched;
    int RecordNum = 0;
    int TotalFetchCalls = 0;

    if (FetchType == FetchScroll)
    {
        ApiUsed = "SQLFetchScroll()";
    }
    else
    {
        ApiUsed = "SQLExtendedFetch()";
    }
    printf("\n\n\n\n START: %s   with  %s", MyBindType, ApiUsed);
    printf("\n{");


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    // Specify the size of the structure with the SQL_ATTR_ROW_BIND_TYPE
    // statement attribute. This also declares that row-wise binding will be used.
    // Declare the rowset size with the SQL_ATTR_ROW_ARRAY_SIZE statement attribute.
    // Set the SQL_ATTR_ROW_STATUS_PTR statement attribute to point to the row status array.
    // Set the SQL_ATTR_ROWS_FETCHED_PTR statement attribute to point to NumRowsFetched.
    rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(MyRowInfo_t), 0);

    if (FetchType == FetchScroll)
    {
        //What the Driver Manager Does with SQL_ATTR_ROW_ARRAY_SIZE setting.
        //http://msdn.microsoft.com/en-us/library/windows/desktop/ms711801(v=vs.85).aspx
        //Sets the rowset size. The following are implementation details:
        //When an application sets this in an ODBC 2.x driver, the ODBC 3.x Driver Manager maps it to the SQL_ROWSET_SIZE statement attribute.
        //When an application sets this in an ODBC 3.x driver, the ODBC 3.x Driver Manager passes the call to the driver.
        //When an application working with an ODBC 3.x driver calls SQLSetScrollOptions,
        //SQL_ROWSET_SIZE is set to the value in the RowsetSize argument if the underlying driver does not support SQLSetScrollOptions.
        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)ROW_READ_CHUNK, 0);

        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0);
        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &NumRowsFetched, 0);
    }
    else
    {
        // If you are using SQLExtendedFetch rather than SQLFetchScroll to fetch the data,
        rc = SQLSetStmtAttr(hstmt, SQL_ROWSET_SIZE, (SQLPOINTER)ROW_READ_CHUNK, 0);
    }


    // Bind Colums
    rc = SQLBindCol(hstmt, 1, SQL_C_ULONG, &MyRowInfo_Array[0].C1_Int, 0, &MyRowInfo_Array[0].C1_Int_Indicator);
    rc = SQLBindCol(hstmt, 2, SQL_C_CHAR, MyRowInfo_Array[0].C2_CharX, sizeof(MyRowInfo_Array[0].C2_CharX), &MyRowInfo_Array[0].C2_CharX_Indicator);
    rc = SQLBindCol(hstmt, 3, SQL_C_DOUBLE, &MyRowInfo_Array[0].C3_Flt, 0, &MyRowInfo_Array[0].C3_Flt_Indicator);
    rc = SQLBindCol(hstmt, 4, SQL_C_BINARY, MyRowInfo_Array[0].C4_CharX, sizeof(MyRowInfo_Array[0].C4_CharX), &MyRowInfo_Array[0].C4_CharX_Indicator);

    rc = SQLExecDirect(hstmt, "SELECT C1_Int, C2_CharX, C3_Flt, C4_CharX FROM mytab1", SQL_NTS);
    GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "SQLExecDirect");

    while (TRUE)
    {
        if (FetchType == FetchScroll)
        {
            if ((rc = SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 0)) == SQL_NO_DATA)
            {
                break;
            }
        }
        else
        {
            NumRowsFetched = 0;
            if ((rc = SQLExtendedFetch(hstmt, SQL_FETCH_NEXT, ROW_READ_CHUNK, &NumRowsFetched, RowStatusArray)) == SQL_NO_DATA)
            {
                break;
            }
        }

        if (rc < 0)
        {
            GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, ApiUsed);
            break;
        }

        ++TotalFetchCalls;

        for (i = 0; i < NumRowsFetched; i++)
        {
            if (RowStatusArray[i] == SQL_ROW_SUCCESS ||
                RowStatusArray[i] == SQL_ROW_SUCCESS_WITH_INFO)
            {
                ++RecordNum;
                printf("\n #%d\t", RecordNum);

                if (MyRowInfo_Array[i].C1_Int_Indicator == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                    printf("%d\t", MyRowInfo_Array[i].C1_Int);


                if (MyRowInfo_Array[i].C2_CharX_Indicator == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                    printf("[%s]\t", MyRowInfo_Array[i].C2_CharX);


                if (MyRowInfo_Array[i].C3_Flt_Indicator == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                    printf("%6.3f\t", MyRowInfo_Array[i].C3_Flt);


                if (MyRowInfo_Array[i].C4_CharX_Indicator == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                {
                    unsigned char   TmpBuff[C4_CharX_SIZE + 1];

                    memcpy(TmpBuff, MyRowInfo_Array[i].C4_CharX, C4_CharX_SIZE);
                    TmpBuff[C4_CharX_SIZE] = 0;

                    printf("[%s]", TmpBuff);
                }
            }
        }
    }
    if (TOTAL_NUM_ROWS == RecordNum)
    {
        rc = 0;
    }
    else
    {
        rc = -1;
    }

    printf("\n----------------------------------------------");
    printf("\n %s   by using  %s", MyBindType, ApiUsed);
    printf("\n Expected Number of Records      = %d", TOTAL_NUM_ROWS);
    printf("\n Total Number of Records got     = %d", RecordNum);
    printf("\n Exp #Records in single fetch    = %d", ROW_READ_CHUNK);
    printf("\n Number of %s calls = %d", ApiUsed, TotalFetchCalls);
    printf("\n Summary : %s", (rc == 0) ? "Success!" : "**Fail**");
    printf("\n------------------end--------------------------");
    printf("\n}");

    SQLCloseCursor(hstmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return (rc);
}



SQLRETURN  ColumnWiseBinding(SQLHDBC hdbc, enum AppFetchType FetchType)
{
    //http://msdn.microsoft.com/en-us/library/windows/desktop/ms713541(v=vs.85).aspx
    char        *MyBindType = "ColumnWiseBinding";
    char        *ApiUsed = NULL;
    SQLRETURN   rc = 0;
    SQLHSTMT    hstmt;
    int         i = 0;

    SQLUINTEGER     C1_Int_Array[ROW_READ_CHUNK];
    SQLLEN          C1_Int_Indicator_Array[ROW_READ_CHUNK];

    SQLCHAR         C2_CharX_Array[ROW_READ_CHUNK][C2_CharX_SIZE + 1];
    SQLLEN          C2_CharX_Indicator_Array[ROW_READ_CHUNK];

    SQLDOUBLE       C3_Flt_Array[ROW_READ_CHUNK];
    SQLLEN          C3_Flt_Indicator_Array[ROW_READ_CHUNK];

    SQLCHAR         C4_CharX_Array[ROW_READ_CHUNK][C4_CharX_SIZE + 1];
    SQLLEN          C4_CharX_Indicator_Array[ROW_READ_CHUNK];

    SQLUSMALLINT    RowStatusArray[ROW_READ_CHUNK];

    SQLULEN         NumRowsFetched = 0;
    int             RecordNum = 0;
    int             TotalFetchCalls = 0;

    if (FetchType == FetchScroll)
    {
        ApiUsed = "SQLFetchScroll()";
    }
    else
    {
        ApiUsed = "SQLExtendedFetch()";
    }

    printf("\n\n\n\n START: %s   with  %s", MyBindType, ApiUsed);
    printf("\n{");


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if (FetchType == FetchScroll)
    {
        // Set the SQL_ATTR_ROW_BIND_TYPE statement attribute to use
        // column-wise binding. Declare the rowset size with the
        // SQL_ATTR_ROW_ARRAY_SIZE statement attribute. Set the
        // SQL_ATTR_ROW_STATUS_PTR statement attribute to point to the
        // row status array. Set the SQL_ATTR_ROWS_FETCHED_PTR statement
        // attribute to point to cRowsFetched.
        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)SQL_BIND_BY_COLUMN, 0);
        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)ROW_READ_CHUNK, 0);
        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)RowStatusArray, 0);
        rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &NumRowsFetched, 0);
    }
    else
    {
        rc = SQLSetStmtAttr(hstmt, SQL_ROWSET_SIZE, (SQLPOINTER)ROW_READ_CHUNK, 0);
    }

    // Bind arrays to the OrderID, SalesPerson, and Status columns.
    rc = SQLBindCol(hstmt, 1, SQL_C_ULONG, C1_Int_Array, 0, C1_Int_Indicator_Array);
    rc = SQLBindCol(hstmt, 2, SQL_C_CHAR, C2_CharX_Array, sizeof(C2_CharX_Array[0]), C2_CharX_Indicator_Array);
    rc = SQLBindCol(hstmt, 3, SQL_C_DOUBLE, C3_Flt_Array, 0, C3_Flt_Indicator_Array);
    rc = SQLBindCol(hstmt, 4, SQL_C_BINARY, C4_CharX_Array, sizeof(C4_CharX_Array[0]), C4_CharX_Indicator_Array);

    // Execute a statement
    rc = SQLExecDirect(hstmt, "SELECT C1_Int, C2_CharX, C3_Flt, C4_CharX FROM mytab1", SQL_NTS);
    GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "SQLExecDirect");


    while (TRUE)
    {
        if (FetchType == FetchScroll)
        {
            if ((rc = SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 0)) == SQL_NO_DATA)
            {
                break;
            }
        }
        else
        {
            NumRowsFetched = 0;
            if ((rc = SQLExtendedFetch(hstmt, SQL_FETCH_NEXT, ROW_READ_CHUNK, &NumRowsFetched, RowStatusArray)) == SQL_NO_DATA)
            {
                break;
            }
        }
        if (NumRowsFetched == 0)
        {
            break;
        }

        if (rc < 0)
        {
            GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, ApiUsed);
            break;
        }

        ++TotalFetchCalls;

        for (i = 0; i < NumRowsFetched; i++)
        {
            if ((RowStatusArray[i] == SQL_ROW_SUCCESS) ||
                (RowStatusArray[i] == SQL_ROW_SUCCESS_WITH_INFO))
            {
                ++RecordNum;
                printf("\n #%d\t", RecordNum);

                if (C1_Int_Indicator_Array[i] == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                    printf("%d\t", C1_Int_Array[i]);


                if (C2_CharX_Indicator_Array[i] == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                    printf("[%s]\t", C2_CharX_Array[i]);


                if (C3_Flt_Indicator_Array[i] == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                    printf("%6.3f\t", C3_Flt_Array[i]);


                if (C4_CharX_Indicator_Array[i] == SQL_NULL_DATA)
                    printf(MY_NULL_VALUE);
                else
                {
                    unsigned char   TmpBuff[C4_CharX_SIZE + 1];

                    memcpy(TmpBuff, C4_CharX_Array[i], C4_CharX_SIZE);
                    TmpBuff[C4_CharX_SIZE] = 0;
                    printf("[%s]", TmpBuff);
                }
            }
        }
    }

    if (TOTAL_NUM_ROWS == RecordNum)
    {
        rc = 0;
    }
    else
    {
        rc = -1;
    }

    printf("\n----------------------------------------------");
    printf("\n %s   by using  %s", MyBindType, ApiUsed);
    printf("\n Expected Number of Records        = %d", TOTAL_NUM_ROWS);
    printf("\n Total Number of Records got       = %d", RecordNum);
    printf("\n Exp #Records in single fetch      = %d", ROW_READ_CHUNK);
    printf("\n Number of %s calls = %d", ApiUsed, TotalFetchCalls);
    printf("\n Summary : %s", (rc == 0) ? "Success!" : "**Fail**");
    printf("\n------------------end--------------------------");
    printf("\n}");

    SQLCloseCursor(hstmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    return (rc);
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
