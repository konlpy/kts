
*** ediff�� sparc�� �ִ� Ǯ�׸�

1. tagger�� �۵���Ű�� (o , x) marker�� ���� ����� �޴´�.
   ���⼭ (o , x)�� ���ְ� ������ ���� �±�� �Ͱ� ��.

   diff ����_�±� �°�_�±� | ediff > dif.dif
   awk ' { print $1 } ' dif.dif > dif.dif2
   calcdiff dif.dif2
   ���¼� level�� ���� level������ Ʋ�� ���� count  

2. ���� �±�� text���� tagger�� �±����� �� known-word�� 
   �ش��ϴ� �������� ���Ϸ��� ?

   1) ../splitdiff diffe_file k_f u_f e_f  
      �̷��� �ϸ� u_f�� unknown_word(����,�°�)�� �κ��� ����
      
   2) ../getdiff u_f tar_u_f
      �̷��� �ϸ� unknown_word(����)�� ����
      awk ' { print $2 } ' tar_u_f > tar_u_f2
      awk -F+ ' { SS = SS + NF } END { print SS } ' tar_u_f2
      �̷��� �ϸ� unknown_word(����)�� ��ü ���¼� ������ count

3. ���� known-word�� �ش��ϴ� ������ ���ϱ� ���ؼ��� ?

   1) ������ ���� unknown_word������ ���� �±�� text�� ��.
      diff ../UnknownTest/un_test_drg.tagged.2 tar_u_f | ediff > kno_dif
      �̷��� �ϸ� "����"�� ���� �κи� kno_dif�� ����.

   2) grep "^o " kno_dif > known_part : known_part�� ���� �����.
      awk ' { print $2 } ' known_part > know_p 
 
   3) awk -F+ ' { SS = SS + NF } END { print SS } ' know_p
      known_word�� ���¼� ���� count 

4. known_word�κ��� hit ratio�� ���ϱ� ����
  
   1) kn_f�� vi�� �̿��� /^o /�� ���ְ� awk�� ù��° field�͸� ������,
      calcdiff�� �Ѵ�.

5. Unknown_word�κ��� hit ratio�� ���ϱ� ����

   1) u_f�� vi�� �̿��� /^o / , /^x / �� ���ְ�  calcdiff�� �Ѵ�.





