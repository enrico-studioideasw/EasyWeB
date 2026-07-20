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
  OP_ADDTOFORM,
  OP_SENDFORM,
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
  OP_GETPATH
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
  {"addtoform",   OP_ADDTOFORM,   ARG_NONE},
  {"sendform",    OP_SENDFORM,    ARG_NONE},
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
