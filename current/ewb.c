#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ewb_vm.h"
int respos, cpos;
#define MAXOBJ 2500 
char variables[MAXOBJ][200]; int nvars=0;    //1 mega complessivo. 
typedef struct 
{ char[200] name, 
  int arity,
  int position; 
} ptr;
ptr functions[MAXOBJ]; int nfuncs=0; 
ptr tasks[MAXOBJ];     int ntasks=0; 
ptr targets[MAXOBJ];   int ntargets=0; 
int infunction=0;    //Sono o meno dentro una funzione o dentro un task/target.. Stesso comportamento. 


char currline[5000]; 
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
eol()
{ delperc(); if ((code[cpos] !=';') && (code[cpos] != 0)) err("missing ;");
  if (code[cpos]==';') cpos++;  
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




//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//Qui è da  rivedere, devo supportare array moltidimensionali. 
//Prima di procedere capisco come tradurre la sintassi nella VM.. meglio. 
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!




calcexp()
{ if (is_assign())
  { //INEXP è integrato, ma lo devo rivedere. Posso avere più livelli di parentesi. 
    
    int a=-1,c=0;  
    token();       //a = varnum, c=var_is_array
    int k; 
    for (k=0; k<nvars; k++)
    { if (!strcmp(variables[k], currtok)) a=k; 
    }; if (a<0) err("Unknown symbol");
    delperc(); 
    while (code[cpos]=='[') 
    { c=1; cpos++; 
      calcexp(); 
      out(PUSH); nvars++; 
      delperc(); if (code[cpos]!=']') err("Missing ]"); 
      cpos++; 
    };          
    delperc();  
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
  if (istoken("sub"))     fundecl();
  if (code[cpos]=='.')    asmline(); 
  if (p==cpos) { calcexp(); eol(); }; 
  if (p==cpos) { err("syntax error"); };
  delperc(); 
};
blocco()
{ codice(); delperc(); 
  while (code[cpos]=='{') 
  { cpos++; 
    codice(); 
    delperc(); 
    if (code[cpos]!='}') err("missing }"); 
    delperc();
  }; 
}
programma()
{ while (code[cpos]!=0) 
  { blocco(); 
    delperc(); 
    if (code[cpos]!=';') err("missing ;"); 
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
