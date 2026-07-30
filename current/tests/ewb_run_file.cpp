#include <fstream>
#include <iterator>
#include <string>
#include "../vm.h"

int main(int argc,char **argv)
{ if (argc!=2) return 2;
  std::ifstream input(argv[1],std::ios::binary);
  std::string program((std::istreambuf_iterator<char>(input)),
                      std::istreambuf_iterator<char>());
  return ewb_run_buffer(program.data(),program.size(),argv[1],0,0,"");
}
