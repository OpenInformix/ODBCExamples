/////////////////////////////////////////////////////////////
// Informix ODBC Applicatin Examples
// Copyright (c) 2017 OpenInformix. All rights reserved.
// Licensed under the Apache License, Version 2.0
//
// Authors:
//      Sathyanesh Krishnan

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

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>


//////////////////// Globals ////////////////////

SQLCHAR *MyLongData []=
{
    "This[\n] is Sample Data For Testing BLOB Data\n!",
    "Aaaaa aaaaaaaaaaa aaaaaaaaa aaaaaaaaA",
    "Bbbbbbbbbbbbbbb bbbbbbbbbbbbbbB",
    "Ccccccccc cccccccccccc cccccccccccc ccccccccccccccC",
    "This is the last line.",
    NULL
};
 
#define TEST_DATA_FILE_SIZE     (1000*100)


#define GET_DATA_BUFF_SIZE             (1024*32)
#define LO_WRITE_BUFF_SIZE             (1024*32)

#define GET_DATA_WITH_SMALL_BUFF       32
#define FULL_BUFF4_SMALL_BUFF_READ     1024

SQLCHAR     *InsertStmt = "INSERT INTO tab1(id, c2 ) VALUES(?, ?)";
SQLCHAR     *SqlSelect  = "SELECT c2 FROM tab1 WHERE id=1"; 
SQLCHAR     szColumnName[32] = "tab1.C2";

/////////////////////////////////////////////////

int GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag );
int OdbcBlobInsert( SQLHENV henv, SQLCHAR *ConnStr, BOOL UnKnownDataSize, BOOL UseExecDirect );
int OdbcBlobSelect( SQLHENV henv, SQLCHAR *ConnStr );
int OdbcBlobSelect_File( SQLHENV henv, SQLCHAR *ConnStr, char *FileName );
int LoBlobInsert_File( SQLHENV henv, SQLCHAR *ConnStr, char *FileName );
int VerifyFiles( const char *FnameIn, const char *FnameOut);
int CreateDataFile( char *FileName, long long DataFileSize );



int main( int argc, char *argv[] )
{
    char *FileNameOut = "c:\\TMP\\my_blob.out.txt";
    char *FileNameOrg =  "c:\\TMP\\my_blob.txt";
    
	//char *FileNameOut = "/work/my_blob.out.txt";
    //char *FileNameOrg =  "/work/run/my_blob.txt";
	
	
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

    rc = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv );
    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3,0);


    //////////////////////////////////////
    CreateDataFile( FileNameOrg, TEST_DATA_FILE_SIZE );

    //OdbcBlobInsert( henv, ConnStrIn, TRUE, TRUE );
    //OdbcBlobSelect( henv, ConnStrIn );

    LoBlobInsert_File(    henv, ConnStrIn, FileNameOrg );
    OdbcBlobSelect_File(  henv, ConnStrIn, FileNameOut );
    if ( VerifyFiles( FileNameOrg, FileNameOut) != 1 )
    {
        printf( "\n Error: VerifyFiles");
    }

    /////////////////////////////////////
    rc = SQLFreeHandle(SQL_HANDLE_ENV,henv);

   return(0);
}

int GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag )
{ 
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode=0;
    SQLSMALLINT length=0;
    int         ErrSt=0;

    if ( szMsgTag == NULL )
    {
        szMsgTag = "---";
    }


    if (rc == 0)
    {
        printf( "\n[OK, rc=%d]  %s: ", rc, szMsgTag);
    }
    else if (rc > 0)
    {
        printf( "\n[W, rc=%d]  %s: ", rc, szMsgTag);
    }
    else
    {  
       int i=1;

       ErrSt=1;
       printf( "\n[FAILED, rc=%d]  %s: ", rc, szMsgTag);
       while (SQLGetDiagRec(htype,
                hndl,
                i,
                sqlstate,
                &sqlcode,
                message,
                SQL_MAX_MESSAGE_LENGTH,
                &length) == SQL_SUCCESS)
       {
            printf( "\n SQLSTATE          = %s", sqlstate);
            printf( "\n Native Error Code = %ld", sqlcode);
            printf( "\n %s", message);
            i++;
       }
       printf( "\n-------------------------\n");
    }
    return( ErrSt );
}


