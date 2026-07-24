#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ewb_vm.h"
#include "opcodes.h"
#include "db_interface.h"
#include "ewb_predef.h"
int respos, cpos;
#define MAXOBJ 2500
#define  MAXLINE 10000
#define MAXVARLEN 80
#define MAXDATASET 100
#define MAXOUTPUTSIZE 1000000
/*
Il contratto delle espressioni: il risultato finisce in A, chi vuole conservarlo fa PUSH.
I costrutti di controllo (if, while, for, do...while): tutti riconducibili a LABEL, JMP, JZ e JNZ.
L'assembler integrato: sintassi minimale, con un controllo decente basato sulla tabella delle istruzioni.
L'ISA della VM come sorgente unica di verità: la stessa tabella servirà alla VM, all'assembler, al disassembler e, un domani, anche alla documentazione.
La separazione dei formati: testo per lo sviluppo, .ewm binario con magic non ASCII e versione quando tutto sarà stabile.

DA FARE: 
- aggiungi la regola JNCONTEXT con destinazione  come parametro e nomecontest in stack: salta se il nome non è una var del contesto corrente di in.
    come negli altri casi lo spazio dei nomi rimane locale alla VM. 
- Su assegnamento a orderby ci vuole una regola apposta.
- Iniziare a inserire le predefinite. 
- Forms web - sono costrutti del linguaggio
  - manca ask
  - manca form
  - manca showform
  - estendertli fino a supportare tuttii tipi amethist e oltre. 
EWBD - loggare le richieste (localmente alla singola macchina). Possibilmente a fine e socket chiusa.
EWBD - Supportare campi a dimensione dinamica
*/

char variables[MAXOBJ][MAXVARLEN]; int nvars=0;    //1 mega complessivo. 
typedef struct 
{ char[MAXVARLEN] name,  
  int arity,
  int position; 
} ptr;
ptr functions[MAXOBJ]; int nfuncs=0; 
ptr tasks[MAXOBJ];     int ntasks=0; 
ptr targets[MAXOBJ];   int ntargets=0; 
int infunction=0;    //Sono o meno dentro una funzione o dentro un task/target.. Stesso comportamento. 
int labelcount=0;    //Non sono in grado di assegnare subito l'indirizzo a alcune variabili.
int PC=0; 
int insideQuery=0;   //Non c'e' annidamento quindi posso mettere un flag qui. 

char res[MAXOUTPUTSIZE];
int respos=0; 

void out(char* dt)
{ char buf[200];
  strcpy((char*)(&res[respos]), dt);
  respos=respos+strlen(dt);
  res[respos]='\n';
  respos++;: 
  res[respos]=0;
  PC++; //Program counter conta le righe di assembler. 
}; 

char currline[MAXLINE]; 
int nline=1; int nlpos=0;  //Linea attuale e posizione di inizio della linea attuale.
delperc() //Contiene gestione commenti e conteggio righe. 
{ while ((code[cpos]==' ') || (code[cpos]=='\t') || (code[cpos]=='\n')
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
        printf("' %s\n",currline);  
      };
      cpos++; 
    };
  };
};

int isatoken()
{ char r=code[cpos]; 
  return ( ((r>='a') && (r<='z')) || ((r>='A') && (r<='Z')) || ((r>='0') && (r<='9')) || (r=='_') );
};

int istoken(char * v)
{ int res=0; 
  if (!strncmp(&code[cpos], v, strlen(v)))
  { char r=code[cpos+strlen(v)]; 
    res= (!( ((r>='a') && (r<='z')) || ((r>='A') && (r<='Z')) || ((r>='0') && (r<='9')) || (r=='_') ));
  }; 
  return res;
};

char currtok[MAXVARLEN]; 
int token()
{ int len=0; currtok[0]=0;  
  delperc(); 
  if (isatoken())
  { char r=code[cpos]; 
    while ( isatoken() )
    { currtok[len]=r; len++; currtok[len]=0; 
      cpos++; r=code[cpos]; 
    };
  };
  return len; 
}; 
int token_var()
{ int len=0; currtok[0]=0;  
  delperc(); 
  if (isatoken())
  { char r=code[cpos]; 
    while ( isatoken() ||  r=='.')
    { currtok[len]=r; len++; currtok[len]=0; 
      cpos++; r=code[cpos]; 
    };
  };
  return len; 
}; 


