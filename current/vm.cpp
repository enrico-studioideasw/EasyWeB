/* VM EWB 2026 */



/* Cose mancanti
   - implementare opcode CRON 


*/



#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <time.h>
#include <cctype>
#include "opcodes.h"
using namespace std;

#define MAXSTACK 10000
#define MAXLINES 100000  //Qui vedremo di faree qualcosa di dinamico. Intanto deve funzionare. 
#define MAXVAR 2000
#define POP(v)  { if (SP <= 0) err("Stack underflow"); (v) = stack[--SP]; }
#define PUSH(v) { if (SP >= MAXSTACK) err("Stack overflow"); stack[SP]=v; SP++; }

string stack[MAXSTACK];
int PC;                       //Program counter
int SP;                       //Stack pointer
string symtname[MAXVAR];      //Symbol table.. demandata alla VM ???
int     symtpos[MAXVAR];
int ST=0;                     //Symbol table pointer.
int IDF=0;
string code[MAXLINES];
string codearg[MAXLINES];
EWBOpcode codeop[MAXLINES];
int codeLines=0;
int RUNNING=1; 
string PROGRAM_URL="";

#include "vm_parts.h"
#include "db_interface.h"
#include "vm.h"

//Queste rimangono qui: fanno parte della logica di linguaggio.
string escapeStack()
{ int i;
  string r;  
  for (i=0; i<SP; i++)
  { r=r + hexEncode(stack[i]);
    if (i<SP-1) r = r + ' ';
  };
  return r; 
};
//Gestione interna errore. Al momento lo sputo sullo STDERR. 
void err(string e)
{ cerr << "Vm error at:  " << PC << "\nCode:\n";
  if (PC>0) cerr << (PC-1) << ":  " << code[PC-1] << "\n"; 
  if (PC>=0 && PC<codeLines) cerr << (PC) << ":  " << code[PC] << "\n";
  if (PC+1<codeLines) cerr << (PC+1) << ":  " << code[PC+1] << "\n";
  cerr << "Error: " << e << "\n";
  cerr <<"Stack: \n"; 
  for (int i=1; i<6 && SP-i>=0; i++) cerr << (SP-i) << ":  [" << stack[SP-i] << "]\n";   
  RUNNING=-1; 
};
//Dato il nome di una variabile restituisce la posizione in stack del dato.
int findvar(string varname) 
{ int i; 
  string n;
  for (i=ST-1; i>=0; i--) 
  { n=symtname[i];
    if (varname==n) return symtpos[i];
    if (n.size()>varname.size() && n[n.size()-varname.size()-1]=='.')
    { if (n.substr(n.size()-varname.size())==varname) return symtpos[i];
    };
  }; 
  return -1; //Non trovata 
};
//Questa fa comodo che diventi una funzione.. dovessimo segnalare qualcosa da sistema.. 
void raise()
{ int x=findvar("_on_error");
  if (x>-1) 
  { PC=x; //Vado all'indirizzo che c'e' dentro X. Quello dell'error handler. 
  } else //Gestione interna: segnalo l'errore e mi fermo.  
  { string e; POP(e); err(e);  
  };
};

void raiseerr(string e)
{ PUSH(e); raise();
};

string setpath(string s, string value, vector<string> key)
{ if (key.size() == 0) return value;
  vector<string> v = split(s, ' ');
  string k = key[0];
  string hk = hexEncode(k);
  string old = "";
  int found = -1;
  for (int i=0; i<(int)v.size(); i++)
  { vector<string> p = split(v[i], ':');
    if (p.size() >= 2 && p[0] == hk)
    { found = i;
      old = hexDecode(p[1]);
      break;
    };
  };
  if (key.size() == 1)  
  { old = value;
  } else 
  { vector<string> keyb(key.begin() + 1, key.end());
    old = setpath(old, value, keyb);
  };
  if (found >= 0)
  { v[found] = hk + ":" + hexEncode(old);
  } else v.push_back(hk + ":" + hexEncode(old));
  return join(v, " ");
}
EWBOpcode opcode(string IR)
{ string op="";
  for (int i=0; i<(int)IR.size() && IR[i]!=' '; i++) op += tolower((unsigned char)IR[i]);
  int i=find_vm_instr(op.c_str());
  if (i<0) return OPCODE_INVALID;
  return vm_instr_table[i].opcode;
}

