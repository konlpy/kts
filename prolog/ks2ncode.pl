/************************************************************************************/
/***        kimmo code �� �ѱ� �ϼ��� �ڵ�(KSC-5601)�� ��ȣ ��ȯ rouine.          ***/
/***                                                                              ***/
/***                            by     Lee sangho.  Kaist, Korea.                 ***/
/***                            Email  shlee@csone.kaist.ac.kr    1993/7/15       ***/
/************************************************************************************/

/*  kstable�� �ϼ��� �ڵ� �� ������ �ڵ��� table�̴�.  */
:- consult('kstable').   

/* whatType�� CH(kimmo code)�� type(�ʼ�,�߼�,����)�� return */ 
whatType(CH,TYPE) :- is_Jungs(CH,TYPE), !.
whatType(CH,TYPE) :- is_Chos(CH,TYPE), !.
whatType(CH,TYPE) :- is_Jongs(CH,TYPE), !.
whatType(_,none).

is_Chos(CH,chos) :- chos(CH).
is_Jungs(CH,jungs) :- jungs(CH).
is_Jongs(CH,jongs) :- jongs(CH).

/* �ʼ��� kimmo code */
chos(p).  chos(g).  chos(h).  chos(q).  chos(n).  chos(d).  chos(l).
chos(m).  chos(b).  chos(r).  chos(s).  chos(v).  chos(f).  chos(j).
chos(z).  chos(c).  chos(k).  chos(t).  
/* �߼��� kimmo code */
jungs(a).   jungs(8).  jungs(ya). jungs(y8).  jungs(e).   jungs(9).
jungs(ye).  jungs(y9). jungs(o).  jungs(wa).  jungs(w8).  jungs(wi).
jungs(yo).  jungs(u).  jungs(we). jungs(w9).  jungs(wu).  jungs(yu).
jungs('_'). jungs(yi). jungs(i). 
/* ������ kimmo code */
jongs('G').  jongs('Q').  jongs('GS'). jongs('N').  jongs('NJ'). jongs('NH').
jongs('D').  jongs('L').  jongs('LG'). jongs('LM'). jongs('LB'). jongs('LS').
jongs('LP'). jongs('LT'). jongs('LH'). jongs('M').  jongs('B').  jongs('BS').
jongs('S').  jongs('V').  jongs('*').  jongs('J').  jongs('C').  jongs('K').
jongs('T').  jongs('P').  jongs('H'). 

/* CH�� ������ ������ return�ϴ� ��(��,����,�ѱ�,����) */
whatKind(CH,eng) :- is_english(CH), !.
whatKind(CH,num) :- is_number(CH), !.
whatKind(_,kor).
is_english(CH) :- name(CH,[47|_]).
is_number(CH) :- name(CH,[124|_]).

/***********************************************************/
/*  �ϼ��� �ѱ� �ڵ带 kimmo code�� ��ȣ ��ȯ �ϴ� routine */
/*                 MAIN  FUNCTION                          */
/***********************************************************/
ks2ncode(X,Y) :- var(X), var(Y), !, fail.
ks2ncode(X,Y) :- var(X), kimmo2ks(Y,X), !.
ks2ncode(X,Y) :- var(Y), ks2ncodeSub(X,Y), !.
ks2ncode(X,Y1) :- ks2ncodeSub(X,Y1).


/* kimmo code�� �ϼ��� �ѱ� �ڵ�� ���� */
kimmo2ks([],[]) :- !.
kimmo2ks(LIST,Output) :- 
	kimmoConv(LIST,Output1), 
	removeSpace(Output1,Output2),
	makeString(Output2,Output), !.

kimmoConv([],[]).
kimmoConv([H|T],Output) :-
	kimmoWord(H,Output1,_), 
	kimmoConv(T,Output2),
	cat(Output1,[' '],Output3),
	cat(Output3,Output2,Output).

kimmoWord([[]],_,_).
kimmoWord([],[],_).                                /* kimmo code�� ���������� ó�� */
kimmoWord([H|T],Output,Next) :-
        whatKind(H,Type), Type == kor,
        kimmoHangul([H|T],Output1,Next1),
        kimmoWord(Next1,Output2,Next),
	cat(Output1,Output2,Output).

kimmoWord([H|T],Output,Next) :-
        whatKind(H,Type), Type == eng,
        kimmoEnglish([H|T],Output1,Next1),
        kimmoWord(Next1,Output2,Next),
	cat(Output1,Output2,Output).

kimmoWord([H|T],Output,Next) :-
        whatKind(H,Type), Type == num,
        kimmoNumber([H|T],Output1,Next1),
        kimmoWord(Next1,Output2,Next),
	cat(Output1,Output2,Output).

kimmoEnglish([[]],[],[[]]).                         /* ���� ó�� */
kimmoEnglish([H|T],[Output],T) :-
	name(H,[_|Output1]),
	name(Output,Output1).

kimmoNumber([[]],[],[[]]).                         /* ���� ó�� */
kimmoNumber([H|T],[Output],T) :-
	name(H,[_|Output1]),
	name(Output,Output1).

