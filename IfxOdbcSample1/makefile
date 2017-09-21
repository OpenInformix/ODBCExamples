
#gcc -g -m64 -I/work/csdk/incl/cli -c IfxOdbcSample1.c
#gcc -g -m64 -o IfxOdbcSample1.exe IfxOdbcSample1.o -L/work/csdk/lib/cli -lifdmr -lthcli


INFORMIXDIR=/work/csdk
ODBCLIB_DIR  = -L$(INFORMIXDIR)/lib/cli   -lthcli

INCLDIR  = -I$(INFORMIXDIR)/incl/cli
CFLAGS  = -g 
CC = gcc
RM = rm

TARGET = IfxOdbcSample1
  
all : $(TARGET)

$(TARGET) : $(TARGET).o
	$(CC) $(CFLAGS)  -m64 -o $(TARGET).exe $(TARGET).o  $(ODBCLIB_DIR)

$(TARGET).o : $(TARGET).c
	$(CC) $(CFLAGS) -m64 $(INCLDIR) -c $(TARGET).c

clean :
	$(RM) $(TARGET).exe $(TARGET).o


	