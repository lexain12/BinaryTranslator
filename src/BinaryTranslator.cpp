#pragma GCC diagnostic ignored "-Wswitch-enum"

#include <cstddef>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>

#include "../include/BinaryTranslator.h"
#include "../language/common.h"
#include "../language/readerLib/functions.h"
#include "../include/translator.h"

extern const char* FullOpArray[];

static void parseStToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function);
static Op_bt* parseCallToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function);
void NodeDtor(Node* node);

static size_t NumberOfTempVars = 0;


void binTranslatorDtor (BinaryTranslator* binTranslator)
{
    NodeDtor(binTranslator->tree);
    free (binTranslator->x86_array);
    free (binTranslator->funcArray);
    free (binTranslator->globalVars);
    free (binTranslator->nameTable.data);
}
// DUMPS
//----------------------------------------

static void printOperand (FILE* fileptr, const Op_bt* op)
{
    assert (op != NULL);

    switch (op->type)
    {
        case Num_t:
            fprintf(fileptr, "%-10d", op->value.num);
            break;

        case Var_t:
            fprintf(fileptr, "%-10s", op->value.var->name);
            break;

        case Pointer_t:
            fprintf(fileptr, "%-10s", op->value.block->name);
            break;

        default:
            assert (0);
    }
}

static void dumpIRBlock (FILE* fileptr, const Block_bt block)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    fprintf (fileptr, "cmdArrayCapacity = %lu, cmdArraySize = %lu\n", block.cmdArrayCapacity, block.cmdArraySize);
    fprintf (fileptr, "%12s: CMD        op1        op2        dest\n\t{\n", block.name);
    for (size_t i = 0; i < block.cmdArraySize; i++)
    {
        fprintf (fileptr, "\t\t\t\t  %-10s ", FullOpArray[block.cmdArray[i].opCode.operation]);

        if (block.cmdArray[i].operator1)
        {
            printOperand(fileptr, block.cmdArray[i].operator1);
            fprintf (fileptr, " ");
        }
        else
            fprintf(fileptr, "%11c", ' ');

        if (block.cmdArray[i].operator2)
        {
            printOperand(fileptr, block.cmdArray[i].operator2);
            fprintf (fileptr, " ");
        }
        else
            fprintf(fileptr, "%11c", ' ');
        if (block.cmdArray[i].dest)
        {
            printOperand(fileptr, block.cmdArray[i].dest);
            fprintf (fileptr, " ");
        }
        else
            fprintf(fileptr, "%11c", ' ');
        fprintf(fileptr, "\n");

    }
    fprintf(fileptr, "\t}\n\n");
}

static void dumpIRFuncion (FILE* fileptr, const Func_bt function)
{
    assert (fileptr != NULL);
    assert (function.blockArray != NULL);
    assert (function.varArray != NULL);
    assert (function.name != NULL);

    fprintf (fileptr, "%s\n", function.name);

    fprintf (fileptr, "Variables:\n{\n");
    for (size_t i = 0; function.varArray[i].name != NULL; i++)
    {
        fprintf(fileptr, "\t%s\n", function.varArray[i].name);
    }
    fprintf (fileptr, "}\n");

    fprintf (fileptr, "blockArraySize = %lu, blockArrayCapacity = %lu\n", function.blockArraySize, function.blockArrayCapacity);
    fprintf (fileptr, "blocks: \n{\n");
    for (size_t i = 0; i < function.blockArraySize; i++)
    {
        fprintf (fileptr, "\t");
        dumpIRBlock(fileptr, function.blockArray[i]);
    }
    fprintf(fileptr, "}\n\n");
}

void dumpIR (const char* fileName, const BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen (fileName, "w");
    assert (fileptr != NULL);

    setvbuf(fileptr, NULL, _IONBF, 0);

    if (binTranslator->globalVars != NULL)
    {
        fprintf (fileptr, "Variables:\n");
        for (size_t i = 0; binTranslator->globalVars[i].name != NULL; i++)
        {
            fprintf (fileptr, "%s\n", binTranslator->globalVars[i].name);
        }
    }

    fprintf(fileptr, "funcArraySize = %lu\n", binTranslator->funcArraySize);
    fprintf (fileptr, "Functions:\n");
    for (size_t i = 0; i < binTranslator->funcArraySize; i++)
    {
        fprintf (fileptr, "--------------------------------------------------------------------------------\n");
        dumpIRFuncion(fileptr, binTranslator->funcArray[i]);
        fprintf (fileptr, "--------------------------------------------------------------------------------\n");
    }
}
//----------------------------------------

