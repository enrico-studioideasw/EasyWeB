#include "ewb_values.h"

#include <cstdio>
#include <cstdlib>

void ewbRaise(const std::string& message)
{
  fprintf(stderr,"RAISE: %s\n",message.c_str());
  exit(2);
}

int main()
{
  printf("2 + 3.5 = %s\n",ewbSum("2","3.5").c_str());
  printf("4 * 5 = %s\n",ewbMul("4","5").c_str());
  printf("\"abc\" < \"abd\" = %s\n",ewbCompare("abc","abd","<").c_str());
  return 0;
}
