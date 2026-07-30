#ifdef EWB_VM_RUNTIME

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <poll.h>
#include <random>
#include <sstream>
#include <thread>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

static string p_html(string value)
{ string result="";
  for (size_t i=0; i<value.size(); i++)
  { if (value[i]=='&') result += "&amp;";
    else if (value[i]=='<') result += "&lt;";
    else if (value[i]=='>') result += "&gt;";
    else if (value[i]=='"') result += "&quot;";
    else result += value[i];
  }
  return result;
}

static vector<string> p_array_values(string value)
{ vector<string> result;
  int count=ewbNumKey(value);
  for (int i=0; i<count; i++)
  { string key=ewbKeyAt(value,i);
    result.push_back(ewbGetElement(value,key));
  }
  return result;
}

static void p_addform(string *accumulator)
{ string name,type,value,label;
  POP(label);
  POP(value);
  POP(type);
  POP(name);

  string field="";
  if (type=="text" || type=="password" || type=="hidden" || type=="file")
  { field="<input id=\"" + p_html(name) +
          "\" class=\"field\" name=\"" + p_html(name) +
          "\" type=\"" + type + "\"";
    if (type!="file") field += " value=\"" + p_html(value) + "\"";
    field += ">";
  }
  else if (type=="textarea")
  { field="<textarea id=\"" + p_html(name) +
          "\" class=\"field\" name=\"" + p_html(name) + "\">" +
          p_html(value) + "</textarea>";
  }
  else if (type=="checkbox")
  { field="<input id=\"" + p_html(name) +
          "\" class=\"field\" name=\"" + p_html(name) +
          "\" type=\"checkbox\" value=\"1\"";
    if (value!="0" && value!="") field += " checked";
    field += ">";
  }
  else if (type=="info")
  { field="<b id=\"" + p_html(name) +
          "\" class=\"field\">" + p_html(value) + "</b>";
  }
  else if (type=="submit")
  { field="<input id=\"" + p_html(name) +
          "\" class=\"field\" name=\"" + p_html(name) +
          "\" type=\"submit\" value=\"" + p_html(label) + "\">";
  }
  else if (type=="option" || type=="options" || type=="radio" ||
           type=="checks" || type=="links")
  { vector<string> values=p_array_values(value);
    if (type=="option" || type=="options")
    { field="<select id=\"" + p_html(name) +
            "\" class=\"field\" name=\"" + p_html(name) + "\"";
      if (type=="options") field += " multiple";
      field += ">";
    }
    for (size_t i=0; i<values.size(); i++)
    { string item=values[i];
      bool selected=false;
      if (item.size()>0 && item[0]=='*')
      { selected=true;
        item=item.substr(1);
      }
      if (type=="option" || type=="options")
      { field += "<option value=\"" + p_html(item) + "\"";
        if (selected) field += " selected";
        field += ">" + p_html(item) + "</option>";
      }
      else if (type=="radio" || type=="checks")
      { string input_type="radio";
        if (type=="checks") input_type="checkbox";
        field += "<input class=\"field\" name=\"" + p_html(name) + "\" type=\"" +
                 input_type + "\" value=\"" + p_html(item) + "\"";
        if (selected) field += " checked";
        field += ">" + p_html(item);
      }
      else if (type=="links")
      { size_t arrow=item.find("->");
        string text=item;
        string destination=item;
        if (arrow!=string::npos)
        { text=item.substr(0,arrow);
          destination=item.substr(arrow+2);
        }
        field += "<a href=\"" + p_html(destination) + "\">" +
                 p_html(text) + "</a>";
      }
    }
    if (type=="option" || type=="options") field += "</select>";
  }
  else
  { raiseerr("Unknown form field type: "+type);
    return;
  }

  if (type!="hidden" && type!="submit" && label!="")
  { field="<label id=\"label" + p_html(name) +
          "\" class=\"label\">" + p_html(label) + "</label>" + field;
  }
  *accumulator=field+"\n";
}

static void p_show(string *accumulator)
{ string value;
  POP(value);
  int template_position=findvar("_template");
  if (template_position<0)
  { raiseerr("Missing _template");
    return;
  }

  string page=stack[template_position];
  string result="";
  size_t start=0;
  size_t marker=page.find("<>");
  while (marker!=string::npos)
  { result += page.substr(start,marker-start);
    result += value;
    start=marker+2;
    marker=page.find("<>",start);
  }
  result += page.substr(start);
  cout << result;
  *accumulator=value;
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
  number=trunc(number);
  if (number==0) number=0;
  ostringstream result;
  result << fixed << setprecision(0) << number;
  *accumulator=result.str();
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
  double parsed=ewbValue(value);
  if (parsed!=trunc(parsed) ||
      parsed<numeric_limits<int32_t>::min() ||
      parsed>numeric_limits<int32_t>::max())
  { raiseerr("Hex range");
    return;
  }
  int32_t number=(int32_t)parsed;
  ostringstream result;
  if (number<0)
  { result << uppercase << hex << setw(8) << setfill('0') << (uint32_t)number;
  } else result << uppercase << hex << number;
  *accumulator=result.str();
}

