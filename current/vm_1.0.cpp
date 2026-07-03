/* VM EWB 2026 */

#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <time.h>
#include <cctype>
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

#include "vm_parts.h"
#include "db_interface.h"
#include "vm_1.0.h"

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
void addCron(string EP, string interval)
{ string x;

  if (interval=="")
  { run_query(DEFAULTENGINE,
      "delete from _cron where nometask=" + db_quote(EP),
      "");
    return;
  }

  x=run_query(DEFAULTENGINE,
    "update _cron set cron=" + db_quote(interval) + " where nometask=" + db_quote(EP),
    "");

  if (ewbIntValue(x)==0)
  { run_query(DEFAULTENGINE,
      "insert into _cron (nometask, cron) values (" + db_quote(EP) + ", " + db_quote(interval) + ")",
      "");
  }
}; //Parte della VM. Se non va bene, eccezione.

//Gestione interna errore. Al momento lo sputo sullo STDERR. 
void err(string e)
{ cerr << "Vm error at:  " << PC << "\nCode:\n";
  if (PC>0) cerr << (PC-1) << ":  " << code[PC-1] << "\n"; 
  cerr << (PC) << ":  " << code[PC] << "\n";
  cerr << (PC+1) << ":  " << code[PC+1] << "\n";
  cerr << "Error: " << e << "\n";
  cerr <<"Stack: \n"; 
  for (int i=1; i<6 && SP-i>=0; i++) cerr << (SP-i) << ":  [" << stack[SP-i] << "]\n";   
  exit(-1);
};
//Dato il nome di una variabile restituisce la posizione in stack del dato.
int findvar(string varname) 
{ int i; for (i=ST-1; i>=0; i--) { if (varname==symtname[i]) return symtpos[i];}; 
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
{ PUSH(e);
  raise();
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
    }
  }

  if (key.size() == 1)  
  { old = value;
  }
  else 
  { vector<string> keyb(key.begin() + 1, key.end());
    old = setpath(old, value, keyb);
  }

  if (found >= 0)
  { v[found] = hk + ":" + hexEncode(old);
  }
  else
  { v.push_back(hk + ":" + hexEncode(old));
  }

  return join(v, " ");
}
string opcode(string IR)
{ string op="";
  for (int i=0; i<(int)IR.size() && IR[i]!=' '; i++) op += tolower((unsigned char)IR[i]);
  return op;
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

void resume(int xPC,int xSP)
{ PC=xPC; SP=xSP;//Program counter e stack pointer
  string A="";   //Accumulatore 
  string x,y;    //x e y diventano registri, come sul 6502.. 
  string IR;     //Istruction register
  string OP;     //Opcode normalizzato
  string varname, vartype, varvalue; //Uso anche questi troppo spesso per non metterli qui. 
  for (;;)
  { if (PC<0 || PC>=MAXLINES || code[PC]=="") raiseerr("PC out of code");
    IR=code[PC];  //Comando, parametri.
    OP=opcode(IR);
    if (OP=="sum")
    { POP(y); POP (x); A=ewbSum(x,y);
    } else if (OP=="concat")
    { POP(y); POP(x); A = x + y; 
    } else if (OP=="sub")
    { POP(y); POP(x); A=ewbSum(x,ewbNegative(y));
    } else if (OP=="mul")
    { POP(y); POP(x); A=ewbMul(x,y);
    } else if (OP=="div")
    { POP(y); POP(x); A=ewbDiv(x,y);
    } else if (OP=="or")
    { POP(y); POP(x); A=ewbOr(x,y);
    } else if (OP=="and")
    { POP(y); POP(x); A=ewbAnd(x,y);
    } else if (OP=="andb")
    { POP(y); POP(x); A=ewbBitwiseAnd(x,y);
    } else if (OP=="orb")
    { POP(y); POP(x); A=ewbBitwiseOr(x,y);
    } else if (OP=="not")
    { POP(x); A=ewbNot(x);
    } else if (OP=="notb")
    { POP(x); A=ewbBitwiseNot(x);
    } else if ((OP=="gt" || OP=="lt" || OP=="eq" || OP=="neq" || OP=="ge" || OP=="le"))
    { POP(y); POP(x); A=ewbCompare(x, y, OP); 
    } else if (OP=="jz")
    { POP(x); if (A=="0" || A=="") PC=ewbIntValue(x)-1; 
    } else if (OP=="jnz")
    { POP(x); if (A!="0" && A!="") PC=ewbIntValue(x)-1; 
    } else if (OP=="call")
    { PUSH(ewbInt(PC)); PC=ewbIntValue(A)-1;  //E sono saltato..  
    } else if (OP=="ret")
    { POP(x); PC=ewbIntValue(x)-1;             //E  sono tornato..
    } else if (OP=="mova")                   //Unica istruzione con parametri.. Mi adeguo.. prendo il dato dallo stack.  
    { A=togliVirgolette(IR.substr(5));                  //Metto il valore in accumulatore.
    } else if (OP=="pusha")
    { PUSH(A); 
    } else if (OP=="push") //Push diretto evita di fare MOVA val, PUSH val e lascia intatti i registri.
    { string t=togliVirgolette(IR.substr(5));
      PUSH(t); 
    } else if (OP=="popa")
    { POP(A);  
    } else if (OP=="addsymtable") //Aggiunge la variabile alla smbol table. Va seguita da un push del valore. 
    { symtname[ST]=A; 
      symtpos[ST]=SP; 
      ST++; if (ST>=MAXVAR) err("Vars overflow");
    } else if (OP=="startform")
    { cout << "<form method=post id=__form" << IDF << " enctype=multipart/form-data>";
      IDF++;  
    } else if (OP=="starttarget")
    { cout << "<form style=visibility:hidden id=__form" << IDF << " method=post enctype=multipart/form-data>"; 
      IDF++; 
    } else if (OP=="addtoform")
    { POP(varvalue); POP(vartype); POP(varname);
      if (vartype=="input" || vartype=="date" || vartype=="numeric")
      { if (vartype=="numeric") vartype="number"; 
        cout << "<input type=" << vartype << " name=" << varname << " id=" << varname << " value=" << mettiVirgolette(varvalue) << " />\n"; 
      } else if (vartype=="textarea")
      { cout << "<textarea  name=" << varname << " id=" << varname << "/>" << escapeTag(varvalue) << "</textarea>";  
      } else  //Gli altri tipi.. ancora da fare.
      {
      };
    } else if (OP=="sendform")
    { //Aggiungo al form lo stack 
      cout << "<textarea name=__stack   id=__stack     >" << escapeStack() << "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint value=\"" << (PC + 1) << "\">\n"; 
      cout << "<input name=__stackpos   id=__stackpos   value=\"" << SP << "\">\n"; 
      cout << "<input name=__signature  id=__signature  value=\"" << signature(ewbInt(SP) + " " + ewbInt(PC+1) + " " + escapeStack()) << "\">\n"; 
      cout << "</form>\n";
    } else if (OP=="stop")
    { exit(0); //In attesa di rientrare dall'entry point PC 
    } else if (OP=="runtarget") //run TARGET - come STOP - Mi faccio passare l'entry point
    { string interval;
      POP(interval); POP(varname); 
      string EP=stack[findvar(varname)]; //posizione del target. 
      cout << "<textarea name=__stack   id=__stack     >" << escapeStack() << "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint value=\"" << EP << "\">\n"; 
      cout << "<input name=__stackpos   id=__stackpos   value=\"" << SP << "\">\n"; 
      cout << "<input name=__signature  id=__signature  value=\"" << signature(ewbInt(SP) + " " + EP + " " + escapeStack()) << "\">\n"; 
      cout << "</form>\n";
      cout << "<script>runtarget(" << (IDF-1) << "," << interval << ");</script>\n";  
    } else if (OP=="crontask") //cron task 
    { string interval;
      POP(interval); POP(varname);
      string EP=stack[findvar(varname)];    //posizione del task. 
      addCron(EP, interval);                //Funzione interna, devo gestire un CRON a minuti o a secondi. 
    } 
// ZENO: da qui intoccabile finche non diventa software definitivo //
      else if (OP=="task" || OP=="target")    //Punto di inizio di una procedura asincrona.
    { POP(varname); //Se gia running impedisco la sovrapposizione
      x=run_query(DEFAULTENGINE,  
        "update _" + OP + "s set status='running', starttime=" + to_string(time(NULL)) + " where name='" + varname + "' and status<>'running'", "");
      if  (ewbIntValue(x)==0) 
      { x=run_query(DEFAULTENGINE,  
        "insert into _" + OP + "s (name, status, starttime) values ('" + varname + "', 'running', " + to_string(time(NULL)) + ")", "");
      };
      if  (ewbIntValue(x)==0) exit(0); //niente sovrapposizione
      PUSH(varname); //Cosi è gia pronto per la ENDTASK
    } else if (OP=="endtask" || OP=="endtarget")    //Punto di inizio di una procedura asincrona.
    { POP(varname);
      string tasktype=OP;
      tasktype=tasktype.substr(3);
      run_query(DEFAULTENGINE, "update _" + tasktype + "s set status='stopped', starttime=" + to_string(time(NULL)) + " where name='" + varname + "'", "");
    } else if (OP=="qlist")  //vedi cicloInEDatabase.txt in doc/
    { string order, query, context; 
      POP(order); 
      POP(query);
      POP(context);
      A=qlist(context, query, order);
    } else if (OP=="qbyid")  //vedi cicloInEDatabase.txt in doc/
    { string order, query, context, id; 
      POP(id);
      POP(order); 
      POP(query);
      POP(context);
      A=qbyid(context, query, order, id);
    } else if (OP=="query")
    { string order, query, context; 
      POP(order); 
      POP(query);
      POP(context);
      A=run_query(context, query, order);
    } 
// ZENO: fino a qui intoccabile finche non diventa software definitivo //
      else if (OP=="onerror") //Visibilità, sempre l'ultima in ricorsione.  
    { POP(varname); 
      symtname[ST]="__on_error"; 
      symtpos[ST]=ewbIntValue(varname); 
      ST++; if (ST>=MAXVAR) err("Vars overflow");
    } else if (OP=="raise") //c'e' l'errore. 
    { raise();    
    } else if (OP=="setpath") //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; PUSH "pippo"; SETPATH 2; */
      int numlev=ewbIntValue(IR.substr(8));
      POP(x); //Value
      //pop array di livelli
      vector <string> v; 
      for (int i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      POP(varname);
      A=stack[findvar(varname)];
      stack[findvar(varname)] = setpath(A, x, v);  
    } else if (OP=="getpath") //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; GETPATH 2; */
      int numlev=ewbIntValue(IR.substr(8));
      vector <string> v; 
      for (int i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      POP(varname);
      A=getpath(stack[findvar(varname)], v);     
    };
    PC++; 
  };
};


void start() 
{ resume(0,0);
};

static void resetVm()
{ PC=0;
  SP=0;
  ST=0;
  IDF=0;
  for (int i=0; i<MAXLINES; i++) code[i]="";
}

static void loadProgram(const char *program)
{ string text;
  if (program) text=program;

  int line=0;
  string IR;
  for (int i=0; i<=(int)text.size(); i++)
  { if (i==(int)text.size() || text[i]=='\n')
    { if (IR!="")
      { code[line]=IR;
        line++;
        if (line>=MAXLINES) raiseerr("Code overflow");
      }
      IR="";
    }
    else if (text[i]!='\r')
    { IR += text[i];
    }
  }
}

static void loadStack(const char *encoded_stack, int stackpos)
{ SP=0;
  if (encoded_stack && encoded_stack[0])
  { vector<string> v=split(encoded_stack, ' ');
    for (int i=0; i<(int)v.size(); i++)
    { if (v[i]!="")
      { stack[SP]=hexDecode(v[i]);
        SP++;
        if (SP>=MAXSTACK) raiseerr("Stack overflow");
      }
    }
  }

  if (stackpos>=0) SP=stackpos;
  if (SP<0 || SP>=MAXSTACK) raiseerr("Stack pointer");
}

extern "C" int ewb_run_text(const char *program, int entrypoint, int stackpos, const char *encoded_stack)
{ resetVm();
  loadProgram(program);
  loadStack(encoded_stack, stackpos);

  if (entrypoint<0) entrypoint=0;
  resume(entrypoint,SP);
  return 0;
}
