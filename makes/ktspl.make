# libktspl.a : Korean Tagging System Prolog Library
#
#          Tau(Word_i) = argmax Pi P(T_i|T_(i-1)) *
#
#                                  P(W_i|T_i)
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
# -DBASELING1     : 특징 품사가 보이면 전체를 명사로 추정안함
#
# -DBASELING2     : 추정한 품사에서 그 품사가 있을 수 없으면 Cutting
#
# -DANALYSIS      : 간단한 통계의 결과를 출력해준다.
#
# 만약 -DANALYSIS를 하면 ktsdemo : 에  -lm을 해야 한다.
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

