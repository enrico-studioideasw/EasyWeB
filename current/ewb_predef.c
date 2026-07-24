#ifdef EWB_VM_RUNTIME

#include <algorithm>
#include <chrono>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <thread>

/*
 * Primitive richiamate dal dispatch della VM.
 * Questo ramo viene incluso da vm.cpp dopo la definizione dello stato VM:
 * evita di esportare stack, symbol table e registri attraverso un'interfaccia
 * artificiosamente larga.
 */

static bool p_db_context(string context, string *url, string *user,
                         string *password)
{ int urlpos=findvar(context+"._url");
  int userpos=findvar(context+"._user");
  int passwordpos=findvar(context+"._password");
  if (urlpos<0 || userpos<0 || passwordpos<0)
  { raiseerr("Incomplete DB context: "+context);
    return false;
  }
  *url=stack[urlpos];
  *user=stack[userpos];
  *password=stack[passwordpos];
  return true;
}

static void p_lock(string *accumulator)
{ string context,url,user,password;
  POP(context);
  if (!p_db_context(context,&url,&user,&password)) return;
  db_begin_transaction(url,user,password,30);
  *accumulator="1";
}

static void p_unlock(string *accumulator)
{ string context,url,user,password;
  POP(context);
  if (!p_db_context(context,&url,&user,&password)) return;
  db_commit_transaction(url,user,password);
  *accumulator="1";
}

static void p_print(string *accumulator)
{ POP(*accumulator);
  cout << *accumulator;
}

static void p_eprint(string *accumulator)
{ POP(*accumulator);
  cerr << *accumulator;
}

static void p_input(string *accumulator)
{ if (!getline(cin,*accumulator)) *accumulator="";
}

static void p_load(string *accumulator)
{ string filename;
  POP(filename);
  ifstream in(filename.c_str(),ios::in|ios::binary);
  if (!in) { raiseerr("Cannot open file: "+filename); return; }
  ostringstream contents;
  contents << in.rdbuf();
  if (in.bad()) { raiseerr("Cannot read file: "+filename); return; }
  *accumulator=contents.str();
}

static void p_save(string *accumulator)
{ string filename,value;
  POP(value);
  POP(filename);
  ofstream out(filename.c_str(),ios::out|ios::binary|ios::trunc);
  if (!out) { raiseerr("Cannot open file: "+filename); return; }
  out.write(value.data(),(streamsize)value.size());
  if (!out) { raiseerr("Cannot write file: "+filename); return; }
  *accumulator=ewbInt((int)value.size());
}

static void p_loaddir(string *accumulator)
{ string dirname;
  POP(dirname);
  DIR *dir=opendir(dirname.c_str());
  if (!dir) { raiseerr("Cannot open directory: "+dirname); return; }
  vector<string> names;
  for (dirent *entry=readdir(dir); entry; entry=readdir(dir))
  { string name=entry->d_name;
    if (name!="." && name!="..") names.push_back(name);
  }
  closedir(dir);
  sort(names.begin(),names.end());
  string result="";
  for (size_t i=0; i<names.size(); i++)
  { vector<string> key;
    key.push_back(ewbInt((int)i));
    result=setpath(result,names[i],key);
  }
  *accumulator=result;
}

static void p_int(string *accumulator)
{ string value;
  POP(value);
  double number=ewbValue(value);
  if (!isfinite(number) || number>2147483647.0 || number<-2147483648.0)
  { raiseerr("Integer range"); return; }
  *accumulator=ewbInt((int)number); /* conversione C++: troncamento verso zero */
}

static void p_sin(string *accumulator)
{ string degrees;
  POP(degrees);
  const double pi=acos(-1.0);
  *accumulator=ewbNumber(sin(ewbValue(degrees)*pi/180.0));
}

static void p_hex(string *accumulator)
{ string value;
  POP(value);
  int number=ewbIntValue(value);
  ostringstream result;
  if (number<0)
  { result << "-0x" << uppercase << hex << -(long long)number;
  } else result << "0x" << uppercase << hex << number;
  *accumulator=result.str();
}

