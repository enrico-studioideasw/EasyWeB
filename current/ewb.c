#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "vm.h"
#include "opcodes.h"
#include "ewb_predef.h"
int cpos;
#define MAXOBJ 2500
#define  MAXLINE 10000
#define MAXVARLEN 80
#define MAXDATASET 100
#define MAXOUTPUTSIZE 1000000

void err(const char *message);
void setLabel(int label);
delperc();
calcexp();
inexp();
blocco();
codice();
fundecl(unsigned char type);
foreachblock();
goalblock();
inblock();
deleteblock();
vardecl();
askblock();
formblock();
showformblock();
downwhileblock();
whileblock();
forblock();
ifblock();
datasetdecl();
cron();
refresh();
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
{ char name[MAXVARLEN];
  int arity;
  int position;
} ptr;
ptr functions[MAXOBJ]; int nfuncs=0; 
ptr tasks[MAXOBJ];     int ntasks=0; 
ptr targets[MAXOBJ];   int ntargets=0; 
int infunction=0;    //Sono o meno dentro una funzione o dentro un task/target.. Stesso comportamento. 
int current_arity=0;
int current_type=0;
int labelcount=0;    //Non sono in grado di assegnare subito l'indirizzo a alcune variabili.
int label_position[MAXOBJ];
unsigned char label_defined[MAXOBJ];
int PC=0; 
int sql_mode=0;   //Non c'e' annidamento quindi posso mettere un flag qui.
char sql_context[MAXVARLEN]="";
int add_debug=0;

char code[MAXOUTPUTSIZE];
char res[MAXOUTPUTSIZE];
int respos=0;
extern char currline[];
extern int nline;

void out(const char* dt)
{ char expanded[MAXLINE];
  int label;
  strcpy(expanded,dt);
  for (label=0; label<labelcount; label++)
  { if (label_defined[label])
    { char marker[40];
      char value[40];
      char *found;
      sprintf(marker,"*LABEL%i*",label);
      sprintf(value,"%i",label_position[label]);
      found=strstr(expanded,marker);
      while (found)
      { int old_length=(int)strlen(marker);
        int new_length=(int)strlen(value);
        int tail=(int)strlen(found+old_length);
        if ((int)strlen(expanded)-old_length+new_length>=MAXLINE)
          err("Output line overflow");
        memmove(found+new_length,found+old_length,tail+1);
        memcpy(found,value,new_length);
        found=strstr(found+new_length,marker);
      }
    }
  }
  int length=(int)strlen(expanded);
  if (respos+length+2>=MAXOUTPUTSIZE) err("Output overflow");
  strcpy(&res[respos],expanded);
  respos=respos+strlen(expanded);
  res[respos]='\n';
  respos++;
  res[respos]=0;
  PC++; //Program counter conta le righe di assembler. 
}; 

void err(const char *message)
{ fprintf(stderr,"Compile error at line %i: %s\n",nline,message);
  if (currline[0]) fprintf(stderr,"%s\n",currline);
  exit(1);
}

void setLabel(int label)
{ char marker[40];
  char value[40];
  char *found;
  if (label<0 || label>=MAXOBJ) err("Label range");
  label_position[label]=PC;
  label_defined[label]=1;
  sprintf(marker,"*LABEL%i*",label);
  sprintf(value,"%i",PC);
  found=strstr(res,marker);
  while (found)
  { int old_length=(int)strlen(marker);
    int new_length=(int)strlen(value);
    int tail=(int)strlen(found+old_length);
    if (respos-old_length+new_length+1>=MAXOUTPUTSIZE) err("Output overflow");
    memmove(found+new_length,found+old_length,tail+1);
    memcpy(found,value,new_length);
    respos+=new_length-old_length;
    found=strstr(found+new_length,marker);
  }
}

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
{ int pos=cpos;
  int level=0;
  if (!((code[pos]>='a' && code[pos]<='z') ||
        (code[pos]>='A' && code[pos]<='Z') || code[pos]=='_')) return 0;
  while ((code[pos]>='a' && code[pos]<='z') ||
         (code[pos]>='A' && code[pos]<='Z') ||
         (code[pos]>='0' && code[pos]<='9') ||
         code[pos]=='_' || code[pos]=='.') pos++;
  while (1)
  { while (code[pos]==' ' || code[pos]=='\t' ||
           code[pos]=='\n' || code[pos]=='\r') pos++;
    if (code[pos]!='[') break;
    level=1;
    pos++;
    while (code[pos] && level>0)
    { if (code[pos]=='"')
      { pos++;
        while (code[pos] && code[pos]!='"')
        { if (code[pos]=='\\' && code[pos+1]) pos++;
          pos++;
        }
        if (code[pos]=='"') pos++;
      } else
      { if (code[pos]=='[') level++;
        if (code[pos]==']') level--;
        pos++;
      }
    }
    if (level!=0) return 0;
  }
  while (code[pos]==' ' || code[pos]=='\t' ||
         code[pos]=='\n' || code[pos]=='\r') pos++;
  return code[pos]=='=' && code[pos+1]!='=';
};