// Counters
//----------------------------------------
static size_t countNumberOfBlocks (Node* node, size_t numOfBlocks)
{
    assert (node != NULL);

    if (node->type == Key_t)
    {
        if (strcmp ("IF", node->Name) == 0)
            numOfBlocks += 2;
        if (strcmp("ELSE", node->Name) == 0)
            numOfBlocks += 1;
    }

    if (node->left)
        numOfBlocks += countNumberOfBlocks(node->left, 0);

    if (node->right)
        numOfBlocks += countNumberOfBlocks(node->right, 0);

    return numOfBlocks;
};

static size_t countNumberOfCmdInFunc (Node* node, size_t numOfCmd)
{
    assert (node != NULL);

    if (node->type == OP_t)
        numOfCmd += 1;

    if (node->type == Key_t)
        numOfCmd += 1;

    if (node->left)
        numOfCmd += countNumberOfCmdInFunc(node->left, 0);

    if (node->right)
        numOfCmd += countNumberOfCmdInFunc(node->right, 0);

    return numOfCmd;
}

static size_t countNumberOfVarsInFunc (Node* node, size_t numOfVars)
{
    assert (node != NULL);

    if (node->type == Key_t)
        numOfVars += 1;

    if (node->type == OP_t)
        numOfVars += 1;

    if (node->type == Var_t)
        numOfVars += 1;

    if (node->left)
        numOfVars += countNumberOfVarsInFunc(node->left, 0);

    if (node->right)
        numOfVars += countNumberOfVarsInFunc(node->right, 0);

    return numOfVars;
}

static size_t countNumberOfFunc (Node* node, size_t numOfFunc)
{
    assert (node != NULL);

    if (node->type == Func_t)
        numOfFunc+= 1;

    if (node->left)
        numOfFunc += countNumberOfCmdInFunc(node->left, numOfFunc);

    if (node->right)
        numOfFunc += countNumberOfCmdInFunc(node->right, numOfFunc);

    return numOfFunc;
}
//----------------------------------------

// Work with var
//----------------------------------------
static Var_bt* addVar (Var_bt* varArray, size_t* varArraySize, char* name, Location location)
{
    assert (varArray != NULL);
    assert (name     != NULL);

    int i = 0;
    for (; varArray[i].name != NULL; i++) {};

    varArray[i].name = name;
    varArray[i].location = location;
    varArray[i].offset = (*varArraySize - NumberOfTempVars + 1) * 8 ;
    *varArraySize += 1;

    varArray[i + 1] = {};
    return &varArray[i];
}

static Var_bt* findVar (Var_bt* varArray, char* name)
{
    assert (name != nullptr);
    assert (varArray != nullptr);

    for (int i = 0; varArray[i].name != NULL; i++)
    {
        if (strcmp (varArray[i].name, name) == 0)
            return &varArray[i];
    }
    return NULL;
}

static Var_bt* addTempVar (Var_bt* varArray, size_t* varArraySize, Location location)
{
    char* tempVarName = (char*) calloc (15, sizeof (char));
    sprintf(tempVarName, "temp%lu", NumberOfTempVars);
    NumberOfTempVars += 1;

    return addVar(varArray, varArraySize, tempVarName, location);
};

static Var_bt* parseVarToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node          != NULL);
    assert (binTranslator != NULL);
    assert (function      != NULL);

    Var_bt* var = findVar(function->varArray, node->var.varName);

    if (var == NULL)
        return addVar(function->varArray, &(function->varArraySize), node->var.varName, Memory);

    return var;
}
//----------------------------------------

//----------------------------------------
static Op_bt* createOpBt (Type type, Value_bt value)
{
    Op_bt* opPointer = (Op_bt*) calloc (1, sizeof (*opPointer));

    opPointer->type = type;
    opPointer->value = value;

    return opPointer;
}
#define NumOP(num) createOpBt (Num_t, num)