static void p_sqr(string *accumulator)
{ string value;
  POP(value);
  double number=ewbValue(value);
  if (number<0) { raiseerr("Square root of negative number"); return; }
  *accumulator=ewbNumber(sqrt(number));
}

static void p_asc(string *accumulator)
{ string value;
  POP(value);
  if (value=="")
  { raiseerr("ASC of empty string");
    return;
  }
  *accumulator=to_string((unsigned int)(unsigned char)value[0]);
}

static void p_char(string *accumulator)
{ string value;
  POP(value);
  double parsed=ewbValue(value);
  if (parsed!=trunc(parsed) || parsed<0 || parsed>255)
  { raiseerr("Invalid byte");
    return;
  }
  *accumulator=string(1,(char)(unsigned char)parsed);
}

static void p_mid(string *accumulator)
{ string text,start_text,length_text;
  POP(length_text);
  POP(start_text);
  POP(text);
  int start=ewbIntValue(start_text);
  int length=ewbIntValue(length_text);
  long long first=start;
  if (first<0) first=(long long)text.size()+first;
  if (first<0 || first>=(long long)text.size())
  { *accumulator="";
    return;
  }
  if (length==-1)
  { *accumulator=text.substr((size_t)first);
    return;
  }
  long long last=first;
  if (length>=0) last+=length;
  else last=(long long)text.size()+length;
  if (last<=first)
  { *accumulator="";
    return;
  }
  if (last>(long long)text.size()) last=(long long)text.size();
  *accumulator=text.substr((size_t)first,(size_t)(last-first));
}

static void p_len(string *accumulator)
{ string value;
  POP(value);
  *accumulator=to_string((unsigned long long)value.size());
}

static void p_uc(string *accumulator)
{ string value;
  POP(value);
  for (size_t i=0; i<value.size(); i++)
  { unsigned char character=(unsigned char)value[i];
    if (character>='a' && character<='z') value[i]=(char)(character-'a'+'A');
  }
  *accumulator=value;
}

static void p_index(string *accumulator)
{ string text,substring;
  POP(substring);
  POP(text);
  size_t found=text.find(substring);
  if (found==string::npos)
  { *accumulator="-1";
    return;
  }
  *accumulator=to_string((unsigned long long)found);
}

static void p_numel(string *accumulator)
{ string value;
  POP(value);
  *accumulator=ewbInt(ewbNumEl(value));
}

static void p_arraypop(string *accumulator)
{ string name;
  POP(name);
  int position=findvar(name);
  if (position<0)
  { raiseerr("Unknown array: "+name);
    return;
  }
  *accumulator=ewbArrayPop(&stack[position]);
}

