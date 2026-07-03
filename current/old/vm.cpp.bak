/* VM EWB 2026 */

#include <string>
#include <time.h>
#define MAXSTACK 10000
#define MAXVAR 2000

string stk[MAXVAR];
string symtname[MAXVAR];      //Symbol table.. demandata alla VM ???
int     symtpos[MAXVAR];
int ST=0;                     //Symbol table pointer.
int IDF=0;
string code[MAXLINES];
void resume(int PC,int SP, string stack[MAXSTACK])
{ string A='';   //Accumulatore 
  string IR;     //Istruction register
  for (;;)
  { cin>>IR;  //Comando, parametri.
    stringstream ss(IR);
    string I=ss>>item; 
    if (i=='SUM')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbSum(x,y);
    } else if (i=='CONCAT')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a = x + y; 
    } else if (i=='SUB')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbSum(x,ewbNegative(y));
    } else if (i=='MUL')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--;if (SP<0) err("Stack underflow");
      a=ewbMul(x,y);
    } else if (i=='DIV')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbDiv(x,y);
    } else if (i=='OR')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbOr(x,y);
    } else if (i=='AND')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbAnd(x,y);
    } else if (i=='ANDB')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbBitwiseAnd(x,y);
    } else if (i=='ORB')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      string y=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbBitwiseOr(x,y);
    } else if (i=='NOT')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbNegative(x);
    } else if (i=='NOTB')
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      a=ewbBitwiseNot(x);
    } else if ((i==">" || i=="<" || i=="=" || i=="!=" || i==">=" || i=="<=")
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      A=ewbCompare(, i); 
    } else if (i=="JZ")
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      if (A=="0" || A=="") PC=ewbIntValue(x); 
    } else if (i=="JNZ")
    { string x=stack[SP - 1]; SP--; if (SP<0) err("Stack underflow");
      if (A!="0" && A!="") PC=ewbIntValue(x); 
    } else if (i=="CALL")
    { stack[SP]=PC; SP++;         //Push di PC in stack. 
      if (SP>=MAXSTACK) err("Stack overflow");
      PC=A;                       //E sono saltato..  
    } else if (i=="RET")
    { PC=[SP - 1]; SP--; if (SP<0) err("Stack underflow");
    } else if (i=="SETA") 
    { A=ss>>item;                //Metto il valore in accumulatore. 
    } else if (i==PUSH)
    { stack[SP]=A; SP++;         //Push di A in stack. 
      if (SP>=MAXSTACK) err("Stack overflow");
    } else if (i==POP)
    { A=stack[SP - 1]; SP--;     //POP di A dallo stack.
      if (SP<0) err("Stack underflow");  
    } else if (i=="addsymtable") //Aggiunge la variabile alla smbol table. Va seguita da un push del valore. 
    { symtname[ST]=A; 
      symtpos[ST]=SP; ST++; 
      if (ST>=MAXVAR) err("Vars overflow");
    } else if (i=="startform")
    { echo "<form method=post id=__form" + IDF + " enctypemultipart/form-data>";
      IDF++;  
    } else if (i=="starttarget")
    { echo "<form style=visibility:hidden id=__form" + IDF + " method=post enctypemultipart/form-data>"; 
      IDF++; 
    } else if (i=="addtoform")
    { string varname=stack[SP - 1]; SP--;     //nome della variabile.
      if (SP<0) err("Stack underflow");
      string vartype=stack[SP - 1]; SP--;     //tipo del campo da chiedere.
      if (SP<0) err("Stack underflow");
      string varvalue=stack[findvar(varname)]; 
      if (vartype=='input' || vartype='date' || vartype='numeric')
      { cout << "<input type=" + vartype + " name=" + varname + " id=" + varname + " value=\"" + escapeVirg(varvalue) + "\" />\n"; 
      } else if (vartype=="textarea")
      { cout << "<textarea  name=" + varname + " id=" + varname + "/>" + escapeTag(varvalue) + "</textarea>";  
      } else  //Gli altri tipi.. ancora da fare.
      {
      };
    } else if (i=="sendForm")
    { //Aggiungo al form lo stack 
      cout << "<textarea name=__stack   id=__stack     >" + escapeStack() + "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint>" + (PC + 1) + "</textarea>\n"; 
      cout << "<input name=__stackpos   id=__stackpos  >" + SP + "</textarea>\n"; 
      cout << "<input name=__signature   id=__signature  >" + signature(SP + " " + (PC+1) + " " + escapeStack()) + "</textarea>\n"; 
      cout << "</form>\n";
    } else if (i=="stop")
    { exit(); //In attesa di rientrare dall'entry point PC 
    } else if (i=="runTarget") //run TARGET - come STOP - Mi faccio passare l'entry point
    { string varname=stack[SP - 1]; SP--;     //nome del target.
      if (SP<0) err("Stack underflow");
      string interval=atack[SP - 1]; SP--;     //intervallo del target.
      if (SP<0) err("Stack underflow");
      string EP=stack[findvar(varname)]; //posizione del target. 
      cout << "<textarea name=__stack   id=__stack     >" + escapeStack() + "</textarea>\n"; 
      cout << "<input name=__entrypoint id=__entrypoint>" + EP + "</textarea>\n"; 
      cout << "<input name=__stackpos   id=__stackpos  >" + SP + "</textarea>\n"; 
      cout << "<input name=__signature   id=__signature  >" + signature(SP + " " + EP + " " + escapeStack()) + "</textarea>\n"; 
      cout << "</form>\n";
      cout << "<script>runtarget(" + (IDF-1) + "," + interval + ");</script>\n";  
    } else if (i=="crontask") //cron task 
    { string varname=stack[SP - 1]; SP--;   //nome del task.
      if (SP<0) err("Stack underflow");
      string interval=stack[SP - 1]; SP--;  //intervallo del task.
      if (SP<0) err("Stack underflow");
      string EP=stack[findvar(varname)];    //posizione del task. 
      addCron(EP, interval);                //Funzione interna, devo gestire un CRON a minuti o a secondi. 
    } else if (i=='task' || i=='target')    //Punto di inizio di una procedura asincrona.
    { string tname=stack[SP - 1]; SP--;   //nome del task.
      if (SP<0) err("Stack underflow");
      //Se gia running impedisco la sovrapposizione
      string stat=q("select status from _" + i + "s where name="'" + tname + "');
      if (stat=='running') exit(); 
      q("update _" + i + "s set status='running', starttime=".time(NULL)." where name='" + tname + "'");
    } else if (i=='endtask' || i=='endtarget')    //Punto di inizio di una procedura asincrona.
    { string tname=stack[SP - 1]; SP--;   //nome del task.
      if (SP<0) err("Stack underflow");
      q("update _" + i + "s set status='stopped', starttime=".time(NULL)." where name='" + tname + "'");
    } else if (i=='onerror') //Visibilità, sempre l'ultima in ricorsione.  
    { string fname=stack[SP - 1]; SP--;   //azione in caso di errore o ''. Alla fine di un blocco onerror c'e' sempre STOP.
      if (SP<0) err("Stack underflow");
      symtname[ST]="__on_error"; 
      symtpos[ST]=SP; ST++; 
      if (ST>=MAXVAR) err("Vars overflow");
    } else if (i=="raise") //c'e' l'errore. 
    { //Non serve un pop, l'errore lo devo passare come parametro all'entry point.. 
      
      var onerr=findvar("_on_error");
      if (onerr>-1) 
      { PC=onerr; 
      } else //Gestione interna: segnalo l'errore e mi fermo.  
      { var errore=stack[SP - 1]; if (SP<0) err("Stack underflow"); 
        err(errore);
      };    
    } else if (i=='in') //Accesso alla base di dati. SQL ha un suo parser che converte il codice EWB in una condizione sql valida o si pianta prima..
    { var context=stack[SP - 1]; SP--;       if (SP<0) err("Stack underflow");
      var condizione=stack[SP - 1]; SP--;  if (SP<0) err("Stack underflow");
      var order=''; 
      var orderpos=findvar("_order_db");
      if (orderpos>-1) order="order by " + SP[orderpos];  
      string q="select * on " + context + " where " + condizione " + order;
      //salvo in stack PC attuale
      stack[SP]=PC; SP++; if (SP>=MAXSTACK) err("Stack overflow");

      //riporto qui il ciclo ma sarà gestito dal compilatore, come un for o un while.. qui solo jmp. 
      //fetch di una riga
      //se non disponibile jz *fineciclo** 
      //  codice interno alla in
      //  prendo PC-1 da stack
      //  salto a PC-1
      //**fineciclo**
    }
    PC++;
  };
};


void start() 
{ symtpos[ST]=SP; ST++;
  resume (0,0,stck);
};

//Main, da estendere, questo dovrà ricevere il GET e il POST. 
main(int argc, char** argv)
{ PC=0; 
  while(cin.hasNext())
  { cin>>IR;  //Comando, parametri.
    stck[PC]=IR; 
  };
  start(); 
};

//Ho tolto il main, implemento un web server in modo da fare caching sia sui sorgenti ewb e da tenere "N to M" processi in parallelo.
