/////////////////////////////////////////////////////////////
// Informix ODBC Applicatin Examples
// Copyright (c) 2017 OpenInformix. All rights reserved.
// Licensed under the Apache License, Version 2.0
//
// Authors:
//      Sathyanesh Krishnan
//
// Odbc Array Insert with Colum wise Parameter Binding


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


#define MY_ARRY_SIZE    3

#define COL_SIZE_C2    20
#define COL_SIZE_C3    30


typedef struct
{
    // Layout for creating array of column values and cb for each param.
    SQLINTEGER      Values_c1[MY_ARRY_SIZE];
    SQLLEN          cbValues_c1[MY_ARRY_SIZE];

    SQLCHAR         Values_c2[MY_ARRY_SIZE][COL_SIZE_C2+1];
    SQLLEN          cbValues_c2[MY_ARRY_SIZE];

    SQLCHAR         Values_c3[MY_ARRY_SIZE][COL_SIZE_C3+1];
    SQLLEN          cbValues_c3[MY_ARRY_SIZE];
}  ColumnWiseData_t;


typedef struct
{
    // Status of the processed parameters bound 
    SQLULEN ParamsProcessed;
    SQLUSMALLINT ParamStatusArray[MY_ARRY_SIZE];
} ArryInsertStatus;


void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szFunTag);
int SetColumnWiseParamBinding(SQLHSTMT hstmt);
void FillData1( ColumnWiseData_t *pMyData1 );
int VerifyRecordCount(SQLHSTMT hstmt, char *TableName, int ExpCount );