int asmline()
{ if (sql_mode) { err("Unsupported in SQL"); return; };
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
      while(code[cpos] && code[cpos]!='"')
      { if (code[cpos]=='\\') //tengo gli escape.
        { arg[p]='\\'; cpos++; p++; 
        }
        arg[p]=code[cpos]; p++; cpos++;
      };
      if (code[cpos]!='"') err("Missing quote");
      arg[p]=0;
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


typedef struct DatasetSpec
{ char name[MAXVARLEN];
  char fields[MAXOBJ][MAXVARLEN];
  int field_count;
  struct DatasetSpec *children[MAXOBJ];
  int child_count;
} DatasetSpec;

static DatasetSpec *parse_dataset(const char *name)
{ DatasetSpec *dataset=(DatasetSpec*)calloc(1,sizeof(DatasetSpec));
  if (!dataset) err("Dataset memory");
  strncpy(dataset->name,name,MAXVARLEN-1);
  delperc();
  if (code[cpos]!='{') err("Missing {");
  cpos++;
  delperc();
  while (code[cpos]!='}')
  { char field[MAXVARLEN];
    if (!token()) err("Missing dataset field");
    strcpy(field,currtok);
    delperc();
    if (code[cpos]=='=')
    { cpos++;
      delperc();
      if (dataset->child_count>=MAXOBJ) err("Too many nested datasets");
      dataset->children[dataset->child_count++]=parse_dataset(field);
      if (dataset->field_count>=MAXOBJ) err("Too many dataset fields");
      snprintf(dataset->fields[dataset->field_count++],MAXVARLEN,"id_%s",field);
    } else
    { if (dataset->field_count>=MAXOBJ) err("Too many dataset fields");
      strcpy(dataset->fields[dataset->field_count++],field);
    }
    delperc();
    if (code[cpos]==',')
    { cpos++;
      delperc();
      if (code[cpos]=='}') err("Missing dataset field");
    } else if (code[cpos]!='}') err("Missing , or }");
  }
  cpos++;
  return dataset;
}

static void remember_variable(const char *name)
{ if (nvars>=MAXOBJ) err("Too many variables");
  strncpy(variables[nvars],name,MAXVARLEN-1);
  variables[nvars][MAXVARLEN-1]=0;
  nvars++;
}

static void emit_dataset(DatasetSpec *dataset)
{ static const char *predefined[]=
    {"_url","_user","_password","_status","_orderby","id"};
  static const char *defaults[]=
    {"mysql://ewb","ewb","ewb","stopped","",""};
  char fullname[2*MAXVARLEN];
  char line[3*MAXVARLEN];
  int i;

  remember_variable(dataset->name);
  snprintf(line,sizeof(line),"ADDSYMTABLE \"%s\"",dataset->name);
  out(line);
  snprintf(line,sizeof(line),"PUSH \"%s\"",dataset->name);
  out(line);

  for (i=0; i<6; i++)
  { snprintf(fullname,sizeof(fullname),"%s.%s",dataset->name,predefined[i]);
    remember_variable(fullname);
    snprintf(line,sizeof(line),"ADDSYMTABLE \"%s\"",fullname);
    out(line);
    snprintf(line,sizeof(line),"PUSH \"%s\"",defaults[i]);
    out(line);
  }
  for (i=0; i<dataset->field_count; i++)
  { if (strcmp(dataset->fields[i],"id")==0) continue;
    snprintf(fullname,sizeof(fullname),"%s.%s",dataset->name,dataset->fields[i]);
    remember_variable(fullname);
    snprintf(line,sizeof(line),"ADDSYMTABLE \"%s\"",fullname);
    out(line);
    out("PUSH \"\"");
  }
  for (i=0; i<dataset->child_count; i++) emit_dataset(dataset->children[i]);
}

static void free_dataset(DatasetSpec *dataset)
{ int i;
  for (i=0; i<dataset->child_count; i++) free_dataset(dataset->children[i]);
  free(dataset);
}

datasetdecl()
{ DatasetSpec *dataset;
  char name[MAXVARLEN];
  cpos+=7;
  delperc();
  if (!token()) err("Missing dataset name");
  strcpy(name,currtok);
  delperc();
  if (code[cpos]!='=') err("Missing =");
  cpos++;
  dataset=parse_dataset(name);
  emit_dataset(dataset);
  free_dataset(dataset);
}

cron()
{ if (sql_mode) { err("Unsupported in SQL"); return; };
  char buf[100];
  delperc();
  token();
  sprintf(buf,"PUSH \"%s\"",currtok);
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
  cpos++;
  sprintf(buf,"CRONTASK %i",arity);
  delperc();
  inexp();
  out("PUSHA");
  out(buf); 
};

targetlaunch(int single)
{ if (sql_mode) { err("Unsupported in SQL"); return; };
  char name[MAXVARLEN];
  char line[2*MAXVARLEN+80];
  int target=-1;
  int arity=0;
  int i;

  cpos+=single ? 3 : 7;
  delperc();
  if (!token()) err("Missing target name");
  strcpy(name,currtok);
  for (i=0; i<ntargets; i++)
  { if (!strcmp(targets[i].name,name)) target=i;
  }
  if (target<0) err("Unknown target");
  snprintf(line,sizeof(line),"PUSH \"%s\"",name);
  out(line);

  delperc();
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  if (code[cpos]!=')')
  { while (1)
    { inexp();
      out("PUSHA");
      arity++;
      delperc();
      if (code[cpos]!=',') break;
      cpos++;
      delperc();
    }
  }
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  if (arity!=targets[target].arity) err("Wrong target arity");
  snprintf(line,sizeof(line),"PUSH %i",arity);
  out(line);
  if (!single)
  { delperc();
    if (code[cpos]!=',') err("Missing refresh interval");
    cpos++;
    delperc();
    inexp();
    out("PUSHA");
    snprintf(line,sizeof(line),"REFRESHTARGET %i",targets[target].position);
  }
  else snprintf(line,sizeof(line),"RUNTARGET %i",targets[target].position);
  out(line);
};

refresh()
{ targetlaunch(0);
};

tok_run()
{ targetlaunch(1);
};

int startparallel()
{ if (!strcmp(currtok,"cron")) { cron(); return 1; };
  if (!strcmp(currtok,"refresh")) { refresh(); return 1; };
  if (!strcmp(currtok,"run")) { tok_run(); return 1; };
  return 0;
};



static int identifier_start()
{ char c=code[cpos];
  if (c>='a' && c<='z') return 1;
  if (c>='A' && c<='Z') return 1;
  return c=='_';
}

static int variable_index(const char *name)
{ int i;
  for (i=0; i<nvars; i++)
  { if (!strcmp(variables[i],name)) return i;
  }
  for (i=0; i<nvars; i++)
  { char *dot=strrchr(variables[i],'.');
    if (dot && !strcmp(dot+1,name)) return i;
  }
  return -1;
}

static void emit_variable_value(const char *name)
{ char buf[2*MAXVARLEN+40];
  int levels=0;
  sprintf(buf,"PUSH \"%s\"",name);
  out(buf);
  delperc();
  while (code[cpos]=='[')
  { int old_sql_mode=sql_mode;
    cpos++;
    sql_mode=0;
    calcexp();
    sql_mode=old_sql_mode;
    out("PUSHA");
    levels++;
    delperc();
    if (code[cpos]!=']') err("Missing ]");
    cpos++;
    delperc();
  }
  sprintf(buf,"PUSH %i",levels);
  out(buf);
  out("GETPATH");
}

static void emit_sql_binary(const char *separator)
{ out("PUSH \"(\"");
  out("PUSHA");
  {
    char buf[80];
    sprintf(buf,"PUSH \"%s\"",separator);
    out(buf);
  }
}

static void finish_sql_binary()
{ out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("PUSH \")\"");
  out("CONCAT");
  out("PUSHA");
  out("CONCAT");
}

typedef struct
{ const char *name;
  int minimum;
  int maximum;
  const char *opcode;
  const char *sql_function;
} Predefined;

static const Predefined predefined[]=
{
  {"print",1,1,"PRINT",NULL},
  {"eprint",1,1,"EPRINT",NULL},
  {"input",0,0,"INPUT",NULL},
  {"show",1,1,"SHOW",NULL},
  {"load",1,1,"FLOAD",NULL},
  {"save",2,2,"FSAVE",NULL},
  {"loaddir",1,1,"FREADDIR",NULL},
  {"exec",1,1,"EXEC",NULL},
  {"int",1,1,"TOINT",NULL},
  {"sin",1,1,"SIND",NULL},
  {"hex",1,1,"TOHEX",NULL},
  {"sqr",1,1,"SQRT","SQRT"},
  {"random",1,1,"RANDOM",NULL},
  {"asc",1,1,"ASC",NULL},
  {"char",1,1,"CHAR",NULL},
  {"mid",2,3,"MID",NULL},
  {"len",1,1,"LEN","LENGTH"},
  {"uc",1,1,"UC","UPPER"},
  {"index",2,2,"INDEX",NULL},
  {"split",2,2,"SPLIT",NULL},
  {"join",2,2,"JOIN",NULL},
  {"pop",1,1,"ARRAYPOP",NULL},
  {"numel",1,1,"NUMEL",NULL},
  {"numkey",1,1,"NUMKEY",NULL},
  {"keyat",2,2,"KEYAT",NULL},
  {"getelem",2,2,"GETELEM",NULL},
  {"haskey",2,2,"HASKEY",NULL},
  {"time",0,0,"TIME",NULL},
  {"date",0,0,"DATE",NULL},
  {"sleep",1,1,"SLEEP",NULL},
  {"socket",2,2,"SOCKET",NULL},
  {"server",1,1,"SERVER",NULL},
  {"accept",1,1,"ACCEPT",NULL},
  {"sread",1,1,"SREAD",NULL},
  {"swrite",2,2,"SWRITE",NULL},
  {"close",1,1,"SCLOSE",NULL},
  {"assert",2,2,"ASSERT",NULL},
  {"retract",2,2,"RETRACT",NULL},
  {NULL,0,0,NULL,NULL}
};

static const Predefined *find_predefined(const char *name)
{ int i;
  for (i=0; predefined[i].name; i++)
    if (!strcmp(predefined[i].name,name)) return &predefined[i];
  return NULL;
}

static int parse_arguments(int sql_arguments,const char *sql_function)
{ int count=0;
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  if (sql_arguments)
  { char line[160];
    snprintf(line,sizeof(line),"PUSH \"%s(\"",sql_function);
    out(line);
  }
  if (code[cpos]!=')')
  { while (1)
    { inexp();
      out("PUSHA");
      count++;
      delperc();
      if (code[cpos]!=',') break;
      cpos++;
      delperc();
      if (sql_arguments) out("PUSH \", \"");
    }
  }
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  if (sql_arguments)
  { int i;
    for (i=0; i<count; i++) out("CONCAT");
    out("PUSHA");
    out("PUSH \")\"");
    out("CONCAT");
  }
  return count;
}

static int compile_composed(const char *name)
{ char variable[MAXVARLEN];
  char line[MAXLINE+20];
  char query[MAXLINE];
  int negative;
  int end;

  if (strcmp(name,"abs") && strcmp(name,"cos") && strcmp(name,"change") &&
      strcmp(name,"push") && strcmp(name,"delkey") &&
      strcmp(name,"add") && strcmp(name,"exists") &&
      strcmp(name,"lock") && strcmp(name,"unlock"))
    return 0;
  if (sql_mode) err("Predefined unsupported in SQL");

  if (!strcmp(name,"abs"))
  { negative=labelcount++;
    end=labelcount++;
    if (parse_arguments(0,NULL)!=1) err("Wrong predefined arity");
    out("PUSHA");
    out("PUSH 0");
    out("GT");
    snprintf(line,sizeof(line),"JZ *LABEL%i*",negative);
    out(line);
    out("POPA");
    snprintf(line,sizeof(line),"JMP *LABEL%i*",end);
    out(line);
    setLabel(negative);
    out("POPA");
    out("PUSH 0");
    out("PUSHA");
    out("SUB");
    setLabel(end);
    return 1;
  }

  if (!strcmp(name,"cos"))
  { if (parse_arguments(0,NULL)!=1) err("Wrong predefined arity");
    out("PUSH 90");
    out("SUM");
    out("PUSHA");
    out("SIND");
    return 1;
  }

  if (!strcmp(name,"change"))
  { if (parse_arguments(0,NULL)!=3) err("Wrong predefined arity");
    out("SPLIT");
    out("PUSHA");
    out("JOIN");
    return 1;
  }

  if (!strcmp(name,"push"))
  { if (code[cpos]!='(') err("Missing (");
    cpos++;
    delperc();
    if (!token_var()) err("PUSH requires a variable");
    strcpy(variable,currtok);
    if (variable_index(variable)<0) err("Unknown PUSH variable");
    delperc();
    if (code[cpos]!=',') err("Missing ,");
    cpos++;
    snprintf(line,sizeof(line),"PUSH \"%s\"",variable);
    out(line);
    delperc();
    inexp();
    out("PUSHA");
    delperc();
    if (code[cpos]!=')') err("Missing )");
    cpos++;
    out("ARRAYPUSH");
    return 1;
  }

  if (!strcmp(name,"lock") || !strcmp(name,"unlock"))
  { if (parse_arguments(0,NULL)!=1) err("Wrong predefined arity");
    if (!strcmp(name,"lock")) out("DBLOCK");
    else out("DBUNLOCK");
    return 1;
  }

  if (!strcmp(name,"add") || !strcmp(name,"exists"))
  { int field_count=0;
    int fields[MAXOBJ];
    size_t prefix_length;
    if (code[cpos]!='(') err("Missing (");
    cpos++;
    delperc();
    if (!token_var()) err("Missing dataset");
    strcpy(variable,currtok);
    if (variable_index(variable)<0) err("Unknown dataset");
    delperc();
    if (code[cpos]!=')') err("Missing )");
    cpos++;

    prefix_length=strlen(variable);
    for (int i=0; i<nvars; i++)
    { if (!strncmp(variables[i],variable,prefix_length) &&
          variables[i][prefix_length]=='.' &&
          variables[i][prefix_length+1]!='_')
      { const char *field=variables[i]+prefix_length+1;
        if (!strcmp(name,"add") && !strcmp(field,"id")) continue;
        fields[field_count++]=i;
      }
    }

    snprintf(line,sizeof(line),"PUSH \"%s\"",variable);
    out(line);
    if (!strcmp(name,"add"))
    { snprintf(query,sizeof(query),"insert into %s (",variable);
      for (int i=0; i<field_count; i++)
      { const char *field=variables[fields[i]]+prefix_length+1;
        if (i) strncat(query,",",sizeof(query)-strlen(query)-1);
        strncat(query,"`",sizeof(query)-strlen(query)-1);
        strncat(query,field,sizeof(query)-strlen(query)-1);
        strncat(query,"`",sizeof(query)-strlen(query)-1);
      }
      strncat(query,") values (",sizeof(query)-strlen(query)-1);
    }
    else
    { snprintf(query,sizeof(query),"select id from %s",variable);
      if (field_count) strncat(query," where ",sizeof(query)-strlen(query)-1);
    }
    snprintf(line,sizeof(line),"MOVA \"%s\"",query);
    out(line);

    for (int i=0; i<field_count; i++)
    { if (i)
      { out("PUSHA");
        if (!strcmp(name,"add")) out("PUSH \",\"");
        else out("PUSH \" and \"");
        out("CONCAT");
      }
      if (!strcmp(name,"exists"))
      { const char *field=variables[fields[i]]+prefix_length+1;
        out("PUSHA");
        snprintf(line,sizeof(line),"PUSH \"`%s`=\"",field);
        out(line);
        out("CONCAT");
      }
      out("PUSHA");
      emit_variable_value(variables[fields[i]]);
      out("PUSHA");
      out("SQLQUOTE");
      out("PUSHA");
      out("CONCAT");
    }
    if (!strcmp(name,"add"))
    { out("PUSHA");
      out("PUSH \")\"");
      out("CONCAT");
    }
    out("PUSHA");
    out("QUERY");
    if (!strcmp(name,"add"))
    { snprintf(line,sizeof(line),"PUSH \"%s.id\"",variable);
      out(line);
      out("PUSHA");
      out("SETPATH 0");
    }
    else
    { out("PUSHA");
      out("PUSH \"\"");
      out("SNEQ");
    }
    return 1;
  }

  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  if (!token_var()) err("DELKEY requires a variable");
  strcpy(variable,currtok);
  if (variable_index(variable)<0) err("Unknown DELKEY variable");
  delperc();
  if (code[cpos]!=',') err("Missing ,");
  cpos++;
  snprintf(line,sizeof(line),"PUSH \"%s\"",variable);
  out(line);
  delperc();
  inexp();
  out("PUSHA");
  delperc();
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  out("DELKEY");
  return 1;
}

static int compile_predefined(const char *name)
{ const Predefined *definition=find_predefined(name);
  int count;
  if (compile_composed(name)) return 1;
  if (!definition) return 0;
  if (sql_mode)
  { if (!definition->sql_function) err("Predefined unsupported in SQL");
    count=parse_arguments(1,definition->sql_function);
  } else
  { count=parse_arguments(0,NULL);
    if (!strcmp(name,"mid") && count==2) out("PUSH \"-1\"");
    out(definition->opcode);
  }
  if (count<definition->minimum || count>definition->maximum)
    err("Wrong predefined arity");
  return 1;
}

numvalue()
{ int start=cpos;
  int base=10;
  int digits=0;
  char buf[2*MAXVARLEN+40];
  if (code[cpos]=='+' || code[cpos]=='-') cpos++;
  if (code[cpos]=='0' && code[cpos+1]=='b') { base=2; cpos+=2; }
  else if (code[cpos]=='0' && code[cpos+1]=='o') { base=8; cpos+=2; }
  else if (code[cpos]=='0' && code[cpos+1]=='x') { base=16; cpos+=2; }
  while (1)
  { int value=-1;
    char c=code[cpos];
    if (c>='0' && c<='9') value=c-'0';
    else if (c>='a' && c<='f') value=c-'a'+10;
    else if (c>='A' && c<='F') value=c-'A'+10;
    if (value<0 || value>=base) break;
    digits++;
    cpos++;
  }
  if (digits==0) err("Bad number");
  if (code[cpos]=='.')
  { cpos++;
    while (1)
    { int value=-1;
      char c=code[cpos];
      if (c>='0' && c<='9') value=c-'0';
      else if (c>='a' && c<='f') value=c-'a'+10;
      else if (c>='A' && c<='F') value=c-'A'+10;
      if (value<0 || value>=base) break;
      cpos++;
    }
  }
  {
    int length=cpos-start;
    if (length>=MAXVARLEN) err("Number too long");
    sprintf(buf,"MOVA \"%.*s\"",length,&code[start]);
  }
  out(buf);
  if (sql_mode)
  { out("PUSHA");
    out("SQLNUMBER");
  }
}

strvalue()
{ char buf[MAXLINE];
  int pos=0;
  if (code[cpos]!='"') err("Missing string");
  buf[pos++]='M'; buf[pos++]='O'; buf[pos++]='V'; buf[pos++]='A';
  buf[pos++]=' '; buf[pos++]='"';
  cpos++;
  while (code[cpos] && code[cpos]!='"')
  { if (pos>=MAXLINE-3) err("String too long");
    if (code[cpos]=='\\')
    { buf[pos++]=code[cpos++];
      if (!code[cpos]) err("Bad string escape");
    }
    buf[pos++]=code[cpos++];
  }
  if (code[cpos]!='"') err("Missing quote");
  cpos++;
  buf[pos++]='"';
  buf[pos]=0;
  out(buf);
  if (sql_mode)
  { out("PUSHA");
    out("SQLQUOTE");
  }
}

tok_var()
{ char name[MAXVARLEN];
  char buf[2*MAXVARLEN+80];
  int variable;
  int function=-1;
  int i;
  strcpy(name,currtok);
  if (code[cpos]=='(' && compile_predefined(name)) return;
  variable=variable_index(name);
  for (i=0; i<nfuncs; i++)
  { if (!strcmp(functions[i].name,name)) function=i;
  }

  if (code[cpos]=='(')
  { int count=0;
    if (function<0) err("Unknown function");
    cpos++;
    out("PUSH \"dummy\"");
    delperc();
    if (code[cpos]!=')')
    { while (1)
      { inexp();
        out("PUSHA");
        count++;
        delperc();
        if (code[cpos]!=',') break;
        cpos++;
      }
    }
    if (code[cpos]!=')') err("Missing )");
    cpos++;
    if (count!=functions[function].arity) err("Wrong arity");
    sprintf(buf,"PUSH %i",count);
    out(buf);
    sprintf(buf,"CALL %i",functions[function].position);
    out(buf);
    if (sql_mode)
    { out("PUSHA");
      out("SQLQUOTE");
    }
    return;
  }

  if (variable<0 && !sql_mode) err("Unknown variable");
  if (sql_mode)
  { if (sql_context[0])
    { char fullname[2*MAXVARLEN];
      snprintf(fullname,sizeof(fullname),"%s.%s",sql_context,name);
      if (variable_index(fullname)>=0)
      { snprintf(buf,sizeof(buf),"MOVA \"%s\"",name);
        out(buf);
        return;
      }
      if (variable>=0) emit_variable_value(variables[variable]);
      else emit_variable_value(name);
      out("PUSHA");
      out("SQLQUOTE");
      return;
    }
    int external=labelcount++;
    int end=labelcount++;
    sprintf(buf,"PUSH \"%s\"",name);
    out(buf);
    sprintf(buf,"JNCONTEXT *LABEL%i*",external);
    out(buf);
    sprintf(buf,"JMP *LABEL%i*",end);
    out(buf);
    setLabel(external);
    if (variable>=0) emit_variable_value(variables[variable]);
    else emit_variable_value(name);
    out("PUSHA");
    out("SQLQUOTE");
    setLabel(end);
    return;
  }
  emit_variable_value(variables[variable]);
}

tok_k()
{ const char *op;
  delperc();
  if (identifier_start())
  { token_var();
    tok_var();
    return;
  }
  if (code[cpos]=='!')
  { cpos++;
    op="NOT";
    if (code[cpos]=='!')
    { cpos++;
      op="NOTB";
    }
    if (sql_mode)
    { if (!strcmp(op,"NOT")) out("PUSH \"NOT(\"");
      else out("PUSH \"~(\"");
      inexp();
      out("PUSHA");
      out("PUSH \")\"");
      out("CONCAT");
      out("PUSHA");
      out("CONCAT");
    } else
    { inexp();
      out("PUSHA");
      out(op);
    }
    return;
  }
  if (code[cpos]=='-' || code[cpos]=='+' ||
      (code[cpos]>='0' && code[cpos]<='9'))
  { numvalue();
    return;
  }
  if (code[cpos]=='"')
  { strvalue();
    return;
  }
  if (code[cpos]=='(')
  { cpos++;
    calcexp();
    delperc();
    if (code[cpos]!=')') err("Missing )");
    cpos++;
    return;
  }
  err("Missing expression");
}

tok_b()
{ const char *op;
  const char *sqlop;
  delperc();
  tok_k();
  delperc();
  if (code[cpos]!='&') return;
  if (code[cpos+1]=='&')
  { op="AND";
    sqlop=" AND ";
    cpos+=2;
  } else
  { op="ANDB";
    sqlop=" & ";
    cpos++;
  }
  if (sql_mode)
  { emit_sql_binary(sqlop);
    tok_b();
    finish_sql_binary();
  } else
  { out("PUSHA");
    tok_b();
    out("PUSHA");
    out(op);
  }
}

tok_f()
{ const char *op;
  const char *sqlop;
  delperc();
  tok_b();
  delperc();
  if (code[cpos]!='|') return;
  if (code[cpos+1]=='|')
  { op="OR";
    sqlop=" OR ";
    cpos+=2;
  } else
  { op="ORB";
    sqlop=" | ";
    cpos++;
  }
  if (sql_mode)
  { emit_sql_binary(sqlop);
    tok_f();
    finish_sql_binary();
  } else
  { out("PUSHA");
    tok_f();
    out("PUSHA");
    out(op);
  }
}

tok_t()
{ const char *op;
  char symbol;
  delperc();
  tok_f();
  delperc();
  symbol=code[cpos];
  if (symbol=='*') op="MUL";
  else if (symbol=='/') op="DIV";
  else if (symbol=='%') op="MOD";
  else return;
  cpos++;
  if (sql_mode)
  { if (symbol=='%')
    { out("PUSH \"MOD(\"");
      out("PUSHA");
      out("PUSH \",\"");
      tok_t();
      out("PUSHA");
      out("CONCAT");
      out("PUSHA");
      out("CONCAT");
      out("PUSHA");
      out("PUSH \")\"");
      out("CONCAT");
      out("PUSHA");
      out("CONCAT");
    } else
    { char sqlop[4];
      sqlop[0]=' ';
      sqlop[1]=symbol;
      sqlop[2]=' ';
      sqlop[3]=0;
      emit_sql_binary(sqlop);
      tok_t();
      finish_sql_binary();
    }
  } else
  { out("PUSHA");
    tok_t();
    out("PUSHA");
    out(op);
  }
}

tok_e()
{ const char *op;
  const char *sqlop;
  delperc();
  tok_t();
  delperc();
  if (code[cpos]=='+')
  { op="SUM";
    sqlop=" + ";
  } else if (code[cpos]=='-')
  { op="SUB";
    sqlop=" - ";
  } else return;
  cpos++;
  if (sql_mode)
  { emit_sql_binary(sqlop);
    tok_e();
    finish_sql_binary();
  } else
  { out("PUSHA");
    tok_e();
    out("PUSHA");
    out(op);
  }
}

tok_exp()
{ delperc();
  tok_e();
  delperc();
  if (code[cpos]!='.') return;
  cpos++;
  if (sql_mode)
  { out("PUSH \"CONCAT(\"");
    out("PUSHA");
    out("PUSH \",\"");
    tok_exp();
    out("PUSHA");
    out("CONCAT");
    out("PUSHA");
    out("CONCAT");
    out("PUSHA");
    out("PUSH \")\"");
    out("CONCAT");
    out("PUSHA");
    out("CONCAT");
  } else
  { out("PUSHA");
    tok_exp();
    out("PUSHA");
    out("CONCAT");
  }
}

const char* is_cfr()
{ char a,b; a=code[cpos]; b=code[cpos+1];
  if ((a=='=') && (b=='=')) { cpos=cpos+2; return  "EQ"; }; 
  if ((a=='>') && (b=='=')) { cpos=cpos+2; return  "GE"; }; 
  if ((a=='<') && (b=='=')) { cpos=cpos+2; return  "LE"; };
  if (a=='>') { cpos++; return  "GT"; }; 
  if (a=='<') { cpos++; return  "LT"; };
  if ((a=='!') && (b=='=')) { cpos=cpos+2; return  "NEQ"; };
  if ((a=='e') && (b=='q')) { cpos=cpos+2; return  "SEQ"; }; 
  if ((a=='g') && (b=='t')) { cpos=cpos+2; return  "SGT"; }; 
  if ((a=='g') && (b=='e')) { cpos=cpos+2; return  "SGE"; }; 
  if ((a=='l') && (b=='t')) { cpos=cpos+2; return  "SLT"; }; 
  if ((a=='l') && (b=='e')) { cpos=cpos+2; return  "SLE"; };  
  if ((a=='n') && (b=='e')) { cpos=cpos+2; return  "SNEQ"; };
  return ""; 
};
inexp()
{ delperc();
  tok_exp();
  delperc();
  const char *op=is_cfr();
  if (op[0]==0) return;
  if (sql_mode)
  { const char *sqlop="";
    const char *plain=op;
    if (plain[0]=='S') plain++;
    if (!strcmp(plain,"EQ")) sqlop=" = ";
    else if (!strcmp(plain,"NEQ")) sqlop=" != ";
    else if (!strcmp(plain,"GT")) sqlop=" > ";
    else if (!strcmp(plain,"GE")) sqlop=" >= ";
    else if (!strcmp(plain,"LT")) sqlop=" < ";
    else if (!strcmp(plain,"LE")) sqlop=" <= ";
    else err("Bad comparison");
    emit_sql_binary(sqlop);
    tok_exp();
    finish_sql_binary();
  } else
  { out("PUSHA");
    tok_exp(); //Un solo confronto: a > b <= c e' errore sintattico.
    out("PUSHA");
    out(op);
  }
}

fundecl(unsigned char type)
{ char name[MAXVARLEN];
  char parameters[MAXOBJ][MAXVARLEN];
  char line[3*MAXVARLEN];
  ptr *declaration;
  int keyword_length=3;
  int end_label=labelcount++;
  int arity=0;
  int entrypoint;
  int body_start;
  int body_empty;
  int base_variables=nvars;
  int i;

  if (type==1) keyword_length=4;
  if (type==2) keyword_length=6;
  cpos+=keyword_length;
  delperc();
  if (!token()) err("Missing function name");
  strcpy(name,currtok);
  delperc();
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  while (code[cpos]!=')')
  { if (arity>=MAXOBJ) err("Too many parameters");
    if (!token()) err("Missing parameter");
    strcpy(parameters[arity++],currtok);
    delperc();
    if (code[cpos]!=',') break;
    cpos++;
    delperc();
  }
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  delperc();

  if (type==0)
  { if (nfuncs>=MAXOBJ) err("Too many functions");
    declaration=&functions[nfuncs++];
  } else if (type==1)
  { if (ntasks>=MAXOBJ) err("Too many tasks");
    declaration=&tasks[ntasks++];
  } else
  { if (ntargets>=MAXOBJ) err("Too many targets");
    declaration=&targets[ntargets++];
  }
  strcpy(declaration->name,name);
  declaration->arity=arity;
  snprintf(line,sizeof(line),"JMP *LABEL%i*",end_label);
  out(line);
  entrypoint=PC;
  declaration->position=entrypoint;

  snprintf(line,sizeof(line),"DECSP %i",arity);
  out(line);
  for (i=0; i<arity; i++)
  { snprintf(line,sizeof(line),"ADDSYMTABLE \"%s\"",parameters[i]);
    out(line);
    out("INCSP 1");
    remember_variable(parameters[i]);
  }
  if (type)
  { snprintf(line,sizeof(line),"PUSH \"%s\"",name);
    out(line);
    if (type==1) out("TASK");
    else out("TARGET");
  }

  infunction=1;
  current_arity=arity;
  current_type=type;
  body_start=PC;
  blocco();
  body_empty=PC==body_start;
  if (type==1) out("ENDTASK");
  if (type==2) out("ENDTARGET");
  snprintf(line,sizeof(line),"DELSYMTABLE %i",arity);
  out(line);
  snprintf(line,sizeof(line),"DECSP %i",arity);
  out(line);
  out("RET");
  infunction=0;
  current_arity=0;
  current_type=0;
  nvars=base_variables;
  setLabel(end_label);

  if (type==0 && !strcmp(name,"_onerror"))
  { if (arity!=1) err("_onerror requires one parameter");
    if (body_empty) out("PUSH \"\"");
    else
    { snprintf(line,sizeof(line),"PUSH %i",entrypoint);
      out(line);
    }
    out("ONERROR");
  }
};


downwhileblock()
{ int lstart;
  char buf[100];
  cpos += 2;          // "do"
  delperc();
  lstart = labelcount++;
  setLabel(lstart);
  blocco();
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
  blocco();
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
  blocco();
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
  blocco();
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
    blocco();
    sprintf(buf, "JMP *LABEL%i*", lend);
    out(buf);
    setLabel(lnext);
  };
  if (istoken("else"))
  { cpos += 4;   // controlla: "else" sono 4 caratteri
    delperc();
    blocco();
  };
  setLabel(lend);
};

