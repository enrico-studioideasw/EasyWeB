/////////////////////Core_di_linguaggio_amethyst/////////////////////////

/* 
   Questo e' l' interprete del linguaggio puro, versione 6.0.
   Funzioni predefinite. Qui c'e' da fare, ma prima di scrivere tutte le
   predefinite, mi scrivo un main e inizio a fare prove. 
*/ 

/////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "ewb_vm.h"

add_predefs()
{ nfuncs=0; 
  strcpy(functions[0],  "return"); arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[1],  "print");  arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;   
  strcpy(functions[2],  "eprint"); arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[3],  "input");  arity[nfuncs]=0; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[4],  "load");   arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[5],  "save");   arity[nfuncs]=2; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[6],  "int");    arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[7],  "abs");    arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[8],  "cos");    arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[9],  "sin");    arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[10], "sqr");    arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;  
  strcpy(functions[11], "rnd");    arity[nfuncs]=0; funcpos[nfuncs]=0; nfuncs++; 
  strcpy(functions[12], "mid");    arity[nfuncs]=3; funcpos[nfuncs]=0; nfuncs++;   
  strcpy(functions[13], "asc");    arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;   
  strcpy(functions[14], "char");   arity[nfuncs]=1; funcpos[nfuncs]=0; nfuncs++;   
};

void remove_sep()
{ delperc(); if (code[cpos]!=',') err ("PAR"); 
  cpos++; 
};
int start_openpar()
{ delperc(); if (code[cpos]=='(') { cpos++; return 1; };
  return 0;  
};
void end_openpar(int p)
{ if (!p) return;  
  delperc(); if (code[cpos]!=')') err("PAR"); 
};

predefs(int nf)
{ if (nf==0) { calcexp(); return 1; };                        //return 
  if (nf==1) { calcexp(); out(PRNT); out("[1]"); return 1; }; //print
  if (nf==2) { calcexp(); out(PRNT); out("[2]"); return 1; }; //eprint
  if (nf==3) {            out(INP);  out("[0]"); return 1; }; //input  //Discutibile [0] e' stdin
  if (nf==4) { calcexp(); out(OPN);  out("[r]");    //load      //Dopo la OPN A contiene il filedes
               out (JEX); int tp=respos; out("....");           //Se negativo, fine load. 
               out (STA); outvar(nvars);                        //Salvo il filedes in una tmp
               out (LDA); out("[]"); out(STA); outvar(nvars+1); //Azzero una var. temporanea.
               int looppos=respos; 
               out (INP); outvar(nvars);                        //leggo una riga
               out (JEX); int tb=respos; out("....");           //Se eccezione in lettura, fine load
               out (STA); outvar(nvars+2); 
               out (LDA); outvar(nvars+1);                      
               out(SCAT); outvar(nvars+2); 
               out (STA); outvar(nvars+1);                      //Aggiungo alla temporanea
               out (JMP); outadr(looppos); 
               putadr(tp, respos); putadr(tb, respos);          //Destinazione di salto
               out (LDA); outvar(nvars); out(CLOS);             //Chiudo lo stream
               out (LDA); outvar(nvars+1);                      //Carico la temporanea in accumulatore
               return 1; };
  if (nf==5) { int op=start_openpar(); 
               calcexp(); out(OPN);  out("[w]");   //save       //Dopo la OPN A contiene il filedes
               out (STA); outvar(nvars);                        //Salvo il filedes in una tmp
               remove_sep(); 				        //Salto la virgola
               calcexp();                                       //In A ora c'e' quello che devo scrivere
               out (JEX); int tp=respos; out("....");           //Se negativo, fine save.
               out (PRNT); outvar(nvars);                       //Scrivo sullo stream                              
               out (LDA); outvar(nvars); out(CLOS);             //Chiudo lo stream
               putadr(tp, respos);                              //Destinazione di termine
               end_openpar(op); 
               return 1; 
             };
  if (nf==6) { calcexp();           //int   = numero % numero+1 //il risultato e' intero. 
               out(PUSH); 
               out(SUM); out("[1]"); 
               out(STA); outvar(nvars);
               out(POP); 
               out(MOD); outvar(nvars);    
               return 1;                              
             };
  if (nf==7) { calcexp();           //ABS 
               out(JP); int dst=respos; out("....");  
               out(NEG);
               putadr(dst, respos); 
               return 1;
             };
  if (nf==8) { calcexp(); out(STA); outvar(nvars);   //Coseno: sen(90-alfa)=cos alfa
               out(LDA); out("[90]"); out(SUB); outvar(nvars);                
               out(SEN); 
               return 1;
             }; 
  if (nf==9) { calcexp(); out(SEN); }; //Seno
  if (nf==10){ calcexp(); out(SQR); return 1; }; //Radice quadrata
  if (nf==11){ out(RND); return 1; }; //Random tra 0 e 1
  if (nf==12){ int op=start_openpar(); 
               calcexp(); out(STA); outvar(nvars); //La stringa da tagliare
               remove_sep(); 
               calcexp(); out(STA); outvar(nvars+1); //limite from
               delperc(); 
               if (code[cpos]==',') 
               { remove_sep(); calcexp(); 
                 out(STA); outvar(nvars+2); 
                 //Mid a tre parametri 
                 out(LFT); outvar(nvars); outvar(nvars+2); //taglio
                 out(STA); outvar(nvars);  //Cosi' tolgo la parte a destra
                 out(LFT); outvar(nvars); outvar(nvars+1); 
                 out(LDA); outvar(nvars); //Cosi' tolgo la parte sinistra. 
               } else 
               { out (LFT); outvar(nvars); outvar(nvars+1); //Mid a due parametri, e' un left 
               };
               end_openpar(op); 
               return 1;
             };
  if (nf==13){ calcexp(); out(ASC); return 1; 
             };
  if (nf==14){ calcexp(); out(CHR); return 1; 
             };
  return 0;
};
