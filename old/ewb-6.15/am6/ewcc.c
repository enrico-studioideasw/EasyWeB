/////////////////////Core_di_linguaggio_amethyst/////////////////////////

/* 
   Questo e' l' interprete del linguaggio puro, versione 6.0.
   La sintassi e' stata ampliata: valori di variabili in dichiarazione e
   migliore gestione di array e foreach.  
   Compila in metacodice per macchina virtuale. 
   Rimangono fuori una serie di predefinite.   Per ora definisco solo la 
   print e inizio a fare dei tests.. Poi costruisco la macchina virtuale
   basandomi sulla tabella di assembler qui indicata.  La macchina sara' 
   un  po' complessa  in  quanto  i  registri sono stringhe di lunghezza 
   indefinita, ma realizzabile sia in perl che in altri linguaggi.
   Mi interessa poterla avere per C, perl, php. 
*/
///////////////////////////Globali ambiente//////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ewb_vm.h"
#include "revtable.h"

char variables[2500][200]; int nvars=0;    //1 mega complessivo. 
char functions[2500][200]; int nfuncs=0; 
int arity[256];   //arity della funzione.
int funcpos[256]; //L' indirizzo della funzione. 
int add_debug=0; 

int infunction=0; 
int endglobal=0; 

char* code; int cpos=0;   //Da valorizzare con il codice da compilare 
unsigned char* res; int respos=0;  //Da allocare per il codice compilato

///////////////////////////////Generiche/////////////////////////////////

//stream di linguaggio
extern char* code; extern int cpos; 
extern unsigned char* res; int respos; 
char currline[2000]; 
int nline=1; int nlpos=0;  
int firstsend=0; 
delperc() //Contiene gestione commenti e conteggio righe. 
{ if ((cpos==0) && (!firstsend))
     { int r; for (r=0; (code[cpos+r]!='\n') && (code[cpos+r]!=0); r++)
        { currline[r]=code[cpos+r]; currline[r+1]=0; 
        }; firstsend++; 
        if (add_debug==1)
        { out(DBG); out("["); 
          int t; char d[2]; d[1]=0; 
          for (t=cpos; (code[t]!=0) && (code[t]!='\n'); t++)
          { if (code[t]==']') { d[0]='\\'; out(d); };
            d[0]=code[t]; out(d); 
          }; out ("]"); 
        };
     };
  while ((code[cpos]==' ') || (code[cpos]=='\t') || (code[cpos]=='\n')
       ||(code[cpos]=='\r') || (code[cpos]=='\''))
  { if (code[cpos]=='\'') 
    { while ((code[cpos]!='\n') && (code[cpos]!=0)) cpos++; 
    }
    else
    { if (code[cpos]=='\n') 
      { int r; 
        nline++; nlpos=cpos+1; 
        for (r=0; (code[cpos+r+1]!='\n') && (code[cpos+r+1]!=0); r++)
        { currline[r]=code[cpos+r+1]; currline[r+1]=0; 
        };
        if (add_debug==1)
        { out(DBG); out("["); 
          int t; char d[2]; d[1]=0; 
          for (t=cpos+1; (code[t]!=0) && (code[t]!='\n'); t++)
          { if (code[t]==']') { d[0]='\\'; out(d); };
            d[0]=code[t]; out(d); 
          }; out ("]"); 
        };
      };
      cpos++; 
    };
  };
};

char currtok[2000]; 
int token()
{ int len=0; currtok[0]=0;  
  delperc(); 
  if (isatoken())
  { char r=code[cpos]; 
    while ( ((r>='a') && (r<='z')) || ((r>='A') && (r<='Z')) 
         || ((r>='0') && (r<='9')) || (r=='_') )
    { currtok[len]=r; len++; currtok[len]=0; 
      cpos++; r=code[cpos]; 
    };
  };
  return len; 
}; 

int istoken(char * v)
{ int res=0; 
  if (!strncmp(&code[cpos], v, strlen(v)))
  { char r=code[cpos+strlen(v)]; 
    res= (!( ((r>='a') && (r<='z')) || ((r>='A') && (r<='Z')) || ((r>='0') && (r<='9')) || (r=='_') ));
  }; 
  return res;
};
int isatoken()     //Legge un token senza mangiarlo
{ delperc(); char r=code[cpos]; 
  return ((r>='a') && (r<='z')) || ((r>='A') && (r<='Z')) || (r=='_');  
};