calcexp()
{ delperc();
  if (is_assign())
  { char target[MAXVARLEN];
    char buf[2*MAXVARLEN+80];
    int variable;
    int levels=0;
    int orderby=0;
    if (!token_var()) err("Missing assignment target");
    strcpy(target,currtok);
    variable=variable_index(target);
    if (variable<0) err("Unknown symbol");
    {
      char *field=strrchr(target,'.');
      if (field) field++;
      else field=target;
      if (!strcmp(field,"_orderby")) orderby=1;
    }
    delperc();
    sprintf(buf,"PUSH \"%s\"",variables[variable]);
    out(buf);
    while (code[cpos]=='[')
    { cpos++;
      calcexp();
      out("PUSHA");
      levels++;
      delperc();
      if (code[cpos]!=']') err("Missing ]");
      cpos++;
      delperc();
    }
    if (code[cpos] != '=') err("Missing =");
    cpos++;
    delperc();
    if (orderby)
    { int empty_orderby=0;
      int after_empty=cpos;
      char context[MAXVARLEN];
      char *field=strrchr(target,'.');
      if (!field) err("_orderby requires a dataset");
      if (code[cpos]=='"' && code[cpos+1]=='"')
      { after_empty=cpos+2;
        while (code[after_empty]==' ' || code[after_empty]=='\t' ||
               code[after_empty]=='\n' || code[after_empty]=='\r')
          after_empty++;
        if (code[after_empty]==';' || code[after_empty]=='}' ||
            code[after_empty]==')' || code[after_empty]==0)
          empty_orderby=1;
      }
      if (empty_orderby)
      { cpos+=2;
        out("MOVA \"\"");
      } else
      {
        {
          int length=(int)(field-target);
          memcpy(context,target,length);
          context[length]=0;
        }
        strcpy(sql_context,context);
        sql_mode=1;
        inexp();
        delperc();
        while (code[cpos]==',')
        { cpos++;
          out("PUSHA");
          out("PUSH \", \"");
          inexp();
          out("PUSHA");
          out("CONCAT");
          out("PUSHA");
          out("CONCAT");
          delperc();
        }
        sql_mode=0;
        sql_context[0]=0;
      };
    } else inexp();
    out("PUSHA");
    sprintf(buf,"SETPATH %i",levels);
    out(buf);
  } else inexp();
}

