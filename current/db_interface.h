#include <string>
#include <vector>
#define DEFAULTENGINE "mysql://ewb"
#define DEFAULTUSER "ewb"
#define DEFAULTPASSWORD "ewb"

using namespace std;

struct EwbRuleRow
{ string id;
  string clause;
};

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
void db_rollback_all(void);
bool db_has_transactions(void);
void db_close_all(void);
vector<EwbRuleRow> db_rule_list(string url, string user, string password,
                                string group);
string db_rule_insert(string url, string user, string password,
                      string group, string clause);
int db_rule_delete(string url, string user, string password,
                   vector<string> ids);
int db_rule_delete_group(string url, string user, string password,
                         string group);