void err(char* er)
{ fprintf(stderr, "Ln %03i: %s\n", nline, currline);
  int k; for (k=0; k<cpos-nlpos+8; k++) fprintf (stderr, " "); 
  fprintf(stderr, "^ "); 
  //Dizionario di traduzione 
  if (!strcmp(er, "SNE")) { fprintf(stderr, "funzione o variabile sconosciuta\n"); exit(-1); };
  if (!strcmp(er, "NVR")) { fprintf(stderr, "nome di variabile errato\n"); exit(-1); };
  if (!strcmp(er, "NFN")) { fprintf(stderr, "nome di funzione errato\n"); exit(-1); };
  if (!strcmp(er, "NO=")) { fprintf(stderr, "mi aspettavo '='\n"); exit(-1); };
  if (!strcmp(er, "NO{")) { fprintf(stderr, "mi aspettavo '{' di apertura blocco\n"); exit(-1); };
  if (!strcmp(er, "NO}")) { fprintf(stderr, "mi aspettavo '}', blocco non chiuso\n"); exit(-1); };
  if (!strcmp(er, "NO(")) { fprintf(stderr, "mi aspettavo '(' dopo for\n"); exit(-1); };
  if (!strcmp(er, "NO)")) { fprintf(stderr, "mi aspettavo '(' \n"); exit(-1); };
  if (!strcmp(er, "NO]")) { fprintf(stderr, "mi aspettavo ']' di chiusura array\n"); exit(-1); };
  if (!strcmp(er, "EOL")) { fprintf(stderr, "mi aspettavo ';' a fine istruzione\n"); exit(-1); };
  if (!strcmp(er, "UNK")) { fprintf(stderr, "variabile o funzione sconosciuta\n"); exit(-1); };
  if (!strcmp(er, "DOW")) { fprintf(stderr, "ciclo do..while errato\n"); exit(-1); };
  if (!strcmp(er, "SYN")) { fprintf(stderr, "Syntax error\n"); exit(-1); };
  if (!strcmp(er, "PAR")) { fprintf(stderr, "Parametro errato a funzione\n"); exit(-1); };
  if (!strcmp(er, "TRP")) { fprintf(stderr, "Troppi parametri a funzione\n"); exit(-1); };
  if (!strcmp(er, "UAS")) { fprintf(stderr, "Codice assembler sconosciuto\n"); exit(-1); };
  if (!strcmp(er, "UAI")) { fprintf(stderr, "Istruzione assembler sconosciuta\n"); exit(-1); };
  if (!strcmp(er, "UAA")) { fprintf(stderr, "Indirizzo sconosciuto in assembler\n"); exit(-1); };
  if (!strcmp(er, "UAL")) { fprintf(stderr, "Label sconosciuta in assembler\n"); exit(-1); };
  if (!strcmp(er, "BOH")) { fprintf(stderr, "Token sconosciuto in assembler\n"); exit(-1); };
  fprintf(stderr, "errore sconosciuto %s\n", er); exit(-1);
};

void outch(unsigned char dt)
{ res[respos]= dt; respos++; res[respos]=0; 
#ifdef OUT_DEBUG
  printf("%c", dt); fflush(stdout); 
#endif
}; 
void out(char* dt)
{ strcpy((char*)(&res[respos]), dt);
  respos=respos+strlen(dt);  
#ifdef OUT_DEBUG
  printf("%s", dt); fflush(stdout); 
#endif
}; 

void outvar(int ncur)
{ if ((infunction) && (ncur > endglobal))
  { int v=nvars-ncur;  //Indirizzamento relativo a SP, e' una locale
    outch((unsigned char) ((v>>6)%64)+32); //Codificate 64*64 valori, big endian
    outch((unsigned char) (v%64)+32); 
  } else 
  { int v=ncur;       //Indirizzamento diretto, e' una globale. 
    outch((unsigned char) (((v>>6)%64)+32) | 0x80); //Codificate 64*64 valori, big endian
    outch((unsigned char) (v%64)+32); 
  }; 
};

void outadr(int adr)                     
{ adr=adr-respos+1; //Segno e spostamento relativo, 21 bit.   
  if (adr<0) { outch('-'); adr=-adr; } else outch('+'); 
  outch((unsigned char) ((adr>>14)%128)+32); 
  outch((unsigned char) ((adr>>7)%128)+32); 
  outch((unsigned char) (adr%128)+32); 
};

void putadr(int pos, int adr)
{ adr=adr-pos+1; //Segno e spostamento relativo, 21 bit.   
  if (adr<0) { res[pos]='-'; adr=-adr; } else res[pos]='+'; 
  res[pos+1]=((unsigned char) ((adr>>14)%128)+32); 
  res[pos+2]=((unsigned char) ((adr>>7)%128)+32); 
  res[pos+3]=((unsigned char) (adr%128)+32); 
};

eol()
{ delperc(); if ((code[cpos] !=';') && (code[cpos] != 0)) err("EOL");
  if (code[cpos]==';') cpos++;  
};

//Queste le uso se devo seguire un tratto di sintassi e poi sono costretto a 
//fare un rollback, dove il linguaggio non e' LL1.. Non bello ma comodo.
int envvr[5][500];
int envsp=0; 
savepoint() //Salva l' ambiente 
{ //printf("Saved cpos is %i, sp %i\n", cpos, envsp); 
  envvr[0][envsp]=cpos; envvr[1][envsp]=respos; envvr[2][envsp]=nvars, envvr[3][envsp]=nfuncs, envvr[4][envsp]=nline;
  envsp++; 
};
rollback() //ripristina l'ambiente
{ envsp--; 
  cpos=envvr[0][envsp]; respos=envvr[1][envsp]; nvars=envvr[2][envsp], nfuncs=envvr[3][envsp], nline=envvr[4][envsp];
  res[respos]=0; 
  //printf("Rollback cpos is %i\n", cpos); 
};

