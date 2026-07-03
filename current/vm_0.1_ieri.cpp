/* VM EWB 2026 */

#include <string>
#include <time.h>
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

#include <vm_parts.h>

//Queste rimangono qui: fanno parte della logica di linguaggio.
string escapeStack()
{ int i;
  string r;  
  for (i=0; i<SP; i++)
  { r=r + hexEncode(stack[SP]);
    if (i<SP-1) r = r + ' ';
  };
  return r; 
};
void addCron(EP, interval)
{  //Devo definire una crontab e avere un servizio che a intervalli chiama in ajax i punti di entrata di questa crontab. 

// [...]

}; //Parte della VM. Se non va bene, eccezione.

//Gestione interna errore. Al momento lo sputo sullo STDERR. 
void err(string e)
{ cerr << "Vm error at:  " << PC << "\nCode:\n";
  if (PC>0) cerr << (PC-1) << ":  " << code[PC-1] << "\n"; 
  cerr << (PC) << ":  " << code[PC] << "\n";
  cerr << (PC+1) << ":  " << code[PC+1] << "\n";
  cerr <<"Stack: \n"; 
  for (i=1; i<6; i++) cerr << (SP-i) << ":  [" << stack[SP-i] << "]\n";   
  exit(-1);
};
//Dato il nome di una variabile restituisce la posizione in stack del dato.
int findvar(string varname) 
{ int i; for (i=ST; i>0; i--) { if (varname==symtname[i]) return symtpos[i];}; 
  return -1; //Non trovata 
};
//Questa fa comodo che diventi una funzione.. dovessimo segnalare qualcosa da sistema.. 
void raise()
{ int x=findvar("_on_error");
  if (x>-1) 
  { PC=x; //Vado all'indirizzo che c'e' dentro X. Quello dell'error handler. 
  } else //Gestione interna: segnalo l'errore e mi fermo.  
  { POP(x); err(x);
  };
};


