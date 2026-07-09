#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ewb_vm.h"
#include "opcodes.h"
#include "db_interface.h"
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
int istoken(char * v)
{ int res=0; 
  if (!strncmp(&code[cpos], v, strlen(v)))
  { char r=code[cpos+strlen(v)]; 
    res= (!( ((r>='a') && (r<='z')) || ((r>='A') && (r<='Z')) || ((r>='0') && (r<='9')) || (r=='_') ));
  }; 
  return res;
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


int asmline()
{ char op[64];
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





datasetbody();
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
    if (i<5) out("PUSH \"%s\"", campi[i]); 
    if (i==1) { sprintf(buf, "PUSH \"%s\", DEFAULTENGINE); out(buf); }; 
    if (i==2) { sprintf(buf, "PUSH \"%s\", DEFAULTUSER); out(buf); };
    if (i==3) { sprintf(buf, "PUSH \"%s\", DEFAULTPASSWORD); out(buf); }; 
    if (i==4) out("PUSH \"stopped\");  
    if (i<5) out("SETPATH 0"); 
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
{ char buf[100];
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
{ 
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
    { int i=0;
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
      sprintf(buf,"SETPATH %i", i);
      out(buf); 
      return; 
    };
  };
  i; for (i=0; i<nvars; i++)
  { if (!strcmp(functions[i].name, currtok)) //Chiamo la funzione. Push di parametri e call.
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
    if (code[cpos]=='!') { cpos++; op="NOTB"; 
    out("PUSH"); 
    inexp(); delperc();  
    out("PUSH");
    out(op);      
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
  out("PUSH"); 
  tok_k(); delperc();  
  out("PUSH");
  out(op);      
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
  out("PUSH"); 
  tok_b(); delperc();  
  out("PUSH");
  out(op);      
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
  cpos++; 
  out("PUSH"); 
  tok_t(); delperc();  
  out("PUSH");
  out(op);      
};

tok_e()    //Somma e sottrazione numeriche 
{ delperc(); tok_t(); delperc(); 
  char op[5]; op[0]=0;
  if (code[cpos]=='+')   
  { op="SUM";
  } else if (code[cpos]=='-')   
  { op="SUB";
  } else return;
  cpos++; 
  out("PUSH"); 
  tok_e();  
  out("PUSH");
  out(op);      
};

tok_exp()    //Concatenamento stringhe 
{ delperc(); tok_e(); delperc(); 
  if (code[cpos]=='.') 
  { cpos++; out("PUSH"); 
    tok_exp();  
    out("PUSH");
    out("CONCAT");      
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
  op=is_cfr();
  if (op!="")   
  { out("PUSH");    
    tok_exp();      
    out("PUSH");              
    out(op); 
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
      out("PUSH"); 
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
    out("PUSH"); 
    char b[100];
    sprintf(b, "SETPATH %i\n", nindexes);
    out(b);
  } else inexp();
};

codice()
{ delperc(); 
  int p=cpos; 
  if (istoken("dataset")) datasetdecl(); 
  if (istoken("task"))    taskdecl(); 
  if (istoken("target"))  targetdecl(); 
  if (istoken("in") || istoken("inall")) inblock();
  if (istoken("if"))      ifblock();
  if (istoken("var"))     vardecl();
  if (istoken("while"))   whileblock();
  if (istoken("do"))      downwhile();
  if (istoken("foreach")) foreachblock();
  if (istoken("for"))     forblock();
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
  { printf ("Uso %s [-d] inputfile.ewb outputfile.cgi\n", argv[0]); 
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
    printf("\nOutput %i bytes\n", respos);  
    f=open(argv[2+dpos], O_WRONLY | O_CREAT | O_TRUNC, 0777); 
    write(f, res, respos); 
    close(f);   
  } else printf("Errore di lettura file origine\n");  
};
