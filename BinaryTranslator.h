#include <cstddef>
#include <stdio.h>
#include <cstdint>

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
    char*    name;
    Location location;
    int offset;
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
    char name[15];
    Cmd_bt* cmdArray;
    size_t  cmdArraySize;
    size_t  cmdArrayCapacity;
    size_t  codeOffset;
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
    size_t varArraySize;
    size_t varArrayCapacity;
    size_t numberOfTempVar;
    size_t blockArraySize;
    size_t blockArrayCapacity;
};

// Elements with nullptr in name are needed in the end of array
struct BinaryTranslator
{
    Func_bt* funcArray;
    size_t   funcArraySize;
    Var_bt*  globalVars;
    size_t   BT_ip;
    size_t x86_arraySize;
    NameTable nameTable;
    unsigned char* x86_array;
    unsigned char x86Mem_array[512];
};

struct x86_cmd
{
    uint64_t code;
    uint64_t size;
};

enum REG_NUM
{
    RAX = 0x00,
    RCX = 0x01,
    RDX = 0x02,
    RBX = 0x03,
    RDI = 0x07,
};

enum OPCODES_x86 : uint64_t // everything reversed
{

// Watch OPCODE_MASKS if you want to construct one of the following cmds.
// ATTENTION: all opcodes are written in reverse order.
    SUB_RAX_RBX = 0xD82948,
    ADD_RAX_RBX = 0xD80148,
    MUL_RBX     = 0xE3F748,
    DIV_RBX     = 0xF3F748,

    // mov [r9 + %d], %d
    // A number after
    MOV_MEM_IMM   = 0xFF41C749,
    //                ^ -offset
    //                mov [r9 + offset], rax
    MOV_MEM_REG   = 0xFF008949,
    //                ^ ff - offset to make [r9 - offset]
    MOV_REG_MEM   = 0xFF008B49,
    MOV_REG_IMM   = 0xB8,

    PUSH_REG = 0x50, //    push/pop r?x
    POP_REG = 0x58,  //              ^--- add 0, 1, 2, 3 to get rax, rcx, rdx or rbx
    MOV_R9_IMM64 = 0xB949,

            //   ^-- by applying bit mask, can get all types f jmp

// Constant expressions, no need for bit masks

    CALL_OP = 0xE8,     // call <32b ptr>
    RET_OP = 0xC3,      // ret

    JMP_OP = 0xE9,      // jmp <32b ptr>
    COND_JMP = 0x000F,

    PUSH_32b = 0x68,

    CMP_RAX_RBX = 0xD83948,

    MOV_RDI_RAX = 0xC78948,
    MOV_RCX_RAX = 0xC88948,
    MOV_RCX_RBX = 0xCB8948,

    PUSH_R10 = 0x5241,
    PUSH_R9  = 0x5141,
    POP_R9   = 0x5941,
    POP_R10  = 0x5a41,
    PUSH_RBP = 0x55,
    PUSH_RSP = 0x54,
    POP_RBP = 0x5D,
    POP_RSP = 0x5C,

    ADD_R9_IMM = 0xC18149,
    SUB_R9_IMM = 0xE98149,
    MOV_RDI_R9 = 0xCF894C,
};

enum OPCODE_SIZES
{
    SIZE_ARITHM   = 3,

    SIZE_CMP_RAX_RBX = 3,
    SIZE_PUSH_32b = 1,

    SIZE_MOV_MEM_IMM = 4,
    SIZE_MOV_REG_IMM = 1,

    SIZE_PUSH_REG    = 1,
    SIZE_POP_REG     = 1,

    SIZE_MOV_REG_REG = 3,
    SIZE_MOV_RCX_RAX = 3,
    SIZE_MOV_RCX_RBX = 3,

    SIZE_JMP_OP     = 1,
    SIZE_COND_JMP   = 2,
    SIZE_PUSH_R9    = 2,
    SIZE_POP_R9    = 2,
    SIZE_PUSH_R10   = 2,
    SIZE_POP_R10    = 2,
    SIZE_RET_OP     = 1,
    SIZE_CALL_OP    = 1,

    SIZE_PUSH_RBP = 1,
    SIZE_PUSH_RSP = 1,
    SIZE_POP_RBP = 1,
    SIZE_POP_RSP = 1,

    SIZE_MOV_RDI_RAX = 3,
    SIZE_ADD_R9_IMM = 3,
    SIZE_SUB_R9_IMM = 3,
    SIZE_MOV_R9_IMM64 = 2,
    SIZE_MOV_RDI_R9   = 3,
};


enum OPCODE_MASKS : uint64_t
{
    MOV_RAX_MASK = 0x41,
    MOV_RBX_MASK = 0x59,
    MOV_RCX_MASK = 0x49,
    XMM0_MASK = 0x44,   // masks for work with xmm registers
    XMM1_MASK = 0x4c,

    JAE_MASK = 0x83,
    JE_MASK = 0x84,     // Conditional jumps
    JNE_MASK = 0x85,
    JA_MASK = 0x87,
    JGE_MASK = 0x8d,
    JG_MASK = 0x8f,

};


//----------------------------------------------------------------------------

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator);
void dumpIR (const char* fileName, const BinaryTranslator* binTranslator);
void dumpIRToAsm (const char* fileName, BinaryTranslator* binTranslator);
void firstIteration (BinaryTranslator* binTranslator);
void dumpBTtable (NameTable nametable);
void startProg (BinaryTranslator* binTranslator);

//----------------------------------------------------------------------------

