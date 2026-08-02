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
#include <algorithm>
#include "opcodes.h"
#include "ewb_hash.h"
using namespace std;

void raiseerr(string e);

#define MAXSTACK 10000
#define MAXLINES 100000  //Qui vedremo di faree qualcosa di dinamico. Intanto deve funzionare.
#define MAXVAR 2000
#define POP(v)  { if (SP <= 0) raiseerr("Stack underflow"); (v) = stack[--SP]; }
#define PUSH(v) { if (SP >= MAXSTACK) raiseerr("Stack overflow"); stack[SP]=v; SP++; }

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
static bool error_handler_running=false;
static int error_handler_return=-1;
static string active_target_name="";

struct PendingFormValue
{ string name;
  string value;
};
struct PendingFormFile
{ string name;
  string filename;
  string content_type;
  string path;
  size_t size;
};
static vector<PendingFormValue> pending_form_values;
static vector<PendingFormFile> pending_form_files;

#include "vm_parts.h"
#include "db_interface.h"
#include "prolog_engine.h"
#include "vm.h"

static bool p_live_resource(string value);
static void p_close_resources(void);

//Queste rimangono qui: fanno parte della logica di linguaggio.
string escapeStack()
{ int i;
  string r="@";
  for (i=0; i<ST; i++)
  { if (i) r+=",";
    r+=hexEncode(symtname[i])+":"+ewbInt(symtpos[i]);
  }
  r+="|";
  for (i=0; i<SP; i++)
  { if (p_live_resource(stack[i])) raiseerr("Live resource cannot be serialized");
    r=r + hexEncode(stack[i]);
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

static void stop_active_target(void)
{ if (active_target_name=="") return;
  int url_position=findvar("ewb._url");
  int user_position=findvar("ewb._user");
  int password_position=findvar("ewb._password");
  if (url_position>=0 && user_position>=0 && password_position>=0)
  { string url=stack[url_position];
    string user=stack[user_position];
    string password=stack[password_position];
    create_base_tables(url,user,password);
    run_query(url,user,password,
      "update _targets set status='stopped', starttime=" +
      to_string(time(NULL)) + " where name='" + active_target_name + "'", "");
  }
  active_target_name="";
}

static int errorHandler(void)
{ for (int i=ST-1; i>=0; i--)
    if (symtname[i]=="__on_error") return symtpos[i];
  return -1;
}
void raiseerr(string e)
{ throw e;
};

string setpath(string s, string value, vector<string> key)
{ return ewbSetPath(s,value,key);
}
EWBOpcode opcode(string IR)
{ string op="";
  for (int i=0; i<(int)IR.size() && IR[i]!=' '; i++) op += tolower((unsigned char)IR[i]);
  int i=find_vm_instr(op.c_str());
  if (i<0) return OPCODE_INVALID;
  return vm_instr_table[i].opcode;
}

string getpath(string s, vector<string> key)
{ return ewbGetPath(s,key);
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
  { if (RUNNING!=1) return RUNNING;
    try
    { if (PC==codeLines)
      { if (db_has_transactions())
        { db_rollback_all();
          raiseerr("Program ended with open transaction");
        }
        p_close_resources();
        db_close_all();
        RUNNING=0;
        continue;
      }
      if (PC<0 || PC>codeLines) raiseerr("PC out of code");
      db_check_transactions();
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
    { if (!ewbTrue(A)) PC=ewbIntValue(codearg[PC])-1;
    } else if (OP==OP_JNZ)
    { if (ewbTrue(A)) PC=ewbIntValue(codearg[PC])-1;
    } else if (OP==OP_JNCONTEXT)
    { POP(varname);
      string context="";
      for (int pos=SP-1; pos>=0 && context==""; pos--)
      { string candidate=stack[pos];
        string dataset=candidate+"._dataset";
        string url=candidate+"._url";
        for (int i=0; i<ST; i++)
        { if (symtname[i]==dataset || symtname[i]==url) context=candidate;
        }
      }
      if (context=="") raiseerr("JNCONTEXT without context");
      string field=varname;
      string prefix=context+".";
      if (field.compare(0,prefix.size(),prefix)==0)
        field=field.substr(prefix.size());
      string fullname=prefix+field;
      bool found=false;
      for (int i=0; i<ST; i++)
      { if (symtname[i]==fullname) found=true;
      }
      if (found) A=field;
      else PC=ewbIntValue(codearg[PC])-1;
    } else if (OP==OP_JMP)
    { PC=ewbIntValue(codearg[PC])-1;
    } else if (OP==OP_CALL)
    { POP(x);
      int arity=ewbIntValue(x);
      int return_position=SP-arity-1;
      if (arity<0 || return_position<0) raiseerr("Bad CALL frame");
      stack[return_position]=ewbInt(PC+1);
      PC=ewbIntValue(codearg[PC])-1;
    } else if (OP==OP_RET)
    { POP(x);
      int destination=ewbIntValue(x);
      if (error_handler_running && destination==error_handler_return)
      { error_handler_running=false;
        error_handler_return=-1;
      }
      PC=destination-1;
    } else if (OP==OP_MOVA)                   //Unica istruzione con parametri.. Mi adeguo.. prendo il dato dallo stack.  
    { A=codearg[PC];                  //Metto il valore in accumulatore.
    } else if (OP==OP_PUSHA)
    { PUSH(A); 
    } else if (OP==OP_PUSH) //Push diretto evita di fare MOVA val, PUSH val e lascia intatti i registri.
    { PUSH(codearg[PC]); 
    } else if (OP==OP_POPA)
    { POP(A);  
    } else if (OP==OP_ADDSYMTABLE) //Aggiunge la variabile alla symbol table. Va seguita da un push del valore. 
    { if (ST>=MAXVAR) raiseerr("Vars overflow");
      symtname[ST]=codearg[PC];
      symtpos[ST]=SP; 
      ST++;
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
      if (SP<0) raiseerr("Stack underflow");
      if (SP>MAXSTACK) raiseerr("Stack overflow");
    } else if (OP==OP_STARTFORM)
    { IDF++;
      A="<form method=post id=form" + to_string(IDF) + " enctype=multipart/form-data>";
    } else if (OP==OP_STARTTARGET)
    { IDF++;
      A="<form style=visibility:hidden id=form" + to_string(IDF) + " method=post enctype=multipart/form-data>";
    } else if (OP==OP_ADDFORM)
    { p_addform(&A);
    } else if (OP==OP_ENDFORM)
    { string form;
      int continuation=ewbIntValue(codearg[PC]);
      POP(form);
      string stacktext=escapeStack();
      int saved_stack_position=SP;
      PUSH(form);
      A="<textarea name=__stack id=__stack>" + stacktext + "</textarea>\n" +
        "<input name=__entrypoint id=__entrypoint value=\"" + ewbInt(continuation) + "\">\n" +
        "<input name=__stackpos id=__stackpos value=\"" + ewbInt(saved_stack_position) + "\">\n" +
        "<input name=__signature id=__signature value=\"" + signature(ewbInt(saved_stack_position) + " " + ewbInt(continuation) + " " + stacktext) + "\">\n</form>\n";
    } else if (OP==OP_DBLOCK)
    { p_lock(&A);
    } else if (OP==OP_DBUNLOCK)
    { p_unlock(&A);
    } else if (OP==OP_DBROLLBACK)
    { db_rollback_all();
      A="1";
    } else if (OP==OP_PRINT)
    { p_print(&A);
    } else if (OP==OP_EPRINT)
    { p_eprint(&A);
    } else if (OP==OP_INPUT)
    { p_input(&A);
    } else if (OP==OP_SHOW)
    { p_show(&A);
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
    } else if (OP==OP_ASC)
    { p_asc(&A);
    } else if (OP==OP_CHAR)
    { p_char(&A);
    } else if (OP==OP_MID)
    { p_mid(&A);
    } else if (OP==OP_LEN)
    { p_len(&A);
    } else if (OP==OP_UC)
    { p_uc(&A);
    } else if (OP==OP_INDEX)
    { p_index(&A);
    } else if (OP==OP_ARRAYPOP)
    { p_arraypop(&A);
    } else if (OP==OP_NUMEL)
    { p_numel(&A);
    } else if (OP==OP_NUMKEY)
    { POP(x);
      A=ewbInt(ewbNumKey(x));
    } else if (OP==OP_KEYAT)
    { POP(y);
      POP(x);
      A=ewbKeyAt(x,ewbIntValue(y));
    } else if (OP==OP_GETELEM)
    { POP(y);
      POP(x);
      A=ewbGetElement(x,y);
    } else if (OP==OP_HASKEY)
    { POP(y);
      POP(x);
      A=ewbHasKey(x,y);
    } else if (OP==OP_DELKEY)
    { POP(y);
      POP(varname);
      int position=findvar(varname);
      if (position<0) raiseerr("Unknown array: "+varname);
      A=ewbDeleteKey(&stack[position],y);
    } else if (OP==OP_SPLIT)
    { POP(y);
      POP(x);
      A=ewbSplit(x,y);
    } else if (OP==OP_JOIN)
    { POP(y);
      POP(x);
      A=ewbJoin(x,y);
    } else if (OP==OP_ARRAYPUSH)
    { POP(x);
      POP(varname);
      int position=findvar(varname);
      if (position<0) raiseerr("Unknown array: "+varname);
      A=ewbArrayPush(&stack[position],x);
    } else if (OP==OP_MD5)
    { POP(x);
      A=ewbMd5(x);
    } else if (OP==OP_SHA256)
    { POP(x);
      A=ewbSha256(x);
    } else if (OP==OP_TIME)
    { p_time(&A);
    } else if (OP==OP_DATE)
    { p_date(&A);
    } else if (OP==OP_RANDOM)
    { p_random(&A);
    } else if (OP==OP_SLEEP)
    { p_sleep(&A);
    } else if (OP==OP_EXEC)
    { p_exec(&A);
    } else if (OP==OP_SOCKET)
    { p_socket_open(&A);
    } else if (OP==OP_SERVER)
    { p_server_open(&A);
    } else if (OP==OP_ACCEPT)
    { p_socket_accept(&A);
    } else if (OP==OP_SREAD)
    { p_socket_read(&A);
    } else if (OP==OP_SWRITE)
    { p_socket_write(&A);
    } else if (OP==OP_SCLOSE)
    { p_socket_close(&A);
    } else if (OP==OP_ASSERT)
    { string clause, group;
      POP(clause);
      POP(group);
      if (!prologValidRule(clause)) raiseerr("Invalid rule");
      int url_position=findvar("ewb._url");
      int user_position=findvar("ewb._user");
      int password_position=findvar("ewb._password");
      if (url_position<0 || user_position<0 || password_position<0)
        raiseerr("Missing ewb dataset");
      db_rule_insert(stack[url_position],stack[user_position],
                     stack[password_position],group,clause);
      A="1";
    } else if (OP==OP_RETRACT)
    { string head, group;
      POP(head);
      POP(group);
      int url_position=findvar("ewb._url");
      int user_position=findvar("ewb._user");
      int password_position=findvar("ewb._password");
      if (url_position<0 || user_position<0 || password_position<0)
        raiseerr("Missing ewb dataset");
      string url=stack[url_position];
      string user=stack[user_position];
      string password=stack[password_position];
      if (head=="")
      { A=ewbInt(db_rule_delete_group(url,user,password,group));
      }
      else
      { vector<EwbRuleRow> rules=db_rule_list(url,user,password,group);
        vector<string> ids;
        for (size_t i=0; i<rules.size(); i++)
        { if (prologHeadMatches(head,rules[i].clause)) ids.push_back(rules[i].id);
        }
        A=ewbInt(db_rule_delete(url,user,password,ids));
      }
    } else if (OP==OP_GOAL)
    { string objective, group;
      POP(objective);
      POP(group);
      int url_position=findvar("ewb._url");
      int user_position=findvar("ewb._user");
      int password_position=findvar("ewb._password");
      if (url_position<0 || user_position<0 || password_position<0)
        raiseerr("Missing ewb dataset");
      vector<EwbRuleRow> stored=db_rule_list(
        stack[url_position],stack[user_position],stack[password_position],group);
      vector<string> clauses;
      for (size_t i=0; i<stored.size(); i++) clauses.push_back(stored[i].clause);
      vector<map<string,string> > solutions;
      try
      { solutions=prologSolve(objective,clauses);
      }
      catch (const exception &)
      { raiseerr("Invalid goal or rule set");
      }
      A="";
      for (size_t i=0; i<solutions.size(); i++)
      { string bindings="";
        for (map<string,string>::const_iterator item=solutions[i].begin();
             item!=solutions[i].end(); ++item)
        { vector<string> path(1,item->first);
          bindings=ewbSetPath(bindings,item->second,path);
        }
        vector<string> path(1,to_string(i));
        A=ewbSetPath(A,bindings,path);
      }
    } else if (OP==OP_STOP)
    { if (db_has_transactions())
      { db_rollback_all();
        raiseerr("STOP with open transaction");
      }
      stop_active_target();
      p_close_resources();
      db_close_all();
      RUNNING=0;
    } else if (OP==OP_RUNTARGET)
    { string arity_text, target_name;
      POP(arity_text);
      int arity=ewbIntValue(arity_text);
      if (arity<0 || arity>SP-1) raiseerr("Bad target arity");
      vector<string> parameters((size_t)arity);
      for (int i=arity-1; i>=0; i--) POP(parameters[(size_t)i]);
      POP(target_name);
      for (size_t i=0; i<parameters.size(); i++)
      { if (p_live_resource(parameters[i]))
          raiseerr("Live resource cannot be serialized");
      }
      string stacktext=escapeStack();
      if (stacktext!="") stacktext+=" ";
      stacktext+=hexEncode(ewbInt(codeLines));
      for (size_t i=0; i<parameters.size(); i++)
        stacktext+=" "+hexEncode(parameters[i]);
      int target_stack_position=SP+1+arity;
      int entrypoint=ewbIntValue(codearg[PC]);
      string signed_state=ewbInt(target_stack_position)+" "+
                          ewbInt(entrypoint)+" "+stacktext;

      IDF++;
      string form_id="form"+to_string(IDF);
      cout << "<form style=\"display:none\" id=\"" << form_id
           << "\" method=\"post\" enctype=\"multipart/form-data\">"
           << "<textarea name=__stack id=__stack>" << stacktext << "</textarea>\n"
           << "<input name=__entrypoint id=__entrypoint value=\"" << entrypoint << "\">\n"
           << "<input name=__stackpos id=__stackpos value=\"" << target_stack_position << "\">\n"
           << "<input name=__signature id=__signature value=\"" << signature(signed_state) << "\">\n"
           << "</form>\n";
      cout << "<script>(async function(){const f=document.getElementById('"
           << form_id << "');const r=await fetch(f.action||location.href,"
           << "{method:'POST',body:new FormData(f)});const t=await r.text();"
           << "const d=new DOMParser().parseFromString(t,'text/html');"
           << "const n=d.getElementById('_" << p_html(target_name)
           << "'),c=document.getElementById('_" << p_html(target_name)
           << "');if(r.ok&&n&&c)c.replaceChildren(...n.childNodes);})();</script>\n";
      A="";
    } else if (OP==OP_REFRESHTARGET)
    { string interval, arity_text, target_name;
      POP(interval);
      int milliseconds=ewbIntValue(interval);
      if (milliseconds<=0) raiseerr("Refresh interval must be positive");
      POP(arity_text);
      int arity=ewbIntValue(arity_text);
      if (arity<0 || arity>SP-1) raiseerr("Bad target arity");
      vector<string> parameters((size_t)arity);
      for (int i=arity-1; i>=0; i--) POP(parameters[(size_t)i]);
      POP(target_name);
      for (size_t i=0; i<parameters.size(); i++)
      { if (p_live_resource(parameters[i]))
          raiseerr("Live resource cannot be serialized");
      }
      string stacktext=escapeStack();
      if (stacktext!="") stacktext+=" ";
      stacktext+=hexEncode(ewbInt(codeLines));
      for (size_t i=0; i<parameters.size(); i++)
        stacktext+=" "+hexEncode(parameters[i]);
      int target_stack_position=SP+1+arity;
      int entrypoint=ewbIntValue(codearg[PC]);
      string signed_state=ewbInt(target_stack_position)+" "+
                          ewbInt(entrypoint)+" "+stacktext;

      IDF++;
      string form_id="form"+to_string(IDF);
      cout << "<form style=\"display:none\" id=\"" << form_id
           << "\" method=\"post\" enctype=\"multipart/form-data\">"
           << "<textarea name=__stack id=__stack>" << stacktext << "</textarea>\n"
           << "<input name=__entrypoint id=__entrypoint value=\"" << entrypoint << "\">\n"
           << "<input name=__stackpos id=__stackpos value=\"" << target_stack_position << "\">\n"
           << "<input name=__signature id=__signature value=\"" << signature(signed_state) << "\">\n"
           << "</form>\n";
      cout << "<script>(function(){let f=document.getElementById('"
           << form_id << "');let busy=false;async function tick(){if(busy)return;"
           << "busy=true;try{const r=await fetch(f.action||location.href,"
           << "{method:'POST',body:new FormData(f)});const t=await r.text();"
           << "const d=new DOMParser().parseFromString(t,'text/html');"
           << "const n=d.getElementById('_" << p_html(target_name)
           << "'),c=document.getElementById('_" << p_html(target_name)
           << "'),nf=d.getElementById('" << form_id << "');"
           << "if(r.ok&&n&&c){c.replaceChildren(...n.childNodes);"
           << "if(nf){f.replaceWith(nf);f=nf;}}}finally{busy=false;}}tick();"
           << "const ms=" << milliseconds
           << ";setInterval(tick,ms);})();</script>\n";
      A="";
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
      string guard=OP==OP_TASK ? " and status<>'running'" : "";
      x=run_query(url,user,password,
        "update _" + tasktype + "s set status='running', starttime=" + to_string(time(NULL)) + " where name='" + varname + "'" + guard, "");
      if  (ewbIntValue(x)==0) 
      { x=run_query(url,user,password,
        "insert into _" + tasktype + "s (name, status, starttime) values ('" + varname + "', 'running', " + to_string(time(NULL)) + ")", "");
      };
      if  (ewbIntValue(x)==0 && OP==OP_TASK) RUNNING=0; //niente sovrapposizione per i task
      if (OP==OP_TARGET) active_target_name=varname;
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
      if (OP==OP_ENDTARGET) active_target_name="";
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
          if (field.empty() || field[0]=='_') continue;
          fields.push_back(field);
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
          if (field.empty() || field[0]=='_') continue;
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
          if (field.empty() || field[0]=='_') continue;
          fields.push_back(field);
        }
      }
      A=run_query(url,user,password,context,fields,query,orderby);
    } 
    else if (OP==OP_ONERROR)
    { POP(varname);
      int write=0;
      for (int read=0; read<ST; read++)
      { if (symtname[read]!="__on_error")
        { if (write!=read)
          { symtname[write]=symtname[read];
            symtpos[write]=symtpos[read];
          }
          write++;
        }
      }
      ST=write;
      if (varname!="")
      { if (ST>=MAXVAR) raiseerr("Vars overflow");
        symtname[ST]="__on_error";
        symtpos[ST]=ewbIntValue(varname);
        ST++;
      }
    } else if (OP==OP_RAISE) //c'e' l'errore. 
    { POP(x);
      raiseerr(x);
    } else if (OP==OP_SETPATH) //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; PUSH "pippo"; SETPATH 2; */
      int numlev=ewbIntValue(codearg[PC]);
      POP(x); //Value
      //pop array di livelli
      vector <string> v;
      for (int i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      reverse(v.begin(),v.end());
      POP(varname);
      int position=findvar(varname);
      if (position<0) raiseerr("Unknown variable: "+varname);
      A=stack[position];
      stack[position]=setpath(A,x,v);
      A=x;
    } else if (OP==OP_GETPATH) //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; PUSH 2; GETPATH */
      POP(x);
      int numlev=ewbIntValue(x);
      vector <string> v;
      for (int i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      reverse(v.begin(),v.end());
      POP(varname);
      int position=findvar(varname);
      if (position<0) raiseerr("Unknown variable: "+varname);
      A=getpath(stack[position],v);
    } else if (OP==OP_SQLQUOTE)
    { POP(x);
      A=db_quote(x);
    } else if (OP==OP_SQLNUMBER)
    { POP(x);
      A=ewbNumber(ewbValue(x));
    } else raiseerr("Opcode not implemented");
    }
    catch (const string &e)
    { db_rollback_all();
      int handler=errorHandler();
      if (error_handler_running || handler<0)
      { stop_active_target();
        err(e);
        continue;
      }
      error_handler_running=true;
      error_handler_return=PC+1;
      PUSH(ewbInt(error_handler_return));
      PUSH(e);
      PC=handler;
      continue;
    }
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
{ RUNNING=1;
  active_target_name="";
  error_handler_running=false;
  error_handler_return=-1;
  if (!program) err("No program");
  if (len>0 && ((const unsigned char*)program)[0]==0)
  { loadProgramBinary((const unsigned char*)program, len);
  } else loadProgramText(program);
}

static void loadStack(const char *encoded_stack, int stackpos)
{ p_close_resources();
  SP=0;
  ST=0;
  string A;
  string encoded=encoded_stack ? encoded_stack : "";
  if (encoded.size()>0 && encoded[0]=='@')
  { size_t separator=encoded.find('|');
    if (separator==string::npos) raiseerr("Serialized symbol table");
    string symbols=encoded.substr(1,separator-1);
    vector<string> entries=split(symbols,',');
    for (size_t i=0; i<entries.size(); i++)
    { if (entries[i]=="") continue;
      size_t colon=entries[i].find(':');
      if (colon==string::npos || ST>=MAXVAR) raiseerr("Serialized symbol");
      symtname[ST]=hexDecode(entries[i].substr(0,colon));
      symtpos[ST]=ewbIntValue(entries[i].substr(colon+1));
      ST++;
    }
    encoded=encoded.substr(separator+1);
  }
  vector<string> v=split(encoded, ' ');
  for (int i=0; i<(int)v.size(); i++) { A=hexDecode(v[i]); PUSH(A); };
  if (stackpos==0)
  { stack[0]="<!doctype html><html><body><div id=_main><></div></body></html>";
    SP=1;
    symtname[0]="_template";
    symtpos[0]=0;
    ST=1;
  }
  else
  { SP=stackpos;
    if (ST==0)
    { symtname[0]="_template";
      symtpos[0]=0;
      ST=1;
    }
  }

  for (size_t i=0; i<pending_form_values.size(); i++)
  { bool handled=false;
    int count=0;
    for (size_t j=0; j<i; j++)
      if (pending_form_values[j].name==pending_form_values[i].name) handled=true;
    if (handled) continue;
    for (size_t j=i; j<pending_form_values.size(); j++)
      if (pending_form_values[j].name==pending_form_values[i].name) count++;
    int position=findvar(pending_form_values[i].name);
    if (position<0) continue;
    if (count==1) stack[position]=pending_form_values[i].value;
    else
    { stack[position]="";
      for (size_t j=i; j<pending_form_values.size(); j++)
      { if (pending_form_values[j].name==pending_form_values[i].name)
          ewbArrayPush(&stack[position],pending_form_values[j].value);
      }
    }
  }
  for (size_t i=0; i<pending_form_files.size(); i++)
  { ifstream input(pending_form_files[i].path.c_str(),ios::binary);
    string content((istreambuf_iterator<char>(input)),istreambuf_iterator<char>());
    int position=findvar(pending_form_files[i].name);
    if (position>=0) stack[position]=content;
    position=findvar("_"+pending_form_files[i].name);
    if (position>=0)
    { vector<string> path(1,"filename");
      stack[position]=ewbSetPath(stack[position],pending_form_files[i].filename,path);
      path[0]="content_type";
      stack[position]=ewbSetPath(stack[position],pending_form_files[i].content_type,path);
      path[0]="size";
      stack[position]=ewbSetPath(stack[position],to_string(pending_form_files[i].size),path);
    }
  }
  pending_form_values.clear();
  pending_form_files.clear();
}

extern "C" void ewb_form_clear(void)
{ pending_form_values.clear();
  pending_form_files.clear();
}

extern "C" void ewb_form_add_field(const char *name, const char *value, size_t size)
{ PendingFormValue item;
  if (name) item.name=name;
  if (value) item.value.assign(value,size);
  pending_form_values.push_back(item);
}

extern "C" void ewb_form_add_file(const char *name, const char *filename,
                                  const char *content_type, const char *path,
                                  size_t size)
{ PendingFormFile item;
  if (name) item.name=name;
  if (filename) item.filename=filename;
  if (content_type) item.content_type=content_type;
  if (path) item.path=path;
  item.size=size;
  pending_form_files.push_back(item);
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