static void p_sqr(string *accumulator)
{ string value;
  POP(value);
  double number=ewbValue(value);
  if (number<0) { raiseerr("Square root of negative number"); return; }
  *accumulator=ewbNumber(sqrt(number));
}

static void p_time(string *accumulator)
{ *accumulator=to_string((long long)time(NULL));
}

static void p_date(string *accumulator)
{ time_t now=time(NULL);
  struct tm current;
  if (!localtime_r(&now,&current)) { raiseerr("Local date"); return; }
  char result[11];
  if (!strftime(result,sizeof(result),"%Y-%m-%d",&current))
  { raiseerr("Date format"); return; }
  *accumulator=result;
}

static void p_random(string *accumulator)
{ static mt19937 generator((random_device())());
  static uniform_int_distribution<int> values(0,2147483647);
  *accumulator=ewbInt(values(generator));
}

static void p_sleep(string *accumulator)
{ string milliseconds;
  POP(milliseconds);
  int delay=ewbIntValue(milliseconds);
  if (delay<0) { raiseerr("Negative sleep"); return; }
  this_thread::sleep_for(chrono::milliseconds(delay));
  *accumulator="";
}

#else

typedef struct
{ char * name;
  //int arity;
  char sql; 
} pref;

/*
 * Convenzione usata negli schizzi sottostanti:
 * - il compilatore valuta gli argomenti da sinistra a destra e li lascia
 *   sullo stack; A contiene ancora l'ultimo argomento;
 * - il microcodice consuma gli argomenti e lascia il risultato in A;
 * - JZ, JNZ e JMP ricevono direttamente il numero assoluto della riga VM;
 *   le label mostrate qui sono simboli temporanei risolti dal compilatore;
 * - "opcode" indica che espandere la funzione con le primitive attuali
 *   sarebbe più lungo, meno chiaro o richiederebbe stato esterno.
 *
 * Decisioni sul microcodice:
 * - JMP resta anche se simulabile con JNZ: completa il set di controllo e
 *   rende leggibili salti e cicli generati dal compilatore;
 * - una predefinita non implica automaticamente un opcode: se la semantica
 *   si esprime bene componendo primitive esistenti, viene espansa;
 * - lo stato runtime non serializzabile (transazioni, file e socket aperti)
 *   appartiene al singolo flusso di esecuzione.
 * - le connessioni DB aperte vengono riusate nello stesso flusso e chiuse
 *   tutte da STOP; il nome del context e' locale come quello di una variabile,
 *   mentre la risorsa DB e' identificata dalla coppia engine/database.
 */

static void p_add(void)
{ /* Combinazione, nessun opcode ADD dedicato.
     Il compilatore conosce context e relativi campi e costruisce:

       PUSH context
       PUSH "insert into context (...) values (...)"
       QUERY

     QUERY assicura lo schema e restituisce in A l'id inserito.
  */
}

static void p_exists(void)
{ /* Combinazione basata su QUERY:

       PUSH context
       PUSH "select id from context where ..."
       QUERY
       PUSHA
       PUSH ""
       SNEQ                 ; A = 1 se QUERY ha trovato un record

     La costruzione della condizione appartiene al compilatore SQL.
  */
}

static void p_lock(void)
{ /* Opcode necessario: DBLOCK.
     Ingresso: [context].
     Apre BEGIN TRANSACTION sul database del context e aggiunge connessione e
     database alla lista transazionale locale del flusso di esecuzione.
     Sono ammesse transazioni contemporanee su database diversi.
     Due context che risolvono alla stessa coppia engine/database indicano la
     stessa risorsa: un secondo DBLOCK esegue raise, senza transazioni annidate.
     Se il database non esiste viene creato con il nome indicato da EWB.
  */
}