int OdbcBlobSelect( SQLHENV henv, SQLCHAR *ConnStr  )
{
    SQLRETURN       rc=0;
    SQLHDBC         hdbc=NULL;
    SQLHSTMT        hstmt=NULL;
    int             ErrorLevel=0;
    SQLCHAR         *ReadBuffer;
    const int       ReadBufferSize = GET_DATA_WITH_SMALL_BUFF;
    const int       FullDataBuffSize = FULL_BUFF4_SMALL_BUFF_READ;
    unsigned char   *FullDataBuff=NULL;
    unsigned char   *ExpData=NULL;

    int             ColReadCount=0;

    ReadBuffer = malloc( ReadBufferSize+4 );
    memset( ReadBuffer, 0, ReadBufferSize+4 );


    rc = SQLAllocHandle( SQL_HANDLE_DBC, henv, &hdbc );
    GetDiagRec ( rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle");

    if( (rc = SQLDriverConnect( hdbc, NULL, ConnStr, 
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT )) != SQL_SUCCESS)
    {
        GetDiagRec ( rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto Exit;
    }

    // Set SQL_INFX_ATTR_LO_AUTOMATIC 
    rc = SQLSetConnectAttr( hdbc, SQL_INFX_ATTR_LO_AUTOMATIC, (SQLPOINTER)SQL_TRUE, (SQLINTEGER)0 );

    //rc= SQLSetConnectAttr( hdbc, SQL_INFX_ATTR_DEFAULT_UDT_FETCH_TYPE, (SQLPOINTER) SQL_FALSE, 0);  

    FullDataBuff = malloc( FullDataBuffSize );
    ExpData = malloc( FullDataBuffSize );
    
    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    GetDiagRec ( rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");


    rc = SQLExecDirect (hstmt, SqlSelect, SQL_NTS);
    if ( (rc ==  SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)  )
    {
        SQLSMALLINT     ColumnCount=0;

        rc = SQLNumResultCols( hstmt, &ColumnCount);
                     
        while ((rc = SQLFetch(hstmt)) != SQL_NO_DATA) 
        {
            SQLLEN StrLen_or_IndPtr=0;
            int NumBytes=0;
            int col=0;

            for( col=1; col<=ColumnCount; ++col)
            {
                void    *vp = (void *)ReadBuffer;
                int     TotalData =  0;
                int     TotalExpData =  0;
                int     i=0;
                int     CompRes=0;

                memset( ReadBuffer, 0, ReadBufferSize );
                memset( FullDataBuff, 0, FullDataBuffSize );
                StrLen_or_IndPtr=0;
        

                //////////////////////////////
                while ((rc = SQLGetData(hstmt, col, SQL_C_BINARY, ReadBuffer, ReadBufferSize, &StrLen_or_IndPtr)) != SQL_NO_DATA) 
                {
                    if ( rc < 0 ) 
                    {
                        GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, "SQLGetData"  );
                        break;
                    }

                    NumBytes = (int)((StrLen_or_IndPtr > ReadBufferSize) || (StrLen_or_IndPtr == SQL_NO_TOTAL) ? ReadBufferSize : StrLen_or_IndPtr);

                    if( (TotalData + NumBytes) > FullDataBuffSize )
                    {
                        // Not enough space in the buffer
                        // Data overflow may happen
                        break;
                    }

                    memcpy( FullDataBuff+TotalData, ReadBuffer, NumBytes);
                    TotalData += NumBytes;

                    memset( ReadBuffer, 0, ReadBufferSize );
                }
                //////////////////////////


                
                memset( ExpData, 0, ReadBufferSize );
                TotalExpData =  0;
                for( i=0; MyLongData[i] != NULL; ++i)
                {
                    NumBytes = (int)strlen( MyLongData[i] );
                    if( (TotalExpData + NumBytes) > FullDataBuffSize )
                    {
                        // Not enough space in the buffer
                        break;
                    }
                    memcpy( ExpData+TotalExpData, MyLongData[i], NumBytes);
                    TotalExpData += NumBytes;
                }

                if( TotalData != TotalExpData )
                {
                    printf( "\n Mismatch in total data length" );
                    printf( "\n Expected=%d, Actual=%d ", TotalData, TotalExpData );
                }
                else
                {
                    //let us compare the data content.
                    CompRes = memcmp( FullDataBuff, ExpData, TotalData);
                    if( CompRes == 0 )
                    {
                        printf( "\n Data Compared and found OK! " );
                    }
                    else
                    {
                        printf( "\n Data not matching" );
                    }
                    // printf("\n [%s]\n", FullDataBuff);
                }
            }
        }
    }
    else
    {
        GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, SqlSelect  );
    }

    Exit:
    if ( ReadBuffer )
    {
        free(ReadBuffer);
    }

    if (FullDataBuff )
    {
        free(FullDataBuff);
        FullDataBuff = NULL;
    }
    if (ExpData )
    {
        free(ExpData);
        ExpData = NULL;
    }
    
    if ( hstmt )
    {
        SQLFreeStmt(hstmt,SQL_CLOSE);
        rc = SQLFreeStmt( hstmt, SQL_UNBIND);
        rc = SQLFreeStmt( hstmt, SQL_RESET_PARAMS);
        SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
    }
    
    if( hdbc )
    {
        rc = SQLDisconnect(hdbc);
        rc = SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
    }

    return (ErrorLevel);
}


int OdbcBlobInsert( SQLHENV henv, SQLCHAR *ConnStr, BOOL UnKnownDataSize, BOOL UseExecDirect )
{
    SQLHDBC hdbc=NULL;
    SQLHSTMT hstmt = NULL;
    const void *DataAtExecParmId = (void *)1; // Set a unique value for this param.
    char DataBuff[1024];

    SQLSMALLINT id_data=0;

    SQLLEN cb_Id=0;
    SQLLEN cb_c2=0;

    SQLPOINTER paramData = NULL;
    SQLRETURN rc=0;
    int ErrorLevel=0;
    char *CreateSql = NULL;
    int TotalDataLen=0;
    int i=0;

    DataBuff[0] = 0;

    rc = SQLAllocHandle( SQL_HANDLE_DBC, henv, &hdbc );
    GetDiagRec ( rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle");

    if( (rc = SQLDriverConnect( hdbc, NULL, ConnStr, 
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT )) != SQL_SUCCESS)
    {
        GetDiagRec ( rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto Exit;
    }

    // Set SQL_INFX_ATTR_LO_AUTOMATIC 
    rc = SQLSetConnectAttr( hdbc, SQL_INFX_ATTR_LO_AUTOMATIC, (SQLPOINTER)SQL_TRUE, (SQLINTEGER)0 );


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    rc = SQLExecDirect(hstmt, "DROP TABLE tab1", SQL_NTS);
    CreateSql = "CREATE TABLE tab1(id INT, c2 BLOB)";
    printf( "\n %s \n", CreateSql);

    rc = SQLExecDirect(hstmt, CreateSql, SQL_NTS);
    if( rc<0 ) 
    {
        printf( "\n FAILED: at setup phase, test not run" );
        GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, CreateSql );
        ++ErrorLevel;
        goto Exit;
    }

    if( UseExecDirect == FALSE )
    {
        rc = SQLPrepare(hstmt, (SQLCHAR *)InsertStmt, SQL_NTS );
    }

    cb_Id=0;
    id_data=1;
    rc= SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &id_data, 0, &cb_Id);
    
    
    if( UnKnownDataSize == TRUE )
    {
        cb_c2 = SQL_DATA_AT_EXEC;
    }
    else
    {
        // KnownDataSize: Calculate the total data size
        TotalDataLen = 0;
        for( i=0; MyLongData[i] != NULL; ++i)
        {
            TotalDataLen += (int)strlen( MyLongData[i] );
        }
        cb_c2 = SQL_LEN_DATA_AT_EXEC(TotalDataLen);
    }
    
    rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, sizeof(DataBuff), 0, (void *)DataAtExecParmId, 0, &cb_c2);

    if( UseExecDirect )
    {
        rc = SQLExecDirect( hstmt, InsertStmt, SQL_NTS);
    }
    else
    {
        rc = SQLExecute(hstmt);
    }

    // Application has specified data will be given at the time of execution
    if(SQL_NEED_DATA == rc) 
    {
        rc = SQLParamData(hstmt, &paramData);
        if ( DataAtExecParmId == paramData ) // Identify the parameter that need more data
        {
            int DataLen=0;
            TotalDataLen=0;

            for( i=0; MyLongData[i] != NULL; ++i)
            {
                strcpy( DataBuff, MyLongData[i]);
                 
                DataLen = (int)strlen(DataBuff);
                TotalDataLen += DataLen;

                rc = SQLPutData(hstmt, DataBuff, DataLen);
                if(!SQL_SUCCEEDED(rc)) 
                {
                    ++ErrorLevel;
                    GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, "SQLPutData"  );
                    break;
                }
            }

            // The final SQLParamData to tell the driver done with this long data
            rc = SQLParamData(hstmt, &paramData);
            if( rc<0 ) 
            {
               ++ErrorLevel;
                GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, "SQLParamData"  );
            }
        }
    }