static Cmd_bt* addCmd (Block_bt* block, OpCode_bt opCode, Op_bt* op1, Op_bt* op2, Op_bt* dest)
{
    assert (block->cmdArray != NULL);
    assert (block->cmdArrayCapacity != 0);

    if (block->cmdArraySize >= block->cmdArrayCapacity)
    {
        block->cmdArray = (Cmd_bt*) realloc(block->cmdArray, block->cmdArrayCapacity * 2 * sizeof (*block->cmdArray));
        assert (block->cmdArray != NULL);
        block->cmdArrayCapacity *= 2;
    }

    block->cmdArray[block->cmdArraySize] = {opCode, op1, op2, dest};
    block->cmdArraySize += 1;

    return &(block->cmdArray[block->cmdArraySize - 1]);
}

static Block_bt* addBlock (Func_bt* function, size_t numOfCmd, char* name)
{
    assert (function != NULL);
    assert (name     != NULL);

    if (function->blockArraySize >= function->blockArrayCapacity)
    {
        Block_bt* blockArray = (Block_bt*) calloc (function->blockArrayCapacity * 2, sizeof(*blockArray));
        assert (blockArray != NULL);
        function->varArrayCapacity *= 2;
        function->blockArray = blockArray;
    }

    Cmd_bt* cmdArray = (Cmd_bt*) calloc (numOfCmd, sizeof(*cmdArray));
    assert (cmdArray != NULL);

    function->blockArray[function->blockArraySize] = {"\0", cmdArray, 0, numOfCmd};
    strcpy (function->blockArray[function->blockArraySize].name, name);
    function->blockArraySize += 1;

    return &function->blockArray[function->blockArraySize - 1];
};

static Block_bt* findFunctionBlock (BinaryTranslator* binTranslator, char* name)
{
    assert (binTranslator != NULL);
    assert (name          != NULL);

    for (size_t i = 0; i < binTranslator->funcArraySize; i++)
    {
        if (strcmp (name, binTranslator->funcArray[i].name) == 0)
        {
            return binTranslator->funcArray[i].blockArray;
        }
    }

    assert (0);
}
//----------------------------------------
// Work with functions
//----------------------------------------

static Func_bt* initFunction (size_t numOfVars, size_t numOfBlocks)
{
    Func_bt* function = (Func_bt*) calloc (1, sizeof (*function));
    assert (function != NULL);

    Var_bt* varArray = (Var_bt*) calloc (numOfVars, sizeof (*varArray));
    assert (varArray != NULL);

    Block_bt* blockArray = (Block_bt*) calloc (numOfBlocks, sizeof (*blockArray));
    assert (blockArray != NULL);

    function->varArray           = varArray;
    function->blockArray         = blockArray;
    function->blockArraySize     = 0;
    function->varArraySize       = 0;
    function->varArrayCapacity   = numOfVars;
    function->blockArrayCapacity = numOfBlocks;
    printf("%lu\n", numOfVars);
    printf("%lu\n", numOfBlocks);

    return function;
}

static void parseFuncParams (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node          != NULL);
    assert (binTranslator != NULL);
    assert (function      != NULL);

    if (node->type == Key_t && strcmp (node->Name, "PARAM") == 0)
    {
        Var_bt* var = addVar (function->varArray, &(function->varArraySize), node->left->left->var.varName, Memory);
        addCmd (&function->blockArray[0], {.operation = OP_PAROUT}, NULL, NULL, createOpBt(Var_t, {.var = var}));
    }

    if (node->right)
        parseFuncParams (node->right, binTranslator, function);
}

static void parseFuncHead (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node != NULL);
    assert (binTranslator != NULL);
    assert (function != NULL);

    Node* leftNode = node->left;
    assert(leftNode != NULL);

    if (node->type == Func_t)
    {
         strcpy (function->name, leftNode->Name);
         addBlock(function, countNumberOfCmdInFunc(node, 0), function->name);
    }

    if (leftNode->left)
    {
        parseFuncParams (leftNode->left, binTranslator, function);
    }
}

