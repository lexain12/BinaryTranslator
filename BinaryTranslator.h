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
    unsigned char* x86_array;
    size_t x86_arraySize;
    NameTable* nameTable;
};

struct x86_cmd
{
    uint64_t code;
    size_t   size;
};

enum REG_NUM
{
    RAX = 0x00,
    RCX = 0x01,
    RDX = 0x02,
    RBX = 0x03,
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

    // mov [r10 + ?], rdi
    MOV_R10_RDI = 0xBA8949, // this must be followed with uint32 ptr

    // transforms double precision num in xmm0 to integer in rsi
    CVTSD2SI_RSI_XMM0 = 0xF02D0F48F2,

    SHL_RSI = 0x00E6C148,   // rsi *= 2^(?)
    //          ^-- how much bites to shift

    PUSH_REG = 0x50, //    push/pop r?x
    POP_REG = 0x58,  //              ^--- add 0, 1, 2, 3 to get rax, rcx, rdx or rbx

    COND_JMP = 0x000F,
            //   ^-- by applying bit mask, can get all types f jmp

// Constant expressions, no need for bit masks

    MOV_R10 = 0xBA49,   // mov r10, <64b ptr>. Begin of memory must be stored in R10
    CALL_OP = 0xE8,     // call <32b ptr>
    RET_OP = 0xC3,      // ret

    JMP_OP = 0xE9,      // jmp <32b ptr>

    PUSH_RSI = 0x56,    // push rsi
    PUSH_RDI = 0x57,    // push rdi
    PUSH_32b = 0x68,

    POP_RSI = 0x5E,     // pop rsi
    POP_RDI = 0x5F,     // pop rdi

    CMP_RDI_RSI = 0xf73948, // cmd rdi, rsi
    CMP_XMM0_XMM1 = 0xC12E0F66, // ucomisd xmm0, xmm1

    PUSH_ALL = 0x505152535241,  // push r10 - rax - ... - rdx
    POP_ALL = 0x5A415B5A5958,   // pop rdx - ... rax - r10

    MOV_RBP_RSP = 0xE48949,
    MOV_RSP_RBP = 0xE4894C,
    AND_RSP_FF = 0xF0E48348,

    MOV_RSI = 0xBE48, // mov rsi, (double num)
                      // next 8 bytes should be double number

    ADD_R10_RSI = 0xF20149,
    SUB_R10_RSI = 0xF22949,
    SQRTPD_XMM0_XMM0 = 0xC0510F66,   // get square root from xmm0 and store it in xmm0


    LEA_RDI_RSP = 0x00247C8D48 // lea rdi, [rsp + ?]
            //       ^----------------------------+

};


enum OPCODE_MASKS : uint64_t
{
    MOV_RAX_MASK = 0x41,
    MOV_RBX_MASK = 0x59,
    MOV_RCX_MASK = 0x49,
    XMM0_MASK = 0x44,   // masks for work with xmm registers
    XMM1_MASK = 0x4c,

    JE_MASK = 0x84,     // Conditional jumps
    JNE_MASK = 0x85,
    JG_MASK = 0x8f,
    JAE_MASK = 0x83,
    JGE_MASK = 0x8d,
    JA_MASK = 0x87,

};


enum OPCODE_SIZES
{
    SIZE_PUSH_RSI = 1,
    SIZE_PUSH_RDI = 1,
    SIZE_PUSH_32b = 1,
    SIZE_POP_RSI = 1,
    SIZE_POP_RDI = 1,

    SIZE_MOV_MEM_IMM = 4,
    SIZE_MOV_REG_IMM = 1,

    SIZE_PUSH_POP_All = 6,
    SIZE_AND_RSP = 4,

    SIZE_CMP_RSI_RDI = 3,
    SIZE_MOV_RSI     = 2,
    SIZE_MOV_R10 = 2,

    SIZE_PUSH_REG = 1,
    SIZE_POP_REG = 1,

    SIZE_ARITHM_XMM = 4,
    SIZE_MOV_XMM_RSP = 6,

    SIZE_ADD_RSP = 4,

    SIZE_MOV_REG_REG = 3,

    SIZE_JMP = 1,
    SIZE_COND_JMP = 2,
    SIZE_RET = 1,

    SIZE_CVTSD2SI = 5,
    SIZE_R10_RSI = 3,

    SIZE_SHL = 4,
    SIZE_SQRT = 4,

    SIZE_CMP_XMM = 4,
    SIZE_LEA_RDI_RSP = 5,
};


//----------------------------------------------------------------------------

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator);
void dumpIR (const char* fileName, const BinaryTranslator* binTranslator);
void dumpIRToAsm (const char* fileName, BinaryTranslator* binTranslator);
void firstIteration (BinaryTranslator* binTranslator);

//----------------------------------------------------------------------------

