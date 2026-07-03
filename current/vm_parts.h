#ifndef EWB_VM_PARTS_H
#define EWB_VM_PARTS_H

#include <string>
#include <vector>

using namespace std;

struct ewb_db;
struct ewb_cursor;
typedef ewb_db* tipodb;
typedef ewb_cursor* tipocursore;

void raiseerr(string e);

int    ewbIntValue(string v); //Qui conversione interna a intero
double ewbValue(string v);    //Questa alza eccezione se non numero
string ewbNumber(double v);
string ewbInt(int v);
string ewbBool(bool v);
string hexEncode(string v);
string hexDecode(string v);
string ewbSum(string x, string y);
string ewbSub(string x, string y);
string ewbNegative(string x);
string ewbMul(string x, string y);
string ewbDiv(string x, string y);
string ewbOr(string x, string y);
string ewbAnd(string x, string y);
string ewbNot(string x);
string ewbBitwiseOr(string x, string y);
string ewbBitwiseAnd(string x, string y);
string ewbBitwiseNot(string x);
string ewbCompare(string x, string y, string cmp);
string togliVirgolette(string s); //Prende un dato tra virgolette ed elimina eventuali escape interni.
string mettiVirgolette(string s);
string signature(string s);
string escapeTag(string s);
vector<string> split(string s, char sep);
string join(vector<string> v, string sep);

// Qui accesso a SQL. Cercheremo di fare query semplici.
tipodb      sql_connect(string uri, string user, string pass, string db);
string      sql_exec(tipodb db, string q);
tipocursore perform_query(tipodb db, string q);
string      sql_fetch(tipocursore c);
void        sql_close(tipocursore c);
string      sqlLastId(tipodb db);

#endif
