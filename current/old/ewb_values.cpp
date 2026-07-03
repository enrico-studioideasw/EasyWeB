#include "ewb_values.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdio>

static bool ewbIsSpace(char c)
{
  return c==' ' || c=='\t' || c=='\n' || c=='\r';
}

static std::string ewbTrim(const std::string& value)
{
  size_t start=0;
  while (start<value.size() && ewbIsSpace(value[start])) start++;

  size_t end=value.size();
  while (end>start && ewbIsSpace(value[end-1])) end--;

  return value.substr(start,end-start);
}

static bool ewbToNumber(const std::string& value, double& out)
{
  std::string v=ewbTrim(value);
  if (v=="") return false;

  const char* s=v.c_str();
  char* end=0;
  errno=0;
  out=strtod(s,&end);
  if (errno!=0 || end==s) return false;

  while (*end!=0)
  { if (!ewbIsSpace(*end)) return false;
    end++;
  }

  return std::isfinite(out);
}

static std::string ewbNumberToString(double value)
{
  if (!std::isfinite(value)) ewbRaise("Invalid numeric result");

  char buffer[128];
  snprintf(buffer,sizeof(buffer),"%.15g",value);

  std::string out=buffer;
  if (out=="-0") out="0";
  return out;
}

static double ewbNeedNumber(const std::string& value, const std::string& message)
{
  double out=0;
  if (!ewbToNumber(value,out)) ewbRaise(message);
  return out;
}

static int ewbNeedInt(const std::string& value, const std::string& message)
{
  double out=ewbNeedNumber(value,message);
  return (int)out;
}

static bool ewbTruth(const std::string& value)
{
  if (value=="") return false;

  double n=0;
  if (ewbToNumber(value,n)) return n!=0;

  return true;
}

bool ewbIsNumber(const std::string& value)
{
  double out=0;
  return ewbToNumber(value,out);
}

int ewbIntValue(const std::string& value)
{
  return ewbNeedInt(value,"Integer conversion error");
}

std::string ewbSum(const std::string& left, const std::string& right)
{
  double a=ewbNeedNumber(left,"Apple and pear sum error");
  double b=ewbNeedNumber(right,"Apple and pear sum error");
  return ewbNumberToString(a+b);
}

std::string ewbSub(const std::string& left, const std::string& right)
{
  double a=ewbNeedNumber(left,"Apple and pear subtraction error");
  double b=ewbNeedNumber(right,"Apple and pear subtraction error");
  return ewbNumberToString(a-b);
}

std::string ewbMul(const std::string& left, const std::string& right)
{
  double a=ewbNeedNumber(left,"Apple and pear multiplication error");
  double b=ewbNeedNumber(right,"Apple and pear multiplication error");
  return ewbNumberToString(a*b);
}

std::string ewbDiv(const std::string& left, const std::string& right)
{
  double a=ewbNeedNumber(left,"Apple and pear division error");
  double b=ewbNeedNumber(right,"Apple and pear division error");
  if (b==0) ewbRaise("Division by zero");
  return ewbNumberToString(a/b);
}

std::string ewbNegative(const std::string& value)
{
  double a=ewbNeedNumber(value,"Apple and pear negative error");
  return ewbNumberToString(-a);
}

std::string ewbOr(const std::string& left, const std::string& right)
{
  return (ewbTruth(left) || ewbTruth(right)) ? "1" : "0";
}

std::string ewbAnd(const std::string& left, const std::string& right)
{
  return (ewbTruth(left) && ewbTruth(right)) ? "1" : "0";
}

std::string ewbBitwiseOr(const std::string& left, const std::string& right)
{
  int a=ewbNeedInt(left,"Apple and pear bitwise or error");
  int b=ewbNeedInt(right,"Apple and pear bitwise or error");
  return ewbNumberToString(a|b);
}

std::string ewbBitwiseAnd(const std::string& left, const std::string& right)
{
  int a=ewbNeedInt(left,"Apple and pear bitwise and error");
  int b=ewbNeedInt(right,"Apple and pear bitwise and error");
  return ewbNumberToString(a&b);
}

std::string ewbBitwiseNot(const std::string& value)
{
  int a=ewbNeedInt(value,"Apple and pear bitwise not error");
  return ewbNumberToString(~a);
}

std::string ewbCompare(const std::string& left, const std::string& right, const std::string& op)
{
  double a=0;
  double b=0;
  bool numeric=ewbToNumber(left,a) && ewbToNumber(right,b);
  bool res=false;

  if (numeric)
  { if (op=="=" || op=="==") res=(a==b);
    else if (op=="!=") res=(a!=b);
    else if (op==">") res=(a>b);
    else if (op=="<") res=(a<b);
    else if (op==">=") res=(a>=b);
    else if (op=="<=") res=(a<=b);
    else ewbRaise("Unknown compare operator");
  }
  else
  { if (op=="=" || op=="==") res=(left==right);
    else if (op=="!=") res=(left!=right);
    else if (op==">" || op=="gt") res=(left>right);
    else if (op=="<" || op=="lt") res=(left<right);
    else if (op==">=" || op=="ge") res=(left>=right);
    else if (op=="<=" || op=="le") res=(left<=right);
    else ewbRaise("Unknown compare operator");
  }

  return res ? "1" : "0";
}
