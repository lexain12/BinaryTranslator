#include "./language/common.h"
#include "BinaryTranslator.h"
#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/types.h>

static inline void writeCmdIntoArray (BinaryTranslator* binTranslator, x86_cmd cmd);

#define Dumpx86Buf(binTranslator, start, end) \
    printf ("Called from %s\n", __PRETTY_FUNCTION__);\
    dumpx86Buf(binTranslator, start, end);

#define SimpleCMD(name) writeCmdIntoArray( binTranslator, {.code = name, .size = SIZE_##name});

void dumpx86Buf (BinaryTranslator* binTranslator, size_t start, size_t end)
{
    printf ("dump current ip = %lu, current size = %lu\n", binTranslator->BT_ip, binTranslator->x86_arraySize);

    if (end >= binTranslator->BT_ip)
        end = binTranslator->BT_ip;

    for (size_t i = 0; i < end; i++)
    {
        printf ("%02x ", binTranslator->x86_array[i]);
    }
    printf ("\n");
}

static inline void writeCmdIntoArray (BinaryTranslator* binTranslator, x86_cmd cmd)
{

    fprintf (stderr, "ERROR CHECKING %lu %lu\n", cmd.size, cmd.code);
    *(uint64_t*)(binTranslator->x86_array + binTranslator->BT_ip) = cmd.code;
    binTranslator->BT_ip += cmd.size;

}

static inline void writeNumIntoArray (BinaryTranslator* binTranslator, int number)
{
    while (number > 0)
    {
        printf ("HERE %x\n", number & 0xFF);
        *(binTranslator->x86_array + binTranslator->BT_ip) = (unsigned char) number & 0xff;
        number >>= 8;
    }

    binTranslator->BT_ip += 4;
}

static inline void write_push_reg (BinaryTranslator* binTranslator, REG_NUM reg)
{
    x86_cmd cmd =
    {
        .code = PUSH_REG + reg,
        .size = SIZE_PUSH_REG,
    };

    writeCmdIntoArray (binTranslator, cmd);
}

static inline void write_pop_reg (BinaryTranslator* binTranslator, REG_NUM reg)
{
    x86_cmd cmd =
    {
        .code = POP_REG + reg,
        .size = SIZE_POP_REG,
    };
    fprintf (stderr, "ERROR IS HERE %lu\n", cmd.code);
    writeCmdIntoArray (binTranslator, cmd);
}

static inline void write_push_num (BinaryTranslator* binTranslator, int number)
{
    x86_cmd cmd =
    {
        .code = PUSH_32b,
        .size = SIZE_PUSH_32b,
    };

    writeCmdIntoArray (binTranslator, cmd);
    writeNumIntoArray (binTranslator, number);
}

static inline void write_mov_mem_imm (BinaryTranslator* binTranslator, size_t offset, int number)
{

    x86_cmd cmd = {
        .code = MOV_MEM_IMM - (offset - 1) * 0x1000000,
        .size = SIZE_MOV_MEM_IMM ,
    };

    writeCmdIntoArray (binTranslator, cmd);
    writeNumIntoArray (binTranslator, number);
}

static inline void write_mov_mem_reg (BinaryTranslator* binTranslator, size_t offset, REG_NUM reg)
{
    fprintf (stderr, "%d\n", reg);
    uint64_t regMasks[] = {MOV_RAX_MASK, MOV_RCX_MASK, 0, MOV_RBX_MASK};
    x86_cmd cmd =
    {
        .code = MOV_MEM_REG + regMasks[reg] * 0x10000 - (offset - 1) * 0x1000000, // offset - 1 because of universal cmd
        .size = SIZE_MOV_MEM_IMM ,
    };

    writeCmdIntoArray(binTranslator, cmd);
}

