/* VM EWB 2026 - bozza compilabile.
 *
 * Questa VM resta volutamente semplice: stile C, funzioni libere e std::string
 * solo per evitare buffer manuali. Le primitive non lanciano eccezioni C++;
 * in caso di errore chiamano ewbRaise(), che qui passa dal meccanismo VM.
 */

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#include "ewb_values.h"

#define MAXSTACK 10000
#define MAXVAR    2000
#define MAXLINES 20000

static std::string stackData[MAXSTACK];
static std::string code[MAXLINES];
static std::string symtname[MAXVAR];
static int symtpos[MAXVAR];

static int ST=0;       /* Symbol table pointer. */
static int IDF=0;      /* Form id progressivo. */
static int gPC=0;
static int gSP=0;
static std::string gA;

static void vmRaise(const std::string& message);

void ewbRaise(const std::string& message)
{
  vmRaise(message);
}

static void die(const char* message)
{
  vmRaise(message);
}

static std::string intToString(int value)
{
  char buffer[64];
  snprintf(buffer,sizeof(buffer),"%d",value);
  return std::string(buffer);
}

static std::string htmlEscape(const std::string& value)
{
  std::string out;
  for (size_t i=0; i<value.size(); i++)
  { char c=value[i];
    if (c=='&') out+="&amp;";
    else if (c=='<') out+="&lt;";
    else if (c=='>') out+="&gt;";
    else if (c=='"') out+="&quot;";
    else out+=c;
  }
  return out;
}

static std::string pop()
{
  if (gSP<=0) die("Stack underflow");
  gSP--;
  return stackData[gSP];
}

static void push(const std::string& value)
{
  if (gSP>=MAXSTACK) die("Stack overflow");
  stackData[gSP]=value;
  gSP++;
}

static int findvar(const std::string& name)
{
  for (int i=ST-1; i>=0; i--)
  { if (symtname[i]==name) return symtpos[i];
  }
  return -1;
}

static std::string getvar(const std::string& name)
{
  int pos=findvar(name);
  if (pos<0) vmRaise("Unknown symbol: " + name);
  return stackData[pos];
}

static void addsymtable(const std::string& name)
{
  if (ST>=MAXVAR) die("Vars overflow");
  symtname[ST]=name;
  symtpos[ST]=gSP;
  ST++;
}

static std::string nextToken(const std::string& line, size_t& pos)
{
  while (pos<line.size() && (line[pos]==' ' || line[pos]=='\t')) pos++;
  if (pos>=line.size()) return "";

  if (line[pos]=='"')
  { pos++;
    std::string out;
    while (pos<line.size())
    { char c=line[pos++];
      if (c=='\\' && pos<line.size())
      { out+=line[pos++];
      }
      else if (c=='"')
      { break;
      }
      else
      { out+=c;
      }
    }
    return out;
  }

  size_t start=pos;
  while (pos<line.size() && line[pos]!=' ' && line[pos]!='\t') pos++;
  return line.substr(start,pos-start);
}

static std::string escapeStack()
{
  std::string out;
  for (int i=0; i<gSP; i++)
  { if (i>0) out+="\n";
    out+=stackData[i];
  }
  return out;
}

static std::string signature(const std::string& value)
{
  /* Placeholder: qui andranno firma forte e, quando serve, cifratura. */
  return "unsigned:" + intToString((int)value.size());
}

static void emitInput(const std::string& varname, const std::string& vartype)
{
  std::string varvalue=getvar(varname);
  std::string type=vartype;
  if (type=="") type="text";

  if (type=="textarea")
  { printf("<textarea name=\"%s\" id=\"%s\">%s</textarea>\n",
           varname.c_str(), varname.c_str(), htmlEscape(varvalue).c_str());
  }
  else
  { printf("<input type=\"%s\" name=\"%s\" id=\"%s\" value=\"%s\" />\n",
           type.c_str(), varname.c_str(), varname.c_str(),
           htmlEscape(varvalue).c_str());
  }
}

static void sendForm(int nextPC)
{
  std::string st=escapeStack();
  std::string ep=intToString(nextPC);
  std::string sp=intToString(gSP);
  std::string sig=signature(sp + " " + ep + " " + st);

  printf("<textarea name=\"__stack\" id=\"__stack\">%s</textarea>\n", htmlEscape(st).c_str());
  printf("<input type=\"hidden\" name=\"__entrypoint\" id=\"__entrypoint\" value=\"%s\" />\n", ep.c_str());
  printf("<input type=\"hidden\" name=\"__stackpos\" id=\"__stackpos\" value=\"%s\" />\n", sp.c_str());
  printf("<input type=\"hidden\" name=\"__signature\" id=\"__signature\" value=\"%s\" />\n", htmlEscape(sig).c_str());
  printf("</form>\n");
}