static void p_unlock(void)
{ /* Opcode necessario: DBUNLOCK.
     Ingresso: [context].
     Esegue COMMIT soltanto sul database indicato e lo rimuove dalla lista
     transazionale del flusso. Non e' una transazione distribuita: un database
     gia' confermato non puo' essere annullato da un rollback successivo.
  */
}

static void p_dbrollback(void)
{ /* Opcode interno necessario: DBROLLBACK, senza argomenti.
     Esegue ROLLBACK, in ordine inverso di acquisizione, su tutti i database
     ancora presenti nella lista locale e poi svuota la lista.
     E' innocuo a lista vuota. Errori, timeout e sospensioni non ammesse
     vengono compilati come DBROLLBACK seguito da RAISE.
  */
}

static void p_print(void)
{ /* Opcode necessario: PRINT.
       [value] -> A = value
     Effetto esterno: scrive value su stdout.
  */
}

static void p_eprint(void)
{ /* Opcode necessario: EPRINT.
       [value] -> A = value
     Effetto esterno: scrive value su stderr.
  */
}

static void p_input(void)
{ /* Opcode necessario: INPUT.
       [] -> A = riga letta senza terminatore
     La VM decide la sorgente concreta: stdin, CGI o altro frontend.
     EOF restituisce stringa vuota.
  */
}

static void p_show(void)
{ /* Opcode necessario: SHOW.
       [value] -> A = value
     Riprende la semantica del vecchio Amethyst: inserisce value nel template
     HTML minimo della VM, non scrive semplicemente value come PRINT.

     Il flusso principale usa <div id="main">. Ogni thread aggiunge al template
     un <div> distinto il cui id e' il nome del thread; un CSS minimo gestisce
     la presentazione. "main" e' quindi un nome riservato e il compilatore deve
     impedire che venga dichiarato come nome di un template/thread utente.
  */
}

static void p_ask(void)
{ /* Combinazione delle primitive web esistenti, nessun opcode ASK:

       STARTFORM
       <per ogni field:
         PUSHA
         PUSH nome
         PUSH stile
         PUSH valore
         ADDFORM
         PUSHA
         CONCAT>
       PUSHA
       ENDFORM
       PUSHA
       CONCAT
       PUSHA
       SHOW
       STOP

     Alla ripresa, il normale caricamento dell'ambiente CGI valorizza i campi.
  */
}

static void p_form(void)
{ /* Restituisce in A i soli campi, senza apertura e chiusura del form:

       MOVA ""
       <per ogni field: PUSHA, PUSH nome, PUSH stile, PUSH valore,
                        ADDFORM, PUSHA, CONCAT>
  */
}

static void p_showform(void)
{ /* Avvolge il frammento prodotto da FORM e lo scrive:

       STARTFORM
       PUSHA
       <valuta frammento>
       PUSHA
       CONCAT
       PUSHA
       ENDFORM
       PUSHA
       CONCAT
       PUSHA
       PRINT
       STOP
  */
}

static void p_load(void)
{ /* Opcode necessario: FLOAD.
       [filename] -> A = contenuto completo
     Gli errori di apertura o lettura eseguono raise.
  */
}

static void p_save(void)
{ /* Opcode necessario: FSAVE.
       [filename,value] -> A = numero di byte scritti
     Gli errori di apertura o scrittura eseguono raise.
  */
}

static void p_loaddir(void)
{ /* Opcode necessario: FREADDIR.
       [dirname] -> A = struttura EWB con i nomi delle entry
     Gli indici partono da zero; ordinamento lessicografico deterministico;
     '.' e '..' sono esclusi.
  */
}

static void p_int(void)
{ /* Opcode necessario: TOINT.
       [x] -> A = parte intera di x, troncata verso zero
     Le primitive aritmetiche attuali non espongono il troncamento.
  */
}

