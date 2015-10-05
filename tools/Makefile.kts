#
# Makefile for Korean-Tagging-System(KTS) Utility
#
# Sangho Lee
#
# jhlee@csone.kaist.ac.kr, shlee2@adam.kaist.ac.kr
#
#
# KTS���� tagged corpus�κ��� ������ �����ϴ�
# Tool�� compile��
#
#
CC = gcc
CFLAGS = -I../include
all :
	$(CC) $(CFLAGS) -O2 -o mktrntbl mktrntbl.c
	$(CC) $(CFLAGS) -O2 -o mkbiprob mkbiprob.c
	$(CC) $(CFLAGS) -O2 -o tag2in1 tag2in1.c
	$(CC) $(CFLAGS) -O2 -o tag2in2 tag2in2.c
	$(CC) $(CFLAGS) -O2 -o gettagseq gettagseq.c
	$(CC) $(CFLAGS) -O2 -o freq freq.c
	$(CC) $(CFLAGS) -O2 -o PutIrr PutIrr.c
	$(CC) $(CFLAGS) -O2 -o printhan printhan.c
	@echo ''
	@echo '������ TRANTBL�� ��� ���ؼ��� ������ �����Ͻÿ�.'
	@echo 'crps2info  : Corpus To Information'
	@echo 'crps2info ../corpus/Tagged/newTrn.Tag total.dict TRANTBL'
	@echo 'ǰ�� ���� Ȯ���� ��� ���ؼ��� ������ �����Ͻÿ�.'
	@echo 'trncrps2info  : Training Corpus To Information'
	@echo 'trncrps2info ../corpus/Tagged/newTrn.Tag TRANFREQ'
	@echo 'freq(ǰ��)�� ���� kts_dict�� ���� �� ����.'