string getpath(string s, vector<string> key)
{ if (key.size() == 0) return s;
  vector<string> v = split(s, ' ');
  string k = key[0];
  string found = "";
  // cerca hex(k):qualcosa
  for (int i=0; i<(int)v.size(); i++)
  { vector<string> p = split(v[i], ':');
    if (p.size() >= 2 && hexDecode(p[0]) == k)
    { found = hexDecode(p[1]);
      break;
    }
  }
  if (found == "") return "";   // chiave non trovata
  vector<string> keyb(key.begin() + 1, key.end());
  return getpath(found, keyb);
}

#define EWB_VM_RUNTIME
#include "ewb_predef.c"
#undef EWB_VM_RUNTIME

int resume(int xPC,int xSP)
{ PC=xPC; SP=xSP;//Program counter e stack pointer
  string A="";   //Accumulatore  
  string x,y;    //x e y diventano registri, come sul 6502.. 
  EWBOpcode OP;  //Opcode normalizzato
  string varname, vartype, varvalue; //Uso anche questi troppo spesso per non metterli qui. 
  RUNNING=1; 
  for (;;)
  { if (PC<0 || PC>=codeLines) raiseerr("PC out of code");
    if (RUNNING!=1) return RUNNING; 
    OP=codeop[PC];
    if (OP==OPCODE_INVALID)
    { raiseerr("Unknown opcode");
    } else if (OP==OP_SUM)
    { POP(y); POP (x); A=ewbSum(x,y);
    } else if (OP==OP_CONCAT)
    { POP(y); POP(x); A = x + y; 
    } else if (OP==OP_SUB)
    { POP(y); POP(x); A=ewbSum(x,ewbNegative(y));
    } else if (OP==OP_MUL)
    { POP(y); POP(x); A=ewbMul(x,y);
    } else if (OP==OP_DIV)
    { POP(y); POP(x); A=ewbDiv(x,y);
    } else if (OP==OP_MOD)
    { POP(y); POP(x); A=ewbMod(x,y);
    } else if (OP==OP_OR)
    { POP(y); POP(x); A=ewbOr(x,y);
    } else if (OP==OP_AND)
    { POP(y); POP(x); A=ewbAnd(x,y);
    } else if (OP==OP_ANDB)
    { POP(y); POP(x); A=ewbBitwiseAnd(x,y);
    } else if (OP==OP_ORB)
    { POP(y); POP(x); A=ewbBitwiseOr(x,y);
    } else if (OP==OP_NOT)
    { POP(x); A=ewbNot(x);
    } else if (OP==OP_NOTB)
    { POP(x); A=ewbBitwiseNot(x);
    } else if (OP==OP_GT || OP==OP_LT || OP==OP_EQ || OP==OP_NEQ || OP==OP_GE || OP==OP_LE ||
               OP==OP_SGT || OP==OP_SLT || OP==OP_SEQ || OP==OP_SNEQ || OP==OP_SGE || OP==OP_SLE)
    { POP(y); POP(x); A=ewbCompare(x, y, vm_instr_table[vm_instr(OP)].name); 
    } else if (OP==OP_JZ)
    { POP(x); if (A=="0" || A=="") PC=ewbIntValue(x)-1; 
    } else if (OP==OP_JNZ)
    { POP(x); if (A!="0" && A!="") PC=ewbIntValue(x)-1; 
    } else if (OP==OP_JMP)
    { PC=ewbIntValue(codearg[PC])-1;
    } else if (OP==OP_CALL)
    { POP(x); 
      int tmp=SP;
      SP = SP - ewbIntValue(x) + 1;
      PUSH(codearg[PC]);    //Metto PC in posizione SP-(arity+1). Ho gia preparato il "buco".
      SP=tmp; 
      PC=ewbIntValue(A)-1;  //E sono saltato..  
    } else if (OP==OP_RET)
    { POP(x); PC=ewbIntValue(x)-1;             //E  sono tornato..
    } else if (OP==OP_MOVA)                   //Unica istruzione con parametri.. Mi adeguo.. prendo il dato dallo stack.  
    { A=codearg[PC];                  //Metto il valore in accumulatore.
    } else if (OP==OP_PUSHA)
    { PUSH(A); 
    } else if (OP==OP_PUSH) //Push diretto evita di fare MOVA val, PUSH val e lascia intatti i registri.
    { PUSH(codearg[PC]); 
    } else if (OP==OP_POPA)
    { POP(A);  
    } else if (OP==OP_ADDSYMTABLE) //Aggiunge la variabile alla symbol table. Va seguita da un push del valore. 
    { symtname[ST]=codearg[PC]; 
      symtpos[ST]=SP; 
      ST++; if (ST>=MAXVAR) err("Vars overflow");
    } else if (OP==OP_DELSYMTABLE)
    { int n=ewbIntValue(codearg[PC]);
      ST=ST-n;
      if (ST<0) err("Vars underflow");
    } else if (OP==OP_DECSP)
    { int n=ewbIntValue(codearg[PC]);
      SP=SP-n;
      if (SP<0) err("Stack underflow");
    } else if (OP==OP_INCSP)
    { int n=ewbIntValue(codearg[PC]);
      SP=SP+n;
      if (SP<0) err("Stack underflow");
    } else if (OP==OP_STARTFORM)
    { A="<form method=post id=__form" + to_string(IDF) + " enctype=multipart/form-data>";
      IDF++;  
    } else if (OP==OP_STARTTARGET)
    { A="<form style=visibility:hidden id=__form" + to_string(IDF) + " method=post enctype=multipart/form-data>";
      IDF++; 
    } else if (OP==OP_ADDFORM)
    { POP(varvalue); POP(vartype); POP(varname);
      if (vartype=="text" || vartype=="input" || vartype=="date" || vartype=="numeric")
      { if (vartype=="input") vartype="text";
        if (vartype=="numeric") vartype="number";
        A="<input type=" + vartype + " name=" + varname + " id=" + varname + " value=" + mettiVirgolette(varvalue) + " />\n";
      } else if (vartype=="textarea")
      { A="<textarea name=" + varname + " id=" + varname + ">" + escapeTag(varvalue) + "</textarea>";
      } else  //Gli altri tipi.. ancora da fare.
      { A="";
      };
    } else if (OP==OP_ENDFORM)
    { string stacktext=escapeStack();
      A="<textarea name=__stack id=__stack>" + stacktext + "</textarea>\n" +
        "<input name=__entrypoint id=__entrypoint value=\"" + ewbInt(PC+1) + "\">\n" +
        "<input name=__stackpos id=__stackpos value=\"" + ewbInt(SP) + "\">\n" +
        "<input name=__signature id=__signature value=\"" + signature(ewbInt(SP) + " " + ewbInt(PC+1) + " " + stacktext) + "\">\n</form>\n";
    } else if (OP==OP_DBLOCK)
    { p_lock(&A);
    } else if (OP==OP_DBUNLOCK)
    { p_unlock(&A);
    } else if (OP==OP_PRINT)
    { p_print(&A);
    } else if (OP==OP_EPRINT)
    { p_eprint(&A);
    } else if (OP==OP_INPUT)
    { p_input(&A);
    } else if (OP==OP_FLOAD)
    { p_load(&A);
    } else if (OP==OP_FSAVE)
    { p_save(&A);
    } else if (OP==OP_FREADDIR)
    { p_loaddir(&A);
    } else if (OP==OP_TOINT)
    { p_int(&A);
    } else if (OP==OP_SIND)
    { p_sin(&A);
    } else if (OP==OP_TOHEX)
    { p_hex(&A);
    } else if (OP==OP_SQRT)
    { p_sqr(&A);
    } else if (OP==OP_TIME)
    { p_time(&A);
    } else if (OP==OP_DATE)
    { p_date(&A);
    } else if (OP==OP_RANDOM)
    { p_random(&A);
    } else if (OP==OP_SLEEP)
    { p_sleep(&A);
    } else if (OP==OP_STOP)
    { db_close_all();
      RUNNING=0; //In attesa di rientrare dall'entry point PC 
    } else if (OP==OP_RUNTARGET) //run TARGET - come STOP - Mi faccio passare l'entry point
    { string interval;
      POP(interval); POP(varname); 
      string EP=stack[findvar(varname)]; //posizione del target. 
      cout << "<textarea name=__stack   id=__stack     >" << escapeStack() << "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint value=\"" << EP << "\">\n"; 
      cout << "<input name=__stackpos   id=__stackpos   value=\"" << SP << "\">\n"; 
      cout << "<input name=__signature  id=__signature  value=\"" << signature(ewbInt(SP) + " " + EP + " " + escapeStack()) << "\">\n"; 
      cout << "</form>\n";
      cout << "<script>runtarget(" << (IDF-1) << "," << interval << ");</script>\n";  
    } else if (OP==OP_CRONTASK) //cron task 
    { int arity=ewbIntValue(codearg[PC]);
      string cronstring, taskname;
      string stacktext;
      POP(cronstring);
      string s=""; 
      for (int i=0; i<arity; i++)
      { POP(x); 
        string p=ewbInt(i); 
        vector <string> v; v.push_back(p); 
        s=setpath(s,x,v); 
      };
      POP(taskname);
      stacktext=escapeStack();
      string url=stack[findvar("ewb._url")];
      string user=stack[findvar("ewb._user")];
      string password=stack[findvar("ewb._password")];
      create_base_tables(url,user,password);
         //Funzione interna, devo gestire un CRON a minuti o a secondi. 
      if (cronstring=="")
      { run_query(url,user,password,
          "delete from _crontab where task='" + taskname + "'", ""); 
      } else  
      { if (!validCronString(cronstring)) err("Invalid cronstring"); 
        run_query(url,user,password,
          "delete from _crontab where task='" + taskname + "'", "");
        run_query(url,user,password,
          "insert into _crontab (task, program_url, stack, parameters, cronstring) values ('" + 
          taskname + "','" + PROGRAM_URL + "','" + stacktext + "','" + s + "','" + cronstring + "')", "");
      };
    } else if (OP==OP_TASK || OP==OP_TARGET)    //Punto di inizio di una procedura asincrona.
    { POP(varname); //Se gia running impedisco la sovrapposizione
      string tasktype="task";
      if (OP==OP_TARGET) tasktype="target";
      string url=stack[findvar("ewb._url")];
      string user=stack[findvar("ewb._user")];
      string password=stack[findvar("ewb._password")];
      create_base_tables(url,user,password);
      x=run_query(url,user,password,
        "update _" + tasktype + "s set status='running', starttime=" + to_string(time(NULL)) + " where name='" + varname + "' and status<>'running'", "");
      if  (ewbIntValue(x)==0) 
      { x=run_query(url,user,password,
        "insert into _" + tasktype + "s (name, status, starttime) values ('" + varname + "', 'running', " + to_string(time(NULL)) + ")", "");
      };
      if  (ewbIntValue(x)==0) RUNNING=0; //niente sovrapposizione
      PUSH(varname); //Cosi è gia pronto per la ENDTASK
    } else if (OP==OP_ENDTASK || OP==OP_ENDTARGET)    //Punto di inizio di una procedura asincrona.
    { POP(varname);
      string tasktype="task";
      if (OP==OP_ENDTARGET) tasktype="target";
      string url=stack[findvar("ewb._url")];
      string user=stack[findvar("ewb._user")];
      string password=stack[findvar("ewb._password")];
      create_base_tables(url,user,password);
      run_query(url,user,password,
        "update _" + tasktype + "s set status='stopped', starttime=" + to_string(time(NULL)) + " where name='" + varname + "'", "");
    } else if (OP==OP_QLIST)  //vedi cicloInEDatabase.txt in doc/
    { string context, filter;
      POP(filter);
      POP(context);
      PUSH(context);
      string url=stack[findvar(context+"._url")];
      string user=stack[findvar(context+"._user")];
      string password=stack[findvar(context+"._password")];
      string orderby=stack[findvar(context+"._orderby")];
      vector<string> fields;
      for (int i=ST-1; i>=0; i--)
      { if (symtname[i].compare(0,context.length()+1,context+".")==0)
        { string field=symtname[i].substr(context.length()+1);
          if (field!="_status") fields.push_back(field);
        }
      }
      A=qlist(url,user,password,context,fields,filter,orderby);
    } else if (OP==OP_QBYID)  //vedi cicloInEDatabase.txt in doc/
    { string context, id;
      POP(id);
      //POP(filter);
      POP(context);
      string url=stack[findvar(context+"._url")];
      string user=stack[findvar(context+"._user")];
      string password=stack[findvar(context+"._password")];
      string orderby=stack[findvar(context+"._orderby")];
      vector<string> fields;
      vector<int> positions;
      for (int i=ST-1; i>=0; i--)
      { if (symtname[i].compare(0,context.length()+1,context+".")==0)
        { string field=symtname[i].substr(context.length()+1);
          if (field=="_status") continue;
          fields.push_back(field);
          positions.push_back(symtpos[i]);
        }
      }
      vector<string> record=qbyid(url,user,password,context,fields, orderby,id);
      for (size_t i=0; i<positions.size(); i++)
      { stack[positions[i]]="";
        if (i<record.size()) stack[positions[i]]=record[i];
      }
      A="";
      if (!record.empty()) A="1";
    } else if (OP==OP_QUERY)
    { string query, context;
      POP(query);
      POP(context);
      string url=stack[findvar(context+"._url")];
      string user=stack[findvar(context+"._user")];
      string password=stack[findvar(context+"._password")];
      string orderby=stack[findvar(context+"._orderby")];
      vector<string> fields;
      for (int i=ST-1; i>=0; i--)
      { if (symtname[i].compare(0,context.length()+1,context+".")==0)
        { string field=symtname[i].substr(context.length()+1);
          if (field!="_status") fields.push_back(field);
        }
      }
      A=run_query(url,user,password,context,fields,query,orderby);
    } 
    else if (OP==OP_ONERROR) //Visibilità, sempre l'ultima in ricorsione.  
    { POP(varname); 
      symtname[ST]="__on_error"; 
      symtpos[ST]=ewbIntValue(varname); 
      ST++; if (ST>=MAXVAR) err("Vars overflow");
    } else if (OP==OP_RAISE) //c'e' l'errore. 
    { raise();    
    } else if (OP==OP_SETPATH) //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; PUSH "pippo"; SETPATH 2; */
      int numlev=ewbIntValue(codearg[PC]);
      POP(x); //Value
      //pop array di livelli
      vector <string> v; 
      for (int i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      POP(varname);
      A=stack[findvar(varname)];
      stack[findvar(varname)] = setpath(A, x, v);  
    } else if (OP==OP_GETPATH) //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; PUSH 2; GETPATH */
      POP(x);
      int numlev=ewbIntValue(x);
      vector <string> v; 
      for (int i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      POP(varname);
      A=getpath(stack[findvar(varname)], v);     
    };
    PC++; 
  };
};