inblock()
{ char line[100];
  int loop_label=labelcount++;
  int next_label=labelcount++;
  int end_label=labelcount++;
  cpos+=2;
  delperc();
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  inexp();
  out("PUSHA");
  delperc();
  if (code[cpos]==',')
  { cpos++;
    sql_mode=1;
    calcexp();
    sql_mode=0;
    out("PUSHA");
    delperc();
  } else out("PUSH \"true\"");
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  delperc();
  out("QLIST");
  out("PUSHA");
  out("PUSH 0");
  out("PUSH \"dummy\"");

  setLabel(loop_label);
  out("DECSP 2");
  out("POPA");
  out("INCSP 3");
  out("PUSHA");
  out("DECSP 2");
  out("POPA");
  out("INCSP 3");
  out("PUSHA");
  out("GETELEM");
  snprintf(line,sizeof(line),"JZ *LABEL%i*",end_label);
  out(line);
  out("DECSP 1");
  out("PUSHA");

  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("QBYID");
  snprintf(line,sizeof(line),"JZ *LABEL%i*",next_label);
  out(line);
  blocco();

  setLabel(next_label);
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("PUSH 1");
  out("SUM");
  out("DECSP 2");
  out("PUSHA");
  out("INCSP 1");
  snprintf(line,sizeof(line),"JMP *LABEL%i*",loop_label);
  out(line);
  setLabel(end_label);
  out("DECSP 4");
};

