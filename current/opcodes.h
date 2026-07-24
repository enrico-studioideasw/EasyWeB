#ifndef EWB_OPCODES_H
#define EWB_OPCODES_H

#include <stdint.h>
#include <string.h>

struct EWBHeader
{ uint8_t magic[4];   // 0x7F 'E' 'W' 'B'
  uint8_t version;    // 1
};

typedef enum
{
  ARG_NONE,
  ARG_INT,
  ARG_STRING,
  ARG_INT_OR_STRING
} ArgType;

typedef enum
{
  OPCODE_INVALID = 0,
  OP_SUM = 0x80,
  OP_CONCAT,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_OR,
  OP_AND,
  OP_ANDB,
  OP_ORB,
  OP_NOT,
  OP_NOTB,
  OP_GT,
  OP_LT,
  OP_EQ,
  OP_NEQ,
  OP_GE,
  OP_LE,
  OP_SGT,
  OP_SLT,
  OP_SEQ,
  OP_SNEQ,
  OP_SGE,
  OP_SLE,
  OP_JZ,
  OP_JNZ,
  OP_CALL,
  OP_RET,
  OP_MOVA,
  OP_PUSHA,
  OP_PUSH,
  OP_POPA,
  OP_ADDSYMTABLE,
  OP_DELSYMTABLE,
  OP_DECSP,
  OP_INCSP,
  OP_STARTFORM,
  OP_STARTTARGET,
  OP_ADDFORM,
  OP_ENDFORM,
  OP_STOP,
  OP_RUNTARGET,
  OP_CRONTASK,
  OP_TASK,
  OP_TARGET,
  OP_ENDTASK,
  OP_ENDTARGET,
  OP_QLIST,
  OP_QBYID,
  OP_QUERY,
  OP_ONERROR,
  OP_RAISE,
  OP_SETPATH,
  OP_GETPATH,
  OP_JMP,
  OP_DBLOCK,
  OP_DBUNLOCK,
  OP_DBROLLBACK,
  OP_PRINT,
  OP_EPRINT,
  OP_INPUT,
  OP_SHOW,
  OP_FLOAD,
  OP_FSAVE,
  OP_FREADDIR,
  OP_TOINT,
  OP_SIND,
  OP_TOHEX,
  OP_SQRT,
  OP_ASC,
  OP_CHAR,
  OP_MID,
  OP_LEN,
  OP_UC,
  OP_INDEX,
  OP_ARRAYPOP,
  OP_NUMEL,
  OP_TIME,
  OP_DATE,
  OP_RANDOM,
  OP_SLEEP,
  OP_SOCKET,
  OP_SERVER,
  OP_ACCEPT,
  OP_SREAD,
  OP_SWRITE,
  OP_DREAD,
  OP_DWRITE
} EWBOpcode;

typedef struct
{
  const char *name;
  EWBOpcode opcode;
  ArgType argtype;
} VmInstr;