static void vmRaise(const std::string& message)
{
  int onerr=findvar("__on_error");
  if (onerr>=0)
  { push(message);
    gPC=ewbIntValue(stackData[onerr]);
    return;
  }

  fprintf(stderr,"EWB raise: %s\n",message.c_str());
  std::exit(1);
}

static void execLine(const std::string& line)
{
  size_t pos=0;
  std::string i=nextToken(line,pos);
  if (i=="" || i[0]=='\'') return;

  if (i=="SUM")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbSum(left,right);
  }
  else if (i=="CONCAT")
  { std::string right=pop();
    std::string left=pop();
    gA=left+right;
  }
  else if (i=="SUB")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbSub(left,right);
  }
  else if (i=="MUL")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbMul(left,right);
  }
  else if (i=="DIV")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbDiv(left,right);
  }
  else if (i=="OR")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbOr(left,right);
  }
  else if (i=="AND")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbAnd(left,right);
  }
  else if (i=="ANDB")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbBitwiseAnd(left,right);
  }
  else if (i=="ORB")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbBitwiseOr(left,right);
  }
  else if (i=="NOT")
  { gA=ewbNegative(pop());
  }
  else if (i=="NOTB")
  { gA=ewbBitwiseNot(pop());
  }
  else if (i==">" || i=="<" || i=="=" || i=="==" || i=="!=" || i==">=" || i=="<=" ||
           i=="gt" || i=="lt" || i=="ge" || i=="le")
  { std::string right=pop();
    std::string left=pop();
    gA=ewbCompare(left,right,i);
  }
  else if (i=="JZ")
  { std::string dst=pop();
    if (gA=="0" || gA=="") gPC=ewbIntValue(dst)-1;
  }
  else if (i=="JNZ")
  { std::string dst=pop();
    if (gA!="0" && gA!="") gPC=ewbIntValue(dst)-1;
  }
  else if (i=="CALL")
  { push(intToString(gPC));
    gPC=ewbIntValue(gA)-1;
  }
  else if (i=="RET")
  { gPC=ewbIntValue(pop())-1;
  }
  else if (i=="SETA")
  { gA=nextToken(line,pos);
  }
  else if (i=="PUSH")
  { push(gA);
  }
  else if (i=="POP")
  { gA=pop();
  }
  else if (i=="ADDSYMTABLE" || i=="addsymtable")
  { std::string name=nextToken(line,pos);
    if (name=="") name=gA;
    addsymtable(name);
  }
  else if (i=="STARTFORM" || i=="startform")
  { printf("<form method=\"post\" id=\"__form%d\" enctype=\"multipart/form-data\">\n",IDF);
    IDF++;
  }
  else if (i=="STARTTARGET" || i=="starttarget")
  { printf("<form style=\"visibility:hidden\" method=\"post\" id=\"__form%d\" enctype=\"multipart/form-data\">\n",IDF);
    IDF++;
  }
  else if (i=="ADDTOFORM" || i=="addtoform")
  { std::string vartype=pop();
    std::string varname=pop();
    emitInput(varname,vartype);
  }
  else if (i=="SENDFORM" || i=="sendForm")
  { sendForm(gPC+1);
  }
  else if (i=="STOP" || i=="stop")
  { std::exit(0);
  }
  else if (i=="ONERROR" || i=="onerror")
  { std::string entry=pop();
    push(entry);
    addsymtable("__on_error");
  }
  else if (i=="RAISE" || i=="raise")
  { vmRaise(pop());
  }
  else if (i=="TASK" || i=="TARGET" || i=="ENDTASK" || i=="ENDTARGET" ||
           i=="RUNTARGET" || i=="CRONTASK" || i=="IN")
  { vmRaise("Instruction not implemented yet: " + i);
  }
  else
  { vmRaise("Unknown instruction: " + i);
  }
}

static void resume(int pc, int sp)
{
  gPC=pc;
  gSP=sp;

  while (gPC>=0 && gPC<MAXLINES && code[gPC]!="")
  { execLine(code[gPC]);
    gPC++;
  }
}

static void start()
{
  symtname[ST]="__global";
  symtpos[ST]=gSP;
  ST++;
  resume(0,0);
}

int main()
{
  char line[8192];
  int n=0;

  while (n<MAXLINES && fgets(line,sizeof(line),stdin)!=NULL)
  { std::string l=line;
    while (l.size()>0 && (l[l.size()-1]=='\n' || l[l.size()-1]=='\r')) l.erase(l.size()-1);
    code[n]=l;
    n++;
  }

  start();
  return 0;
}