static void p_abs(void)
{ /* Combinazione breve, nessun opcode ABS dedicato:

       PUSHA                 ; [x,x]
       PUSH 0                ; [x,x,0]
       GT                    ; [x], A=(x>0)
       JZ negativo_o_zero
       POPA                  ; A=x, consuma l'argomento originale
       JMP fine
     negativo_o_zero:
       POPA                  ; A=x, consuma l'argomento originale
       PUSH 0
       PUSHA                 ; [0,x]
       SUB                   ; A=0-x
     fine:
  */
}

static void p_cos(void)
{ /* Nessun opcode COSD:
       cos(gradi) = sin(90 + gradi)
     Il compilatore espande COS usando SUM e SIND.
  */
}

static void p_sin(void)
{ /* Opcode necessario: SIND.
       [gradi] -> A = sin(gradi * pi / 180)
  */
}

static void p_hex(void)
{ /* Opcode necessario: TOHEX.
       [intero] -> A = rappresentazione esadecimale canonica
     Usa il prefisso 0x e cifre A-F maiuscole; per i negativi il segno precede
     il prefisso, per esempio -0x2A.
  */
}

static void p_sqr(void)
{ /* Opcode necessario: SQRT.
       [x] -> A = radice quadrata di x
     x negativo esegue raise.
  */
}

static void p_asc(void)
{ /* Opcode necessario: ASC.
       [testo] -> A = codice Unicode del primo carattere
     Stringa vuota o UTF-8 non valido eseguono raise.
  */
}

static void p_char(void)
{ /* Opcode necessario: CHAR.
       [codice] -> A = carattere codificato UTF-8
     Un code point Unicode non valido esegue raise.
  */
}

static void p_mid(void)
{ /* Opcode necessario: MID.
       [testo,inizio]           -> A = suffisso
       [testo,inizio,lunghezza] -> A = sottostringa
     Indici e lunghezza contano caratteri UTF-8, non byte.
  */
}

static void p_len(void)
{ /* Opcode necessario: LEN.
       [testo] -> A = numero di caratteri UTF-8
  */
}

static void p_uc(void)
{ /* Opcode necessario: UC.
       [testo] -> A = testo convertito in maiuscolo
     La semantica Unicode/locale deve vivere nel runtime, non nel compilatore.
  */
}

static void p_index(void)
{ /* Opcode necessario: INDEX.
       [testo,sottostringa] -> A = posizione oppure -1
     La posizione conta caratteri UTF-8.
  */
}

static void p_change(void)
{ /* Nessun opcode CHANGE.
       [nuova,vecchia,testo] -> SPLIT(vecchia,testo), poi JOIN(nuova,lista).
  */
}

static void p_split(void)
{ /* Combinazione, nessun opcode SPLIT dedicato.
       [separatore,testo] -> A = struttura EWB indicizzata da 0
     Il compilatore genera un ciclo basato su INDEX, MID e LEN; ogni segmento
     viene aggiunto alla struttura risultante con SETPATH.
     Il caso del separatore vuoto deve essere trattato esplicitamente per
     evitare un ciclo senza avanzamento.
  */
}

static void p_join(void)
{ /* Combinazione, nessun opcode JOIN dedicato.
       [separatore,lista] -> A = elementi concatenati
     Il compilatore usa NUMEL e scorre gli indici con GETPATH, concatenando il
     separatore soltanto fra due elementi mediante CONCAT.
  */
}

static void p_push(void)
{ /* Forma speciale del compilatore; nessun opcode PUSHARRAY dedicato se
     esiste NUMEL. Il primo argomento deve essere una variabile assegnabile:

       PUSH "nome_lista"     ; lvalue conservato per SETPATH
       PUSH "nome_lista"
       PUSH 0
       GETPATH               ; A = valore corrente della lista
       PUSHA
       NUMEL                 ; A = prossimo indice
       PUSHA
       <valuta elemento>
       PUSHA
       SETPATH 1

     Il compilatore deve conservare nome_lista: passare soltanto il suo valore
     non basta per aggiornare la variabile.
  */
}