static const VmInstr vm_instr_table[] =
{
  {"sum",         OP_SUM,         ARG_NONE},
  {"concat",      OP_CONCAT,      ARG_NONE},
  {"sub",         OP_SUB,         ARG_NONE},
  {"mul",         OP_MUL,         ARG_NONE},
  {"div",         OP_DIV,         ARG_NONE},
  {"mod",         OP_MOD,         ARG_NONE},
  {"or",          OP_OR,          ARG_NONE},
  {"and",         OP_AND,         ARG_NONE},
  {"andb",        OP_ANDB,        ARG_NONE},
  {"orb",         OP_ORB,         ARG_NONE},
  {"not",         OP_NOT,         ARG_NONE},
  {"notb",        OP_NOTB,        ARG_NONE},
  {"gt",          OP_GT,          ARG_NONE},
  {"lt",          OP_LT,          ARG_NONE},
  {"eq",          OP_EQ,          ARG_NONE},
  {"neq",         OP_NEQ,         ARG_NONE},
  {"ge",          OP_GE,          ARG_NONE},
  {"le",          OP_LE,          ARG_NONE},
  {"sgt",         OP_SGT,         ARG_NONE},
  {"slt",         OP_SLT,         ARG_NONE},
  {"seq",         OP_SEQ,         ARG_NONE},
  {"sneq",        OP_SNEQ,        ARG_NONE},
  {"sge",         OP_SGE,         ARG_NONE},
  {"sle",         OP_SLE,         ARG_NONE},
  {"jz",          OP_JZ,          ARG_NONE},
  {"jnz",         OP_JNZ,         ARG_NONE},
  {"call",        OP_CALL,        ARG_INT},
  {"ret",         OP_RET,         ARG_NONE},
  {"mova",        OP_MOVA,        ARG_STRING},
  {"pusha",       OP_PUSHA,       ARG_NONE},
  {"push",        OP_PUSH,        ARG_INT_OR_STRING},
  {"popa",        OP_POPA,        ARG_NONE},
  {"addsymtable", OP_ADDSYMTABLE, ARG_STRING},
  {"delsymtable", OP_DELSYMTABLE, ARG_INT},
  {"decsp",       OP_DECSP,       ARG_INT},
  {"incsp",       OP_INCSP,       ARG_INT},
  {"startform",   OP_STARTFORM,   ARG_NONE},
  {"starttarget", OP_STARTTARGET, ARG_NONE},
  {"addform",     OP_ADDFORM,     ARG_NONE},
  {"endform",     OP_ENDFORM,     ARG_NONE},
  {"stop",        OP_STOP,        ARG_NONE},
  {"runtarget",   OP_RUNTARGET,   ARG_NONE},
  {"crontask",    OP_CRONTASK,    ARG_INT},
  {"task",        OP_TASK,        ARG_NONE},
  {"target",      OP_TARGET,      ARG_NONE},
  {"endtask",     OP_ENDTASK,     ARG_NONE},
  {"endtarget",   OP_ENDTARGET,   ARG_NONE},
  {"qlist",       OP_QLIST,       ARG_NONE},
  {"qbyid",       OP_QBYID,       ARG_NONE},
  {"query",       OP_QUERY,       ARG_NONE},
  {"onerror",     OP_ONERROR,     ARG_NONE},
  {"raise",       OP_RAISE,       ARG_NONE},
  {"setpath",     OP_SETPATH,     ARG_INT},
  {"getpath",     OP_GETPATH,     ARG_NONE},
  {"jmp",         OP_JMP,         ARG_INT},
  {"dblock",      OP_DBLOCK,      ARG_NONE},
  {"dbunlock",    OP_DBUNLOCK,    ARG_NONE},
  {"dbrollback",  OP_DBROLLBACK,  ARG_NONE},
  {"print",       OP_PRINT,       ARG_NONE},
  {"eprint",      OP_EPRINT,      ARG_NONE},
  {"input",       OP_INPUT,       ARG_NONE},
  {"show",        OP_SHOW,        ARG_NONE},
  {"fload",       OP_FLOAD,       ARG_NONE},
  {"fsave",       OP_FSAVE,       ARG_NONE},
  {"freaddir",    OP_FREADDIR,    ARG_NONE},
  {"toint",       OP_TOINT,       ARG_NONE},
  {"sind",        OP_SIND,        ARG_NONE},
  {"tohex",       OP_TOHEX,       ARG_NONE},
  {"sqrt",        OP_SQRT,        ARG_NONE},
  {"asc",         OP_ASC,         ARG_NONE},
  {"char",        OP_CHAR,        ARG_NONE},
  {"mid",         OP_MID,         ARG_NONE},
  {"len",         OP_LEN,         ARG_NONE},
  {"uc",          OP_UC,          ARG_NONE},
  {"index",       OP_INDEX,       ARG_NONE},
  {"arraypop",    OP_ARRAYPOP,    ARG_NONE},
  {"numel",       OP_NUMEL,       ARG_NONE},
  {"time",        OP_TIME,        ARG_NONE},
  {"date",        OP_DATE,        ARG_NONE},
  {"random",      OP_RANDOM,      ARG_NONE},
  {"sleep",       OP_SLEEP,       ARG_NONE},
  {"socket",      OP_SOCKET,      ARG_NONE},
  {"server",      OP_SERVER,      ARG_NONE},
  {"accept",      OP_ACCEPT,      ARG_NONE},
  {"sread",       OP_SREAD,       ARG_NONE},
  {"swrite",      OP_SWRITE,      ARG_NONE},
  {"dread",       OP_DREAD,       ARG_NONE},
  {"dwrite",      OP_DWRITE,      ARG_NONE},
  {0,             OPCODE_INVALID, ARG_NONE}
};

static ArgType argtype;

static int find_vm_instr(const char *op)
{ int i;
  argtype=ARG_NONE;
  if (!op) return -1;

  for (i=0; vm_instr_table[i].name; i++)
  { if (strcmp(op,vm_instr_table[i].name)==0)
    { argtype=vm_instr_table[i].argtype;
      return i;
    };
  };
  return -1;
}

static int vm_instr(EWBOpcode op)
{ int i;
  argtype=ARG_NONE;

  for (i=0; vm_instr_table[i].name; i++)
  { if (op==vm_instr_table[i].opcode)
    { argtype=vm_instr_table[i].argtype;
      return i;
    };
  };
  return -1;
}

#endif
