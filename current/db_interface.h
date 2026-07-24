#include <string>
#include <vector>
#define DEFAULTENGINE "mysql://ewb"
#define DEFAULTUSER "ewb"
#define DEFAULTPASSWORD "ewb"

using namespace std;

string qlist(string url, string user, string password, string table,
             vector<string> fields, string filter, string orderby);
vector<string> qbyid(string url, string user, string password, string table,
                     vector<string> fields, string orderby,
                     string id);
string run_query(string url, string user, string password, string query,
                 string orderby);
string run_query(string url, string user, string password, string table,
                 vector<string> fields, string query, string orderby);
string db_quote(string value);
void create_base_database(void);
void db_begin_transaction(string url, string user, string password, int timeout_seconds);
void db_commit_transaction(string url, string user, string password);
void db_check_transactions(void);
void db_close_all(void);