static void parseFuncToIR (Node* node, BinaryTranslator* binTranslator)
{
    assert (node != NULL);
    assert (binTranslator != NULL);

    Func_bt* function = initFunction(countNumberOfVarsInFunc(node, 0) + 1, countNumberOfBlocks(node, 0) + 1); // +1 for NULL element
    NumberOfTempVars = 0;

    if (node->left)
    {
        parseFuncHead (node, binTranslator, function);
        binTranslator->funcArray[binTranslator->funcArraySize] = *function;
        binTranslator->funcArraySize += 1;
    }
    else
        assert (0);

    if (node->right)
        parseStToIR   (node->right, binTranslator, function);
    else
        assert(0);

    function->numberOfTempVar = NumberOfTempVars;
    binTranslator->funcArray[binTranslator->funcArraySize - 1] = *function;
    free (function);
}
//----------------------------------------

//----------------------------------------
#define CMD2op(opCode) addCmd(&function->blockArray[function->blockArraySize - 1], {(unsigned int) opCode, 0, 0, 0}, \
        parseExpToIR(node->left, binTranslator, function),                        \
        parseExpToIR(node->right, binTranslator, function),                       \
        tempOp);
#define CMD1op(opCode, direction) addCmd (function, {(unsigned int) opCode, 0, 0, 0}, \
        parseExpToIR (direction, binTranslator, function), NULL, tempOp, NULL);

static Op_bt* parseExpToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node          != NULL);
    assert (binTranslator != NULL);
    assert (function      != NULL);

    switch (node->type)
    {
        case OP_t:
        {
            Var_bt* tempVar = addTempVar (function->varArray, &function->varArraySize, Stack);
            Value_bt value = {};
            value.var = tempVar;
            Op_bt*  tempOp  = createOpBt(Var_t, value);

            switch (node->opValue)
            {
                case OP_ADD:
                    CMD2op(OP_ADD);
                    return createOpBt(Var_t, value);
                    break;

                case OP_SUB:
                    CMD2op(OP_SUB);
                    return createOpBt(Var_t, value);
                    break;

                case OP_MUL:
                    CMD2op(OP_MUL);
                    return createOpBt(Var_t, value);
                    break;

                case OP_DIV:
                    CMD2op(OP_DIV);
                    return createOpBt(Var_t, value);
                    break;

                case OP_EQ:
                    addCmd (&function->blockArray[function->blockArraySize - 1], {(unsigned int) OP_EQ, 0, 0, 0},
                        parseExpToIR (node->right, binTranslator, function), NULL, parseExpToIR(node->left, binTranslator, function));
                    free (tempOp);
                    break;

                default:
                    assert (0);
            }
            break;
        }

        case Var_t:
            return createOpBt(Var_t, {.var = parseVarToIR(node, binTranslator, function)});
            break;

        case Num_t:
            return NumOP({.num = (int) node->numValue});
            break;

        case Func_t:
            if (strcmp ("CALL", node->Name) == 0)
            {
               return parseCallToIR(node, binTranslator, function);
            }
            break;

        default:
            assert (0);
    }
    return NULL;
}
#undef CMD

static void parseIfToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node != NULL);
    assert (function != NULL);
    assert (binTranslator != NULL);

    static int numberOfIf = 0;
    int curNumberOfIf = numberOfIf;
    numberOfIf += 1;

    char buf[15] = "";
    Block_bt* ifBlock    = NULL;
    Block_bt* elseBlock  = NULL;
    Block_bt* elderBlock = &function->blockArray[function->blockArraySize - 1];

    Op_bt* condition = parseExpToIR(node->left, binTranslator, function);

    sprintf(buf, "IF%d", curNumberOfIf);
    if (node->right)
    {
        if (strcmp (node->right->Name, "ELSE") == 0)
        {
            ifBlock = addBlock (function, countNumberOfCmdInFunc (node->right, 0), buf);
            parseStToIR (node->right->left, binTranslator, function);

            sprintf(buf, "ELSE%d", curNumberOfIf);

            elseBlock = addBlock(function,  countNumberOfCmdInFunc (node->right, 0), buf);
            parseStToIR (node->right->right, binTranslator, function);
        }
        else
        {
            ifBlock = addBlock (function, countNumberOfCmdInFunc (node->right, 0), buf);
            parseStToIR (node->right, binTranslator, function);
        }

        sprintf(buf, "MERGE%d", curNumberOfIf);
        Block_bt* mergeBlock = addBlock(function, 20, buf);

        addCmd (ifBlock, {OP_JMP, 0, 0, 0}, createOpBt(Pointer_t, {.block = mergeBlock}), NULL, NULL);

        if (elseBlock != NULL)
        {
            addCmd (elseBlock, {OP_JMP, 0, 0, 0}, createOpBt(Pointer_t, {.block = mergeBlock}), NULL, NULL);
            addCmd (elderBlock, {OP_IF, 0, 0, 0}, createOpBt(Pointer_t, {.block = ifBlock}), createOpBt(Pointer_t, {.block = elseBlock}), condition);
        }
        else
            addCmd (elderBlock, {OP_IF, 0, 0, 0}, createOpBt(Pointer_t, {.block = ifBlock}), createOpBt(Pointer_t, {.block = mergeBlock}), condition);

    }

}