///////////////////////////////Espressioni///////////////////////////////

strvalue()
{ //char val[200000];  //Stiamo laaarghi.. 
  //val[0]=0; int pval=0; 
  outch('['); 
  if (code[cpos]=='"')
  { cpos++; 
    do 
    { if (code[cpos]=='\\')
      {  //Sequenza di escape.
         outch('\\'); cpos++; outch(code[cpos]); cpos++;  
      } else
      { if (code[cpos]!='"') 
       { if (code[cpos]==']') outch('\\'); 
         outch(code[cpos]); cpos++; }; 
      };
    } while (code[cpos]!='"'); 
  } else
  { if (code[cpos]=='[')
    { cpos++; 
      do 
      { if (code[cpos]=='\\')
        {  //Sequenza di escape.
           outch('\\'); cpos++; outch(code[cpos]); cpos++;  
        } else
        { if (code[cpos]!=']') { outch(code[cpos]); cpos++; }; 
        };
      } while (code[cpos]!=']');
    };
  };
  cpos++; outch(']'); 
};

char* dezero(char* c) //Tolgo da un numero con la virgola gli zeri finali
{ int ptpos=-1; int k; 
  for (k=0; k<strlen(c); k++)
  {if (c[k]=='.') ptpos=k; };
  if (ptpos>0) 
  { int ptz=1; 
    for (k=strlen(c)-1; k>ptpos; k--)
    { if (ptz)
      { if (c[k]=='0') { c[k]=0; } else ptz=0; };  
    };
    if (c[ptpos+1]==0) c[ptpos]=0; 
  }; return c; 
};

int ishexdigit()
{ char c=code[cpos]; 
  return ((c>='0') && (c<='9')) || ((c>='a') && (c<='f')) || ((c>='A') && (c<='F')); 
};
int hexdigit()
{ int res=0; char c = code[cpos];  
  if (c<='9') res= c-'0'; 
  if ((c>='a') && (c<='f')) res=c-'a' + 10; 
  if ((c>='A') && (c<='F')) res=c-'A' + 10; 
  cpos++; return res; 
};

num_hex()
{ float r=0, pos=1; 
  if (!ishexdigit()) err("SYN");
  while (ishexdigit())
  { r=r*16 + hexdigit() - '0'; cpos++; 
  }; 
  pos=pos/16; 
  if (code[cpos]=='.')
  { cpos++; if (!ishexdigit()) err("SYN");    
    while (ishexdigit())
    { r=r + ((float)(hexdigit()))*pos; cpos++; pos=pos/16;  
    }; 
  };
  out(LDA); char c[100]; sprintf(c, "%f", r); 
  out("["); out(dezero(c)); out("]");  
};

num_dec()
{ float r=0, pos=1;
  if ((code[cpos]<'0') && (code[cpos]>'9')) err("SYN");
  while ((code[cpos]>='0') && (code[cpos]<='9'))
  { r=r*10 + code[cpos]-'0'; cpos++; 
  }; 
  pos=pos/10; 
  if (code[cpos]=='.')
  { cpos++; if ((code[cpos]<'0') || (code[cpos]>'9')) err("SYN");
    while ((code[cpos]>='0') && (code[cpos]<='9'))
    { r=r + ((float)(code[cpos]-'0'))*pos; cpos++; pos=pos/10;  
    }; 
  };
  out(LDA); char c[100]; sprintf(c, "%f", r); 
  out("["); out(dezero(c)); out("]");  
};

num_oct()
{ float r=0, pos=1; 
  if ((code[cpos]<'0') && (code[cpos]>'7')) err("SYN");
  while ((code[cpos]>='0') && (code[cpos]<='7'))
  { r=r*8 + code[cpos]-'0'; cpos++; 
  }; 
  pos=pos/8; 
  if (code[cpos]=='.')
  { cpos++; if ((code[cpos]<'0') || (code[cpos]>'7')) err("SYN");     
    while ((code[cpos]>='0') && (code[cpos]<='7'))
    { r=r + ((float)(code[cpos]-'0'))*pos; cpos++; pos=pos/8;  
    }; 
  };
  out(LDA); char c[100]; sprintf(c, "%f", r); 
  out("["); out(dezero(c)); out("]");  
};

num_bin()
{ float r=0, pos=1; 
  if ((code[cpos]!='0') && (code[cpos]!='1')) err("SYN");
  while ((code[cpos]=='0') || (code[cpos]=='1'))
  { r=r*2 + code[cpos]-'0'; cpos++; 
  }; 
  pos=pos/2; 
  if (code[cpos]=='.')
  { cpos++; if ((code[cpos]!='0') && (code[cpos]!='1')) err("SYN");     
    while ((code[cpos]=='0') || (code[cpos]=='1'))
    { r=r + ((float)(code[cpos]-'0'))*pos; cpos++; pos=pos/2;  
    }; 
  };
  out(LDA); char c[100]; sprintf(c, "%f", r); 
  out("["); out(dezero(c)); out("]");  
};

