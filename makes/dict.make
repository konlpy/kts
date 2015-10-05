# 
#
# mkdict.make : MakeDictionary from dict.freq to DBM Dictinary
#
#
CC = cc 
#CFLAGS = -DDEBUG
CFLAGS = -O2 -I/usr/include/db1 -lndbm

all : mkdict lkdict excdict getdict putsem putmarker predict chkcon

mkdict : mkdict.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o mkdict mkdict.c ktsutil.c kimmocode.c

lkdict : lkdict.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o lkdict lkdict.c ktsutil.c kimmocode.c

excdict : excdict.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o excdict excdict.c ktsutil.c kimmocode.c

getdict : getdict.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o getdict getdict.c ktsutil.c kimmocode.c

predict : predict.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o predict predict.c ktsutil.c kimmocode.c

putsem : putsem.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o putsem putsem.c ktsutil.c kimmocode.c

putmarker : putmarker.c ktsutil.c kimmocode.c
	$(CC) $(CFLAGS) -o putmarker putmarker.c ktsutil.c kimmocode.c

chkcon : chkcon.c
	$(CC) $(CFLAGS) -o chkcon chkcon.c