kimmoHangul([[]],[],[[]]).                         /* kimmo code�� �ϼ��� �ѱ۷� ���� */
kimmoHangul([H|T],[Output2],Next) :-
	whatType(H,Type), 
	kimmoHangulSub(Type,[H|T],Output1,Next),
	name(Output2,Output1).

/*************************************************************************************/
/** kimmoHangulSub�� kimmoHangulChos, kimmoHangulChosSub�� ���������� ������      **/
/** kimmo code�� �� ���ڴ����� �߶� �ϼ��� code�� �ٲٴ� ���̴�.                  **/
/** ���⼭ kimmo code�� �ϼ��� �ڵ�� mapping��Ű�� routine�� ksc_s_c�̴�.          **/
/** ���⼭ ����� rule�� ������ ����.                                               **/
/**   �ʼ� -> �߼� -> ����                                                          **/ 
/**   �ʼ� -> �߼�                                                                  **/
/**   �ʼ�                                                                          **/
/**   �߼� -> ����                                                                  **/
/**   �߼�                                                                          **/
/**   ����                                                                          **/
/*************************************************************************************/


kimmoHangulSub(chos,[H|[]],[C1|[C2]],[[]]) :-
	Output1 = [H], ksc_s_c(C1,C2,Output1).
kimmoHangulSub(chos,[H|T],[C1|[C2]],Next) :-
	move(H1,_,T), whatType(H1,Type), 
	kimmoHangulChos(Type,T,Output1,Next), 
	Output2 = [H|Output1], ksc_s_c(C1,C2,Output2).
kimmoHangulSub(jungs,[H|[]],[C1|[C2]],[[]]) :-
	Output1 = [H], ksc_s_c(C1,C2,Output1).
kimmoHangulSub(jungs,[H|T],[C1|[C2]],Next) :-
	move(H1,_,T), whatType(H1,Type), 
	kimmoHangulJungs(Type,T,Output1,Next), 
	Output2 = [H|Output1], ksc_s_c(C1,C2,Output2).
kimmoHangulSub(jongs,[H|[]],[C1|[C2]],[[]]) :-
	Output1 = [H], ksc_s_c(C1,C2,Output1).
kimmoHangulSub(jongs,[H|Next],[C1|[C2]],Next) :-
	Output1 = [H], ksc_s_c(C1,C2,Output1).
kimmoHangulSub(none,[_|[]],[],[[]]).
kimmoHangulSub(none,[_|Next],[],Next).

kimmoHangulChos(none,Next,[],Next).
kimmoHangulChos(jongs,Next,[],Next).
kimmoHangulChos(chos,Next,[],Next).
kimmoHangulChos(jungs,[H|[]],[H],[[]]).
kimmoHangulChos(jungs,[H|T],[H],[H1|T1]) :-
	move(H1,T1,T), whatType(H1,Type), 
	Type \== jongs.
kimmoHangulChos(jungs,[H|T],[H|[H1]],Next) :-
	move(H1,T1,T), whatType(H1,Type), 
	Type == jongs, kimmoHangulChosSub(T1,Next).

kimmoHangulJungs(none,Next,[],Next).
kimmoHangulJungs(jungs,Next,[],Next).
kimmoHangulJungs(chos,Next,[],Next).
kimmoHangulJungs(jongs,[H|[]],[H],[[]]).
kimmoHangulJungs(jongs,[H|T],[H],T).

kimmoHangulChosSub([],[[]]).
kimmoHangulChosSub(T,T).

/*  �ϼ��� �ѱ� �ڵ带 kimmo code�� �ٲٴ� �� */
ks2ncodeSub('',[]) :- !. 
ks2ncodeSub(LIST,Output) :- 
	name(LIST,LIST1), 
	ksConv([],LIST1,Output,_,_), !.


ksConv([],[[]],[],[],[[]]).
ksConv([],[H|T],Output,_,_) :- 
	ksConv(H,T,Output,_,_).
ksConv(9,[],[],[],[[]]). 
ksConv(9,[H|T],Output,X,Y) :- 
	ksConv(H,T,Output,X,Y).
ksConv(10,[],[],[],[[]]). 
ksConv(10,[H|T],Output,X,Y) :- 
	ksConv(H,T,Output,X,Y).
ksConv(32,[],[],[],[[]]). 
ksConv(32,[H|T],Output,X,Y) :- 
	ksConv(H,T,Output,X,Y).
ksConv(H,T,[Output1|Output2],X,Y) :-
	ksWord(H,T,Output1,X,Y), 
	ksConv(X,Y,Output2,_,_).

ksWord(9,[],[],[],[[]]).                           /* �Է��� ���������� ó�� */
ksWord(9,[H|T],[],H,T).
ksWord(10,[],[],[],[[]]).                           /* �Է��� ���������� ó�� */
ksWord(10,[H|T],[],H,T).
ksWord(32,[],[],[],[[]]).                           /* �Է��� ���������� ó�� */
ksWord(32,[H|T],[],H,T).
ksWord([],[[],[]],[],[[]]).
ksWord([],[[]],[],[],[[]]).
ksWord(H,T,Output,X,Y) :-
	H > 127, ksHangul(H,T,Output1,X1,Y1), 
	ksWord(X1,Y1,Output2,X,Y),
	cat(Output1,Output2,Output).