numvalue()
{ delperc(); 
  int sign=1; 
  if ((code[cpos]=='-') || (code[cpos]=='+'))
  { if (code[cpos]=='-') sign=-1;
    cpos++;
  };
  if ((code[cpos]=='0') && (code[cpos+1]=='b')) { num_bin(); }
  else if ((code[cpos]=='0') && (code[cpos+1]=='o')) { num_oct(); }
  else if ((code[cpos]=='0') && (code[cpos+1]=='x')) { num_hex(); }
  else num_dec(); 
  if (sign<0) out (NEG); 
};

funzione(int num)
{ if (!predefs(num))  //Qui gestione funzione
  { delperc(); int pars=0; 
    if (code[cpos]=='(') { pars=1; cpos++; }; 

    int k; for(k=0; k<arity[num]; k++)
    { delperc(); 
      if (k>0)
      { if (code[cpos]!=',') err("PAR");
        cpos++; 
      };
      calcexp(); out(PUSH); 
    };
    delperc(); 
    if (pars) 
    { if (code[cpos]!=')') err("TRP"); 
      cpos++; 
    }; out(CALL); 
    int r=funcpos[num];
    outadr(r); 
    for(k=0; k<arity[num]; k++) out(DSP);  //Ho riempito lo stack.. lo svuoto.
  };
};

identifier()
{ if (!token()) err("SYN"); 
  int k, vp=-1; 
  for (k=0; k<nvars; k++)
      if (!strcmp(variables[k], currtok)) vp=k;
  if (vp>=0) { out (LDA); outvar(vp);      //VAR 
  } else    
  { for (k=0; k<nfuncs; k++)
        if (!strcmp(functions[k], currtok)) vp=k; 
    if (vp<0) err("SNE"); 
    funzione(vp); 
  };
};

tok_c()                         //token elementare
{ delperc(); 
  char r=code[cpos];
  if (r=='!')
  { cpos++; inexp(); out(NOT);  //Not 
  } else 
  { 
    if ((r=='+') || (r=='-') || ((r>='0') && (r<='9')) ) 
    { numvalue();               //numero
    } else
    { if ((r=='"') || (r=='['))
      { out (LDA); strvalue();             //stringa
      } else 
      { if (r=='(')             //espressione tra parentesi
        { cpos++; calcexp(); delperc();  
          if (code[cpos]!=')') err("NO)"); 
          cpos++; delperc(); 
        } else 
        { identifier();         //nome di tabella o di identificatore.
        }; 
      };
    }; 
  };
};

tok_k() //Gestione array. 
{ savepoint();  //Salvo il punto
  int done=0; 
  if (token()) 
  { delperc();
    int k, nv; nv=-1;  
    for(k=0; k<nvars; k++) { if(!strcmp(variables[k], currtok)) nv=k; };
    if (nv>=0)
    { if (code[cpos]=='[')                   //; variabile [calcexp]
      { done=1; 
        calcexp();                           //calcexp
        out(PUSH); nvars++;                  //STA tempvar 
        out(AGET); outvar(nv); 
        outvar(nvars-1);                     //AGET nv tempvar  //togo
        out(DSP); variables[nvars][0]=0; nvars--; 
      }; 
    };  
  } 
  if (!done)  
  { rollback(); //rollback 
    tok_c(); 
  };
};

tok_b() //And logico (&&) o bit-a-bit (&)
{ tok_k(); delperc(); 
  if (code[cpos]=='&') 
  { char ch=code[cpos]; 
    cpos++; out(PUSH); nvars++;  //Push A
    if (code[cpos]=='&') { cpos++; ch='#'; }; 
    tok_b();                 //Seconda espressione
    out(PUSH); nvars++;      //a->tmpvar
    out(LDA); outvar(nvars-2);//pop A 
    if (ch=='&') { out(AND); }
    else out(LAND);
            outvar(nvars-1);   //concatena tmpvar
    out(DSP); out(DSP); 
    variables[nvars][0]=0; nvars--; 
    variables[nvars][0]=0; nvars--;     
  };
};

tok_f() //Or logico (||) o bit-a-bit (|)
{ tok_b(); delperc(); 
  if (code[cpos]=='|') 
  { char ch=code[cpos]; 
    cpos++; out(PUSH); nvars++;//Push A
    if (code[cpos]=='|') { cpos++; ch='!'; }; 
    tok_f();                //Seconda espressione
    out(PUSH); nvars++;//a->tmpvar
    out(LDA); outvar(nvars-2);    //pop A 
    if (ch=='|') { out(OR); }
    else out(LOR);
            outvar(nvars-1);  //concatena tmpvar  
    out(DSP); out(DSP); 
    variables[nvars][0]=0; nvars--; 
    variables[nvars][0]=0; nvars--;     
  };
};

