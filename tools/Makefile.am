#
# Makefile for Korean-Tagging-System(KTS) Utility
#
# Sangho Lee
#
# jhlee@csone.kaist.ac.kr, shlee2@adam.kaist.ac.kr
#
#
# KTS에서 tagged corpus로부터 정보를 추출하는
# Tool을 compile함
#
#

bin_PROGRAMS = mktrntbl mkbiprob tag2in1 tag2in2 gettagseq freq PutIrr printhan

INCLUDES = -I../include

readme:
	@echo ' *********************************************************'
	@echo ' 사전과 TRANTBL을 얻기 위해서는 다음을 수행하시오.'
	@echo '  crps2info  : Corpus To Information'
	@echo '   crps2info ../corpus/Tagged/newTrn.Tag total.dict TRANTBL'
	@echo ' 품사 전이 확률을 얻기 위해서는 다음을 수행하시오.'
	@echo '  trncrps2info  : Training Corpus To Information'
	@echo '   trncrps2info ../corpus/Tagged/newTrn.Tag TRANFREQ'
	@echo ' freq(품사)는 사전 kts_dict에 넣을 때 계산됨.'
	@echo ' *********************************************************'

test:
	csh crps2info ../corpus/Tagged/newTrn.Tag total.dict TRANTBL
