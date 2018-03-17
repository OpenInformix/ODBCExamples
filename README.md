## Informix ODBC Applicatin Examples
Copyright (c) 2017 OpenInformix. All rights reserved.

Licensed under the Apache License, Version 2.0

## ODBCExamples
---------------
The source code for Informix ODCB Examples are at **src** directory. The **IfxOdbcSample1** directory has ODBC application build infrastructure; including a Visual Studio 2015 project.


### SSL configuration setup
Client side SSL configuration setup needed for drivers to connect Informix server on the cloud. 

### Scenario: Connecting to a Informix server on AWS

Please see the official page for more up-to-date information and it is [Getting started AWS](https://www.hcltech.com/products-and-platforms/informix/informix-AWS/getting-started)   
Let us consider a scenario we would like to run the sample application against an Informix server provisioned from AWS.  

- The database server may not have created a user that can be shared with applications.
- A client computer on the internet connecting to this server will be using SSL connection.


### Server Side Setup
SSH to the terminal system at AWS and do the following setup by using dbaccess
```bash
# The 'my-private-key1.pem' is the private key file you have created for your AWS
ssh -i ~/.ssh/my-private-key1.pem centos@ec2-54-196-209-131.compute-1.amazonaws.com
sudo -u informix bash
dbaccess 


CREATE   database   db1  with log;

create user dbuser1 with password 'mypwd123' properties USER nobody authorization (DBSA);

GRANT CONNECT TO dbuser1;
GRANT DBA     TO dbuser1;
```

Copy the following two files to your client computer
```bash
/home/informix/client_ssl/client.kdb
/home/informix/client_ssl/client.sth
```


### Client Side Setup
On your client computer copy **client.kdb** and **client.sth** to a secure location   
let us say you have decided to put it at **c:\informix\etc\ssh** on your windows client and **$INFORMIXDIR/etc/ssh** on your Linux client. Then update the information in **$INFORMIXDIR/etc/conssl.cfg** of your client computer. The **$INFORMIXDIR** is the locaiton where you have installed Informix Client SDK.

##### Windows: c:\informix\etc\conssl.cfg
```
SSL_KEYSTORE_FILE c:\informix\etc\ssl\client.kdb
SSL_KEYSTORE_STH  c:\informix\etc\ssl\client.sth
```


##### Linux: $INFORMIXDIR/etc/conssl.cfg
```
SSL_KEYSTORE_FILE $INFORMIXDIR/etc/ssh/client.kdb
SSL_KEYSTORE_STH  $INFORMIXDIR/etc/ssh/client.sth
```


### You are ready to run your application ((ODBC, OLE DB, ESQLC, .NET, Noede.js, Python, R)
Here is a sample connection string that you may use it for ODBC application to connect to the database. The same setup is applicable for application using these the folloing Informix drivers   
ODBC, .NET, Noede.js, Python, R, ESQLC and OLE DB.

```C
char   *MyLocalConnStr = 

"DRIVER={IBM INFORMIX ODBC DRIVER (64-bit)};HOST=ec2-54-196-209-131.compute-1.amazonaws.com;\
SERVER=ol_aws;SERVICE=9089;PROTOCOL=olsocssl;DATABASE=db1;UID=dbuser1;PWD=mypwd123;\
CLIENT_LOCALE=en_us.8859-1;DB_LOCALE=en_us.utf8";
```



### Reference links
-------------------
* [ODBC Fundamentals](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/odbc-fundamentals)
* [C Data Types in ODBC](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/c-data-types-in-odbc)
* [Column-Wise Binding](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/column-wise-binding)
* [Row-Wise Binding](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/row-wise-binding)
* [SQLGetDiagRec and SQLGetDiagField](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/using-sqlgetdiagrec-and-sqlgetdiagfield)
* [Using SQLBindCol](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/using-sqlbindcol)
* [Converting Data from SQL to C Data Types](https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/converting-data-from-sql-to-c-data-types)
* [Transactions in ODBC](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/transactions-in-odbc-odbc)
* [Character Data and C Strings](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/character-data-and-c-strings)
* [Date, Time, and Timestamp Literals](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/date-time-and-timestamp-literals)
* [Data Length, Buffer Length, and Truncation](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/data-length-buffer-length-and-truncation)
* [Writing ODBC 3.x Applications](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/writing-odbc-3-x-applications)
* [Asynchronous Execution](https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/asynchronous-execution)