Exit:
    if ( hstmt )
    {
        rc = SQLFreeStmt(hstmt,SQL_CLOSE);
        rc = SQLFreeStmt( hstmt, SQL_UNBIND);
        rc = SQLFreeStmt( hstmt, SQL_RESET_PARAMS);
        rc = SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
    }
    
    if( hdbc )
    {
        rc = SQLDisconnect(hdbc);
        rc = SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
    }

    return (ErrorLevel);
}


int OdbcBlobSelect_File( SQLHENV henv, SQLCHAR *ConnStr, char *FileName )
{
    SQLHDBC         hdbc=NULL;
    SQLHSTMT        hstmt=NULL;
    SQLRETURN       rc=0;
    int             ErrorLevel=0;
    SQLCHAR         *ReadBuffer = NULL; 
    int             ReadBufferSize = GET_DATA_BUFF_SIZE;
    long long       NumByteWritten=0;
    long            NumGetDataCalls=0;

    int             i=0;

    FILE *fpOut=NULL;


    printf( "\n\n\nReading Smart Large Object");
    ReadBuffer = malloc( ReadBufferSize+4 );
    memset( ReadBuffer, 0, ReadBufferSize+4 );

    

    rc = SQLAllocHandle( SQL_HANDLE_DBC, henv, &hdbc );
    GetDiagRec ( rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle");

    if( (rc = SQLDriverConnect( hdbc, NULL, ConnStr, 
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT )) != SQL_SUCCESS)
    {
        GetDiagRec ( rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto Exit;
    }

    // Set SQL_INFX_ATTR_LO_AUTOMATIC 
    rc = SQLSetConnectAttr( hdbc, SQL_INFX_ATTR_LO_AUTOMATIC, (SQLPOINTER)SQL_TRUE, (SQLINTEGER)0 );


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc==0 ? 0 : GetDiagRec ( rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");


    printf( "\n Starting OdbcBlobSelect2File...");

    rc = SQLExecDirect (hstmt, SqlSelect, SQL_NTS);

    if ( (rc ==  SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)  )
    {
        if ( (fpOut=fopen( FileName, "wb" )) != NULL )
        {
                
            printf( "\n Starting SQLGetData calls, Please Wait ...");
                         
            while ((rc = SQLFetch(hstmt)) != SQL_NO_DATA) 
            {
                SQLLEN StrLen_or_IndPtr=0;
                int NumBytes=0;

                if ( rc <= SQL_ERROR ) 
                {
                    GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, SqlSelect  );
                    break;
                }
                
                while ( 1 ) 
                {
                    memset( ReadBuffer, 0, ReadBufferSize );
                    StrLen_or_IndPtr=0;

                    // _OdbcIntBlob2ExtBinary(
                    rc = SQLGetData(hstmt, 1, SQL_C_BINARY, ReadBuffer, ReadBufferSize, &StrLen_or_IndPtr);
                    if ( rc == SQL_NO_DATA) 
                    {
                        break;
                    }
                    if ( rc <= SQL_ERROR ) 
                    {
                        GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, SqlSelect  );
                        break;
                    }

                    NumBytes = (int)((StrLen_or_IndPtr > ReadBufferSize) || (StrLen_or_IndPtr == SQL_NO_TOTAL) ? ReadBufferSize : StrLen_or_IndPtr);
                    ++NumGetDataCalls;
                   
                    for( i=0; i<NumBytes; ++i )
                    {
                        if( ReadBuffer[i] == 0 )
                        {
                            break;
                        }

                        fputc( ReadBuffer[i], fpOut);
                        ++NumByteWritten;
                    }

                   if ( NumGetDataCalls % 500 == 0 )
                   {
                       printf( "." );
                   }
                }
            }

            printf( "\nTotal number of SQLGetData calls = %d", NumGetDataCalls);

            fclose(fpOut);
            fpOut=NULL;
        }
        else
        {
            ++ErrorLevel;
            printf( "\n FAILED: to open Out data file %s \n", FileName);
        }
    }
    else
    {
        GetDiagRec ( rc, SQL_HANDLE_STMT, hstmt, SqlSelect  );
    }

Exit:
    if ( ReadBuffer )
    {
        free(ReadBuffer);
    }

    if ( hstmt )
    {
        SQLFreeStmt(hstmt,SQL_CLOSE);
        rc = SQLFreeStmt( hstmt, SQL_UNBIND);
        rc = SQLFreeStmt( hstmt, SQL_RESET_PARAMS);
        SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
    }

    if( hdbc )
    {
        rc = SQLDisconnect(hdbc);
        rc = SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
    }


    printf( "\n %lld Byte Written to the Out file \n %s  ", NumByteWritten, FileName);

    return (ErrorLevel);
}


int VerifyFiles( const char *FnameIn, const char *FnameOut)
{
    int rc=0;
    FILE *fpIn=NULL;
    FILE *fpOut=NULL;
    int ErrorLevel=0;
    long long NumByteRead=0;
    unsigned int c1=0;
    unsigned int c2=0;
    
    int eof1=0;
    int eof2=0;

    printf( "\n\n\n VerifyFiles Data, Please Wait...");

    if( (fpIn=fopen( FnameIn, "rb" )) == NULL )
    {
        ++ErrorLevel;
        goto Exit;
    }

    if( (fpOut=fopen( FnameOut, "rb" )) == NULL )
    {
        ++ErrorLevel;
        goto Exit;
    }

    while( eof1==0 && eof2==0 )
    {
        c1 = fgetc(fpIn);
        if( feof(fpIn) )
        { 
           eof1=1;
        }

        c2 = fgetc(fpOut);
        if( feof(fpOut) )
        { 
           eof2=1;
        }
        ++NumByteRead;

        if( c1 != c2 )
        {
            ++ErrorLevel;
            printf( "\n FAILED: Data mismatch");
            break;
        }

        if( eof1 != eof2 )
        {
            ++ErrorLevel;
            printf( "\n FAILED: EOF mismatch eof1=%d, eof2=%d", eof1, eof2);
            break;
        }

        if( NumByteRead % 1000000 == 0)
        {
            printf(".");
        }
    }
    printf( "\n Total Byte Compared %lld ", NumByteRead);
    if ( ErrorLevel == 0)
    {
        rc = 1;
        printf( "\n Identical Data" );
    }
    else
    {
        printf( "\n Data is not identical");
        rc = 0;
    }


Exit:
    if( fpIn )
        fclose( fpIn );

    if( fpOut )
        fclose( fpOut );

    return(rc);
}



int LoBlobInsert_File(SQLHENV henv, SQLCHAR *ConnStr, char *FileName)
{
	int             ErrorLevel = 0;
	SQLHDBC         hdbc = NULL;
	SQLHSTMT        hstmt = NULL;
	SQLRETURN       rc = 0;

	void            *pLoSpec = NULL;
	void            *pLoPtr = NULL;
	SQLSMALLINT     siLoSpecSize = 0;
	SQLSMALLINT     siLoptrSize = 0;
	SQLLEN          LoFD = 0;


	printf("\n\n\nInserting Smart Large Object");
	rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	GetDiagRec(rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle");

	if ((rc = SQLDriverConnect(hdbc, NULL, ConnStr,
		SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT)) != SQL_SUCCESS)
	{
		GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
		++ErrorLevel;
		goto Exit;
	}

	rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (rc <= SQL_ERROR)
	{
		GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");
		++ErrorLevel;
		goto Exit;
	}

	rc = SQLExecDirect(hstmt, "DROP TABLE tab1;", SQL_NTS);
	rc = SQLExecDirect(hstmt, "CREATE TABLE tab1(id INT, c2 BLOB)", SQL_NTS);
	if (rc <= SQL_ERROR)
	{
		GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "CREATE TABLE");
		++ErrorLevel;
		goto Exit;
	}

	// Get the size of a smart large object specification structure
	rc = SQLGetInfo(hdbc, SQL_INFX_LO_SPEC_LENGTH, &siLoSpecSize, sizeof(siLoSpecSize), NULL);
	ErrorLevel += GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLGetInfo:SQL_INFX_LO_SPEC_LENGTH");


	// STEP 1: creates a smart-large-object specification structure
	if (!ErrorLevel)
	{
		// creates a smart-large-object specification structure and initializes the fields to null values.
		// If you do not change these values, the null values tell the database server to use 
		// the system-specified defaults for the storage characteristics of the smart large object.
		SQLLEN          cbLoDef = 0;


		// allocate memory for smart-large-object specification structure 
		pLoSpec = malloc(siLoSpecSize);
		memset(pLoSpec, 0, siLoSpecSize);

		// Parameter-1 Smart-large-object specification structure
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT_OUTPUT,
			SQL_C_BINARY, SQL_INFX_UDT_FIXED, (SQLULEN)siLoSpecSize, 0,
			pLoSpec, (SQLLEN)siLoSpecSize, &cbLoDef);

		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 1 -- SQLBindParameter-P1");

		rc = SQLExecDirect(hstmt, "{call ifx_lo_def_create_spec(?)}", SQL_NTS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 1 -- SQLExecDirect ifx_lo_def_create_spec");

		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	}


	// STEP 2: Initialise the smart large object specification structure 
	if (!ErrorLevel)
	{
		SQLLEN          cbColumnName = SQL_NTS;
		SQLLEN          cbLoSpec = siLoSpecSize;

		// Parameter-1: Pointer to a buffer that contains the name of a database column 
		// eg : database@server_name:table.column 
		// owner name also can be included if it is ANSI database. 
		// eg: database@server_name:owner.table.column 
		//  szColumnName[32] = "tab1.C2";
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			sizeof(szColumnName), 0, szColumnName, sizeof(szColumnName), &cbColumnName);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 2 -- SQLBindParameter-P1");

		// Parameter-2: smart-large-object specification structure
		rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT_OUTPUT, SQL_C_BINARY, SQL_INFX_UDT_FIXED,
			(SQLULEN)siLoSpecSize, 0, pLoSpec, (SQLLEN)siLoSpecSize, &cbLoSpec);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 2 -- SQLBindParameter-P2");

		// Call SP to updates a smart-large-object specification structure 
		// with column-level storage characteristics
		rc = SQLExecDirect(hstmt, (SQLCHAR *) "{call ifx_lo_col_info(?, ?)}", SQL_NTS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 2 -- SQLExecDirect ifx_lo_col_info");

		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 2 -- SQLFreeStmt");
	}

	// STEP 3:  
	// Get the size of the smart large object structure pointer
	// Create and Open a new smart large object. 
	if (!ErrorLevel)
	{
		SQLINTEGER      AccessMode = LO_RDWR;
		SQLLEN          cbAccessMode = 0;
		SQLLEN          cbLoPtr = 0;
		SQLLEN          cbLoSpec = siLoSpecSize;
		SQLLEN          cbLoFD = 0;


		// Get the size of the smart large object pointer structure
		rc = SQLGetInfo(hdbc, SQL_INFX_LO_PTR_LENGTH, &siLoptrSize, sizeof(siLoptrSize), NULL);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_DBC, hstmt, "Step 3 -- SQL_INFX_LO_PTR_LENGTH");

		// Allocate memory for SLOB structure
		pLoPtr = malloc(siLoptrSize);
		memset(pLoPtr, 0, siLoptrSize);

		////// Creates and Opens a new smart large object //////

		// Parameter-1: Smart-large-object specification structure that 
		// contains storage characteristics for the new smart large object
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_INFX_UDT_FIXED,
			(SQLULEN)siLoSpecSize, 0, pLoSpec, (SQLLEN)siLoSpecSize, &cbLoSpec);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 3 -- SQLBindParameter-P1");


		// Parameter-2: Mode in which to open the new smart large object. For more information, 
		// Access Modes are LO_RDONLY, LO_DIRTY_READ, LO_WRONLY, LO_APPEND, LO_RDWR, LO_BUFFER, LO_NOBUFFER 
		// AccessMode = LO_RDWR;
		rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
			(SQLULEN)0, 0, &AccessMode, sizeof(AccessMode), &cbAccessMode);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 3 -- SQLBindParameter-P2");


		// Parameter-3: Smart-large-object pointer structure
		cbLoPtr = siLoptrSize;
		rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT_OUTPUT, SQL_C_BINARY, SQL_INFX_UDT_FIXED,
			(SQLULEN)siLoptrSize, 0,
			pLoPtr, siLoptrSize, &cbLoPtr);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 3 -- SQLBindParameter-P3");


		// Parameter-4: Smart-large-object file descriptor. 
		// This file descriptor is only valid within the current database connection.
		rc = SQLBindParameter(hstmt, 4, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER,
			(SQLULEN)0, 0, &LoFD, sizeof(LoFD), &cbLoFD);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 3 -- SQLBindParameter-P4");


		// creates and opens a new smart large object and 
		// Returns a file descriptor that identifies the smart large object
		rc = SQLExecDirect(hstmt, (SQLCHAR *) "{call ifx_lo_create(?, ?, ?, ?)}", SQL_NTS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 3 -- ifx_lo_create");

		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 3 -- SQLFreeStmt");
	}



	//////////////////////////////

	// STEP 4 : writes data to an open smart large object.
	if (!ErrorLevel)
	{
		struct stat     statbuf = { 0 };
		SQLLEN          DataToBeWritten = 0;
		SQLLEN          DataWritten = 0;
		SQLLEN          DataBuffSize = 0;
		SQLCHAR         *pDataBuff = NULL;
		SQLLEN          cbDataBuff = 0;
		SQLLEN          cbLoFD = 0;
		int             LoopCount = 0;
		int             fdData = 0;

		// Get the size of the file containing data for the new smart large object
		if (stat(FileName, &statbuf) == -1)
		{
			printf("Error %d reading %s\n", errno, FileName);
			++ErrorLevel;
		}

		DataToBeWritten = statbuf.st_size;
		DataBuffSize = (DataToBeWritten < LO_WRITE_BUFF_SIZE) ? DataToBeWritten : LO_WRITE_BUFF_SIZE;

		// Allocate a buffer to hold the smart large object data
		pDataBuff = malloc(DataBuffSize + 4);
		memset(pDataBuff, 0, (DataBuffSize + 4));

		// Smart-large-object file descriptor
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
			0, 0, &LoFD, sizeof(LoFD), &cbLoFD);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 4 -- SQLBindParameter-P1");


		// Buffer that contains the data that the function writes to the smart large object. 
		// The size of the buffer cannot exceed 2 gigabytes.
		cbDataBuff = DataBuffSize;
		rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
			DataBuffSize, 0, pDataBuff, DataBuffSize, &cbDataBuff);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 4 -- SQLBindParameter-P2)");

		// Prepare the SP call to writes data.
		rc = SQLPrepare(hstmt, (SQLCHAR *)"{call ifx_lo_write(?, ?)}", SQL_NTS);

		//////////////////////////////////////////////
		printf("\n");
		printf("\n Transporting %lld bytes of smart blob data", DataToBeWritten);
		if (DataToBeWritten > LO_WRITE_BUFF_SIZE)
		{
			SQLLEN NumIterations = DataToBeWritten / LO_WRITE_BUFF_SIZE;

			if (DataToBeWritten > (NumIterations*LO_WRITE_BUFF_SIZE))
			{
				++NumIterations;
			}

			printf(" in %lld iterations", NumIterations);
		}
		printf(", Please Wait.\n");


		// Open the input data file (of smart large object data from file)
		fdData = _open((char *)FileName, O_RDONLY | _O_BINARY);
		if (fdData == -1)
		{
			printf("Error %d creating file descriptor for %s\n", errno, FileName);
			exit(1);
		}

		/////////////////////////////////////////
		while (DataToBeWritten > 0)
		{
			size_t          ReadDataSize = 0;

			++LoopCount;
			cbDataBuff = (DataToBeWritten < DataBuffSize) ? DataToBeWritten : DataBuffSize;

			memset(pDataBuff, 0, DataBuffSize + 4);
			ReadDataSize = _read(fdData, pDataBuff, (unsigned int)cbDataBuff);
			if (ReadDataSize == 0)
			{
				break;
			}
			if (ReadDataSize < 0)
			{
				printf("Error %d reading %s\n", errno, FileName);
				break;
			}

			// cbDataBuff is being bind to the parm 2 for ifx_lo_write
			if (cbDataBuff != ReadDataSize)
			{
				cbDataBuff = ReadDataSize;
			}

			// Execute ifx_lo_write
			if ((rc = SQLExecute(hstmt)) != 0)
			{
				ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 4 -- SQLExecDirect ifx_lo_write");
			}
			if (ErrorLevel)
			{
				break;
			}

			DataWritten += ReadDataSize;
			DataToBeWritten -= ReadDataSize;

			if (LoopCount % 100 == 0)
				printf(".");
		}
		printf("\n Write to LOB Done \n");

		///////////////////////////
		if (_close(fdData) < 0)
		{
			printf("Error %d closing the file %s\n", errno, FileName);
			++ErrorLevel;
		}


		// Reset the statement parameters
		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 6 -- SQLFreeStmt");

		printf("\n Inserted DATA size : %ld byte \n", DataWritten);
	}

	// STEP 5: Insert the new smart large object into a row.
	if (!ErrorLevel)
	{
		SQLLEN      cb_Id = 0;
		SQLSMALLINT id_data = 0;
		SQLLEN      cbLoPtr = 0;

		cbLoPtr = siLoptrSize;

		cb_Id = 0;
		id_data = 1;
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &id_data, 0, &cb_Id);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 5 -- SQLBindParameter-P1");

		// The new smart large object ptr
		// pointer to SLOB structure which is bind to 
		// parm 3 of ifx_lo_create() at step 3
		rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_INFX_UDT_FIXED,
			(SQLULEN)siLoptrSize, 0, pLoPtr, siLoptrSize, &cbLoPtr);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 5 -- SQLBindParameter-P2");

		printf("\n Execute %s", InsertStmt);
		// InsertStmt = "INSERT INTO tab1(id, c2 ) VALUES(?, ?)";
		rc = SQLExecDirect(hstmt, InsertStmt, SQL_NTS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 5 -- SQLExecDirect INSERT");

		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 5 -- SQLFreeStmt");
	}

	//STEP 6.  Close the smart large object
	if (!ErrorLevel)
	{
		SQLLEN          cbLoFD = 0;

		// Parameter-1: Smart-large-object file descriptor
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
			(SQLULEN)0, 0, &LoFD, sizeof(LoFD), &cbLoFD);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 6 -- SQLBindParameter-P1");

		rc = SQLExecDirect(hstmt, (SQLCHAR *) "{call ifx_lo_close(?)}", SQL_NTS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 6 -- SQLExecDirect ifx_lo_close");

		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		ErrorLevel += GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "Step 6 -- SQLFreeStmt");
	}


