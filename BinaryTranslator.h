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

struct Op_bt  // operator type
{
    Type type;
    int num;
    Var_bt* var;
};

struct OpCode_bt
{
    unsigned int operation:6;
    unsigned int imm:1;
    unsigned int reg:1;
    unsigned int mem:1;
};

struct Cmd_bt
{
    OpCode_bt opCode;
    Op_bt*   operator1;
    Op_bt*   operator2;
    Op_bt*   dest;
    char*    name;
};

struct Func_bt
{
    char*  name;
    Var_bt* varArray;
    Cmd_bt* cmdArray;
    int varArraySize;
    int cmdArraySize;
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
void dumpIR (const char* fileName, const BinaryTranslator* binTranslator);

//----------------------------------------------------------------------------

