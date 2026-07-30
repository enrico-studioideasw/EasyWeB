#include "prolog_engine.h"

#include <cctype>
#include <set>
#include <stdexcept>

using namespace std;

namespace {

struct Term
{ string name;
  vector<Term> args;
};

struct Rule
{ Term head;
  vector<Term> body;
  vector<bool> negated;
  bool cut;
};

typedef map<string,Term> Bindings;

class Parser
{ const string &text;
  size_t pos;

  void spaces()
  { while (pos<text.size() && isspace((unsigned char)text[pos])) pos++;
  }

public:
  Parser(const string &source): text(source), pos(0) {}

  bool take(const string &token)
  { spaces();
    if (text.compare(pos,token.size(),token)!=0) return false;
    pos+=token.size();
    return true;
  }

  Term term()
  { spaces();
    Term out;
    while (pos<text.size())
    { unsigned char c=(unsigned char)text[pos];
      if (!isalnum(c) && c!='_' && c!='~') break;
      out.name+=(char)c;
      pos++;
    }
    if (out.name=="") throw runtime_error("term");
    if (!take("(")) return out;
    if (take(")")) return out;
    for (;;)
    { out.args.push_back(term());
      if (take(")")) break;
      if (!take(",")) throw runtime_error("comma");
    }
    return out;
  }

  bool end()
  { spaces();
    return pos==text.size();
  }
};

static bool variable(const Term &term)
{ if (term.name=="") return false;
  unsigned char c=(unsigned char)term.name[0];
  return (c>='A' && c<='Z') || c=='_';
}

static Term resolve(Term term, const Bindings &bindings)
{ set<string> seen;
  while (variable(term) && term.args.empty())
  { Bindings::const_iterator found=bindings.find(term.name);
    if (found==bindings.end()) break;
    if (seen.count(term.name)) break;
    seen.insert(term.name);
    term=found->second;
  }
  return term;
}

static bool unify(Term left, Term right, Bindings &bindings)
{ left=resolve(left,bindings);
  right=resolve(right,bindings);

  if (variable(left) && left.args.empty())
  { if (left.name!=right.name) bindings[left.name]=right;
    return true;
  }
  if (variable(right) && right.args.empty())
  { bindings[right.name]=left;
    return true;
  }
  if (left.name!=right.name || left.args.size()!=right.args.size()) return false;
  for (size_t i=0; i<left.args.size(); i++)
  { if (!unify(left.args[i],right.args[i],bindings)) return false;
  }
  return true;
}

static string render(Term term, const Bindings &bindings)
{ term=resolve(term,bindings);
  string out=term.name;
  if (!term.args.empty())
  { out+="(";
    for (size_t i=0; i<term.args.size(); i++)
    { if (i) out+=",";
      out+=render(term.args[i],bindings);
    }
    out+=")";
  }
  return out;
}

static Rule parse_rule(const string &text)
{ Parser parser(text);
  Rule rule;
  rule.cut=false;
  rule.head=parser.term();
  if (parser.take(":="))
  { for (;;)
    { bool negated=parser.take("~");
      rule.body.push_back(parser.term());
      rule.negated.push_back(negated);
      if (!parser.take(",")) break;
    }
  }
  if (parser.take("!")) rule.cut=true;
  else if (!parser.take(".")) throw runtime_error("terminator");
  if (!parser.end()) throw runtime_error("tail");
  return rule;
}

static void collect_variables(const Term &term, vector<string> &names)
{ if (variable(term) && term.args.empty())
  { if (term.name!="_")
    { bool present=false;
      for (size_t i=0; i<names.size(); i++)
      { if (names[i]==term.name) present=true;
      }
      if (!present) names.push_back(term.name);
    }
  }
  for (size_t i=0; i<term.args.size(); i++) collect_variables(term.args[i],names);
}

static Term rename_term(Term term, unsigned long serial)
{ if (variable(term) && term.args.empty()) term.name+="__"+to_string(serial);
  for (size_t i=0; i<term.args.size(); i++) term.args[i]=rename_term(term.args[i],serial);
  return term;
}

static Rule rename_rule(Rule rule, unsigned long serial)
{ rule.head=rename_term(rule.head,serial);
  for (size_t i=0; i<rule.body.size(); i++) rule.body[i]=rename_term(rule.body[i],serial);
  return rule;
}

static void solve_terms(const vector<Term> &goals, const vector<bool> &negated,
                        const vector<Rule> &rules, Bindings bindings,
                        vector<Bindings> &solutions, int depth,
                        unsigned long &serial)
{ if (depth>256) throw runtime_error("recursion");
  if (goals.empty())
  { solutions.push_back(bindings);
    return;
  }

  Term goal=goals[0];
  vector<Term> tail(goals.begin()+1,goals.end());
  vector<bool> tail_negated(negated.begin()+1,negated.end());

  if (negated[0])
  { vector<Term> one(1,goal);
    vector<bool> positive(1,false);
    vector<Bindings> found;
    solve_terms(one,positive,rules,bindings,found,depth+1,serial);
    if (found.empty()) solve_terms(tail,tail_negated,rules,bindings,solutions,depth+1,serial);
    return;
  }

  for (size_t i=0; i<rules.size(); i++)
  { Rule rule=rename_rule(rules[i],++serial);
    Bindings next=bindings;
    if (!unify(goal,rule.head,next)) continue;

    vector<Term> combined=rule.body;
    combined.insert(combined.end(),tail.begin(),tail.end());
    vector<bool> combined_negated=rule.negated;
    combined_negated.insert(combined_negated.end(),tail_negated.begin(),tail_negated.end());
    solve_terms(combined,combined_negated,rules,next,solutions,depth+1,serial);
    if (rule.cut) break;
  }
}

}

bool prologValidRule(const string &text)
{ try
  { parse_rule(text);
    return true;
  }
  catch (...)
  { return false;
  }
}

bool prologHeadMatches(const string &pattern, const string &rule_text)
{ try
  { Parser parser(pattern);
    Term wanted=parser.term();
    if (!parser.end()) return false;
    Rule rule=parse_rule(rule_text);
    Bindings bindings;
    return unify(wanted,rule.head,bindings);
  }
  catch (...)
  { return false;
  }
}

vector<map<string,string> > prologSolve(const string &goal_text,
                                       const vector<string> &rule_texts)
{ Parser parser(goal_text);
  Term goal=parser.term();
  if (!parser.end()) throw runtime_error("Invalid goal");

  vector<Rule> rules;
  for (size_t i=0; i<rule_texts.size(); i++) rules.push_back(parse_rule(rule_texts[i]));

  vector<string> variables;
  collect_variables(goal,variables);
  vector<Term> goals(1,goal);
  vector<bool> negated(1,false);
  vector<Bindings> raw;
  Bindings empty;
  unsigned long serial=0;
  solve_terms(goals,negated,rules,empty,raw,0,serial);

  vector<map<string,string> > out;
  for (size_t i=0; i<raw.size(); i++)
  { map<string,string> row;
    for (size_t j=0; j<variables.size(); j++)
      row[variables[j]]=render(Term{variables[j],vector<Term>()},raw[i]);
    out.push_back(row);
  }
  return out;
}