string setpath(string s, string value, vector<string> key)
{ if (key.size() == 0) return value;
  vector<string> v = split(s, ' ');
  string k = key[0];
  // cerca valore attuale associato a k
  string old = ""; // default se non esiste
  if (key.size() == 1)  
  {old = value;
  }
  else 
  { vector<string> keyb(key.begin() + 1, key.end());
    old = setpath(old, value, keyb);
  }
  v.push_back(hexEncode(k) + ":" + hexEncode(old));
  return join(v, " ");
}
string getpath(string s, vector<string> key)
{ if (key.size() == 0) return s;
  vector<string> v = split(s, ' ');
  string k = key[0];
  string found = "";
  // cerca hex(k):qualcosa
  for (int i=0; i<v.size(); i++)
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

void resume(int xPC,int xSP, string code)
{ PC=xPC; SP=xSP;//Program counter e stack pointer
  string A='';   //Accumulatore 
  string x,y;    //x e y diventano registri, come sul 6502.. 
  string IR;     //Istruction register
  string varname, vartype, varvalue; //Uso anche questi troppo spesso per non metterli qui. 
  for (;;)
  { cin>>IR;  //Comando, parametri.
    if (IR=='SUM')
    { POP(y); POP (x); A=ewbSum(x,y);
    } else if (IR=='CONCAT')
    { POP(y); POP(x); A = x + y; 
    } else if (IR=='SUB')
    { POP(y); POP(x); A=ewbSum(x,ewbNegative(y));
    } else if (IR=='MUL')
    { POP(y); POP(x); A=ewbMul(x,y);
    } else if (IR=='DIV')
    { POP(y); POP(x); A=ewbDiv(x,y);
    } else if (IR=='OR')
    { POP(y); POP(x); A=ewbOr(x,y);
    } else if (IR=='AND')
    { POP(y); POP(x); A=ewbAnd(x,y);
    } else if (IR=='ANDB')
    { POP(y); POP(x); A=ewbBitwiseAnd(x,y);
    } else if (IR=='ORB')
    { POP(y); POP(x); A=ewbBitwiseOr(x,y);
    } else if (IR=='NOT')
    { POP(x); A=ewbNot(x);
    } else if (IR=='NOTB')
    { POP(x); A=ewbBitwiseNot(x);
    } else if ((IR=="GT" || IR=="LT" || IR=="EQ" || IR=="NEQ" || IR=="GE" || IR=="LE"))
    { POP(y); POP(x); A=ewbCompare(x, y, i); 
    } else if (IR=="JZ")
    { POP(x); if (A=="0" || A=="") PC=ewbIntValue(x); 
    } else if (IR=="JNZ")
    { POP(x); if (A!="0" && A!="") PC=ewbIntValue(x); 
    } else if (IR=="CALL")
    { PUSH(PC); PC=A;                       //E sono saltato..  
    } else if (IR=="RET")
    { POP(PC);                              //E  sono tornato..
    } else if (IR.substring(0,5)=="MOVA ")                   //Unica istruzione con parametri.. Mi adeguo.. prendo il dato dallo stack.  
    { A=togliVirgolette(IR.substring(5));                  //Metto il valore in accumulatore.
    } else if (IR=="PUSHA")
    { PUSH(A); 
    } else if (IR.substr(0,5)=="PUSH ") //Push diretto evita di fare MOVA val, PUSH val e lascia intatti i registri.
    { string t=togliVirgolette(IR.substring(2));
      PUSH(t); 
    } else if (IR=="POPA")
    { POP(A);  
    } else if (IR=="addsymtable") //Aggiunge la variabile alla smbol table. Va seguita da un push del valore. 
    { symtname[ST]=A; 
      symtpos[ST]=SP; 
      ST++; if (ST>=MAXVAR) err("Vars overflow");
    } else if (IR=="startform")
    { echo "<form method=post id=__form" + IDF + " enctype=multipart/form-data>";
      IDF++;  
    } else if (IR=="starttarget")
    { echo "<form style=visibility:hidden id=__form" + IDF + " method=post enctypemultipart/form-data>"; 
      IDF++; 
    } else if (IR=="addtoform")
    { POP(varvalue); POP(vartype); POP(varname);
      if (vartype=="input" || vartype=="date" || vartype=="numeric")
      { if (vartype=="numeric") vartype="number"; 
        cout << "<input type=" << vartype << " name=" << varname << " id=" << varname << " value=" << mettiVirgolette(varvalue) << " />\n"; 
      } else if (vartype=="textarea")
      { cout << "<textarea  name=" << varname << " id=" << varname << "/>" << escapeTag(varvalue) << "</textarea>";  
      } else  //Gli altri tipi.. ancora da fare.
      {
      };
    } else if (IR=="sendForm")
    { //Aggiungo al form lo stack 
      cout << "<textarea name=__stack   id=__stack     >" << escapeStack() << "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint>" << (PC + 1) << "</textarea>\n"; 
      cout << "<input name=__stackpos   id=__stackpos  >" << SP << "</textarea>\n"; 
      cout << "<input name=__signature   id=__signature  >" << signature(SP + " " + (PC+1) + " " + escapeStack()) << "</textarea>\n"; 
      cout << "</form>\n";
    } else if (IR=="stop")
    { exit(0); //In attesa di rientrare dall'entry point PC 
    } else if (IR=="runTarget") //run TARGET - come STOP - Mi faccio passare l'entry point
    { POP(interval); POP(varname); 
      string EP=stack[findvar(varname)]; //posizione del target. 
      cout << "<textarea name=__stack   id=__stack     >" << escapeStack() << "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint>" << EP << "</textarea>\n"; 
      cout << "<input name=__stackpos   id=__stackpos  >" << SP << "</textarea>\n"; 
      cout << "<input name=__signature   id=__signature  >" << signature(SP + " " + EP + " " + escapeStack()) << "</textarea>\n"; 
      cout << "</form>\n";
      cout << "<script>runtarget(" << (IDF-1) << "," << interval << ");</script>\n";  
    } else if (IR=="crontask") //cron task 
    { string EP;
      POP(interval); POP(varname);
      string EP=stack[findvar(varname)];    //posizione del task. 
      addCron(EP, interval);                //Funzione interna, devo gestire un CRON a minuti o a secondi. 
    } else if (IR=="task" || IR=="target")    //Punto di inizio di una procedura asincrona.
    { POP(varname);
      //Se gia running impedisco la sovrapposizione
      string stat=sql_query("select status from _" + i + "s where name="'" + tname + "');
      if (stat=='running') exit(); 
      sql_query("update _" << i << "s set status='running', starttime=" << time(NULL) << " where name='" << varname << "'");
    } else if (IR=="endtask" || IR=="endtarget")    //Punto di inizio di una procedura asincrona.
    { POP(varname);
      sql_query("update _" + i + "s set status='stopped', starttime=".time(NULL)." where name='" + tname + "'");
    } else if (IR=="onerror") //Visibilità, sempre l'ultima in ricorsione.  
    { POP(varname); 
      symtname[ST]="__on_error"; 
      symtpos[ST]=varname; 
      ST++; if (ST>=MAXVAR) err("Vars overflow");
    } else if (IR=="raise") //c'e' l'errore. 
    { raise(SP);    
    } else if (IR=="QOPEN") //Accesso alla base di dati. SQL ha un suo parser che converte il codice EWB in una condizione sql valida o si pianta prima..
    { string context,condizione,order,campi;
      int orderos;  
/* context cond campi QOPEN      prepara/esegue la query
   FETCH                         carica la riga successiva nelle variabili, A=1 se ok, A=0 se fine
   QCLOSE                        libera risultato/cursore
*/
      POP(campi);
      POP(condizione);
      POP(context); 
      if (condizione != "") condizione = " where " + condizione;
      order=''; 
      orderpos=findvar("_order_db"); //Sto usando una variabile, se la ridefinisco torna alla vecchia definizione quando lo SP torna indietro. 
      if (orderpos>-1) order="order by " + SP[orderpos];  
      string q="select " + campi + " from " + context + condizione  + " " + order;
      tipocursore crsr = perform_query(q); 
      PUSH(crsr); 
    } else if (IR=="FETCH") //Accesso alla base di dati. SQL ha un suo parser che converte il codice EWB in una condizione sql valida o si pianta prima..
    { tipocursore crsr; POP(crsr); col=0; 
      tipodati data;
      if (data=sql_fetch(crsr)) 
      { A=1;
        for(i=0; i<data.len; i++)
        { string varname=data[i][0];
          string varvalue=data[i][1];
          int v=findvar(varname);
          if (v>=0) stack[v]=varvalue;
        };    
      } else A=0; 
      PUSH(crsr); 
    } else if (IR=="QCLOSE") //Prendo la prossima colonna
    { POP(crsr); 
      sql_close(crsr); 
    } else if (IR.substr(0,8)=="SETPATH ") //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; PUSH "pippo"; SETPATH 2; */
      int numlev=ewbIntValue(IR.substring(8));
      POP(x); //Value
      //pop array di livelli
      vector <string> v; 
      for (i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      POP(varname);
      A=SP[findvar(varname)];
      SP[findvar(varname)] = setpath(A, x, v);  
    } else if (IR.substr(0,8)=="GETPATH ") //Nasconde nelle stringhe la complessità degli array
    { /* PUSH "a"; PUSH "3"; PUSH "5"; GETPATH 2; */
      int numlev=ewbIntValue(IR.substring(8));
      vector <string> v; 
      for (i=0; i<numlev; i++) { POP(A); v.push_back(A); };
      POP(varname);
      A=getpath(varname, v);     
    };
    PC++; 
  };
};



void start() 
{ resume(0,0);
};

//Main, questa qui non centra niente, se non per fare prove. 
main(int argc, char** argv)
{ PC=0; 
  while(cin.hasNext())
  { cin>>IR;  //Comando, parametri.
    code[PC]=IR; PC++; 
  };
  start(); 
};
