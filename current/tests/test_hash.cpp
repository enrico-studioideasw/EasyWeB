#include "../ewb_hash.h"

#include <iostream>

static int check(const std::string& name,const std::string& actual,
                 const std::string& expected)
{ if (actual==expected) return 0;
  std::cerr << name << ": expected " << expected << ", got " << actual << "\n";
  return 1;
}

int main()
{ int failures=0;
  failures+=check("md5 empty",ewbMd5(""),"d41d8cd98f00b204e9800998ecf8427e");
  failures+=check("sha256 empty",ewbSha256(""),
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  failures+=check("md5 abc",ewbMd5("abc"),"900150983cd24fb0d6963f7d28e17f72");
  failures+=check("sha256 abc",ewbSha256("abc"),
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  failures+=check("md5 utf8",ewbMd5("caffè ☕"),"8e677fcb875c0bb819ba5c2cb92cbad7");
  failures+=check("sha256 utf8",ewbSha256("caffè ☕"),
    "8b284d2dc1b074fd2160218662b9a7297fa69e9eeb3e067fa86de1fac67500f8");
  return failures ? 1 : 0;
}