static void p_pop(void)
{ /* Opcode necessario: ARRAYPOP.
       [lista] -> A = elemento estratto; la variabile riceve la lista accorciata.
     Una ricostruzione con SPLIT, eliminazione e JOIN e' possibile, ma richiede
     comunque microcodice dedicato alla rimozione. ARRAYPOP resta per mantenere
     atomica e leggibile l'operazione sulla rappresentazione EWB.
  */
}

static void p_numel(void)
{ /* Opcode necessario: NUMEL.
       [struttura] -> A = numero di elementi al primo livello
     La conoscenza della codifica hex(key):hex(value) resta confinata alla VM.
  */
}

static void p_time(void)
{ /* Opcode necessario: TIME.
       [] -> A = Unix time corrente in secondi
  */
}

static void p_date(void)
{ /* Opcode necessario: DATE.
       [] -> A = data corrente in formato YYYY-MM-DD
     Il formato e' ordinabile lessicograficamente e facilmente separabile.
     DATE resta una primitiva e non viene ricostruita a partire da TIME.
  */
}

static void p_random(void)
{ /* Opcode necessario: RANDOM.
       [] -> A = intero pseudocasuale fra 0 e 2147483647 inclusi
     Seed e generatore sono stato della VM.
  */
}

static void p_sleep(void)
{ /* Opcode necessario: SLEEP.
       [millisecondi] -> A = ""
     Il runtime può bloccare il processo oggi e in futuro schedulare la
     continuazione, senza cambiare il linguaggio.
  */
}

static void p_socket(void)
{ /* Opcode necessario: SOCKET.
       [host,port] -> A = handle della socket client IPv4
     Errori di risoluzione o connessione eseguono raise.
  */
}

static void p_server(void)
{ /* Opcode necessario: SERVER.
       [port] -> A = handle della socket in ascolto
  */
}

static void p_accept(void)
{ /* Opcode necessario: ACCEPT.
       [server_handle] -> A = handle della socket connessa
  */
}

static void p_sread(void)
{ /* Opcode necessario: SREAD.
       [socket]        -> A = una riga
       [socket,size]   -> A = al massimo size byte
     EOF restituisce stringa vuota; errore esegue raise.
  */
}

static void p_swrite(void)
{ /* Opcode necessario: SWRITE.
       [socket,dati] -> A = numero di byte scritti
     Gestisce internamente le scritture parziali.
  */
}

static void p_read(void)
{ /* Opcode necessario: DREAD.
       [port] -> A = pacchetto disponibile, oppure stringa vuota
     E' la primitiva datagram e resta distinta dalle socket a flusso.
     Attende per un timeout ragionevole fissato dal runtime; alla scadenza
     esegue raise invece di attendere indefinitamente.
  */
}

static void p_write(void)
{ /* Opcode necessario: DWRITE.
       [host,port,dati] -> A = numero di byte inviati
     E' la primitiva datagram e resta distinta dalle socket a flusso.
     Nel documento manca il parametro dati: va reso esplicito nella sintassi.
  */
}

static void p_assert(void)
{ /* Combinazione minima, nessun opcode ASSERT dedicato.
       [dataset,clausola] -> QUERY di inserimento nel dataset delle clausole.
     Per ora la clausola viene conservata senza parsing o normalizzazione
     avanzata. I controlli verranno estesi se l'uso reale lo richiedera'.
  */
}

static void p_retract(void)
{ /* Combinazione minima, nessun opcode RETRACT dedicato.
       [dataset,clausola] -> QUERY che elimina le clausole uguali.
     A riceve il numero di righe rimosse. Per ora non viene svolta
     unificazione o equivalenza logica: il confronto e' sul valore conservato.
  */
}

