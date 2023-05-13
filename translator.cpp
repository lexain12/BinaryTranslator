#include "./language/common.h"
#include "BinaryTranslator.h"
#include <bits/types/FILE.h>
#include <cstdint>
#include <cstdio>

#define Dumpx86Buf(binTranslator, start, end) \
    printf ("Called from %s\n", __PRETTY_FUNCTION__);\
    dumpx86Buf(binTranslator, start, end);

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
    Dumpx86Buf(binTranslator, 0, 16);

    *(uint64_t*)(binTranslator->x86_array + binTranslator->BT_ip) = cmd.code;
    binTranslator->BT_ip += cmd.size;

    Dumpx86Buf(binTranslator, 0, 16);
}

static inline void writeNumIntoArray (BinaryTranslator* binTranslator, int Number)
{
    Dumpx86Buf(binTranslator, 0, 30);
    while (Number > 0)
    {
        printf ("HERE %x\n", Number & 0xFF);
        *(binTranslator->x86_array + binTranslator->BT_ip) = (unsigned char) Number & 0xff;
        Number >>= 8;
    }

    binTranslator->BT_ip += 4;
    Dumpx86Buf(binTranslator, 0, 30);
}

static inline void writePushPop (BinaryTranslator* binTranslator, size_t reg)
{

}

static inline void write_mov_xmm_rsp(BinaryTranslator* binTranslator, int numberOfXmm, uint64_t offset)
{
    Dumpx86Buf(binTranslator, 0, 16);
    x86_cmd mov_xmm_rsp =
    {
        .code = MOV_XMM_RSP + offset,
        .size = SIZE_MOV_XMM_RSP,
    };

    switch (numberOfXmm)
    {
        case 0:
            mov_xmm_rsp.code += 0x1000000 * XMM0_MASK;

            break;

        case 1:
            mov_xmm_rsp.code += 0x1000000 * XMM1_MASK;
            break;
    }

    writeCmdIntoArray(binTranslator, mov_xmm_rsp);
}

static inline void write_mov_rsp_xmm (BinaryTranslator* binTranslator, int numberOfXmm, uint64_t offset)
{
    Dumpx86Buf(binTranslator, 0, 16);
    x86_cmd mov_rsp_xmm =
    {
        .code = MOV_RSP_XMM + offset,
        .size = SIZE_MOV_XMM_RSP,
    };

    switch (numberOfXmm)
    {
        case 0:
            mov_rsp_xmm.code += 0x10000 * XMM0_MASK;

            break;

        case 1:
            mov_rsp_xmm.code += 0x10000 * XMM1_MASK;
            break;
    }

    writeCmdIntoArray(binTranslator, mov_rsp_xmm);
}

static inline void write_add_rsp (BinaryTranslator* binTranslator, uint64_t offset)
{
    Dumpx86Buf(binTranslator, 0, 16);
    x86_cmd cmd =
    {
        .code = ADD_RSP + offset * 0x1000000,
        .size = SIZE_ADD_RSP,
    };

    writeCmdIntoArray(binTranslator, cmd);
}

static inline void write_sub_rsp (BinaryTranslator* binTranslator, uint64_t offset)
{
    Dumpx86Buf(binTranslator, 0, 16);
    x86_cmd cmd =
    {
        .code = SUB_RSP + offset * 0x1000000,
        .size = SIZE_ADD_RSP,
    };

    writeCmdIntoArray(binTranslator, cmd);
}

static inline void dumpOperatorToAsm (FILE* fileptr, BinaryTranslator* binTranslator, Op_bt* op, int numOfOp)
{
    Dumpx86Buf(binTranslator, 0, 16);
    const char* regArr[] = {"rbx", "rcx"};

    switch (op->type)
    {
        case Pointer_t:
            write_mov_xmm_rsp(binTranslator, numOfOp, 0);
            fprintf (fileptr, "ja %s\n", op->value.block->name);
            break;

        case Var_t:
            switch (op->value.var->location)
            {
                case Register:
                    break;

                case Memory:
                    fprintf (fileptr, "movsd xmm%d, [r9 - %lu]\n", numOfOp, op->value.var->offset);
                    break;

                case Stack:
                    fprintf (fileptr, "movsd xmm%d, [rsp]\n", numOfOp);
                    fprintf (fileptr, "\tadd rsp, 8\n");
                    break;
            }
            break;

        case Num_t:
            binTranslator->x86_array[binTranslator->BT_ip] = PUSH_32b;
            binTranslator->BT_ip += SIZE_PUSH_32b;

            Dumpx86Buf(binTranslator, 0, 16);

            writeNumIntoArray(binTranslator, op->value.num);
            write_mov_xmm_rsp(binTranslator, numOfOp, 0);

            write_add_rsp (binTranslator, 8);

            Dumpx86Buf(binTranslator, 0, 16);

            fprintf (fileptr, "push %d\n", op->value.num);
            fprintf (fileptr, "\tmovsd xmm%d, [rsp]\n", numOfOp);
            fprintf (fileptr, "\tadd rsp, 8\n");
            break;
    }
}