tok_t()  //Prodotto, rapporto, modulo
{ tok_f(); delperc(); 
  if ((code[cpos]=='*') || (code[cpos]=='/') || (code[cpos]=='%')) 
  { char ch=code[cpos]; 
    cpos++; out(PUSH); nvars++; //Push A 
    tok_t();                //Seconda espressione
    out(PUSH); nvars++;//a->tmpvar
    out(LDA); outvar(nvars-2);  //pop A 
    if (ch=='*') { out(PRD); }
    else if (ch=='/') { out(DIV); }
    else out(MOD);
            outvar(nvars-1);  //concatena tmpvar
    out(DSP); out(DSP); 
    variables[nvars][0]=0; nvars--; 
    variables[nvars][0]=0; nvars--;     
  };
};

tok_e()                     //somma, differenza
{ tok_t(); delperc(); 
  if ((code[cpos]=='+') || (code[cpos]=='-')) 
  { char ch=code[cpos]; 
    cpos++; out(PUSH); nvars++; //VAR A 
    tok_e();                //Seconda espressione
    out(PUSH); nvars++;     //    VAR B
    out(LDA); outvar(nvars-2);  // SUM A, B 
    if (ch=='+') { out(SUM); }
    else out(SUB);
            outvar(nvars-1);  //concatena tmpvar  
    out(DSP); out(DSP); 
    variables[nvars][0]=0; nvars--; 
    variables[nvars][0]=0; nvars--;     
  };
};

tok_exp()    //Concatenamento stringhe 
{ tok_e(); delperc(); 
  if (code[cpos]=='.') 
  { cpos++; out(PUSH); nvars++;//Push A 
    tok_exp();                //Seconda espressione
    out(PUSH); nvars++;       //a->tmpvar
    out(LDA);  outvar(nvars-2); //pop A 
    out(SCAT); outvar(nvars-1); //concatena tmpvar  
    out(DSP); out(DSP); 
    variables[nvars][0]=0; nvars--; 
    variables[nvars][0]=0; nvars--;     
  };
};

int is_cfr()
{ char a,b; a=code[cpos]; b=code[cpos+1];
  if ((a=='=') && (b=='=')) { cpos=cpos+2; return  1; }; 
  if ((a=='>') && (b=='=')) { cpos=cpos+2; return  2; }; 
  if ((a=='<') && (b=='=')) { cpos=cpos+2; return  3; };
  if (a=='>') { cpos++; return  4; }; 
  if (a=='<') { cpos++; return  5; };
  if ((a=='!') && (b=='=')) { cpos=cpos+2; return  6; }; 
  if ((a=='e') && (b=='q')) { cpos=cpos+2; return  7; }; 
  if ((a=='g') && (b=='t')) { cpos=cpos+2; return  8; }; 
  if ((a=='g') && (b=='e')) { cpos=cpos+2; return  9; }; 
  if ((a=='l') && (b=='t')) { cpos=cpos+2; return 10; }; 
  if ((a=='l') && (b=='e')) { cpos=cpos+2; return 11; };  
  if ((a=='n') && (b=='e')) { cpos=cpos+2; return 12; }; 
  return 0; 
};

cfr(int a)  //operandi in A e var(nvars)
{ int apos, bpos; 
  switch(a)  //Aggiungere istruzioni VM e eseguire
  { case 1: 	out(SUB);  outvar(nvars-1); 
                out(JZ);   apos=respos; out("...."); 
		out(LDA);  out("[0]");  
		out(JMP);  bpos=respos; out("....");
                putadr(apos, respos); 
                out(LDA);  out("[1]");    
                putadr(bpos, respos); 
		break; 
    case 2:     out(SUB);  outvar(nvars-1); 
                out(JN);   apos=respos; out("...."); 
		out(LDA);  out("[1]");  
		out(JMP);  bpos=respos; out("....");
                putadr(apos, respos); 
		out(LDA);  out("[0]");    
                putadr(bpos, respos); 
		break; 
    case 3:     out(SUB);  outvar(nvars-1); 
                out(JZN);  apos=respos; out("...."); 
		out(LDA);  out("[0]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[1]");    
		putadr(bpos, respos); 
		break; 
    case 4: 	out(SUB);  outvar(nvars-1); 
                out(JZN);  apos=respos; out("...."); 
		out(LDA);  out("[1]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[0]");    
		putadr(bpos, respos); 
		break;  
    case 5:  	out(SUB);  outvar(nvars-1); 
                out(JN);  apos=respos; out("...."); 
		out(LDA);  out("[0]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[1]");    
		putadr(bpos, respos); 
		break;
    case 6: 	out(SUB);  outvar(nvars-1); 
                out(JZ);   apos=respos; out("...."); 
		out(LDA);  out("[1]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[0]");    
		putadr(bpos, respos); 
		break;
    case 7: 	out(JEQ);  outvar(nvars-1); apos=respos; out("...."); 
		out(LDA);  out("[0]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[1]");    
		putadr(bpos, respos); 
		break;
    case 8: 	out(SUB);  outvar(nvars-1); 
                out(JLE);  apos=respos; out("...."); 
		out(LDA);  out("[1]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[0]");    
		putadr(bpos, respos); 
		break;
    case 9: 	out(SUB);  outvar(nvars-1); 
                out(JLT);  apos=respos; out("...."); 
		out(LDA);  out("[1]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[0]");    
		putadr(bpos, respos); 
		break;
    case 10: 	out(SUB);  outvar(nvars-1); 
                out(JLT);  apos=respos; out("...."); 
		out(LDA);  out("[0]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[1]");    
		putadr(bpos, respos); 
		break;
    case 11: 	out(SUB);  outvar(nvars-1); 
                out(JLE);  apos=respos; out("...."); 
		out(LDA);  out("[0]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[1]");    
		putadr(bpos, respos); 
		break;
    case 12: 	out(JEQ);  outvar(nvars-1); apos=respos; out("...."); 
		out(LDA);  out("[1]");  
		out(JMP);  bpos=respos; out("....");
		putadr(apos, respos); 
		out(LDA);  out("[0]");    
		putadr(bpos, respos); 
		break;
  };
};


