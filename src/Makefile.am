bin_PROGRAMS = ktspell ktsdemo ktsdemo2 ktsdemo3

LDADD = libkts.a -ldb
DEPENDENCIES=libkts.a

INCLUDES = -I../include

noinst_LIBRARIES = libkts.a

libkts_a_SOURCES = kts.c ktsmoran.c ktstagger.c ktsformat.c ktsunknown.c \
       ktsirr.c ktsutil.c kimmocode.c lookup.c
# --- 원본의 이상호님의 설명 ---
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
#CFLAGS = -DONLINE -DUNKNOWNMODEL2 -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING1 -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DBASELING1 -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING2 -DANALYSIS
#CFLAGS = -DUNKNOWNMODEL3 -DBASELING2 -DANALYSIS

#CFLAGS = -DUNKNOWNMODEL2 -DBASELING1 -DBASELING2 -DANALYSIS

AM_CFLAGS = -O2 -DUNKNOWNMODEL3 -DBASELING1 -DBASELING2 -I../include -I/usr/include

test:
	export KDICT_PATH=../dict;./ktspell <../corpus/UnTagged/barun