static void parseCallParam (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node != NULL);
    assert (function != NULL);
    assert (binTranslator != NULL);

    if (node->left)
    {
        switch (node->left->type)
        {
            case Num_t:
                addCmd (&function->blockArray[function->blockArraySize - 1], {.operation = OP_PARIN}, createOpBt(Num_t, {.num = (int) node->left->numValue}), NULL, NULL);
                break;
            case Var_t:
                findVar (function->varArray, node->left->var.varName);
                addCmd (&function->blockArray[function->blockArraySize - 1], {.operation = OP_PARIN}, createOpBt(Var_t, {.var = findVar (function->varArray, node->left->var.varName)}), NULL, NULL);
                break;

            case OP_t:
                addCmd(&function->blockArray[function->blockArraySize - 1], {.operation = OP_PARIN}, parseExpToIR(node->left, binTranslator, function), NULL, NULL);
                break;

            default:
                assert (0);

        }
    }

    if (node->right)
        parseCallParam(node->right, binTranslator, function);
}

static Op_bt* parseCallToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node != NULL);
    assert (function != NULL);
    assert (binTranslator != NULL);

    Node* curNode = node->left;

    if (curNode->left)
    {
        parseCallParam(curNode->left, binTranslator, function);
    }

    Op_bt* dest = createOpBt(Var_t, {.var = addTempVar(function->varArray, &function->varArraySize, Register)});
    addCmd (&function->blockArray[function->blockArraySize - 1], {.operation = OP_CALL}, createOpBt(Pointer_t, {.block=findFunctionBlock(binTranslator, curNode->Name)}), NULL, dest);
    return createOpBt(Var_t, {.var = dest->value.var});
}

static inline void parseOutToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    if (node->left)
    {
        if (node->left->left)
        {
            addCmd (&function->blockArray[function->blockArraySize - 1], {.operation = OP_OUT}, createOpBt(Var_t, {.var = findVar(function->varArray, node->left->left->var.varName)}), NULL, NULL);
        }
    }
}

static inline void parseInToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    if (node->left)
    {
        if (node->left->left)
        {
            addCmd (&function->blockArray[function->blockArraySize - 1], {.operation = OP_IN},  NULL, NULL, createOpBt(Var_t, {.var = findVar(function->varArray, node->left->left->var.varName)}));
        }
    }
}

static void parseStToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node          != NULL);
    assert (binTranslator != NULL);
    assert (function      != NULL);

     if (node->left)
     {
         switch (node->left->type)
         {
            case OP_t:
                free (parseExpToIR (node->left, binTranslator, function));
                break;

            case Var_t:
                free (parseExpToIR (node->left, binTranslator, function));
                break;

            case Num_t:
                free (parseExpToIR (node->left, binTranslator, function));
                break;

            case Func_t:
                if (strcmp (node->left->Name, "CALL") == 0)
                    free (parseCallToIR (node->left, binTranslator, function));
                break;

            case Key_t:
                if (strcmp (node->left->Name, "ST") == 0)
                    parseStToIR (node->left, binTranslator, function);

                else if (strcmp (node->left->Name, "RET") == 0)
                    addCmd(&function->blockArray[function->blockArraySize - 1], {(unsigned int) OP_RET, 0, 0, 0}, parseExpToIR(node->left->left, binTranslator, function),
                                        NULL, NULL);
                else if (strcmp (node->left->Name, "IF") == 0)
                    parseIfToIR (node->left, binTranslator, function);
                else if (strcmp (node->left->Name, "VAR") == 0)
                {
                    addCmd (&function->blockArray[function->blockArraySize - 1], {(unsigned int) OP_EQ, 0, 0, 0},
                        parseExpToIR (node->left->right, binTranslator, function), NULL, parseExpToIR(node->left->left, binTranslator, function));
                }


                break;

            case BuiltIn_t:
                    if (node->left->opValue == OP_OUT)
                        parseOutToIR (node->left, binTranslator, function);
                    else if (node->left->opValue == OP_IN)
                        parseInToIR (node->left, binTranslator, function);
                    break;

            case Unknown:
                fprintf (stderr, "Writer unknown %s\n", node->Name);
                assert (0);
                break;

            default:
                assert (0);
         }
     }

     if (node->right)
         parseStToIR(node->right, binTranslator, function);
}
//----------------------------------------

