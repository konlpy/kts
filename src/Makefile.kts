# kts : Korean Tagging System
#          Tau(Word_i) = argmax Pi P(T_i|T_(i-1)) *
#
#                                  P(T_i|W_i)/P(T_i)
#
# -DDEBUG         : 형태소 분석 과정을 볼 수 있다.
# -DDEBUG2        : Tagging 과정을 볼 수 있다.
#                   어절 tagging 방법을 알 수 있다.
# -DDEBUG3        : UNKNOWNMODEL3에서 Unknown-Word 추정에 관한 
#                   정보를 볼 수 있다.
# -DONLINE        : On-Line Mode로 동작 , if not : batch job 
#
# -DPUTENDOFSTATEMENT : 문장이 끝나면 `EOS' symbol을 쓴다.
#
# -DUNKNOWNMODEL1 : Default : P(W|T) = 1.0 / (Freq(T) + 1)
#
# -DUNKNOWNMODEL2 : Unknown Word Model 2 : P(W|T) = 1.0으로 간주
#
# -DUNKNOWNMODEL3 : ProbInvLex사용 (unk-tag|right-lexical-feature)
#
# -DUNKNOWNMODEL3 -DCASE1 : ProbLex사용 (right-lexical-feature|unk-tag)
#
# -DBASELING1     : 단서 형태소 이용 미등록어 후보 여과기 
#
# -DBASELING2     : 음절 정보 이용 미등록어 후보 여과기 
#
# -DANALYSIS      : 간단한 통계의 결과를 출력해준다.
#
#
# 만약 -DANALYSIS를 하면 ktsdemo : 에  -lm을 해야 한다.
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

all : libkts.a ktsdemo ktsdemo2 ktsdemo3 ktspell

libkts.a : $(OBJS) #$(HDRS)
	rm -f libkts.a
	ar q libkts.a $(OBJS)
	$(RANLIB) libkts.a

ktsdemo : ktsdemo.o libkts.a
	$(CC) -o ktsdemo ktsdemo.o libkts.a $(LIB)

ktspell : ktspell.o libkts.a
	$(CC) $(CFLAGS) -DONLINE -o ktspell ktspell.o libkts.a $(LIB)

ktsdemo2 : ktsdemo2.o libkts.a
	$(CC) $(CFLAGS) -o ktsdemo2 ktsdemo2.o libkts.a $(LIB)

ktsdemo3 : ktsdemo3.o libkts.a
	$(CC) $(CFLAGS) -o ktsdemo3 ktsdemo3.o libkts.a $(LIB)

ktsdemo.o : ktsdemo.c
	$(CC) $(CFLAGS) -DONLINE -c ktsdemo.c

ktsdemo2.o : ktsdemo2.c
	$(CC) $(CFLAGS) -DONLINE -c ktsdemo2.c

ktsdemo3.o : ktsdemo3.c
	$(CC) $(CFLAGS) -DONLINE -c ktsdemo3.c

ktspell.o : ktspell.c
	$(CC) $(CFLAGS) -DONLINE -c ktspell.c

kts.o : kts.c
	$(CC) $(CFLAGS) -c kts.c
ktsmoran.o : ktsmoran.c
	$(CC) $(CFLAGS) -c ktsmoran.c
ktstagger.o : ktstagger.c
	$(CC) $(CFLAGS) -c ktstagger.c
ktsformat.o : ktsformat.c
	$(CC) $(CFLAGS) -c ktsformat.c
ktsunknown.o : ktsunknown.c
	$(CC) $(CFLAGS) -c ktsunknown.c
ktsirr.o : ktsirr.c
	$(CC) $(CFLAGS) -c ktsirr.c
ktsutil.o : ktsutil.c
	$(CC) $(CFLAGS) -c ktsutil.c
lookup.o : lookup.c
	$(CC) $(CFLAGS) -c lookup.c
kimmocode.o : kimmocode.c
	$(CC) $(CFLAGS) -c kimmocode.c

test:
	export KDICT_PATH=../dict
	./ktspell <../Corpus/UnTagged/barun

clean :
	/bin/rm -f *.o ktsdemo ktsdemo2 ktsdemo3 ktspell libkts.a

