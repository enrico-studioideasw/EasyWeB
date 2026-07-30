#ifndef EWB_PROLOG_ENGINE_H
#define EWB_PROLOG_ENGINE_H

#include <map>
#include <string>
#include <vector>

bool prologValidRule(const std::string &text);
bool prologHeadMatches(const std::string &pattern, const std::string &rule);
std::vector<std::map<std::string,std::string> > prologSolve(
  const std::string &goal,
  const std::vector<std::string> &rules);

#endif