static void parseProgToIR (Node* node, BinaryTranslator* binTranslator)
{
    if (node->left)
    {
        if (node->left->type == Func_t)
            parseFuncToIR(node->left, binTranslator);
        else
            assert (0);
    }

    if (node->right)
    {
        parseProgToIR(node->right, binTranslator);
    }

}

void startProg (BinaryTranslator* binTranslator)
{
    assert (binTranslator != NULL);

    mprotect(binTranslator->x86_array, binTranslator->x86_arraySize, PROT_EXEC);

    void (*func) (void) = ((void (*) (void)) binTranslator->x86_array);
    func();
    mprotect(binTranslator->x86_array, binTranslator->x86_arraySize, PROT_READ | PROT_WRITE);
}

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen(fileName, "r");
    assert (fileptr != NULL);
    Node* tree = getTreeFromStandart(fileName);

    treeDump(tree, "HEYY\n");
    binTranslator->tree = tree;

    binTranslator->funcArray = (Func_bt*) calloc (countNumberOfFunc(tree, 0) + 1, sizeof (Func_bt));

    parseProgToIR(tree, binTranslator);
    dumpIR("Dump.txt", binTranslator);
}

//----------------------------------------

static void varArrayDtor (Func_bt* function)
{
    for (size_t i = 0; i < function->varArraySize; i++)
    {
        if (strstr(function->varArray[i].name, "temp"))
            free (function->varArray[i].name);
    }

    free (function->varArray);
}

void IRdtor (BinaryTranslator* binTranslator)
{
    for (size_t i = 0; i < binTranslator->funcArraySize; i++)
    {

        for (size_t j = 0; j < binTranslator->funcArray[i].blockArraySize; j ++)
        {
            for (size_t k = 0; k < binTranslator->funcArray[i].blockArray[j].cmdArraySize; k++)
            {
                if (binTranslator->funcArray[i].blockArray[j].cmdArray[k].operator1)
                {
                    free (binTranslator->funcArray[i].blockArray[j].cmdArray[k].operator1);
                }

                if (binTranslator->funcArray[i].blockArray[j].cmdArray[k].operator2)
                {
                    free (binTranslator->funcArray[i].blockArray[j].cmdArray[k].operator2);
                }

                if (binTranslator->funcArray[i].blockArray[j].cmdArray[k].dest)
                {
                    free (binTranslator->funcArray[i].blockArray[j].cmdArray[k].dest);
                }

            }

            free (binTranslator->funcArray[i].blockArray[j].cmdArray);

        }

        free (binTranslator->funcArray[i].blockArray);
        varArrayDtor(&binTranslator->funcArray[i]);

    }


}

void NodeDtor(Node* node)
{
    if (node->left)
        NodeDtor(node->left);

    if (node->right)
        NodeDtor(node->right);

    switch (node->type)
    {
        case Var_t:
            free (node->var.varName);
            break;

        case Num_t:
            break;

        case Unknown:
            break;

        case BuiltIn_t:
            break;

        case Pointer_t:
            break;

        case Func_t:
            if ((strcmp (node->Name, "CALL") != 0) && (strcmp(node->Name, "FUNC") != 0))
                free (node->Name);
            break;

        case Key_t:
            break;

        case OP_t:
            break;

        default:
            assert(0);
    }

    free(node);
}