static inline void write_mov_reg_mem (BinaryTranslator* binTranslator, size_t offset, REG_NUM reg)
{
    int regMasks[] = {MOV_RAX_MASK, MOV_RCX_MASK, 0, MOV_RBX_MASK};
    x86_cmd cmd =
    {
        .code = MOV_REG_MEM + regMasks[reg] * 0x10000 - (offset - 1) * 0x1000000,
        .size = SIZE_MOV_MEM_IMM ,
    };

    writeCmdIntoArray(binTranslator, cmd);
}

static inline void write_mov_reg_num (BinaryTranslator* binTranslator, REG_NUM reg, int number)
{
    x86_cmd cmd =
    {
        .code = MOV_REG_IMM + reg,
        .size = SIZE_MOV_REG_IMM,
    };

    writeCmdIntoArray (binTranslator, cmd);
    writeNumIntoArray (binTranslator, number);
}


static inline void dumpOperatorToAsm (FILE* fileptr, BinaryTranslator* binTranslator, Op_bt* op, REG_NUM reg)
{
    const char* regArr[] = {"rax", "rcx", "rdx","rbx"};

    switch (op->type)
    {
        case Pointer_t:
            fprintf (fileptr, "ja %s\n", op->value.block->name);
            break;

        case Var_t:
            switch (op->value.var->location)
            {
                case Register:
                    fprintf (fileptr, "mov %s, rcx\n", regArr[reg]);
                    break;

                case Memory:
                    fprintf (fileptr, "mov %s, [r9 - %lu]\n", regArr[reg], op->value.var->offset);
                    write_mov_reg_mem (binTranslator, op->value.var->offset, reg);
                    break;

                case Stack:
                    fprintf (fileptr, "pop %s\n", regArr[reg]);
                    write_pop_reg (binTranslator, reg);
                    break;
            }
            break;

        case Num_t:
            fprintf (fileptr, "mov %s, %d\n", regArr[reg], op->value.num);
            write_mov_reg_num(binTranslator, reg, op->value.num);
            break;
    }
}

static void translateBaseMath (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    x86_cmd x86_cmd = {};
    fprintf (fileptr, "\n ;Arithm");
    fprintf (fileptr, "\n\t");
    dumpOperatorToAsm(fileptr, binTranslator, cmd.operator1, RAX);
    fprintf (fileptr, "\t");
    dumpOperatorToAsm(fileptr, binTranslator, cmd.operator2, RBX);
    fprintf (fileptr, "\t");

    switch (cmd.opCode.operation)
    {
        case OP_ADD:
            fprintf (fileptr, "add rax, rbx\n");
            x86_cmd.code = ADD_RAX_RBX;
            break;

        case OP_SUB:
            fprintf (fileptr, "sub rax, rbx\n");
            x86_cmd.code = SUB_RAX_RBX;
            break;

        case OP_MUL:
            fprintf (fileptr, "mul rbx\n");
            x86_cmd.code = MUL_RBX;
            break;

        case OP_DIV:
            fprintf (fileptr, "div rbx\n");
            x86_cmd.code = DIV_RBX;
            break;

    }

    x86_cmd.size = SIZE_ARITHM;
    writeCmdIntoArray(binTranslator, x86_cmd);

    fprintf (fileptr, "\tpush rax\n");
    write_push_reg (binTranslator, RAX);
    fprintf (fileptr, ";end of Arithm\n");
}

static inline void translateIf (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    x86_cmd x86_cmd = {};

    dumpOperatorToAsm(fileptr, binTranslator, cmd.dest, RAX);
    fprintf (fileptr, "\t xor rbx, rbx\n");
    write_mov_reg_num(binTranslator, RBX, 0);

    fprintf(fileptr, "\t cmp rax, rbx\n");
    SimpleCMD(CMP_RAX_RBX);

    fprintf (fileptr, "\t");
    dumpOperatorToAsm(fileptr, binTranslator, cmd.operator1, RAX);

    if (cmd.operator2)
    {
        fprintf (fileptr, "\t jmp %s\n", cmd.operator2->value.block->name);
    }
}