inexp()
{ tok_exp(); 
  delperc(); int op;  
  if (op=is_cfr())        //.. Ho in accumulatore il primo valore e  
  { out(PUSH); nvars++;   //in una variabile temporanea il secondo.  
    tok_exp();       //Chiamo cfr con l'operazione di confronto
    out(PUSH); nvars++; out(LDA); outvar(nvars-2);              
    cfr(op); 
    out(DSP); out(DSP); 
    variables[nvars][0]=0; nvars--; 
    variables[nvars][0]=0; nvars--;     
  }; 
};

int is_assign()      //assign=variabile[calcexp] | variabile =
{ savepoint(); //push all 
  int mres=0; 
  if (token())
  { //Se e' una funzione ritorno 0
    int k; for (k=0; k<nfuncs; k++)
    { if (!strcmp(functions[k], currtok)) { rollback(); return 0; };
    };
    //delperc(); questo puo' dare problemi, lo tolgo.
    if (code[cpos]=='[')
    { cpos++; 
      calcexp();
      delperc(); 
      if (code[cpos]==']') 
       { cpos++; delperc(); 
         if ((code[cpos]=='=') && (code[cpos+1]!='=')) mres=1;
       }; 
    } else  
    { if ((code[cpos]=='=') && (code[cpos+1]!='=')) mres=1;
    };
  };
  rollback(); //Rollback
  return mres; 
};

calcexp()
{ if (is_assign())
  { int a=-1,c=0;  
    token();       //a = varnum, c=var_is_array
    int k; 
    for (k=0; k<nvars; k++)
    { if (!strcmp(variables[k], currtok)) a=k; 
    }; if (a<0) err("SNE");
    delperc(); 
    if (code[cpos]=='[') 
    { c=1; cpos++; 
      calcexp(); 
      out(PUSH); nvars++; 
      delperc(); if (code[cpos]!=']') err("NO]"); 
      cpos++; 
    };             //push the calcexp  
    delperc();   //Qui c'e' per forza l'uguale 
    cpos++; delperc(); 
    inexp();       //inexp 
    if (!c) { out(STA); outvar(a); }  //pop tmpvar
    else 
    { out (APUT); outvar(a); outvar(nvars-1); 
      out(DSP); variables[nvars][0]=0; nvars--; 
    };
  } else
  { inexp(); 
  };
};

//////////////////////////////////Cicli//////////////////////////////////

block() 
{ int nva=nvars; int nfu=nfuncs;  //Da qui ambiente locale
  delperc(); if (code[cpos]!='{') err("NO{");
  cpos++; delperc();  
  while ((code[cpos]!='}') && (code[cpos]!=0))
  { codice(); delperc(); 
  };     
  if(code[cpos]!='}') err("NO}"); 
  cpos++; delperc();              
  while (nvars>nva) 
  { out(DSP); variables[nvars][0]=0; nvars--; }; //dealloco le var. locali. 
  nfuncs=nfu;          //Qui torno al globale
};

downwhile()
{ cpos=cpos+2; delperc();       //do { [codice] } while calcexp;
  int point1=respos;            //1:
  block(); 			//codice
  if (!istoken("while")) err("DOW");  
  cpos=cpos+5; delperc(); 
  calcexp();                    //calcexp
  out (JZN); int jump2=respos; 
             out("....");       //JZN 2 
  out(JMP); 
  outadr(point1);              //JMP 1
  putadr(jump2, respos);       //2: 
  eol();  
};

whileblock()
{ cpos=cpos+5; delperc(); 
  //if (code[cpos]!='(') err("NO(");     //while (calcexp) { [codice] };
  int point1=respos; 
                 calcexp(); delperc(); //1: calcexp
  //if (code[cpos]!=')') err("NO)"); 
  //cpos++; 
  delperc();  
  out(JZN); int jump2=respos; out("...."); //jzn 2
  block(); eol();                       //codice 
  out(JMP);                            //jmp 1
  outadr(point1); 
  putadr(jump2, respos);              //2:
};