Exit:
	if (hstmt)
	{
		rc = SQLFreeStmt(hstmt, SQL_CLOSE);
		rc = SQLFreeStmt(hstmt, SQL_UNBIND);
		rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}

	if (hdbc)
	{
		rc = SQLDisconnect(hdbc);
		rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	}

	if (pLoSpec)
	{
		free(pLoSpec);
	}

	if (pLoPtr)
	{
		free(pLoPtr);
	}


	return(rc);
}



int CreateDataFile( char *FileName, long long DataFileSize )
{
    FILE *fp=NULL;
    char Buff[256];
    int ErrorLevel=0;
    long long NumByteWritten=0;
    long Line =  0;
    unsigned int c1=0;
    unsigned char     *cp=NULL;
    
    int eof=0;


    if( (fp=fopen( FileName, "wb" )) == NULL )
    {
        ++ErrorLevel;
        goto Exit;
    }


    printf( "\n\n\n Creating Data File of %lld bytes, Please Wait...", DataFileSize);
    while( 1  )
    {
        if ( NumByteWritten >= DataFileSize)
        {
            break;
        }
        sprintf( Buff, "[Line %9d, Total Byte Written Prior To this Line is %10lld ] \n", ++Line, NumByteWritten);


        cp = Buff;
        while( *cp )
        {
            if ( NumByteWritten >= DataFileSize)
            {
                break;
            }

            fputc( *cp, fp);
            ++NumByteWritten;
            ++cp;
        }

        if ( Line % 10000 == 0 )
            printf( ".");

    }
    printf( "\n Data File=%s", FileName);
    printf( "\n Total Byte Writtern = %lld ", NumByteWritten);


Exit:
    if( fp )
        fclose( fp );

    if( ErrorLevel == 0 )
        return(1);

   return(0);
}