ksWord(H,T,Output,X,Y) :-
	47 < H, H < 58, ksNum(H,T,Output1,X1,Y1), 
	ksWord(X1,Y1,Output2,X,Y),
	cat(Output1,Output2,Output).
ksWord(H,T,Output,X,Y) :-
	H < 128, ksEng(H,T,Output1,X1,Y1), 
	ksWord(X1,Y1,Output2,X,Y),
	cat(Output1,Output2,Output).

ksChar(H,T,Code,X,Y) :-
        move(H1,T1,T), ksc_s_c(H,H1,Code), !,
        countAtom(Code), Code = [CH], !, 
        not(jungs(CH)), move(X,Y,T1). 

ksHangul([],_,[],[],[[]]).                             /* �ϼ��� �ڵ� --> kimmo code */
ksHangul(H,T,Output,X,Y) :-
	move(H1,T1,T), ksc_s_c(H,H1,Code), move(H2,T2,T1),
	ksHangulSub(H2,T2,Output1,X,Y),
	cat(Code,Output1,Output).
	
ksHangulSub([],_,[],[],[[]]). 
ksHangulSub(9,T,[],9,T).
ksHangulSub(10,T,[],10,T).
ksHangulSub(32,T,[],32,T).
ksHangulSub(H,T,[],H,T) :- 
	(H < 128; (47 < H, H < 58)). 
ksHangulSub(H,T,Output,X,Y) :-
	H > 127, ksHangul(H,T,Output,X,Y).

ksNum([],_,[],[],[[]]).                                /* ���ڸ� ó�� */
ksNum(H,T,[Code|Output1],X,Y) :-
	move(H1,T1,T), 
	ksNumSub(H1,T1,Output1,X,Y),
	name(Code,[124|[H]]).

ksNumSub([],_,[],[],[[]]).
ksNumSub(9,T,[],9,T).
ksNumSub(10,T,[],10,T).
ksNumSub(32,T,[],32,T).
ksNumSub(H,T,[],H,T) :-
	(48 > H; H > 57). 
ksNumSub(H,T,Output,X,Y) :-
	47 < H,H < 58, ksNum(H,T,Output,X,Y).

ksEng([],_,[],[],[[]]).                                /* ��� ó�� */
ksEng(H,T,[Code|Output1],X,Y) :-
	move(H1,T1,T),
	ksEngSub(H1,T1,Output1,X,Y),
	name(Code,[47|[H]]).

ksEngSub([],_,[],[],[[]]).
ksEngSub(9,T,[],9,T).
ksEngSub(10,T,[],10,T).
ksEngSub(32,T,[],32,T).
ksEngSub(H,T,[],H,T) :-
	(H > 127; (47 < H,H < 58)). 
ksEngSub(H,T,Output,X,Y) :-
	H < 128, ksEng(H,T,Output,X,Y).


cat([],Y,Y) :- !.                                       /* [A],[B] --> [A,B] */     
cat([H|[]],Y,[H|Y]) :-  !.
cat([H|T],Y,[H|Output1]) :- 
	cat(T,Y,Output1).


move([],[[]|[]],[]).                                 /* �� list���� head,tail�� ���� */
move(H,T,[H|T]).

moveN(Z,T,T1,[_]) :- move(Z,T,T1).              /* move�� ����ϴ� �̰��� 4��° arg   */
moveN(Z,T,[_|T1],[_|T2]) :- moveN(Z,T,T1,T2).   /* �� ���� �պκ��� ������move�� ���� */

removeSpace([' '],[]).                             /* list�� ���� �ִ� space�� ���� */
removeSpace([H|[]],H).
removeSpace([H|T],[H|Output1]) :- 
	removeSpace(T,Output1). 

makeString([],'').                             /* list --> string */
makeString(Input,Output) :- 
        makeStringSub(Input,Output1), 
        name(Output,Output1).
makeStringSub([H|[]],Output) :- name(H,Output).
makeStringSub([H|T],Output) :- 
        name(H,Output1), makeStringSub(T,Output2), 
        cat(Output1,Output2,Output).

makeList(``,[]).                               /* kimmo string --> kimmo list */
makeList([Input],Output) :- 
	name(Input,Output1), 
	makeListSub(Output1,Output).

makeListSub([],[]).
makeListSub([H|[]],[C]) :- name(C,[H]).
makeListSub([H|T],[Output1|Output2]) :-
	makeListSubTest([H|T]),move(H1,T1,T),
	name(C1,[H]), name(C2,[H1]),
	makeString([C1,C2],Output1),
	makeListSub(T1,Output2).
makeListSub([H|T],[C|Output1]) :-
	makeListSub(T,Output1), name(C,[H]).

makeListSubTest([_|[]]) :- fail.
makeListSubTest([H|[H1|_]]) :-
	(H == 0'y; H == 0'w; (64 < H, H < 91 ,64 < H1, H1 < 91)).

countAtom([_|[]]).

not(X) :- (X, !, fail) ; true.

