%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                 %%
%% kts.pl : Korean Tagging System Library for Prolog (Version 0.9) %%
%%                                                                 %%
%% by Sang Ho Lee                                                  %%
%%                                                                 %%
%% Computer System Lab.                                            %%
%% Computer Science , KAIST                                        %%
%%                                                                 %%
%% 1995.1.                                                         %%
%%                                                                 %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                 %%
%% Functions : open_kts/2    : Open KTS Library                    %%
%%             mat/2         : Morphological Analysis and Tagging  %%
%%             close_kts/2   : Close KTS Library                   %%
%%                                                                 %%
%% | ?- open_kts(0,X). : System-Initialization                     %%
%%                       : Dictionary , Tables Loading             %%
%%      result : X = 1                                             %%
%%                                                                 %%
%%                                                                 %%
%% | ?- close_kts(0,X). : KTS System Down                          %%
%%                                                                 %%
%%      result : X = 1                                             %%
%%                                                                 %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                 %%
%% KTS Library needs :                                             %%
%%                                                                 %%
%%          1. kts*.o     : kts main library                       %%
%%          2. kts.pl     : interface definition for Prolog        %%
%%          3. kts_dict.* : dbm dictionary                         %% 
%%          4. TRANTBL    : morpheme connectability matrix         %% 
%%          5. TRANPROB   : Probability(Tag_(i)|Tag_(i-1))         %% 
%%          6. TAGFREQ    : Marginal Probability(Tag|Word)         %% 
%%                                                                 %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%
%% 이성진 코드로의 변환
%%
:- consult('ks2kimmo').
%%

foreign_file('libktspl.a',['OpenKTS','CloseKTS','MAT',
			   'OpenDict','LkupDict','CloseDict']).
foreign('OpenKTS',c,open_kts(+integer,[-integer])).
foreign('CloseKTS',c,close_kts(+integer,[-integer])).
foreign('MAT',c,mat(+string,+integer,+float,+float,+integer,[-integer])).
foreign('OpenDict',c,open_dict(+integer,[-integer])).
foreign('CloseDict',c,close_dict(+integer,[-integer])).
foreign('LkupDict',c,lookup_dict(+string,[-integer])).

:- load_foreign_files(['libktspl.a'],[]).

:- open_kts(0,X).
:- open_dict(0,X).


%%
%% constants(+Operator,-Integer).
%%
constants(state_best,-2).
constants(state_mult,-1).
constants(state_path,0).
constants(path_mult,1).
constants(path_best,2).


%%
%% safe_recordz/2
%%
safe_recordz(Name,Record) :-
	  recordz(Name,Record,_).


%%
%% erase_token/0 : erase morphemes in DB
%%
erase_token :-
	recorded(token,_,Ref) , 
	! , 
	erase(Ref) , 
	erase_token.
erase_token :- !.

%%
%% tester_ma/4 (+Operator,+State_th,+Path_th,+Num_path)
%%
%% Operator : state_best : State-Based Tagging Best Candidate Only
%%            state_mult : State-Based Tagging Multiple Candidates
%%            state_path : State-Based & Path-Based
%%             path_mult : Path-Based Tagging Multiple
%%             path_best : Path-Based Tagging Best Candidate Only
%%
%% State_th : State-Based Tagging Threshold : 0.0 .. 1.0
%%
%%  Path_th : Path-Based Tagging Threshold : 0.0 .. 1.0
%%
%% Num_path : the number of the paths : 1 .. 
%%
%%
tester_ma(Operator,State_th,Path_th,Num_path) :-
        constants(Operator,Oprtr),
	write('input : ') , 
        read(Input) , 
	\+ (Input = quit) , 
	mat(Input,Oprtr,State_th,Path_th,Num_path,_) , 
	write('Result is as followings...') , nl , 
	foreach(recorded(token,Morph,_) , 
			(write(Morph) , nl)
        ),
	erase_token , 
	tester_ma(Operator,State_th,Path_th,Num_path).

tester_ma(_,_,_,_) :- !.



%%
%% erase_dict/0 : erase dict in DB
%%
erase_dict :-
	recorded(dict,_,Ref) , 
	! , 
	erase(Ref) , 
	erase_dict.
erase_dict :- !.

%%
%% tester
%%
tester_ld :-
	write('input : ') , 
        read(Input) , 
	\+ (Input = quit),
	lookup_dict(Input,_) , 
	write('Result is as followings...') , nl , 
	foreach(recorded(dict,Morph,_) , 
			(write(Morph) , nl)
        ),
	erase_dict , 
        tester_ld.

tester_ld :- !.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                 %%
%% Functions : open_dict/2   : Open Dictionary                     %%
%%             lookup_dict/2 : Lookup Dictionary                   %%
%%             close_dict/2  : Close Dictionary                    %% 
%%                                                                 %%
%% | ?- open_dict(0,X). : Open Dictionary                          %%
%%      result : X = 1                                             %%
%%                                                                 %%
%%                                                                 %%
%% | ?- lookup_dict('나',X) : Lookup Dictionary with key           %%
%%      result : X = 1                                             %% 
%%        recordz(dict,[postag,ecs,0],_).                          %%
%%        recordz(dict,[postag,ef,0],_).                           %%
%%        recordz(dict,[postag,jj,0],_).                           %%
%%        recordz(dict,[postag,jx,0],_).                           %%
%%        recordz(dict,[postag,npp,0],_).                          %%
%%        recordz(dict,[postag,pv,0],_).                           %%
%%        recordz(dict,[postag,px,0],_).                           %%
%%        recordz(dict,[semtag,pv,[[2121,2122],[321]]],_).         %%
%%        recordz(dict,[semtag,pv,[[2122,211],[122,321,2223]]],_). %%
%%        recordz(dict,[semtag,pv,[[2122,211],[321]]],_).          %%
%%                                                                 %%
%% | ?- close_dict(0,X). : Close Dictionary                        %%
%%      result : X = 1                                             %%
%%                                                                 %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%                                                                 %%
%% Lookup-Dictionary needs :                                       %%
%%                                                                 %%
%%          1. lookup.o    : LookUp main library                   %%
%%          2. ktsutil.o   : Utility library                       %%
%%          3. kimmocode.o : KimmoCode library                     %%
%%          2. lookup.pl   : interface definition for Prolog       %%
%%          3. kts_dict.*  : dbm dictionary                        %% 
%%                                                                 %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