foreachblock()  //foreach varname(calcexp) { [codice] }
{ cpos=cpos+7; delperc(); 
  if (!token()) err("NVR"); 
  int varnum=-1, k; 
  for (k=0; k<nvars; k++)
  { if (!strcmp(variables[k], currtok)) varnum=k;  
  }; if (varnum<0) err("UNK"); 
  delperc(); if (code[cpos]!='(') err("NO(");
  int tmpvar=nvars;  
  calcexp(); out(PUSH); nvars++;  //outvar(tmpvar);  //tmpvar=calcexp
  int point1=respos;           //1:
  out(LDA); outvar(tmpvar); 
  out(JES); 
  int jump2=respos; out("....");//if tmpvar=="" jmp 2
  out(APOP); outvar(tmpvar); out(STA); 
  outvar(varnum);              //varname = pop tmpvar 
  block(); eol();              //codice 
  out(JMP);                    //jmp 1
  outadr(point1); 
  putadr(jump2, respos);      //2:
};

forblock()
{ cpos=cpos+3; delperc();        //for (exp0, exp1, exp2) ciclo: 
  if (code[cpos] != '(') err("NO("); 
  cpos++; calcexp();              //exp0
  int point1=respos;              //1:
  eol(); calcexp(); 
  out(JZN); 
  int jump2=respos; out("...."); //if !exp1 jmp 2 
  out(JMP);
  int jump3=respos; out("...."); //jmp 3 
  int point4=respos;              //4:
  eol(); calcexp(); delperc();    //exp2
  if(code[cpos]!=')') err("NO)"); 
  cpos++;
  out(JMP);                       //jmp 1
  outadr(point1); 
  delperc(); 
  int point3=respos; 
  block(); eol();                 //ciclo
  out(JMP);                       //jmp 4
  outadr(point4); 
  //Rimetto il valore di point2 in posizione
  int point2=respos; putadr(jump2, point2); 
  //Rimetto il valore di point3 in posizione
  putadr(jump3, point3); 
};

ifblock()
{ cpos=cpos+2; 
  calcexp(); //Qui la condizione e' vara o falsa
  out(JZN); 
  int endpos[1000]; int endnum=0;  
  int r=respos; 
  out("....");
  block(); 
  endpos[endnum]=respos+1; out(JMP); out("....");  //Salto sempre a fine ciclo
  endnum++;
  putadr(r, respos);  //Corretto l'indirizzo del salto.  
  while (istoken("elseif"))
  { cpos=cpos+6; delperc(); 
    calcexp(); 
    out(JZN); 
    int r=respos; 
    out("....");
    block(); 
    endpos[endnum]=respos+1; out("J....");  //Salto sempre a fine ciclo
    endnum++;
    putadr(r, respos);  //Corretto l'indirizzo del salto.      
  };
  if (istoken("else"))
  { cpos=cpos+4; block(); 
  };
  int k; for (k=0; k<endnum; k++)
  { putadr(endpos[k], respos);  //Jumps a fine ciclo
  };
  eol(); 
};

///////////////////////////Gestione/ambiente/////////////////////////////

fundecl()
{ //sub name([varname [,varname] ]) [ { [codice] } ];
  out(JMP); int endpos=respos; out("...."); //Salto a fine funzione
  infunction=1; 
  endglobal=nvars;
  int s=nvars;  
  variables[nvars][0] = 0; nvars++; //la posizione del return
//printf("Nvars: %i, vars (%s)(%s)(%s)(%s)(%s)(%s)(%s)\n", nvars, 
//variables[0], variables[1], variables[2], variables[3], variables[4], variables[5], variables[6]); 

  cpos=cpos+3; delperc(); 
  if (!token()) err("NFN"); 
  strcpy(functions[nfuncs], currtok); 
  delperc(); 
  if (code[cpos]!='(') err("NO("); 
  cpos++; delperc();  
  int *art=&(arity[nfuncs]); (*art)=0; 
  funcpos[nfuncs]=respos;  
  nfuncs++; 
  int k=nvars; 
  if (code[cpos]!=')') 
  { if (!token()) err("NVR");  //Dichiaro la variabile e uso pop per pescare i parametri.
    strcpy(variables[k], currtok); 
    (*art)++;  //Per rimettere l' indirizzo in stack faccio due pop e una push
    k++; 
    delperc(); 
  };
  while (code[cpos]==',')
  { cpos++;
    if (!token()) err("NVR");  //Dichiaro la variabile e uso pop per pescare i parametri.
    strcpy(variables[k], currtok); 
    (*art)++; 
    k++; 
    delperc();
  };
  if (code[cpos]!=')') err("NO)");   
  cpos++; delperc(); 
  if (code[cpos]=='{') 
  { nvars=k+1; //Aggiungo le locali
    block(); 
  }; 
  out(RET); 
  putadr(endpos, respos); //Valorizzo il jump 
  eol(); 
  for (k=s; k<=nvars; k++) variables[k][0]=0; 
  nvars=s; //L' indirizzo di ritorno.
  infunction=0;  
};


singlevar()
{ if (!token()) err("NVR"); //Dichiaro la variabile
  strcpy(variables[nvars], currtok); 
  int va=nvars;  
  delperc(); 
  if (code[cpos]=='=') //Assegnamento di valore
  { cpos++; delperc; inexp(); 
  } else { out(LDA); out ("[]"); }; //lda 0 
  out(PUSH); nvars++; 
  //out(STA); outvar(va); 
  delperc(); 
};