deleteblock()
{ cpos+=6;
  delperc();
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  inexp();
  out("PUSHA");
  delperc();
  if (code[cpos]!=',') err("DELETE requires a filter");
  cpos++;
  out("PUSH \"delete from \"");
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("PUSH \" where \"");
  out("CONCAT");
  out("PUSHA");
  sql_mode=1;
  calcexp();
  sql_mode=0;
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("QUERY");
  delperc();
  if (code[cpos]!=')') err("Missing )");
  cpos++;
}

foreachblock()
{ char first[MAXVARLEN];
  char key[MAXVARLEN];
  char value[MAXVARLEN];
  char line[3*MAXVARLEN];
  int second_position;
  int has_key=0;
  int loop_label=labelcount++;
  int end_label=labelcount++;

  cpos+=7;
  delperc();
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  if (!token_var()) err("Missing FOREACH variable");
  strcpy(first,currtok);
  delperc();
  if (code[cpos]!=',') err("Missing ,");
  cpos++;
  delperc();
  second_position=cpos;
  if (token_var())
  { delperc();
    if (code[cpos]==',')
    { has_key=1;
      strcpy(key,first);
      strcpy(value,currtok);
      cpos++;
      delperc();
    } else cpos=second_position;
  } else cpos=second_position;
  if (!has_key) strcpy(value,first);
  if (variable_index(value)<0) err("Unknown FOREACH value variable");
  if (has_key && variable_index(key)<0) err("Unknown FOREACH key variable");

  calcexp();
  out("PUSHA");
  out("PUSHA");
  out("NUMKEY");
  out("PUSHA");
  out("PUSH 0");
  out("PUSH \"dummy\"");
  delperc();
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  delperc();

  setLabel(loop_label);
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("LT");
  snprintf(line,sizeof(line),"JZ *LABEL%i*",end_label);
  out(line);

  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("DECSP 2");
  out("POPA");
  out("INCSP 3");
  out("PUSHA");
  out("KEYAT");
  out("DECSP 1");
  out("PUSHA");
  if (has_key)
  { snprintf(line,sizeof(line),"PUSH \"%s\"",key);
    out(line);
    out("PUSHA");
    out("SETPATH 0");
  }

  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("GETELEM");
  snprintf(line,sizeof(line),"PUSH \"%s\"",value);
  out(line);
  out("PUSHA");
  out("SETPATH 0");

  blocco();
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("PUSH 1");
  out("SUM");
  out("DECSP 2");
  out("PUSHA");
  out("INCSP 1");
  snprintf(line,sizeof(line),"JMP *LABEL%i*",loop_label);
  out(line);
  setLabel(end_label);
  out("DECSP 4");
}

