# kts : Korean Tagging System
#          Tau(Word_i) = argmax Pi P(T_i|T_(i-1)) *
#
#                                  P(T_i|W_i)/P(T_i)
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
# -DBASELING1     : �ܼ� ���¼� �̿� �̵�Ͼ� �ĺ� ������ 
#
# -DBASELING2     : ���� ���� �̿� �̵�Ͼ� �ĺ� ������ 
#
# -DANALYSIS      : ������ ����� ����� ������ش�.
#
#
# ���� -DANALYSIS�� �ϸ� ktsdemo : ��  -lm�� �ؾ� �Ѵ�.
#

CC = cc 

RANLIB = ranlib

#CFLAGS = -DONLINE -DUNKNOWNMODEL2 -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING1 -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DBASELING1 -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING2 -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DBASELING2 -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING1 -DBASELING2 -DANALYSIS

CFLAGS = -O2 -DUNKNOWNMODEL3 -DBASELING1 -DBASELING2 -I../include -I/usr/include/db1

OBJS = kts.o ktsmoran.o ktstagger.o ktsformat.o ktsunknown.o \
	ktsirr.o ktsutil.o kimmocode.o lookup.o

HDRS = ktsdefs.h ktsds.h ktstbls.h kts.h sem.h ktssyll.h \
	irrtbl.h kimmocode.h hantable.h ktslib.h

LIB = -lndbm

all : libkts.a ktsdemo ktsdemo2 ktsdemo3

libkts.a : $(OBJS) #$(HDRS)
	rm -f libkts.a
	ar q libkts.a $(OBJS)
	$(RANLIB) libkts.a

ktsdemo : ktsdemo.o libkts.a
	$(CC) -o ktsdemo ktsdemo.o libkts.a $(LIB)

ktspell : ktspell.c libkts.a
	$(CC) -DONLINE -o ktspell ktspell.c libkts.a $(LIB)

ktsdemo2 : ktsdemo2.o libkts.a
	$(CC) -o ktsdemo2 ktsdemo2.o libkts.a $(LIB)

ktsdemo3 : ktsdemo3.o libkts.a
	$(CC) -o ktsdemo3 ktsdemo3.o libkts.a $(LIB)

ktsdemo.o : ktsdefs.h kts.h ktslib.h ktsdemo.c
	$(CC) -DONLINE -c ktsdemo.c

ktsdemo2.o : ktsdefs.h kts.h ktslib.h ktsdemo2.c
	$(CC) -DONLINE -c ktsdemo2.c

ktsdemo3.o : ktsdefs.h kts.h ktslib.h ktsdemo3.c
	$(CC) -DONLINE -c ktsdemo3.c

kts.o : ktsdefs.h ktsds.h ktstbls.h kts.h kts.c
	$(CC) $(CFLAGS) -c kts.c
ktsmoran.o : ktsdefs.h ktstbls.h ktsds.h sem.h kimmocode.h kts.h ktsmoran.c
	$(CC) $(CFLAGS) -c ktsmoran.c
ktstagger.o : ktsdefs.h ktstbls.h ktsds.h kts.h ktstagger.c
	$(CC) $(CFLAGS) -c ktstagger.c
ktsformat.o : ktsdefs.h ktstbls.h ktsds.h kts.h ktsformat.c
	$(CC) $(CFLAGS) -c ktsformat.c
ktsunknown.o : ktsdefs.h ktsds.h ktstbls.h kimmocode.h ktssyll.h ktsunknown.c
	$(CC) $(CFLAGS) -c ktsunknown.c
ktsirr.o : ktsdefs.h ktstbls.h ktsds.h irrtbl.h kimmocode.h ktsirr.c
	$(CC) $(CFLAGS) -c ktsirr.c
ktsutil.o : ktsdefs.h ktsds.h ktstbls.h sem.h ktsutil.c
	$(CC) $(CFLAGS) -c ktsutil.c
lookup.o : ktsdefs.h ktsds.h lookup.c
	$(CC) $(CFLAGS) -c lookup.c
kimmocode.o : hantable.h kimmocode.h kimmocode.c
	$(CC) $(CFLAGS) -c kimmocode.c

clean :
	/bin/rm *.o