static inline void translateEq (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    switch (cmd.operator1->type)
    {
        case Var_t:
            switch (cmd.operator1->value.var->location)
            {
                case Register:
                    fprintf (fileptr, "mov [r9 - %d], rcx\n", cmd.dest->value.var->offset);
                    write_mov_reg_mem (binTranslator, cmd.dest->value.var->offset, RCX);
                    break;

                case Stack:
                    fprintf (fileptr, "pop rax\n");
                    write_pop_reg (binTranslator, RAX);
                    fprintf (fileptr, "mov [r9 - %d], rax\n", cmd.dest->value.var->offset);
                    write_mov_mem_reg(binTranslator, cmd.dest->value.var->offset, RAX);
                    break;

                case Memory:
                    fprintf (fileptr, "mov rax, [r9 + %d]\n", cmd.dest->value.var->offset);
                    write_mov_mem_reg (binTranslator, cmd.dest->value.var->offset, RAX);
                    fprintf (fileptr, "mov [r9 + %d], rax\n", cmd.operator1->value.var->offset);
                    write_mov_mem_reg (binTranslator, cmd.operator1->value.var->offset, RAX);
                    break;
            }
            break;

        case Num_t:
            fprintf (fileptr, "mov qword [r9 - %d], %d\n", cmd.dest->value.var->offset, cmd.operator1->value.num);
            write_mov_mem_imm (binTranslator, cmd.dest->value.var->offset, cmd.operator1->value.num);
            break;
    }
}

static inline void translateRet (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    switch (cmd.operator1->type)
    {
        case Var_t:
            switch (cmd.operator1->value.var->location)
            {
                case Stack:
                    fprintf (fileptr, "pop rcx\n");
                    write_pop_reg (binTranslator, RCX);
                    break;

                case Memory:
                    fprintf (fileptr, "mov rcx, [r9 - %d]\n", cmd.operator1->value.var->offset);
                    write_mov_reg_mem (binTranslator, cmd.operator1->value.var->offset, RCX);
                    break;

            }
            break;

        case Num_t:
            fprintf (fileptr, "mov rcx, %d\n", cmd.operator1->value.num);
            write_mov_reg_num (binTranslator, RCX, cmd.operator1->value.num);
            break;
    }
}

static inline void translateJmp (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "jmp %s\n", cmd.operator1->value.block->name);
}

static inline void translateParamOut (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "pop r10\n");
    SimpleCMD(POP_R10);
    fprintf (fileptr, "pop rax\n");
    write_pop_reg (binTranslator, RAX);
    fprintf (fileptr, "mov [r9 - %d], rax\n", cmd.dest->value.var->offset);
    write_mov_mem_reg (binTranslator, cmd.dest->value.var->offset, RAX);
    fprintf (fileptr, "push r10\n");
    SimpleCMD(PUSH_R10);
}

static inline void translateParamIn (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    switch (cmd.operator1->type)
    {
        case Num_t:
            fprintf (fileptr, "push %d\n", cmd.operator1->value.num);
            write_push_num (binTranslator, cmd.operator1->value.num);
            break;

        case Var_t:
            if (cmd.operator1->value.var->location == Memory)
            {
                fprintf (fileptr, "mov rax, [r9 - %d]\n", cmd.operator1->value.var->offset);
                write_mov_reg_mem (binTranslator, cmd.operator1->value.var->offset, RAX);
                fprintf (fileptr, "push rax\n" );
                write_push_reg (binTranslator, RAX);
            }

            break;
    }
}

static inline void translateCall (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "call %s\n", cmd.operator1->value.block->name);
}

