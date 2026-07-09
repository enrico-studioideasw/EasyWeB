#include <string>
#define DEFAULTENGINE "mysql://ewb"
#define DEFAULTUSER "ewb"
#define DEFAULTPASSWORD "ewb"

using namespace std;

string qlist(string context, string query, string orderby);
string qbyid(string context, string query, string orderby, string id);
string run_query(string context, string query, string orderby);
string db_quote(string value);