vardecl()
{ cpos=cpos+3; 
  singlevar(); 
  while (code[cpos]==',') { cpos++; singlevar();  };
  eol(); 
};

tabdecl()
{ //Memorizza l'associazione variabile->numero, e nella VM usa num 
  cpos=cpos+5; 
  if (!token()) err("NVR");
  strcpy(variables[nvars], currtok); 
  delperc(); 
  if (code[cpos]!='=') err("NO="); 
  cpos++; delperc(); 
  int tbpos=nvars; nvars++; 
  if (!token()) err("NOV"); delperc(); strcpy(variables[nvars], currtok);  
  while (code[cpos]==',')  
  { cpos++; 
    if (!token()) err("NOV"); delperc(); strcpy(variables[nvars], currtok); 
  };
  int k; char fields[50000]; fields[0]=0;
  for (k=tbpos+1; k<nvars; k++)
  { strcat(fields, variables[k]);
    if (k<(nvars-1)) strcat(fields, ":");  
  };
  out(LDA); out("["); out(fields); out("]"); //lda fields, sta var.tabella
  out(STA); outvar(tbpos);  
  eol(); 
};

//Da qui l' assemblatore in linea.. cerco di farlo one-pass
char labels[200][100];   int labpos[200]; int nlab=0;      //Label note
char requests[200][100]; int reqpos[200]; int nreq=0; //Label richieste

aparam()       //Gli array non sono accettati come parametri in asm
{ delperc(); 
  char r=code[cpos]; 
  if ((r=='"') || (r=='['))
    { strvalue(); return; 
    };  
  int k, vp=-1; delperc(); 
  if (token()<=0) err("BOH");
  for (k=0; k<nvars; k++)
  if (!strcmp(variables[k], currtok)) vp=k;
  if (vp>=0) { outvar(vp); } else err("SNE"); 
};

aaddress()
{ delperc();
  if (token()==0) err("UAA");
  int g; int fnd=0; 
  for (g=0; g<nlab; g++)
  { if (!strcmp(currtok, labels[g])) 
    { fnd=1; outadr(labpos[g]); 
    };
  }; 
  if (fnd==0) 
  { strcpy(requests[nreq], currtok);
    requests[nreq+1][0]=0; //Ricordo una richiesta di label
    reqpos[nreq]=respos; nreq++; 
    out("...."); 
  };
};

ainstr()
{ cpos++; 
  char fnd=' ';
  int t; for (t=0; currtok[t]!=0; t++)            //Upcase
    if ((currtok[t]>='a') && (currtok[t]<='z')) 
      currtok[t]=currtok[t]+'A'-'a';     
  int j; for(j=0; (fnd ==' ') && (j< NUM_ICODES); j++)
  { if (!strcmp(currtok, instructions[j][0]))
    { outch(instructions[j][1][0]); 
      fnd = instructions[j][1][1]; 
    };
  }; if (fnd==' ') err("UAI"); 
  if (fnd=='A') aaddress();   //indirizzo come parametro
  if (fnd=='S') { aparam(); aaddress(); }; //stringa e indirizzo, confronto di stringhe e salto
  if (fnd=='1') aparam();             // un parametro
  if (fnd=='2') { aparam(); aparam(); };  //due parametri 
};

alabel()
{ strcpy(labels[nlab], currtok);
  labels[nlab+1][0]=0; 
  labpos[nlab]=respos;
  int t; for (t=0; t<nreq; t++) //Se c'era una richiesta per la label
  { while (!strcmp(requests[t], currtok))    //la inserisco in posizione
    { putadr(reqpos[t], respos); int y;
      for (y=t; y<nreq; y++) 
      { strcpy(requests[y], requests[y+1]);  //Cancello la request
        reqpos[y]=reqpos[y+1]; 
      };
      nreq--; 
    };
  };
  nlab++; cpos++; delperc();   
};

asmline()
{ cpos++; 
  if (token() <= 0) err("UAS"); 
  if (code[cpos]==':')
  { alabel(); } else ainstr(); 
};

afinalcheck()
{ if (nreq>0) 
  { fprintf(stderr, "[%s]\n", requests[0]); 
    err("UAL"); 
  };
};

//Fino a qui

codice()
{ delperc(); 
  int p=cpos; 
  if (istoken("table"))   tabdecl(); 
  if (istoken("var"))     vardecl();
  if (istoken("if"))      ifblock();
  if (istoken("while"))   whileblock();
  if (istoken("do"))      downwhile();
  if (istoken("foreach")) foreachblock();
  if (istoken("for"))     forblock();
  if (istoken("sub"))     fundecl();
  if (code[cpos]=='.')    asmline(); 
  if (p==cpos) { calcexp(); eol(); }; 
  if (p==cpos) { err("SNE"); };
  delperc(); 
};
programma()
{ respos=0; cpos=0; while (code[cpos]!=0) codice(); 
  afinalcheck(); 
  //Inutile ma corretto. 
  while (nvars>0) { out(DSP); variables[nvars][0]=0; nvars--; };
};