int is_assign()      //assign=variabile[calcexp] | variabile =
{ int back=cpos; //push all 
  int mres=0; 
  if (token())
  { //Se e' una funzione ritorno 0
    int k; for (k=0; k<nfuncs; k++)
    { if (!strcmp(functions[k], currtok)) { cpos=back; return 0; };
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
  cpos=back; //Rollback
  return mres; 
};

int asmline()
{ if (inside_sql) { err("Unsuported in SQL"); return; };
  char op[64];
  char arg[5000];
  char buf[5200];
  int hasarg = 0;
  cpos++;
  while(code[cpos]==' ' || code[cpos]=='\t') cpos++;  
  token();                    // legge IDENT
  strcpy(op, currtok);
  if (!op[0]) err("Missing asm instruction");
  int instr = find_vm_instr(op);
  if (instr < 0) err("Unknown asm instruction");
  while(code[cpos]==' ' || code[cpos]=='\t') cpos++;  
  if (code[cpos] != '\n' && code[cpos] != '\'' && code[cpos] != 0)
  { hasarg = 1;
    if (code[cpos] == '"')
    { if (argtype!=ARG_STRING && argtype!=ARG_INT_OR_STRING) err("Opcode doesn't accept strings"); 
      cpos++; 
      int p=0; arg[0]=0;
      while(code[cpos]!='"')
      { if (code[cpos])='\\') //tengo gli escape. 
        { arg[p]='\\'; cpos++; p++; 
        } 
        arg[p]=code[cpos]; p++; cpos++;
      };
      cpos++; 
      sprintf(buf, "%s \"%s\"\n", op, arg);
    } else if (code[cpos] >= '0' && code[cpos] <= '9')
    { if (argtype!=ARG_INT && argtype!=ARG_INT_OR_STRING) err("Opcode doesn't accept ints"); 
      int i=code[cpos] - '0'; 
      cpos++;
      while (code[cpos] >= '0' && code[cpos] <= '9')
      { i=i*10 + code[cpos] - '0'; 
        cpos++; 
      };
      sprintf(buf, "%s %i\n", op, i);
    } else sprintf(buf, "%s\n", op);        
    while(code[cpos]==' ' || code[cpos]=='\t') cpos++;  
  }
  if (code[cpos] == '\'')
  { while (code[cpos] && code[cpos] != '\n') cpos++;
  }
  if (code[cpos] != '\n') err("Bad asm line");
  cpos++;
  out(buf);   
}


datasetbody(); //Doppia ricorsione: Mi serve ma è sotto..

campidataset(char campi[][MAXDATASET])
{ int ncampi=5; //Ci sono sempre i predefiniti.  
  delperc();
  if (code[cpos]!='{') err("Missing {");
  cpos++; delperc();
  while (code[cpos]!='}')
  { token(); delperc();
    if (code[cpos]=='=') //Sto definendo un altro dataset in ricorsione
    { datasetbody();
      sprintf(campi[ncampi],"id_%s",currtok); 
      ncampi++; 
    } else if (code[cpos]==',' || code[cpos]=='}') 
    { cpos++; delperc; 
    } else err("dataset sn error"); 
  };
  //Se sono arrivato in fondo devo inserire il dataset.. 
  //QUI EMITDATASET 
  int i;
  char buf[200];
  for (i=1; i<ncampi; i++)
  { sprintf(buf, "ADDSYMTABLE %s.%s\"", campi[0], campi[i]);
    out(buf);
    if (i<6) out("PUSH \"%s\"", campi[i]); 
    if (i==1) { sprintf(buf, "PUSH \"%s\", DEFAULTENGINE); out(buf); }; 
    if (i==2) { sprintf(buf, "PUSH \"%s\", DEFAULTUSER); out(buf); };
    if (i==3) { sprintf(buf, "PUSH \"%s\", DEFAULTPASSWORD); out(buf); }; 
    if (i==4) out("PUSH \"stopped\");
    if (i==5) out("PUSH \"id\");  
    if (i<6) out("SETPATH 0"); 
  }; 
};

datasetbody()
{ char campi[MAXVARLEN][MAXDATASET]; 
  char buf[200];
  sprintf(buf,"ADDSYMTABLE \"%s\"", currtok);
  out(buf); 
  sprintf(campi[0],"%s", currtok);
  sprintf(buf,"PUSH \"%s\"", currtok);
  out(buf); 
  out("PUSH \"_dataset\"");
  out("SETPATH 0"); 
  if (code[cpos]!='=') err("Missing ="); 
  cpos++; 
  sprintf(campi[1],"_url");
  sprintf(campi[2],"_user");
  sprintf(campi[3],"_password");
  sprintf(campi[4],"_status");
  sprintf(campi[5],"_orderby");
  campidataset(campi);  
};

datasetdecl()
{ char campi[200][MAXDATASET]; 
  char buf[200];
  cpos=cpos + 7;
  delperc();
  token();
  datasetbody();
};

cron()
{ if (inside_sql) { err("unsuported in SQL"); return; };
  char buf[100];
  delperc();
  token();
  sprintf(buf("PUSH \"%s\"",currtok);
  out(buf);
  if (code[cpos]!='(') err("Missing ("); 
  cpos++; 
  int arity=0; 
  while (code[cpos]!=')') 
  { if (code[cpos]==')') err("Missing parameter"); 
    delperc();
    inexp();
    out("PUSHA");
    delperc();
    if (code[cpos]!=',' && code[cpos]!=')') err("Missing )"); 
    if (code[cpos]==',') cpos++;  
    arity++; 
  };
  sprintf(buf("CRONTASK %i", arity);
  delperc();
  inexp();
  out("PUSHA");
  out(buf); 
};

refresh()
{ if (inside_sql) { err("unsuported in SQL"); return; };



};

int startparallel()
{ if (!strcmp(currtoken,"cron")) { cron(); return 1; };
  if (!strcmp(currtoken,"refresh")) { refresh(); return 1; };
  if (!strcmp(currtoken,"run")) { tok_run(); return 1; };
  return 0;
};



tok_var()
{ char buf[200];
  int i; for (i=0; i<nvars; i++)
  { if (!strcmp(variables[i], currtok)) //Trovata la variabile. Potrei avere []
    { int endvar= labelcount++;
      if (inside_sql)
      { //Se la variabile fa parte di un context restituisco il suo nome. Dinamicamente. Ci pensa la VM.  
        sprintf(buf, ("PUSH \"%s\"",currtok); out(buf);
        int endcontext= labelcount++;
        sprintf(buf, ("JNCONTEXT \"%i\"",endcontext); out(buf);
        sprintf(buf, ("PUSH \"%s\"",currtok);
        out(buf);
        out("POPA");
        sprintf(buf, "JZ \"%i\", endvar); out(buf); //A!=0.. salto sempre.    
        setLabel(endcontext); 
      } 
      //La variabile è un caso particolare, viene passata per valore anche a sql 
      //Come nel normale codice: sostituisco la variabile con il suo valore. 
      int i=0;
      sprintf(buf,"PUSH \"%s\"", currtok);
      out(buf); 
      while (code[cpos]=='[')
      { cpos++; 
        inexp(); 
        out ("PUSHA");  
        delperc();
        if (code[cpos]!=']') err("Missing ]"); 
        cpos++; 
      };
      inexp();
      out ("PUSHA");   
      sprintf(buf,"PUSH %i", i);
      out(buf); 
      out ("GETPATH");
      setLabel(endvar);    
      return; 
    };
  };
  int i; for (i=0; i<nvars; i++)
  { if (is_predef(functions[i]) || !strcmp(functions[i].name, currtok)) //Chiamo la funzione. Push di parametri e call.
    { if (startparallel()) return;           //Le chiamate parallele sono trattate come funzioni 
      if (code[cpos]!='(') err("Missing (");
      cpos++; 
      int i=functions[i].arity; 
      out ("PUSHA"); //Un dato qualsiasi, qui ci va indirizzo di ritorno.   
      while(i>0)
      { inexp(); 
        delperc();
        out ("PUSHA");  
      };
      sprintf(buf,"PUSH %i", functions[i].arity);
      out(buf);
      if (is_predef(functions[i])) { predef(); return; };
      sprintf(buf,"CALL \"%s\"", functions[i].name);
      out(buf);
      return; 
    };
  };
  chr buf[200];
  sprintf(buf,"Unkown symbol %s", currtok); 
  err(buf); 
}

tok_k()
{ delperc()
  if (is_token())  //Qui gestione funzioni e variabili
  { tok_var();
  } else if (code[cpos]=='!')   
  { cpos++; 
    op="NOT";
    if (code[cpos]=='!') { cpos++; op="NOTB"; }; 
    if (inside_sql)
    { // NOT A  oppure ~ A
      if (!strcmp(OP,"NOT")) 
      { out("PUSH \"NOT(\"");    //      [not(]
      } else out("PUSH \"~(\"");
      inexp(); delperc();  //Unario 
      out("PUSHA");              //    [E,not)]
      out("PUSH \")\"");         //  [),E,not(]             
      out("CONCAT");             //      [not(]  A="E)"
      out("PUSHA");              //   [E),not(]
      out("CONCAT");             //          []  A="not(E)"
      }
    } else 
    { inexp(); delperc();  //Unario 
      out("PUSHA");
      out(op);
    };      
  } else (if ((code[cpos]=='-') || (code[cpos]=='+') || (code[cpos]>='0' && code[cpos]<='9'))
  { numvalue();
  } else (if ((code[cpos]=='"')
  { strvalue();
  } else (if ((code[cpos]=='(')
  { cpos++; 
    calcexp();
    if (code[cpos]!=')') err("Missing )"); 
    cpos++; 
  };
};

tok_b()    //And bit a bit, and logico
{ delperc(); tok_b(); delperc(); 
  char op[5]; op[0]=0;
  if (code[cpos]=='&' && code[cpos+1]!='&')   
  { cpos++; 
   op="AND";
  } else if (code[cpos]=='&' && code[cpos+1]=='&')   
  { op="ANDB";
    cpos++; cpos++; 
  } else return;
  if (inside_sql)
  { out("PUSHA");         //            [E1]
    if(!strcmp(op,"AND")) out("PUSH \" AND \"");
    if(!strcmp(op,"ANDB")) out("PUSH \" & \"");
                             //         [*,E1]
      tok_b();                         
      out("PUSHA");         //       [E2,*,E1]
      out("CONCAT");        //            [E1]  a="*e2"
      OUT("PUSH");          //      ["*E2",E1]
      out("CONCAT");        //              []  a="e1*e2" 
  } else
  { out("PUSHA"); 
    tok_b(); delperc();  
    out("PUSHA");
    out(op);      
  };
};

tok_f()    //Or bit a bit, or logico
{ delperc(); tok_b(); delperc(); 
  char op[5]; op[0]=0;
  if (code[cpos]=='|' && code[cpos+1]!='|')   
  { cpos++; 
   op="OR";
  } else if (code[cpos]=='|' && code[cpos+1]=='|')   
  { op="ORB";
    cpos++; cpos++; 
  } else return;
  if (inside_sql)
  { out("PUSHA");         //            [E1]
    if(!strcmp(op,"OR")) out("PUSH \" OR \"");
    if(!strcmp(op,"ORB")) out("PUSH \" | \"");
                             //         [*,E1]
      tok_f();                         
      out("PUSHA");         //       [E2,*,E1]
      out("CONCAT");        //            [E1]  a="*e2"
      OUT("PUSH");          //      ["*E2",E1]
      out("CONCAT");        //              []  a="e1*e2" 
  } else
  { out("PUSHA"); 
    tok_b(); delperc();  
    out("PUSHA");
    out(op);      
  };
};

tok_t()    //Prodotto, divisione, modulo
{ delperc(); tok_f(); delperc(); 
  char op[5]; op[0]=0;
  if (code[cpos]=='*')   
  { op="MUL";
  } else if (code[cpos]=='/')   
  { op="DIV";
  } else if (code[cpos]=='%')   
  { op="MOD";
  } else return;
  //Per sql: A * B 
  cpos++; 
  if (inside_sql)
  { if (code[cpos-1]=='/' || code[cpos-1]=='*') 
    { out("PUSHA");         //            [E1]
      sprintf(buf("PUSH \"%s\"", op); 
      out(buf);             //          [*,E1]
      tok_t();                         
      out("PUSHA");         //       [E2,*,E1]
      out("CONCAT");        //            [E1]  a="*e2"
      OUT("PUSH");          //      ["*E2",E1]
      out("CONCAT");        //              []  a="e1*e2" 
    } else  //mod(a,b) 
    { out("PUSH \" MOD(\"");  //                        [mod(]
      out("PUSHA");           //                     [E1,mod(] 
      tok_t();                          
      out("PUSH \",\"");      //                 [',',E1,mod(]
      out("PUSHA");           //              [E2,',',E1,mod(]
      out("CONCAT");          //                     [E1,mod(] A=",E2"
      out("PUSHA");           //               [',E2',E1,mod(]  
      out("CONCAT");          //                        [mod(] A="E1,E2"
      out("PUSHA");           //                ['E1,E2',mod(]
      out("PUSH \")\"");      //            [')','E1,E2',mod(]
      out("CONCAT");          //                        [mod(] A="E1,E2)"
      out("PUSHA");           //               ['E1,E2)',mod(]
      out("CONCAT");          //                            [] A="mod(E1,E2)"
    };
  } else 
  { out("PUSHA"); 
    tok_t(); delperc();  
    out("PUSHA");
    out(op);      
  };
};

tok_e()    //Somma e sottrazione numeriche 
{ delperc(); tok_t(); delperc(); 
  char op[5]; op[0]=0;
  char buf[10];
  if (code[cpos]=='+')   
  { op="SUM";
  } else if (code[cpos]=='-')   
  { op="SUB";
  } else return;
  cpos++; 
  //Per sql: A + B 
  if (inside_sql)
  { out("PUSHA");         //            [E1]
    sprintf(buf("PUSH \"%s\"", op); 
    out(buf);             //          [+,E1]
    tok_e();                         
    out("PUSHA");         //       [E2,+,E1]
    out("CONCAT");        //            [E1]  a="+e2"
    OUT("PUSH");          //      ["+E2",E1]
    out("CONCAT");        //              []  a="e1+e2" 
  } else 
  { out("PUSHA"); 
    tok_e();  
    out("PUSHA");
    out(op);      
  };
};

tok_exp()    //Concatenamento stringhe 
{ delperc(); tok_e(); delperc(); 
  if (code[cpos]=='.') 
  { cpos++;
    //Per sql: concat(A,B) 
    if (inside_sql)
    { out("PUSH \" CONCAT(\""); //                      [concat(]
      out("PUSHA");             //                   [E1,concat(] 
      tok_exp();                          
      out("PUSH \",\"");        //               [',',E1,concat(]
      out("PUSHA");             //            [E2,',',E1,concat(]
      out("CONCAT");            //                   [E1,concat(] A=",E2"
      out("PUSHA");             //             [',E2',E1,concat(]  
      out("CONCAT");            //                      [concat(] A="E1,E2"
      out("PUSHA");             //              ['E1,E2',concat(]
      out("PUSH \")\"");        //          [')','E1,E2',concat(]
      out("CONCAT");            //                      [concat(] A="E1,E2)"
      out("PUSHA");             //             ['E1,E2)',concat(]
      out("CONCAT");            //                             [] A="concat(E1,E2)"
    } else
    { //Concatenamento : A = EXP1 . EXP2.
      cpos++; out("PUSHA"); 
      tok_exp();  
      out("PUSHA");
      out("CONCAT");
    };      
  };
};

char* is_cfr()
{ char a,b; a=code[cpos]; b=code[cpos+1];
  if ((a=='=') && (b=='=')) { cpos=cpos+2; return  "EQ"; }; 
  if ((a=='>') && (b=='=')) { cpos=cpos+2; return  "GE"; }; 
  if ((a=='<') && (b=='=')) { cpos=cpos+2; return  "LE"; };
  if (a=='>') { cpos++; return  "GT"; }; 
  if (a=='<') { cpos++; return  "LT"; };
  if ((a=='!') && (b=='=')) { cpos=cpos+2; return  "NE"; }; 
  if ((a=='e') && (b=='q')) { cpos=cpos+2; return  "SEQ"; }; 
  if ((a=='g') && (b=='t')) { cpos=cpos+2; return  "SGT"; }; 
  if ((a=='g') && (b=='e')) { cpos=cpos+2; return  "SGE"; }; 
  if ((a=='l') && (b=='t')) { cpos=cpos+2; return  "SLT"; }; 
  if ((a=='l') && (b=='e')) { cpos=cpos+2; return  "SLE"; };  
  if ((a=='n') && (b=='e')) { cpos=cpos+2; return  "SNE"; }; 
  return ""; 
};
inexp()
{ delperc();
  tok_exp(); 
  delperc(); int op;  
  char* op=is_cfr();
  if (op[0]!=0)   
  { if (inside_sql)  
    { //Per confronto numerico: A = EXP1 . {sqlcond} . EXP2
      //PUSH "("
      //PUSH DUMMY             [DUMMY,(]
      // PUSH EXP1        [EXP1,DUMMY,(]
      // Se confronto numerico, eccezione se non sono numeri
      //    PUSH 0    
      //    SUM
      //    PUSH A 
      // DECSP 2          EXP1,DUMMY,[(]
      // tok_exp()            
      // PUSH EXP2         EXP1,[EXP2,(]
      // Se confronto numerico, eccezione se non sono numeri
      //    PUSH 0    
      //    SUM
      //    PUSH A 
      // INCSP 1           [EXP1,EXP2,(]
      // push OP        [OP,EXP1,EXP2,(]  " >= " 
      // CONCAT                 [EXP1,(] A="EXP1 OP "
      // PUSHA        ["EXP1 OP",EXP1,(]
      // CONCAT                      [(] A="EXP1 OP EXP2"
      // PUSHA        ["EXP1 OP EXP2",(]
      // CONCAT                       [] A="(EXP1 OP EXP2"
      // PUSHA         ["(EXP1 OP EXP2"]
      // PUSH ")"    [),"(EXP1 OP EXP2"]
      // CONCAT                       [] A="(EXP1 OP EXP2)"
      out("PUSH \"(\""); out("PUSH \"DUMMY\""); out("PUSHA");
      if (op[0]!='S') { out("PUSH 0"); out("SUM"); out("PUSHA"); };
      out("DECSP 2");
      tok_exp(); //Ora in A c'e' EXP2
      if (op[0]!='S') { out("PUSH 0"); out("SUM"); out("PUSHA"); };
      out("INCSP 1");
      if (op[0]=='E' && op[1]=='Q') out("PUSH \" = \"");
      if (op[0]=='G' && op[1]=='E') out("PUSH \" >= \"");
      if (op[0]=='L' && op[1]=='E') out("PUSH \" <= \"");
      if (op[0]=='G' && op[1]=='T') out("PUSH \" > \"");
      if (op[0]=='L' && op[1]=='T') out("PUSH \" < \"");
      if (op[0]=='N' && op[1]=='E') out("PUSH \" != \"");
      if (op[1]=='E' && op[2]=='Q') out("PUSH \" = \"");
      if (op[1]=='G' && op[2]=='E') out("PUSH \" >= \"");
      if (op[1]=='L' && op[2]=='E') out("PUSH \" <= \"");
      if (op[1]=='G' && op[2]=='T') out("PUSH \" > \"");
      if (op[1]=='L' && op[2]=='T') out("PUSH \" < \"");
      if (op[1]=='N' && op[2]=='E') out("PUSH \" != \"");
      out("CONCAT"); 
      out("PUSHA"); out("CONCAT"); 
      out("PUSHA"); out("CONCAT"); 
      out("PUSHA"); out("PUSH \")\""); out("CONCAT");
    } else 
    { out("PUSHA");    
      tok_exp(); //Qui nessuna ripetizione del confronto..  a > b <= c -> SN error
      out("PUSHA");              
      out(op);
    }; 
  }; 
};

fundecl(unsigned char type)
{ char buf[200]; 
  size=3;
  if (type==1) size=4; //TASK 
  if (type==2) size=6; //TARGET 
  cpos=cpos+size; delperc(); 
  int lend = labelcount++; //Salto a fine funzione. 
  infunction=1; 
  int endglobal=nvars;
  int basevars=nvars; //Questo mi serve per contare anche eventuali variabili locali. 
  variables[nvars][0] = 0; nvars++; //la posizione del return
//printf("Nvars: %i, vars (%s)(%s)(%s)(%s)(%s)(%s)(%s)\n", nvars, 
//variables[0], variables[1], variables[2], variables[3], variables[4], variables[5], variables[6]); 
  
  if (!token()) err("Syntax error in function"); 
  strcpy(functions[nfuncs], currtok); 
  funtypes[npos]=type;
  sprintf(buf, "ADDSYMTABLE \"%s\"", currtok);
  out(buf); 
  sprintf(buf, "PUSH %s", currtok); //Nome
  out(buf);
  sprintf(buf, "PUSH %i", PC+1); //Valore 
  out(buf);
  out("setpath 0"); 
  sprintf(buf, "JMP *LABEL%i*", lnext);
  out(buf);
  //Se è un task o un target serve il punto di lancio.
  if (type==1) out("TASK");
  if (type==2) out("TARGET"); 
  delperc(); 
  if (code[cpos]!='(') err("Missing ("); 
  cpos++; delperc();  
  int *art=&(arity[nfuncs]); (*art)=0; 
  funcpos[nfuncs]=respos;  
  nfuncs++; 
  int k=nvars; 
  if (code[cpos]!=')') 
  { if (!token()) err("Fun.par syntax error");  //Dichiaro la variabile e uso pop per pescare i parametri.
    strcpy(variables[k], currtok); 
    (*art)++;  
    k++; 
    delperc();
  };
  while (code[cpos]==',')
  { cpos++;
    if (!token()) err("Fun.par syntax error");  //Dichiaro la variabile e uso pop per pescare i parametri.
    strcpy(variables[k], currtok); 
    (*art)++; 
    k++; 
    delperc();
  };
  nvars=k+1; //Aggiungo le locali
  sprintf(buf, "DECSP %i", (*art));
  out(buf);
  int i; 
  for (i=; i<nvars; i++)
  { sprintf(buf, "ADDSYMTABLE \"%s\"", variables[i]);
    out(buf); 
  };
  if (code[cpos]!=')') err("Missing )");   
  cpos++; delperc(); 
  block();  
  setLabel(lend); 
  sprintf(buf, "DELSYMTABLE", nvars-basevars);
  out(buf);
  //Se è un task o un target serve il punto di termine.
  if (type==1) out("ENDTASK");
  if (type==2) out("ENDTARGET"); 
  out("RET"); 
  nvars=basevars; 
  infunction=0;  
};


downwhileblock()
{ int lstart;
  char buf[100];
  cpos += 2;          // "do"
  delperc();
  lstart = labelcount++;
  setLabel(lstart);
  block();
  if (!istoken("while")) err("Missing while");
  cpos += 5;
  delperc();
  calcexp();
  sprintf(buf, "JNZ *LABEL%i*\n", lstart);
  out(buf);
}

whileblock()
{ int lstart, lend;
  char buf[100];
  cpos += 5;        // "while"
  delperc();
  lstart = labelcount++;
  lend   = labelcount++;
  setLabel(lstart);
  calcexp();
  sprintf(buf, "JZ *LABEL%i*\n", lend);
  out(buf);
  block();
  sprintf(buf, "JMP *LABEL%i*\n", lstart);
  out(buf);
  setLabel(lend);
}

forblock()
{ int lcond, lstep, lbody, lend;
  char buf[100];
  cpos += 3;
  delperc();
  if (code[cpos] != '(') err("Missing (");
  cpos++;
  calcexp();              // exp0
  if (code[cpos] != ';') err("Missing ;");
  cpos++;
  delperc();
  lcond = labelcount++;
  lstep = labelcount++;
  lbody = labelcount++;
  lend  = labelcount++;
  setLabel(lcond);
  calcexp();              // exp1
  sprintf(buf, "JZ *LABEL%i*\n", lend);
  out(buf);
  sprintf(buf, "JMP *LABEL%i*\n", lbody);
  out(buf);
  setLabel(lstep);
  if (code[cpos] != ';') err("Missing ;");
  cpos++;
  delperc();
  calcexp();              // exp2
  if (code[cpos] != ')') err("Missing )");
  cpos++;
  delperc();
  sprintf(buf, "JMP *LABEL%i*\n", lcond);
  out(buf);
  setLabel(lbody);
  block();
  sprintf(buf, "JMP *LABEL%i*\n", lstep);
  out(buf);
  setLabel(lend);
}

ifblock()
{ int lend, lnext;
  char buf[100];
  cpos += 2;
  delperc();
  lend = labelcount++;
  calcexp();
  lnext = labelcount++;
  sprintf(buf, "JZ *LABEL%i*", lnext);
  out(buf);
  block();
  sprintf(buf, "JMP *LABEL%i*", lend);
  out(buf);
  setLabel(lnext);
  while (istoken("elseif"))
  { cpos += 6;   // controlla: "elseif" sono 6 caratteri
    delperc();
    calcexp();
    lnext = labelcount++;
    sprintf(buf, "JZ *LABEL%i*", lnext);
    out(buf);
    block();
    sprintf(buf, "JMP *LABEL%i*", lend);
    out(buf);
    setLabel(lnext);
  };
  if (istoken("else"))
  { cpos += 4;   // controlla: "else" sono 4 caratteri
    delperc();
    block();
  };
  setLabel(lend);
};

calcexp()
{ delperc();
  if (is_assign())
  { int a=-1;
    token_var(); //Questo DEVE accettare anche i puntini  nome.nome.nome ...
    int k;
    for (k = 0; k < nvars; k++) 
    { char *p = variables[k];
      while (p) 
      { if (!strcmp(p, currtok)) { a = k; break; }
        p = strchr(p, '.');
        if (p) p++;
      };
    };
    if (a < 0) err("Unknown symbol");  //Ok fa match anche se è parte di una variabile in piu sezioni. 
    delperc();
    //Caso particolare: se assegno a _orderby devo verificare la sintassi come espressione SQL 
    if (currtok==_orderby)
    { 

[...]

    } else 
    { out("PUSH \"");
      out(variables[a]);
      out("\"\n");
      int nindexes=0;
      while (code[cpos]=='[')
      { cpos++;
        calcexp();
        out("PUSHA"); 
        nindexes++;
        delperc();
        if (code[cpos]!=']') err("Missing ]");
        cpos++;
        delperc();
      };
    if (code[cpos] != '=') err("Missing =");
    cpos++;
    delperc();
    inexp();
    out("PUSHA"); 
    char b(100);
    sprintf(b,"SETPATH %i\n", nindexes);
    out(b);
  } else inexp();
};

inblock()
{ cpos=cpos+2; 
  //in ( variabile[.variabile] [, condizione] ) blocco
  if (code[cpos] != '(') err("Missing (");
  cpos++;
  inexp(); //Qui A contiene il come della tabella.                                STACK
  out("PUSH \"dummy\""); //                                                     [dummy]
  out("PUSHA");  //PUSH CONTEXT                                         [CONTEXT,dummy]
  out("PUSHA");  //PUSH CONTEXT                                 [CONTEXT,CONTEXT,dummy]
  delperc;
  if (code[cpos]==',')
  { cpos++; 
    inside_query=true;  
    calcexp(); //Qui viene eseguito codice e A contiene la query normalizzata.
    inside_query=false; 
    out("PUSHA");  //PUSH CONDICTION                       [COND,CONTEXT,CONTEXT,dummy]
    delperc;
  } else //La inall è una in senza condizione. 
  { out("PUSH \"true\""); 
  };
  if (code[cpos] != ')') err("Missing )");
  cpos++; 
  //Qui devo inserire l'ordinamento. E' context.orderby ma non ho il dato qui.
  //Ci pensa la QLIST..  
  out("QLIST");  //                                                     [context,dummy]  
  out ("PUSH 0");//                                                   [0,CONTEXT,dummy]
  out("DECSP 4");                                      
  out ("PUSHA");                                                                                 
  out("INCSP 4");//                                                 [0,IDS,CONTEXT,IDS]
  int loop=PC; 
//LOOP:
  //Ho A e IDS gia pronti per una getpath.. ma poi devo rimetterli in stack. La getpatch deve avere i TRE parametri in stack
  out("GETPATH"); //A=IDS[ID]                                             [CONTEXT,IDS]
  out("JZ *LABEL%i*\n", labelcount); //Finiti gli id, Salta al termine
  int endin=labelcount; 
  labelcount++; 
  out ("PUSH A");//                                                    [ID,CONTEXT,IDS]   
  out("QBYID");  //Variabili contesto valorizzate da qbyid                        [IDS]
  out("JNZ *LABEL%i*\n", labelcount); //Evita il codice sotto se il dato c'e'
  int co=labelcount; 
  labelcount++; 
  //Riporta a posto lo stack e salta a LOOP  
  out("INCSP 2");    //                                                [ID,CONTEXT,IDS]
  out("PUSH 1");     //                                              [1,ID,CONTEXT,IDS]                                                   
  out("SUM");        //A=ID+1                                             [CONTEXT,IDS]
  out("push \"dummy\"");   //                                       [dummy,CONTEXT,IDS]  
  out("push A");     //                                          [id,dummy,CONTEXT,IDS]
  out("DECSP 4");    //
  out("POP A");      //                                            id,ids,context,IDS[]
  out("INCSP 3");    //                                            id,ids,[context,IDS]
  out("PUSH A");     //                                            id,[IDS,CONTEXT,IDS]
  out("INCSP 1");    //                                            [id,IDS,CONTEXT,IDS]
  sprintf(buf,"JMP %i", loop); out(buf);  
//CONT:
  setLabel(co); 
  out("INCSP 2");    //                                                [ID,CONTEXT,IDS]
  out("PUSH 1");     //                                              [1,ID,CONTEXT,IDS]                                                   
  out("SUM");        //A=ID+1                                             [CONTEXT,IDS]
  out("push \"dummy\"");   //                                       [dummy,CONTEXT,IDS]  
  out("push A");     //                                          [id,dummy,CONTEXT,IDS]
  out("DECSP 4");    //
  out("POP A");      //                                            id,ids,context,IDS[]
  out("INCSP 3");    //                                            id,ids,[context,IDS]
  out("PUSH A");     //                                            id,[IDS,CONTEXT,IDS]
  out("INCSP 1");    //                                            [id,IDS,CONTEXT,IDS]
  delperc();
  blocco();  
  sprintf(buf,"JMP %i", loop); out(buf);  
  setLabel(endin);
};

forblock()
{ int lcond, lstep, lbody, lend;
  char buf[100];
  cpos += 3;
  delperc();
  if (code[cpos] != '(') err("Missing (");
  cpos++;
  calcexp();              // exp0
  if (code[cpos] != ';') err("Missing ;");
  cpos++;
  delperc();
  lcond = labelcount++;
  lstep = labelcount++;
  lbody = labelcount++;
  lend  = labelcount++;
  setLabel(lcond);
  calcexp();              // exp1
  sprintf(buf, "JZ *LABEL%i*\n", lend);
  out(buf);
  sprintf(buf, "JMP *LABEL%i*\n", lbody);
  out(buf);
  setLabel(lstep);
  if (code[cpos] != ';') err("Missing ;");
  cpos++;
  delperc();
  calcexp();              // exp2
  if (code[cpos] != ')') err("Missing )");
  cpos++;
  delperc();
  sprintf(buf, "JMP *LABEL%i*\n", lcond);
  out(buf);
  setLabel(lbody);
  block();
  sprintf(buf, "JMP *LABEL%i*\n", lstep);
  out(buf);
  setLabel(lend);
}

ifblock()
{ int lend, lnext;
  char buf[100];
  cpos += 2;
  delperc();
  lend = labelcount++;
  calcexp();
  lnext = labelcount++;
  sprintf(buf, "JZ *LABEL%i*", lnext);
  out(buf);
  block();
  sprintf(buf, "JMP *LABEL%i*", lend);
  out(buf);
  setLabel(lnext);
  while (istoken("elseif"))
  { cpos += 6;   // controlla: "elseif" sono 6 caratteri
    delperc();
    calcexp();
    lnext = labelcount++;
    sprintf(buf, "JZ *LABEL%i*", lnext);
    out(buf);
    block();
    sprintf(buf, "JMP *LABEL%i*", lend);
    out(buf);
    setLabel(lnext);
  };
  if (istoken("else"))
  { cpos += 4;   // controlla: "else" sono 4 caratteri
    delperc();
    block();
  };
  setLabel(lend);
};

calcexp()
{ delperc();
  if (is_assign())
  { int a=-1;
    token();
    int k;
    for (k=0; k<nvars; k++)
      if (!strcmp(variables[k], currtok)) a=k;
    if (a<0) err("Unknown symbol");
    delperc();
    out("PUSH \"");
    out(variables[a]);
    out("\"\n");
    int nindexes=0;
    while (code[cpos]=='[')
    { cpos++;
      calcexp();
      out("PUSHA"); 
      nindexes++;
      delperc();
      if (code[cpos]!=']') err("Missing ]");
      cpos++;
      delperc();
    };
    if (code[cpos] != '=') err("Missing =");
    cpos++;
    delperc();
    inexp();
    out("PUSHA"); 
    char b[100];
    sprintf(b, "SETPATH %i\n", nindexes);
    out(b);
  } else inexp();
};

/* ========================================================================
   FORMS: DA VERIFICARE
   Ogni primitiva produce un frammento in A. I frammenti vengono accodati
   esplicitamente con PUSHA e CONCAT.
   ======================================================================== */

int formvar(char *name) /* FORMS: DA VERIFICARE */
{ int i;
  for (i=0; i<nvars; i++)
  { char *p=variables[i];
    while (p)
    { if (!strcmp(p,name)) return i;
      p=strchr(p,'.');
      if (p) p++;
    };
  };
  return -1;
};

formfield() /* FORMS: DA VERIFICARE */
{ char name[MAXVARLEN];
  char style[MAXVARLEN];
  char buf[3*MAXVARLEN];
  int var;

  if (!token_var()) err("Missing form field");
  strcpy(name,currtok);
  var=formvar(name);
  if (var<0) err("Unknown form field");

  strcpy(style,"text");
  delperc();
  if (code[cpos]==':')
  { cpos++;
    if (!token()) err("Missing form field type");
    strcpy(style,currtok);
  };

  out("PUSHA");
  sprintf(buf,"PUSH \"%s\"",name); out(buf);
  sprintf(buf,"PUSH \"%s\"",style); out(buf);
  sprintf(buf,"PUSH \"%s\"",variables[var]); out(buf);
  out("PUSH 0");
  out("GETPATH");
  out("PUSHA");
  out("ADDFORM");
  out("PUSHA");
  out("CONCAT");
};

formfields() /* FORMS: DA VERIFICARE */
{ formfield();
  delperc();
  while (code[cpos]==',')
  { cpos++;
    formfield();
    delperc();
  };
};

formblock() /* FORMS: DA VERIFICARE */
{ cpos+=4;
  delperc();
  out("MOVA \"\"");
  formfields();
};

askblock() /* FORMS: DA VERIFICARE */
{ cpos+=3;
  delperc();
  out("STARTFORM");
  formfields();
  out("PUSHA");
  out("ENDFORM");
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("SHOW");
  out("STOP");
};

showformblock() /* FORMS: DA VERIFICARE */
{ cpos+=8;
  delperc();
  out("STARTFORM");
  out("PUSHA");
  inexp();
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("ENDFORM");
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("PRINT");
  out("STOP");
};

/* ======================== FINE FORMS: DA VERIFICARE ===================== */

codice()
{ delperc(); 
  int p=cpos; 
//Tipi strutturati
  if (istoken("dataset")) datasetdecl(); 
  if (istoken("task"))    taskdecl(); 
  if (istoken("target"))  targetdecl(); 
//Database
  if (istoken("in"))      inblock();
  if (istoken("delete"))  deleteblock();
  if (istoken("add"))     addblock();
//Web: DA VERIFICARE
  if (istoken("ask"))      askblock();
  if (istoken("form"))     formblock();
  if (istoken("showform")) showformblock();
//Cicli
  if (istoken("if"))      ifblock();
  if (istoken("while"))   whileblock();
  if (istoken("do"))      downwhile();
  if (istoken("foreach")) foreachblock();
  if (istoken("for"))     forblock();
//Vaiabili e funzioni
  if (istoken("var"))     vardecl();
  if (istoken("sub"))     fundecl(0);
  if (code[cpos]=='.')    asmline(); 
  if (p==cpos) { calcexp(); }; 
  if (p==cpos) { err("syntax error"); };
  delperc(); 
  if (code[cpos]!=';') err("missing ;");
  cpos++;  
};
blocco()
{ delperc(); 
  if (code[cpos]=='{') 
  { cpos++; 
    codice(); 
    delperc(); 
    if (code[cpos]!='}') err("missing }"); 
    cpos++;
    delperc();
  } else codice();  
}
programma()
{ while (code[cpos]!=0) 
  { blocco(); 
    delperc(); 
    if (code[cpos]!=';') err("missing ;");
    cpos++;  
  };
};

main(int argc, char** argv)
{ if ((argc != 3) && (argc != 4)) //per ora rigido: inputfile, outputfile
  { printf ("Uso %s [-d] inputfile.ewb outputfile.evm\n", argv[0]); 
    exit(-1); 
  };
  int dpos=0; 
  if (!strcmp(argv[1], "-d")) { dpos++; add_debug=1; };
  int f=open(argv[1+dpos], O_RDONLY); 
  if (f>0) 
  { code = (char*)malloc(100000);
    res = (unsigned char*)malloc(50000);
    add_predefs(); 
    int i=read(f, code, 100000); 
    close(f); printf("Read %i bytes\n", i);
    code[i]=0; 
    respos=0; 
    cpos=0; 
    programma();
    printf("\nOutput %i lines\n", PC);  
    f=open(argv[2+dpos], O_WRONLY | O_CREAT | O_TRUNC, 0777); 
    write(f, res, respos); 
    close(f);   
  } else printf("Errore di lettura file origine\n");  
};