int main( int argc, char *argv[] )
{
    SQLHENV         henv=NULL;
    SQLHDBC         hdbc=NULL;

    SQLRETURN       rc = 0;
    SQLCHAR         ConnStrIn[1024]="DSN=odbc1";
    

    if( argc == 1 )
    {
        char *MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER};SERVER=myids1;DATABASE=db1;HOST=myhost.ibm.com;PROTOCOL=onsoctcp;SERVICE=5550;UID=user1;PWD=xyz;";

        if( sizeof (int *) == 8 )
        {
            MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER (64-bit)};SERVER=myids1;DATABASE=db1;HOST=myhost.ibm.com;PROTOCOL=onsoctcp;SERVICE=5555;UID=user1;PWD=xyz;";
        }

        strcpy( (char *)ConnStrIn, MyLocalConnStr );
    }
    else if( argc == 2 )
    {
        // Add buffer overflow protection if taken into production
        sprintf( (char *)ConnStrIn, "DSN=%s", argv[1] );
    }
    else
    {
        printf( "\n Unknown command line option!");
        printf( "\n");
        return(0);
    }


    rc = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv );

    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3,0);
    GetDiagRec( rc, SQL_HANDLE_ENV, henv, "SQL_ATTR_ODBC_VERSION TO SQL_OV_ODBC3" );

    rc = SQLAllocHandle( SQL_HANDLE_DBC, henv, &hdbc );
    GetDiagRec( rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle: SQL_HANDLE_DBC" );


    printf( "\n\n\n***********************************************************");
    printf( "\n Connection String passed to SQLDriverConnect is : \n" );
    printf(  (char *)ConnStrIn );
    printf( "\n***********************************************************\n\n");

    rc = SQLDriverConnect( hdbc, NULL, ConnStrIn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
    GetDiagRec( rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect" );

    if (rc != SQL_SUCCESS)
    {
        printf( "\n SQLDriverConnect Failed, fix connection problem.... \n");
        exit(1);
    }


    //////////////////// Actual Demo ////////////
    {
        int       RunFailures = 0;
        
        if( 1 ) // To turn on Insert Cursor
        {
            // FYI: SQL_INFX_ATTR_ENABLE_INSERT_CURSORS is specific to Informix
            rc = SQLSetConnectAttr(hdbc, SQL_INFX_ATTR_ENABLE_INSERT_CURSORS, (SQLPOINTER)1, 0);
        }

        {
            SQLHSTMT  hstmt = NULL;

            rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
            RunFailures += SetColumnWiseParamBinding(hstmt);

            rc = SQLFreeStmt(hstmt, SQL_CLOSE);
            rc = SQLFreeStmt(hstmt, SQL_UNBIND);
            rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
            rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
        

        if (RunFailures == 0 ) 
        {
            printf( "\nAll Good !\n\n" );
        }
        else
        {
            printf( "\n*** Number of failure detected is=%d **** \n\n", RunFailures);
        }

    }    ///////////////////////////////////////////


    rc = SQLDisconnect(hdbc);
    rc = SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
    rc = SQLFreeHandle(SQL_HANDLE_ENV,henv);

    return(0);
}


void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag )
{
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode=0;
    SQLSMALLINT length=0;

    if ( szMsgTag == NULL )
    {
        szMsgTag = "---";
    }

    printf( "\n %s: %d : ", szMsgTag, rc);
    if (rc == 0)
    {
       printf( "[rc=%d] OK \n", rc);
    }
    else
    {
        int i=1;

        if (rc == 1)
            printf( " WARNING : %d", rc);
        else
            printf( " FAILED : %d", rc);

        while (SQLGetDiagRec(htype,
                 hndl,
                 i,
                 sqlstate,
                 &sqlcode,
                 message,
                 SQL_MAX_MESSAGE_LENGTH + 1,
                 &length) == SQL_SUCCESS)
        {
             printf( "\n SQLSTATE          = %s", sqlstate);
             printf( "\n Native Error Code = %ld", sqlcode);
             printf( "\n %s", message);
             i++;
        }
        printf( "\n-------------------------\n");
    }
}

void CreateData( char *buff, char *tag, int size)
{
    int i=0;
    char *cps = tag;
    char *cpt = buff;

    if( size < 1 )
        return;

    memset( buff, 0, size);

    for ( i=0; *cps && i<size-1; ++i)
    {
        *cpt++ = *cps++;
    }

    for ( ; i<size-1; ++i)
    {
        *cpt++ = 'A';
    }
}


void FillData1( ColumnWiseData_t *pMyData1 )
{
    int i=0;

    memset( pMyData1, 0, sizeof(ColumnWiseData_t) );

    for( i=0; i<MY_ARRY_SIZE; ++i)
    {
        char tag[32];
        int ArryIndex=i+1;
        int size=0;

        pMyData1->Values_c1[i] = (ArryIndex + 100);


        sprintf((char *) pMyData1->Values_c3[i], "Val_C3-%03d", ArryIndex );

        pMyData1->cbValues_c1[i] = 0;


        ////////// c2 /////////
        size = COL_SIZE_C2;
        sprintf( tag, "Val_C2-%03d-", ArryIndex );
        CreateData( pMyData1->Values_c2[i], tag, size);
        size = (int)strlen( pMyData1->Values_c2[i] );

        // data for the parameter will be sent with SQLPutData
        pMyData1->cbValues_c2[i] = SQL_LEN_DATA_AT_EXEC( size ); 

        pMyData1->cbValues_c2[i] = SQL_NTS; 

        ////////// c3 /////////
        size = COL_SIZE_C3;
        sprintf( tag, "Val_C3-%03d", ArryIndex );
        CreateData( pMyData1->Values_c3[i], tag, size);
        size = (int)strlen( pMyData1->Values_c3[i] );
        pMyData1->cbValues_c3[i] = SQL_NTS;
    }
}





int VerifyRecordCount(SQLHSTMT hstmt, char *TableName, int ExpCount )
{
    SQLRETURN   rc = 0;
    char        SqlSelect[256];
    SQLINTEGER  RecCount=0;
    SQLLEN      StrLen_or_IndPtr=0;

    sprintf( SqlSelect, "SELECT COUNT(*) FROM %s", TableName);

    rc = SQLExecDirect (hstmt, SqlSelect, SQL_NTS);
    if ( !(rc ==  SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) )
    {
        GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "FAILED: SQLExecDirect" );
    }

    if ( (rc = SQLFetch(hstmt)) == SQL_NO_DATA)
    {
        GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "FAILED: SQLFetch" );
    }

    if ( (rc = SQLGetData(hstmt, 1, SQL_C_SLONG, &RecCount, sizeof(RecCount), &StrLen_or_IndPtr)) == SQL_NO_DATA)
    {
        GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "FAILED: SQLGetData" );
    }

    rc = SQLFreeStmt( hstmt, SQL_CLOSE);
    rc = SQLFreeStmt( hstmt, SQL_UNBIND);
    rc = SQLFreeStmt( hstmt, SQL_RESET_PARAMS);

    rc = 0;
    if ( ExpCount != RecCount )
    {
        printf( "FAILED: Record count %d is not matching with expected count %d", RecCount, ExpCount);
        rc = 1;
    }

    return( rc );
}