static void dumpBlockToAsm (FILE* fileptr, BinaryTranslator* binTranslator, Block_bt* block)
{
    for (int i = 0; i < block->cmdArraySize; i++)
    {
        fprintf (fileptr, "\t");
        Cmd_bt cmd = block->cmdArray[i];

        x86_cmd x86_cmd = {};
        switch (cmd.opCode.operation)
        {
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
                translateBaseMath (fileptr, binTranslator, cmd);
                break;

            case OP_IF:
                translateIf (fileptr, binTranslator, cmd);
                break;

            case OP_EQ:
                translateEq (fileptr, binTranslator, cmd);
                break;

            case OP_RET:
                translateRet (fileptr, binTranslator, cmd);
                break;

            case OP_JMP:
                translateJmp (fileptr, binTranslator, cmd);
                break;

            case OP_PAROUT:
                translateParamOut (fileptr, binTranslator, cmd);
                break;

            case OP_PARIN:
                translateParamIn (fileptr, binTranslator, cmd);
                break;

            case OP_CALL:
                translateCall (fileptr, binTranslator, cmd);
                break;
        }
    }
}

static void dumpFunctionToAsm (FILE* fileptr, BinaryTranslator* binTranslator, Func_bt* function)
{
    fprintf (fileptr, "%s:\n", function->blockArray[0].name);
    fprintf (fileptr, "add r9, %lu\n", (function->varArraySize - function->numberOfTempVar)*8);
    SimpleCMD(ADD_R9_IMM);
    writeNumIntoArray(binTranslator, (function->varArraySize - function->numberOfTempVar)*8);
    dumpBlockToAsm (fileptr, binTranslator, &function->blockArray[0]);

    for (int i = 1; i < function->blockArraySize; i++)
    {
        fprintf (fileptr, "%s:\n", function->blockArray[i].name);
        dumpBlockToAsm (fileptr, binTranslator, &function->blockArray[i]);
    }
    fprintf (fileptr, "sub r9, %lu\n", (function->varArraySize - function->numberOfTempVar)*8);
    SimpleCMD(SUB_R9_IMM);
    writeNumIntoArray(binTranslator, (function->varArraySize - function->numberOfTempVar)*8);
    fprintf (fileptr, "ret\n");
    SimpleCMD(RET_OP);
}

void dumpStart (FILE* fileptr)
{
    fprintf (fileptr, "section .text\n");
    fprintf (fileptr, "global _start\n");
    fprintf (fileptr, "_start:\n");
    fprintf (fileptr, "lea r9, Buf\n");
    fprintf (fileptr, "\tcall main\n");
    fprintf (fileptr, "mov rax, 0x3c\n");
    fprintf (fileptr, "xor rdi, rdi\n");
    fprintf (fileptr, "syscall\n");
}

void dumpEnd (FILE* fileptr)
{
    fprintf (fileptr, "section .data\n");
    fprintf (fileptr, "Buf: times 512 db 0\n");
}

void dumpIRToAsm (const char* fileName, BinaryTranslator* binTranslator)
{
    FILE* mainFilePtr = fopen (fileName, "wb");
    FILE* fileptr = fopen ("DebugAsm.s", "w");
    dumpStart(fileptr);

    for (int i = 0; i < binTranslator->funcArraySize; i++)
    {
        dumpFunctionToAsm (fileptr, binTranslator, &binTranslator->funcArray[i]);
    }

    dumpEnd(fileptr);

    Dumpx86Buf(binTranslator, 0, binTranslator->BT_ip);

    fwrite(binTranslator->x86_array, sizeof(unsigned char), binTranslator->x86_arraySize, mainFilePtr);
}

void firstIteration (BinaryTranslator* binTranslator)
{
    size_t ip = 0;

    for (int i = 0; i < binTranslator->funcArraySize; i++)
    {
        for (int j = 0; j < binTranslator->funcArray[i].blockArraySize; j ++)
        {
            for (int k = 0; k < binTranslator->funcArray[i].blockArray[j].cmdArraySize; k++)
            {
                ip+=4*8;
            }

            binTranslator->funcArray[i].blockArray[j].codeOffset = ip;
        }
    }

    binTranslator->x86_arraySize = ip;
    binTranslator->x86_array = (unsigned char*) calloc (ip, sizeof (char));
    binTranslator->BT_ip = 0;
}

void secondIteration (BinaryTranslator* binTranslator)
{

}
