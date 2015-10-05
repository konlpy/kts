#
#
# mkdict.make : MakeDictionary from dict.freq to DBM Dictinary
#
#
bin_PROGRAMS = mkdict lkdict excdict getdict putsem putmarker predict chkcon
LDADD = ../src/libkts.a -ldb
DEPENDENCIES=../src/libkts.a
AM_CFLAGS = -g -O2 -I/usr/include -I../include #-DDEBUG
CLEANFILES = kts_dict.db

dict:
	./mkdict	total.dict	kts_dict
	./mkdict	add.dict	kts_dict
	./excdict	exc.dict	kts_dict
	./predict	prefer.dict	kts_dict