int SetColumnWiseParamBinding(SQLHSTMT hstmt )
{
    ColumnWiseData_t             ObjMyData1={0};
    ColumnWiseData_t             *pObjMyData = &ObjMyData1;
    SQLRETURN           rc = 0;
    int                 ErrorStatus=0;
    ArryInsertStatus    ObjArryInsertStatus = {0};
    ArryInsertStatus    *pObjArryInsertStatus = &ObjArryInsertStatus;
    char                *sql = NULL;


    if( 1 )  // if needed initial Setup
    {
        sql = "drop   table foo";
        rc = SQLExecDirect( hstmt, sql,    SQL_NTS);
        printf( "\n [%d] %s", rc, sql);

        sql = "create table foo ( c1 int, c2 varchar(30), c3 varchar(30) )";
        //sql = "create table foo ( c1 int, c2 text,        c3 text )";
        rc = SQLExecDirect( hstmt, sql,    SQL_NTS);
        printf( "\n [%d] %s", rc, sql);
    }

    //////// Data In //////////
    FillData1( pObjMyData );

    sql = "insert into foo ( c3, c2, c1) values (?,?,?)";
    rc = SQLPrepare( hstmt, sql, SQL_NTS);

    rc = SQLSetStmtAttr( hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)((SQLLEN) MY_ARRY_SIZE), SQL_IS_INTEGER);
    GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "SQL_ATTR_PARAMSET_SIZE" );

    rc = SQLSetStmtAttr( hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, (SQLPOINTER)&(pObjArryInsertStatus->ParamsProcessed), SQL_IS_INTEGER );
    GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "SQL_ATTR_PARAMS_PROCESSED_PTR" );

    rc = SQLSetStmtAttr(hstmt, SQL_ATTR_PARAM_STATUS_PTR,(SQLPOINTER)pObjArryInsertStatus->ParamStatusArray, SQL_IS_POINTER);
    GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "SQL_ATTR_PARAM_STATUS_PTR" );

    rc = SQLBindParameter( hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR,  SQL_VARCHAR, SQL_NTS,  0, pObjMyData->Values_c3, COL_SIZE_C3, pObjMyData->cbValues_c3);
    rc = SQLBindParameter( hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,  SQL_VARCHAR, SQL_NTS,  0, pObjMyData->Values_c2, COL_SIZE_C2, pObjMyData->cbValues_c2);
    rc = SQLBindParameter( hstmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, SQL_NTS,  0, pObjMyData->Values_c1, 0,           pObjMyData->cbValues_c1);




    rc = SQLExecute(hstmt);
    GetDiagRec( rc, SQL_HANDLE_STMT, hstmt, "SQLExecute" );

    rc = SQLFreeStmt( hstmt, SQL_CLOSE);
    rc = SQLFreeStmt( hstmt, SQL_UNBIND);
    rc = SQLFreeStmt( hstmt, SQL_RESET_PARAMS);

    ///////////// Verify Rec Count  ///////////////
    {
        int ExpCount = MY_ARRY_SIZE;

        ErrorStatus += VerifyRecordCount( hstmt, "foo", ExpCount );
    }


    if (ErrorStatus > 0)
        ErrorStatus=1;

    printf("\n");
    return(ErrorStatus);
}