goalblock()
{ char line[100];
  int solution_loop=labelcount++;
  int solution_end=labelcount++;
  int binding_loop=labelcount++;
  int binding_end=labelcount++;

  cpos+=4;
  delperc();
  if (code[cpos]!='(') err("Missing (");
  cpos++;
  delperc();
  calcexp();
  out("PUSHA");
  delperc();
  if (code[cpos]!=',') err("Missing ,");
  cpos++;
  delperc();
  calcexp();
  out("PUSHA");
  delperc();
  if (code[cpos]!=')') err("Missing )");
  cpos++;
  out("GOAL");

  /* Frame esterno: [soluzioni, numero, posizione, dummy]. */
  out("PUSHA");
  out("PUSHA");
  out("NUMKEY");
  out("PUSHA");
  out("PUSH 0");
  out("PUSH \"dummy\"");
  delperc();

  setLabel(solution_loop);
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("LT");
  snprintf(line,sizeof(line),"JZ *LABEL%i*",solution_end);
  out(line);

  /* A = soluzione corrente, senza consumare il frame esterno. */
  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("DECSP 2");
  out("POPA");
  out("INCSP 3");
  out("PUSHA");
  out("KEYAT");
  out("DECSP 1");
  out("PUSHA");
  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("GETELEM");

  /* Frame interno: [soluzione, numero binding, posizione, dummy]. */
  out("PUSHA");
  out("PUSHA");
  out("NUMKEY");
  out("PUSHA");
  out("PUSH 0");
  out("PUSH \"dummy\"");

  setLabel(binding_loop);
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("LT");
  snprintf(line,sizeof(line),"JZ *LABEL%i*",binding_end);
  out(line);

  /* A = chiave del binding. */
  out("DECSP 3");
  out("POPA");
  out("INCSP 4");
  out("PUSHA");
  out("DECSP 2");
  out("POPA");
  out("INCSP 3");
  out("PUSHA");
  out("KEYAT");
  out("PUSHA");

  /* Recupera il valore lasciando [chiave,valore] sopra il frame. */
  out("DECSP 4");
  out("POPA");
  out("INCSP 5");
  out("PUSHA");
  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("GETELEM");
  out("PUSHA");
  out("SETPATH 0");

  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("PUSH 1");
  out("SUM");
  out("DECSP 2");
  out("PUSHA");
  out("INCSP 1");
  snprintf(line,sizeof(line),"JMP *LABEL%i*",binding_loop);
  out(line);

  setLabel(binding_end);
  out("DECSP 4");
  blocco();

  out("DECSP 1");
  out("POPA");
  out("INCSP 2");
  out("PUSHA");
  out("PUSH 1");
  out("SUM");
  out("DECSP 2");
  out("PUSHA");
  out("INCSP 1");
  snprintf(line,sizeof(line),"JMP *LABEL%i*",solution_loop);
  out(line);

  setLabel(solution_end);
  out("DECSP 4");
}

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

  if (!strcmp(style,"file"))
  { char metadata[MAXVARLEN];
    if (strlen(name)>=MAXVARLEN-1) err("Form field name too long");
    snprintf(metadata,sizeof(metadata),"_%s",name);
    if (variable_index(metadata)<0)
    { remember_variable(metadata);
      snprintf(buf,sizeof(buf),"ADDSYMTABLE \"%s\"",metadata);
      out(buf);
      out("PUSH \"\"");
    }
  }

  out("PUSHA");
  sprintf(buf,"PUSH \"%s\"",name); out(buf);
  sprintf(buf,"PUSH \"%s\"",style); out(buf);
  emit_variable_value(variables[var]);
  out("PUSHA");
  delperc();
  if (code[cpos]=='(')
  { cpos++;
    calcexp();
    delperc();
    if (code[cpos]!=')') err("Missing )");
    cpos++;
  } else
  { sprintf(buf,"MOVA \"%s\"",name);
    out(buf);
  }
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
{ char line[100];
  int continuation=labelcount++;
  cpos+=3;
  delperc();
  out("STARTFORM");
  formfields();
  out("PUSHA");
  snprintf(line,sizeof(line),"ENDFORM *LABEL%i*",continuation);
  out(line);
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("SHOW");
  out("STOP");
  setLabel(continuation);
};