// Estrae l'eventuale argomento dalla riga testuale gia' isolata dal loader.
static string textArg(string IR)
{ int i=0;
  while (i<(int)IR.size() && IR[i]!=' ') i++;
  while (i<(int)IR.size() && IR[i]==' ') i++;
  if (i>=(int)IR.size()) return "";
  string a=IR.substr(i);
  if (a.size()>0 && a[0]=='"') return togliVirgolette(a);
  return a;
}

// Inserisce una istruzione normalizzata: opcode numerico, argomento e testo debug.
static void addCode(int line, EWBOpcode op, string arg, string debug)
{ if (line>=MAXLINES) err("Code overflow");
  codeop[line]=op;
  codearg[line]=arg;
  code[line]=debug;
}

// Loader testuale: una riga non vuota corrisponde a una istruzione VM.
static void loadProgramText(const char *program)
{ string text;
  if (program) text=program;
  string IR;
  codeLines=0;
  for (int i=0; i<=(int)text.size(); i++)
  { if (i==(int)text.size() || text[i]=='\n')
    { if (IR!="")
      { EWBOpcode op=opcode(IR);
        addCode(codeLines, op, textArg(IR), IR);
        codeLines++;
      }
      IR="";
    } else if (text[i]!='\r') IR += text[i];
  }
  SP=0; IDF=0; ST=0; 
}

