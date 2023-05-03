#include <stdio.h>

#include "./language/common.h"

struct Block_bt;
struct Op_bt;  // operator type

enum Location
{
    Register = 1,
    Memory   = 2,
    Stack    = 3,
};

struct OpCode_bt
{
    unsigned int operation:6;
    unsigned int imm:1;
    unsigned int reg:1;
    unsigned int mem:1;
};

struct Var_bt
{
    char* name;
    Location location;
    size_t pointer;
};

struct Cmd_bt
{
    OpCode_bt opCode;
    Op_bt*   operator1;
    Op_bt*   operator2;
    Op_bt*   dest;
};

struct Block_bt
{
    Cmd_bt* cmdArray;
    int     cmdArraySize;
    int     cmdArrayCapacity;
};

union Value_bt
{
    int num;
    Var_bt* var;
    Block_bt* block;
};

struct Op_bt  // operator type
{
    Type type;
    Value_bt value;
};

struct Func_bt
{
    char    name[15];
    Var_bt* varArray;
    Block_bt* blockArray;
    int varArraySize;
    int varArrayCapacity;
    int blockArraySize;
    int blockArrayCapacity;
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