showformblock() /* FORMS: DA VERIFICARE */
{ char line[100];
  int continuation=labelcount++;
  cpos+=8;
  delperc();
  out("STARTFORM");
  out("PUSHA");
  inexp();
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  snprintf(line,sizeof(line),"ENDFORM *LABEL%i*",continuation);
  out(line);
  out("PUSHA");
  out("CONCAT");
  out("PUSHA");
  out("SHOW");
  out("STOP");
  setLabel(continuation);
};

/* ======================== FINE FORMS: DA VERIFICARE ===================== */

vardecl()
{ char line[3*MAXVARLEN];
  cpos+=3;
  while (1)
  { char name[MAXVARLEN];
    delperc();
    if (!token()) err("Missing variable name");
    strcpy(name,currtok);
    if (variable_index(name)>=0) err("Variable already declared");
    remember_variable(name);
    snprintf(line,sizeof(line),"ADDSYMTABLE \"%s\"",name);
    out(line);
    delperc();
    if (code[cpos]=='=')
    { cpos++;
      calcexp();
    } else out("MOVA \"\"");
    out("PUSHA");
    delperc();
    if (code[cpos]!=',') break;
    cpos++;
  }
}

returnblock()
{ char line[100];
  if (!infunction) err("RETURN outside function");
  cpos+=6;
  calcexp();
  if (current_type==1) out("ENDTASK");
  if (current_type==2) out("ENDTARGET");
  snprintf(line,sizeof(line),"DELSYMTABLE %i",current_arity);
  out(line);
  snprintf(line,sizeof(line),"DECSP %i",current_arity);
  out(line);
  out("RET");
}