static void p_goal(void)
{ /* Intenzione futura, non necessaria alla prima implementazione della VM.
     Quando verra' ripreso, sara' un costrutto del compilatore basato su
     primitive dedicate del solver:

       PUSH dataset
       PUSH goal
       GOALOPEN              ; A = handle/iteratore delle soluzioni
     ciclo:
       PUSHA
       GOALNEXT              ; valorizza le variabili, A=1 finché trova
       JZ fine
       <blocco>
       JMP ciclo
     fine:
       GOALCLOSE

     Un singolo opcode GOAL che esegue anche il blocco nasconderebbe troppo
     controllo di flusso alla VM. GOALOPEN, GOALNEXT e GOALCLOSE restano per
     ora nomi progettuali, non opcode richiesti dall'implementazione corrente.
  */
}

pref pred[100];
int is_predef(char* f)
{ for (i=0; pred[i]!=NULL; i++) 
  if (!strcmp(tolower(f),pred[i].name)
  { if (inside_sql && pred[i].sql==0) err("Unsupported in SQL"); 
    return true;
  };
  return false;
};
predef(char* name)
{ //Ho i parametri, qui gestisco la funzione. Se sono predefinite di solito vuol dire che serve un istruzione vm
  //Database
  if (!strcmp(name,"add"))          p_add();
  else if (!strcmp(name,"exists"))   p_exists();
  else if (!strcmp(name,"lock"))     p_lock();
  else if (!strcmp(name,"unlock"))   p_unlock();
  //Input/output
  else if (!strcmp(name,"print"))    p_print();
  else if (!strcmp(name,"eprint"))   p_eprint();
  else if (!strcmp(name,"input"))    p_input();
  else if (!strcmp(name,"show"))     p_show();
  else if (!strcmp(name,"ask"))      p_ask();
  else if (!strcmp(name,"form"))     p_form();
  else if (!strcmp(name,"showform")) p_showform();
  //Filesystem
  else if (!strcmp(name,"load"))     p_load();
  else if (!strcmp(name,"save"))     p_save();
  else if (!strcmp(name,"loaddir"))  p_loaddir();
  //Matematiche e stringhe
  else if (!strcmp(name,"int"))      p_int();
  else if (!strcmp(name,"abs"))      p_abs();
  else if (!strcmp(name,"cos"))      p_cos();
  else if (!strcmp(name,"sin"))      p_sin();
  else if (!strcmp(name,"hex"))      p_hex();
  else if (!strcmp(name,"sqr"))      p_sqr();
  else if (!strcmp(name,"asc"))      p_asc();
  else if (!strcmp(name,"char"))     p_char();
  else if (!strcmp(name,"mid"))      p_mid();
  else if (!strcmp(name,"len"))      p_len();
  else if (!strcmp(name,"uc"))       p_uc();
  else if (!strcmp(name,"index"))    p_index();
  else if (!strcmp(name,"change"))   p_change();
  else if (!strcmp(name,"split"))    p_split();
  else if (!strcmp(name,"join"))     p_join();
  //Gestione strutture multidimensionali
  else if (!strcmp(name,"push"))     p_push();
  else if (!strcmp(name,"pop"))      p_pop();
  else if (!strcmp(name,"numel"))    p_numel();
  //Timer e varie 
  else if (!strcmp(name,"time"))     p_time();
  else if (!strcmp(name,"date"))     p_date();
  else if (!strcmp(name,"random"))   p_random();  
  else if (!strcmp(name,"sleep"))    p_sleep();
  //Gestione base sockets 
  else if (!strcmp(name,"socket"))   p_socket();  
  else if (!strcmp(name,"server"))   p_server();  
  else if (!strcmp(name,"accept"))   p_accept();  
  else if (!strcmp(name,"sread"))    p_sread();  
  else if (!strcmp(name,"swrite"))   p_swrite();  
  else if (!strcmp(name,"read"))     p_read();  
  else if (!strcmp(name,"write"))    p_write();  
  //Motore inferenziale "vecchia maniera"
  else if (!strcmp(name,"assert"))   p_assert();  
  else if (!strcmp(name,"retract"))  p_retract();    
  else if (!strcmp(name,"goal"))     p_goal();    
};

#endif
