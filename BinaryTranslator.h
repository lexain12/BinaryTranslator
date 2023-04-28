#include <stdio.h>

#include "./language/common.h"

enum Location
{
    Register = 1,
    Memory   = 2,
    Stack    = 3,
};

struct Var_bt
{
    char* name;
    Location location;
    size_t pointer;
};

struct OpCode_bt
{
    unsigned int operation:5;
    unsigned int imm:1;
    unsigned int reg:1;
    unsigned int mem:1;
};

struct Cmd_bt
{
    OpCode_bt opCode;
    Var_bt    operator1;
    Var_bt    operator2;
    Var_bt    dest;
};

struct Func_bt
{
    Var_bt* varArray;
    char*  name;
    Cmd_bt* cmdArray;
};

// Elements with nullptr in name are needed in the end of array
struct BinaryTranslator
{
    Func_bt* funcArray;
    Var_bt*  globalVars;
    size_t   BT_ip;
    Func_bt* x86_array;
};

//----------------------------------------------------------------------------

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator);

//----------------------------------------------------------------------------