codice()
{ delperc(); 
  int p=cpos; 
//Tipi strutturati
  if (istoken("dataset")) datasetdecl(); 
  else if (istoken("task")) fundecl(1);
  else if (istoken("target")) fundecl(2);
//Database
  else if (istoken("in")) inblock();
  else if (istoken("delete")) deleteblock();
//Web: DA VERIFICARE
  else if (istoken("ask")) askblock();
  else if (istoken("form")) formblock();
  else if (istoken("showform")) showformblock();
//Cicli
  else if (istoken("if")) ifblock();
  else if (istoken("while")) whileblock();
  else if (istoken("do")) downwhileblock();
  else if (istoken("foreach")) foreachblock();
  else if (istoken("goal")) goalblock();
  else if (istoken("for")) forblock();
  else if (istoken("cron")) cron();
  else if (istoken("refresh")) refresh();
  else if (istoken("run")) tok_run();
//Vaiabili e funzioni
  else if (istoken("var")) vardecl();
  else if (istoken("sub")) fundecl(0);
  else if (istoken("return")) returnblock();
  else if (code[cpos]=='.') asmline();
  else calcexp();
  if (p==cpos) err("Syntax error");
};
blocco()
{ delperc(); 
  if (code[cpos]=='{') 
  { cpos++;
    delperc();
    while (code[cpos]!='}')
    { codice();
      delperc();
      if (code[cpos]!=';') err("Missing ;");
      cpos++;
      delperc();
    }
    cpos++;
    delperc();
  } else codice();  
}
programma()
{ while (code[cpos]!=0) 
  { codice();
    delperc(); 
    if (code[cpos]!=';') err("Missing ;");
    cpos++;  
    delperc();
  };
};

int main(int argc, char** argv)
{ if ((argc != 3) && (argc != 4)) //per ora rigido: inputfile, outputfile
  { printf ("Uso %s [-d] inputfile.ewb outputfile.evm\n", argv[0]); 
    exit(-1); 
  };
  int dpos=0; 
  if (!strcmp(argv[1], "-d")) { dpos++; add_debug=1; };
  int f=open(argv[1+dpos], O_RDONLY); 
  if (f>=0)
  { int i=(int)read(f,code,MAXOUTPUTSIZE-1);
    if (i<0) err("Read error");
    close(f); printf("Read %i bytes\n", i);
    code[i]=0; 
    respos=0; 
    cpos=0; 
    programma();
    printf("\nOutput %i lines\n", PC);  
    f=open(argv[2+dpos],O_WRONLY|O_CREAT|O_TRUNC,0644);
    if (f<0) err("Output open error");
    if (write(f,res,respos)!=respos) err("Output write error");
    close(f);   
  } else err("Input open error");
  return 0;
};
