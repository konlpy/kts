
=======================================================

   K T S : Korean Tagging System       ver 0.9


   Sangho Lee      
   jhlee@csone.kaist.ac.kr    shlee2@adam.kaist.ac.kr

   Computer Systems Lab.
   KAIST.

=======================================================

KTS�� [�� ��ȣ, `�̵�Ͼ ����� �ѱ��� ǰ�� �±� �ý��� ����',
          �ѱ����б����, �������� ��, 1995]���� ������
�ý����� ���ʷ� �Ͽ����ϴ�. 

�� KTS�� �ѱ��� ǰ�� �±� �ý������� ������ ���� 
���丮�� �����Ǿ� �ֽ��ϴ�.


  ./       :

       KTS�� C-code, ������ ���õ� Utility�� �����Ѵ�.

  ./manual :

       KTS�� ��� ���,  main.ps�� �о�� �ٶ��ϴ�.

  ./Tool/  :

       KTS�� �ʿ�� �ϰ� �ִ� ����, Ȯ�� ������ 
       �����ϱ� ���� ���Ǵ� Utility.
       �̹� �±�� ȭ�Ϸκ��� ������ �����Ѵ�.
             
       TRANTBL    : ���¼��� ǰ�� ����ǥ
       TRANFREQ   : ǰ���� ���� Ȯ��
       total.dict : ����

  ./Tool/Irregular/ :

       �±�� ȭ�Ϸκ��� ������ ������ ��, �ұ�Ģ
       �ڵ带 �ֱ� ���� �������� �ۼ��� �ұ�Ģ ����. 
       (�ѱ��� �ұ�Ģ ����� ��� ���ϵǾ� �ִ� ���� �ƴϴ�.)

  ./Tool2/GetUNKNOWNFREQ/ :

       - KTS���� ����ϴ� �̵�Ͼ� �߻� Ȯ���� ���ϴ� Utility.
       - UNKNOWNFREQ�� ������ ���۽��� KTS�� �±��ϰ� ���� �±��
         �Ͱ� ���Ͽ� ��� Ȯ�� ���̴�. KTS�� �±��� ���,
         �̵�Ͼ ���ؼ��� �տ� `x'�� marking�Ǿ� �ִ�.
       - KTS���� ����ϴ� P(k_i^1|t_i^1).

  ./Tool2/GetLEXFEATURE/ :
 
       - KTS���� ����ϴ� P(t_i^1|t_i^2,m_i^2)�� ���ϴ� Utility.

  ./Tool2/Evaluation/ :
     
       KTS�� ���� �±��� �ϰ� �� �±� ��Ȯ���� ���ϴ� Utility.
       �̰��� ������ ����� Utility�� ���Դϴ�.
 
  ./Corpus/Tagged/ :

       - ���� �±�� ���۽�.
       - TotCrps.Tag�� ��ü ���۽��� �±�� ����. 
                   (�±��� �߸��� �κ��� ����.)
       - raw ���۽��� �ϴ��� matching�� �Ǿ� ���� ����.
       - �ϴ��� matching�� �Ǵ� �κ��� newTest.Tag, newUnTrn.Tag�Դϴ�.
       - �׷��Ƿ� ������ �κ��� KTS�� �ʿ�� �ϴ� ����, Ȯ�� ������
         ��� ���� ���Ǿ����ϴ�.

  ./Corpus/UnTagged/ :

       �±뿡 ���� raw ���۽�.


=========================================

  KTS�� Makefile 

=========================================

  ./Makefile : KTS�� compile�ϰ� libkts.a�� �����.
               �� ��, ������ �����Ѵ�.
               (libktspl.a�� Prolog�� ���� library�ε�
                �� �κ��� compile���� �ʴ´�.)

  ./kts.make : C ��� ����ڸ� ����
               libkts.a�� �����.(Makefile���� �����)

  ./ktspl.make : Prolog ����ڸ� ����
               libktspl.a�� �����.(Makefile���� ��� ����)

  * Prolog ����ڴ� libktspl.a�� ����� ���ؼ���

  environment variable SP_PATH�� Sicstus-Prolog Directory��
  assign�Ǿ�� �Ѵ�. (.cshrc�� ������ �ȴ�.)

  ��) setenv SP_PATH /user/local/lib/sicstus2.1

  make -f ktspl.make
 

==========================================

  KTS�� �ʿ�� �ϴ� �Ķ���͸� ��� ���.

==========================================

  corpus�� Trn.Tag(Trn.raw), unknTrn.Tag(unknTrn.raw), 
  Test.Tag(Test.raw)�� �������� ��,

  1.  Trn.Tag�� TRANTBL, TRANFREQ, total.dict�� ���� ��,
      ���� kts_dict�� �����Ѵ�.

  2.  KTS�� UNKNOWNMODEL2�� compile�� ��,  
      LEXFEATURE�� Trn.Tag�κ��� �����Ѵ�.
      (������ �䱸�ϴ� LEXFEATURE�� ����ϱ� ���ؼ���
       UNKNOWNMODEL3�� compile�ؾ� �Ѵ�.) 
 
      KTS�� unknTrn.raw�� �±��ϰ� ���� �±�� �Ͱ� 
      ���Ͽ� UNKNOWNFREQ�� ���Ѵ�.