static void translateBaseMath (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    x86_cmd x86_cmd = {};
    fprintf (fileptr, "\n\t");
    dumpOperatorToAsm(fileptr, binTranslator, cmd.operator1, 0);
    fprintf (fileptr, "\t");
    dumpOperatorToAsm(fileptr, binTranslator, cmd.operator2, 1);
    fprintf (fileptr, "\t");

    switch (cmd.opCode.operation)
    {
        case OP_ADD:
            fprintf (fileptr, "addsd xmm0, xmm1\n");
            x86_cmd.code = ARITHM_XMM0_XMM1 + 0x100 * ADD_MASK;
            break;

        case OP_SUB:
            fprintf (fileptr, "subsd xmm0, xmm1\n");
            x86_cmd.code = ARITHM_XMM0_XMM1 + 0x100 * SUB_MASK;
            break;

        case OP_MUL:
            fprintf (fileptr, "mulsd xmm0, xmm1\n");
            x86_cmd.code = ARITHM_XMM0_XMM1 + 0x100 * MUL_MASK;
            break;

        case OP_DIV:
            fprintf (fileptr, "divsd xmm0, xmm1\n");
            x86_cmd.code = ARITHM_XMM0_XMM1 + 0x100 * DIV_MASK;
            break;

    }

    x86_cmd.size = SIZE_ARITHM_XMM;
    writeCmdIntoArray(binTranslator, x86_cmd);

    write_mov_rsp_xmm(binTranslator, 0, 0);
    x86_cmd.code = ADD_RSP + 0x08;
    x86_cmd.size = SIZE_ADD_RSP;
    writeCmdIntoArray(binTranslator, x86_cmd);

    write_sub_rsp(binTranslator, 0);
    fprintf (fileptr, "\tsub rsp, 0x08\n");
    fprintf (fileptr, "\tmovsd [rsp], xmm0\n");
}

static inline void translateIf (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    x86_cmd x86_cmd = {};

    write_mov_xmm_rsp(binTranslator, 0, 0);
    fprintf (fileptr, "\t movsd xmm0, [rsp]\n");
    write_mov_xmm_rsp (binTranslator, 1, 0);
    fprintf (fileptr, "\t movsd xmm1, [rsp]\n");

    x86_cmd.code = CMP_XMM0_XMM1;
    x86_cmd.code = SIZE_CMP_XMM;
    writeCmdIntoArray(binTranslator, x86_cmd);
    fprintf(fileptr, "\t ucomisd xmm0, xmm1\n");

    fprintf (fileptr, "\t");
    dumpOperatorToAsm(fileptr, binTranslator, cmd.operator1, 0);
    fprintf (fileptr, "\t jmp %s\n", cmd.operator1->value.block->name);

    if (cmd.operator2)
    {
        x86_cmd.code = JMP_OP;
        x86_cmd.size = SIZE_JMP;
        writeCmdIntoArray(binTranslator, x86_cmd);
        fprintf (fileptr, "\t jmp %s\n", cmd.operator2->value.block->name);
    }
}

static inline void translateEq (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "movsd xmm0, [rsp]\n");
    fprintf (fileptr, "movsd [r9 - %lu], xmm0\n", cmd.dest->value.var->offset);
}

static inline void translateRet (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "Ret\n");
}

static inline void translateJmp (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "jmp %s\n", cmd.operator1->value.block->name);
}

static inline void translateParamOut (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    fprintf (fileptr, "movsd xmm0, [rsp]\n");
    fprintf (fileptr, "add rsp, 8\n");
    fprintf (fileptr, "movsd [r9 - %lu], xmm0\n", cmd.dest->value.var->offset);
}

static inline void translateParamIn (FILE* fileptr, BinaryTranslator* binTranslator, Cmd_bt cmd)
{
    switch (cmd.operator1->type)
    {
        case Num_t:
            fprintf (fileptr, "push %d\n", cmd.operator1->value.num);
            break;

        case Var_t:
            fprintf (fileptr, "movsd xmm0, [r9 - %d]\n", cmd.operator1->value.var->offset);
            fprintf (fileptr, "sub rsp, 8\n");
            fprintf (fileptr, "movsd [rsp], xmm0\n" );

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
    dumpBlockToAsm (fileptr, binTranslator, &function->blockArray[0]);

    for (int i = 1; i < function->blockArraySize; i++)
    {
        fprintf (fileptr, "%s:\n", function->blockArray[i].name);
        dumpBlockToAsm (fileptr, binTranslator, &function->blockArray[i]);
    }
}

void dumpIRToAsm (const char* fileName, BinaryTranslator* binTranslator)
{
    FILE* mainFilePtr = fopen (fileName, "wb");
    FILE* fileptr = fopen ("DebugAsm.s", "w");

    for (int i = 0; i < binTranslator->funcArraySize; i++)
    {
        dumpFunctionToAsm (fileptr, binTranslator, &binTranslator->funcArray[i]);
    }

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