// Loader binario v1: 0x00, versione 0x01, poi opcode >=0x80 e argomenti.
static void loadProgramBinary(const unsigned char *program, size_t len)
{ if (len<2 || program[0]!=0 || program[1]!=1) err("Bad binary header");
  size_t pos=2;
  codeLines=0;
  while (pos<len)
  { EWBOpcode op=(EWBOpcode)program[pos];
    pos++;
    int instr=vm_instr(op);
    if (instr<0) err("Bad binary opcode");
    string arg="";
    if (argtype==ARG_INT) arg=readBinaryInt(program,len,&pos);
    else if (argtype==ARG_STRING) arg=readBinaryString(program,len,&pos);
    else if (argtype==ARG_INT_OR_STRING)
    { if (pos<len && program[pos]=='"') arg=readBinaryString(program,len,&pos);
      else arg=readBinaryInt(program,len,&pos);
    };
    addCode(codeLines, op, arg, vm_instr_table[instr].name + string(" ") + arg);
    codeLines++;
  }
  SP=0; IDF=0; ST=0; 
}

// Se il primo byte e' 0x00 usa il formato binario, altrimenti il testo.
static void loadProgram(const char *program, size_t len)
{ if (!program) err("No program");
  if (len>0 && ((const unsigned char*)program)[0]==0)
  { loadProgramBinary((const unsigned char*)program, len);
  } else loadProgramText(program);
}

static void loadStack(const char *encoded_stack, int stackpos)
{ SP=0; string A; 
  vector<string> v=split(encoded_stack, ' ');
  for (int i=0; i<(int)v.size(); i++) if (v[i]!="") { A=hexDecode(v[i]); PUSH(A); };
  SP=stackpos; 
}

extern "C" int ewb_run_text(const char *program, const char *program_url, int entrypoint, int stackpos, const char *encoded_stack)
{ if (program_url) PROGRAM_URL=program_url;
  else PROGRAM_URL="";
  if (program) loadProgram(program, strlen(program));
  else loadProgram(program, 0);
  loadStack(encoded_stack, stackpos);
  if (entrypoint<0) err("Wrong EP");
  return resume(entrypoint,SP);
}

extern "C" int ewb_run_buffer(const char *program, size_t len, const char *program_url, int entrypoint, int stackpos, const char *encoded_stack)
{ if (program_url) PROGRAM_URL=program_url;
  else PROGRAM_URL="";
  loadProgram(program, len);
  loadStack(encoded_stack, stackpos);
  if (entrypoint<0) err("Wrong EP");
  return resume(entrypoint,SP);
}