static void p_time(string *accumulator)
{ long long milliseconds=chrono::duration_cast<chrono::milliseconds>(
      chrono::system_clock::now().time_since_epoch()).count();
  *accumulator=to_string(milliseconds);
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
{ string limit_text;
  POP(limit_text);
  double parsed=ewbValue(limit_text);
  if (parsed!=trunc(parsed) || parsed<1 ||
      parsed>(double)numeric_limits<long long>::max())
  { raiseerr("Random range");
    return;
  }
  unsigned long long limit=(unsigned long long)parsed;
  static mt19937_64 generator((random_device())());
  uniform_int_distribution<unsigned long long> values(0,limit-1);
  *accumulator=to_string(values(generator));
}

static void p_sleep(string *accumulator)
{ string milliseconds;
  POP(milliseconds);
  double parsed=ewbValue(milliseconds);
  if (parsed!=trunc(parsed) || parsed<0 ||
      parsed>(double)numeric_limits<long long>::max())
  { raiseerr("Sleep range");
    return;
  }
  this_thread::sleep_for(chrono::milliseconds((long long)parsed));
  *accumulator="";
}

static map<string,int> p_sockets;
static unsigned long long p_next_socket=1;

static string p_socket_handle(int descriptor)
{ string handle;
  handle+=(char)0x1d;
  handle+="socket:";
  handle+=to_string(p_next_socket++);
  p_sockets[handle]=descriptor;
  return handle;
}

static int p_socket_descriptor(string handle)
{ map<string,int>::iterator found=p_sockets.find(handle);
  if (found==p_sockets.end()) return -1;
  return found->second;
}

static bool p_live_resource(string value)
{ return p_sockets.find(value)!=p_sockets.end();
}

static void p_close_resources(void)
{ for (map<string,int>::iterator i=p_sockets.begin(); i!=p_sockets.end(); i++)
    close(i->second);
  p_sockets.clear();
}

static void p_socket_open(string *accumulator)
{ string host,port;
  POP(port);
  POP(host);
  struct addrinfo hints;
  struct addrinfo *addresses=NULL;
  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  if (getaddrinfo(host.c_str(),port.c_str(),&hints,&addresses)!=0)
  { *accumulator="";
    return;
  }
  int descriptor=-1;
  for (struct addrinfo *address=addresses; address; address=address->ai_next)
  { descriptor=socket(address->ai_family,address->ai_socktype,address->ai_protocol);
    if (descriptor<0) continue;
    if (connect(descriptor,address->ai_addr,address->ai_addrlen)==0) break;
    close(descriptor);
    descriptor=-1;
  }
  freeaddrinfo(addresses);
  if (descriptor<0) *accumulator="";
  else *accumulator=p_socket_handle(descriptor);
}

static void p_server_open(string *accumulator)
{ string port;
  POP(port);
  struct addrinfo hints;
  struct addrinfo *addresses=NULL;
  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;
  if (getaddrinfo(NULL,port.c_str(),&hints,&addresses)!=0)
  { *accumulator="";
    return;
  }
  int descriptor=-1;
  for (struct addrinfo *address=addresses; address; address=address->ai_next)
  { descriptor=socket(address->ai_family,address->ai_socktype,address->ai_protocol);
    if (descriptor<0) continue;
    int reuse=1;
    setsockopt(descriptor,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    if (bind(descriptor,address->ai_addr,address->ai_addrlen)==0 &&
        listen(descriptor,5)==0) break;
    close(descriptor);
    descriptor=-1;
  }
  freeaddrinfo(addresses);
  if (descriptor<0) *accumulator="";
  else *accumulator=p_socket_handle(descriptor);
}

static void p_socket_accept(string *accumulator)
{ string handle;
  POP(handle);
  int descriptor=p_socket_descriptor(handle);
  if (descriptor<0)
  { *accumulator="";
    return;
  }
  int client=accept(descriptor,NULL,NULL);
  if (client<0) *accumulator="";
  else *accumulator=p_socket_handle(client);
}

static void p_socket_read(string *accumulator)
{ string handle;
  POP(handle);
  int descriptor=p_socket_descriptor(handle);
  if (descriptor<0)
  { *accumulator="";
    return;
  }
  string result;
  char byte;
  while (1)
  { ssize_t count=read(descriptor,&byte,1);
    if (count==1)
    { result+=byte;
      if (byte=='\n') break;
      continue;
    }
    if (count<0 && errno==EINTR) continue;
    break;
  }
  *accumulator=result;
}

static void p_socket_write(string *accumulator)
{ string handle,data;
  POP(data);
  POP(handle);
  int descriptor=p_socket_descriptor(handle);
  if (descriptor<0)
  { *accumulator="";
    return;
  }
  size_t done=0;
  while (done<data.size())
  { ssize_t count=write(descriptor,data.data()+done,data.size()-done);
    if (count>0)
    { done+=(size_t)count;
      continue;
    }
    if (count<0 && errno==EINTR) continue;
    *accumulator="";
    return;
  }
  *accumulator="1";
}

static void p_socket_close(string *accumulator)
{ string handle;
  POP(handle);
  map<string,int>::iterator found=p_sockets.find(handle);
  if (found==p_sockets.end())
  { *accumulator="";
    return;
  }
  int result=close(found->second);
  p_sockets.erase(found);
  if (result==0) *accumulator="1";
  else *accumulator="";
}

static void p_exec(string *accumulator)
{ string command;
  POP(command);
  int output_pipe[2];
  int error_pipe[2];
  if (pipe(output_pipe)<0 || pipe(error_pipe)<0) raiseerr("EXEC pipe");
  pid_t child=fork();
  if (child<0) raiseerr("EXEC fork");
  if (child==0)
  { dup2(output_pipe[1],STDOUT_FILENO);
    dup2(error_pipe[1],STDERR_FILENO);
    close(output_pipe[0]);
    close(output_pipe[1]);
    close(error_pipe[0]);
    close(error_pipe[1]);
    execl("/bin/sh","sh","-c",command.c_str(),(char*)NULL);
    _exit(127);
  }
  close(output_pipe[1]);
  close(error_pipe[1]);
  string output;
  string error;
  struct pollfd descriptors[2];
  descriptors[0].fd=output_pipe[0];
  descriptors[0].events=POLLIN;
  descriptors[1].fd=error_pipe[0];
  descriptors[1].events=POLLIN;
  int open_count=2;
  while (open_count)
  { if (poll(descriptors,2,-1)<0)
    { if (errno==EINTR) continue;
      break;
    }
    for (int i=0; i<2; i++)
    { if (descriptors[i].fd<0) continue;
      if (!(descriptors[i].revents&(POLLIN|POLLHUP|POLLERR))) continue;
      char buffer[4096];
      ssize_t count=read(descriptors[i].fd,buffer,sizeof(buffer));
      if (count>0)
      { if (i==0) output.append(buffer,(size_t)count);
        else error.append(buffer,(size_t)count);
      } else
      { close(descriptors[i].fd);
        descriptors[i].fd=-1;
        open_count--;
      }
    }
  }
  int status;
  while (waitpid(child,&status,0)<0 && errno==EINTR) {}
  int position=findvar("_err");
  if (position<0)
  { if (ST>=MAXVAR || SP>=MAXSTACK) raiseerr("EXEC _err");
    symtname[ST]="_err";
    symtpos[ST]=SP;
    ST++;
    stack[SP]=error;
    SP++;
  } else stack[position]=error;
  *accumulator=output;
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
     Non applica un limite artificiale signed 32 bit.
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
       [intero signed 32 bit] -> A = rappresentazione esadecimale canonica
     Nessun prefisso; positivi minimi, negativi in complemento a due su
     otto cifre. Frazioni e valori fuori intervallo eseguono raise.
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
       [testo] -> A = valore 0..255 del primo byte
     La stringa vuota esegue raise.
  */
}

static void p_char(void)
{ /* Opcode necessario: CHAR.
       [codice] -> A = singolo byte
     Codice non intero o fuori 0..255 esegue raise.
  */
}

static void p_mid(void)
{ /* Opcode necessario: MID.
       [testo,inizio,-1]        -> A = suffisso
       [testo,inizio,lunghezza] -> A = sottostringa
     Il compilatore aggiunge -1 come terzo parametro per la forma MID a due
     argomenti: la VM riceve quindi sempre lo stesso numero di valori.
     Offset e lunghezza sono in byte; un offset negativo conta dalla fine.
  */
}

static void p_len(void)
{ /* Opcode necessario: LEN.
       [testo] -> A = numero di byte
  */
}

static void p_uc(void)
{ /* Opcode necessario: UC.
       [testo] -> A = testo convertito in maiuscolo ASCII
     Converte soltanto a-z; gli altri byte restano invariati.
  */
}

static void p_index(void)
{ /* Opcode necessario: INDEX.
       [testo,sottostringa] -> A = posizione oppure -1
     La posizione e' un offset in byte.
  */
}

static void p_change(void)
{ /* Nessun opcode CHANGE.
       [nuova,vecchia,testo] -> SPLIT(vecchia,testo), poi JOIN(nuova,lista).
  */
}

static void p_split(void)
{ /* Opcode SPLIT.
       [separatore,testo] -> A = struttura EWB indicizzata da 0
     Il compilatore genera un ciclo basato su INDEX, MID e LEN; ogni segmento
     viene aggiunto alla struttura risultante con SETPATH.
     Il caso del separatore vuoto deve essere trattato esplicitamente per
     evitare un ciclo senza avanzamento.
  */
}

static void p_join(void)
{ /* Opcode JOIN.
       [separatore,lista] -> A = elementi concatenati
     Il compilatore usa NUMEL e scorre gli indici con GETPATH, concatenando il
     separatore soltanto fra due elementi mediante CONCAT.
  */
}

static void p_push(void)
{ /* Forma speciale del compilatore. ARRAYPUSH calcola max(chiavi
     numeriche)+1; NUMEL da solo non basta sugli array radi. Il primo
     argomento deve essere una variabile assegnabile:

       PUSH "nome_lista"
       <valuta elemento>
       PUSHA
       ARRAYPUSH

     Il compilatore deve conservare nome_lista: passare soltanto il suo valore
     non basta per aggiornare la variabile.
  */
}

static void p_pop(void)
{ /* Opcode necessario: ARRAYPOP.
       [nome_lista] -> A = elemento estratto
     La variabile semplice indicata da nome_lista riceve la lista accorciata.
     Su lista vuota A riceve "". Per percorsi annidati si usa GETELEM.
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
       [] -> A = Unix time corrente in millisecondi
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
       [X] -> A = intero pseudocasuale uniforme, 0 <= A < X
     X deve essere un intero strettamente positivo.
     Seed e generatore sono stato della VM.
  */
}

static void p_sleep(void)
{ /* Opcode necessario: SLEEP.
       [millisecondi] -> A = ""
     Il valore deve essere un intero non negativo.
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
