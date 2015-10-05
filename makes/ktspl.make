# libktspl.a : Korean Tagging System Prolog Library
#
#          Tau(Word_i) = argmax Pi P(T_i|T_(i-1)) *
#
#                                  P(W_i|T_i)
#
# -DDEBUG         : ���¼� �м� ������ �� �� �ִ�.
# -DDEBUG2        : Tagging ������ �� �� �ִ�.
#                   ���� tagging ����� �� �� �ִ�.
# -DDEBUG3        : UNKNOWNMODEL3���� Unknown-Word ������ ����
#                   ������ �� �� �ִ�.
# -DONLINE        : On-Line Mode�� ���� , if not : batch job
#
# -DPUTENDOFSTATEMENT : ������ ������ `EOS' symbol�� ����.
#
# -DUNKNOWNMODEL1 : Default : P(W|T) = 1.0 / (Freq(T) + 1)
#
# -DUNKNOWNMODEL2 : Unknown Word Model 2 : P(W|T) = 1.0���� ����
#
# -DUNKNOWNMODEL3 : ProbInvLex��� (unk-tag|right-lexical-feature)
#
# -DUNKNOWNMODEL3 -DCASE1 : ProbLex��� (right-lexical-feature|unk-tag)
#
# -DBASELING1     : Ư¡ ǰ�簡 ���̸� ��ü�� ���� ��������
#
# -DBASELING2     : ������ ǰ�翡�� �� ǰ�簡 ���� �� ������ Cutting
#
# -DANALYSIS      : ������ ����� ����� ������ش�.
#
# ���� -DANALYSIS�� �ϸ� ktsdemo : ��  -lm�� �ؾ� �Ѵ�.
#

CC = cc -I/usr/include/db1

RANLIB = ranlib

OBJS = kts.o ktsmoran.o ktstagger.o ktsformat.o ktsunknown.o \
	ktsirr.o ktsutil.o lookup.o kimmocode.o 

HDRS = ktsdefs.h ktsds.h ktstbls.h kts.h sem.h ktssyll.h \
	irrtbl.h kimmocode.h hantable.h ktslib.h

#CFLAGS = -DONLINE -DUNKNOWNMODEL2 -DPROLOG -DPROTO -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DPROLOG -DPROTO -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING1 -DPROLOG -DPROTO -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DBASELING1 -DPROLOG -DPROTO -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING2 -DPROLOG -DPROTO -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DBASELING2 -DPROLOG -DPROTO -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING1 -DBASELING2 -DPROLOG -DPROTO -DANALYSIS

CFLAGS = -O2 -DONLINE -DUNKNOWNMODEL3 -DBASELING1 -DBASELING2 -DPROLOG -DPROTO 

all : libktspl.a 

libktspl.a : $(OBJS) $(HDRS)
	rm -f libktspl.a
	ar q libktspl.a $(OBJS)
	$(RANLIB) libktspl.a

kts.o : ktsdefs.h ktsds.h ktstbls.h kts.h 
	$(CC) $(CFLAGS) -c kts.c
ktsmoran.o : ktsdefs.h ktstbls.h ktsds.h sem.h kimmocode.h kts.h
	$(CC) $(CFLAGS) -c ktsmoran.c
ktstagger.o : ktsdefs.h ktstbls.h ktsds.h kts.h
	$(CC) $(CFLAGS) -c ktstagger.c
ktsformat.o : ktsdefs.h ktstbls.h ktsds.h kts.h 
	$(CC) $(CFLAGS) -I$(SP_PATH)/Runtime -c ktsformat.c
ktsunknown.o : ktsdefs.h ktsds.h ktstbls.h kimmocode.h ktssyll.h 
	$(CC) $(CFLAGS) -c ktsunknown.c
ktsirr.o : ktsdefs.h ktstbls.h ktsds.h irrtbl.h kimmocode.h
	$(CC) $(CFLAGS) -c ktsirr.c
ktsutil.o : ktsdefs.h ktsds.h ktstbls.h sem.h
	$(CC) $(CFLAGS) -c ktsutil.c
lookup.o : ktsdefs.h ktsds.h 
	$(CC) $(CFLAGS) -I$(SP_PATH)/Runtime -c lookup.c
kimmocode.o : hantable.h kimmocode.h
	$(CC) $(CFLAGS) -c kimmocode.c

clean :
	/bin/rm *.o

