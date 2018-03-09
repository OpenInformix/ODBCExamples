

### Scenario: Connecting to a Informix server on AWS
* [Getting started AWS](https://www.hcltech.com/products-and-platforms/informix/informix-AWS/getting-started)
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

