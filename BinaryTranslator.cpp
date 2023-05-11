#include <cstddef>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "BinaryTranslator.h"
#include "language/common.h"
#include "./language/readerLib/functions.h"

extern const char* FullOpArray[];

static void parseStToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function);
void dumpIRToAsm (const char* fileName, BinaryTranslator* binTranslator);
void firstIteration (BinaryTranslator* binTranslator);

// DUMPS
//----------------------------------------

static void printOperand (FILE* fileptr, const Op_bt* op)
{
    fprintf (stderr, "HERE\n");
    assert (op != NULL);
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fprintf (stderr, "%ul\n", op->type);
    fprintf (stderr, "HERE\n");

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
    }
}

static void dumpIRBlock (FILE* fileptr, const Block_bt block)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    fprintf (fileptr, "cmdArrayCapacity = %d, cmdArraySize = %d\n", block.cmdArrayCapacity, block.cmdArraySize);
    fprintf (fileptr, "%12s: CMD        op1        op2        dest\n\t{\n", block.name);
    for (int i = 0; i < block.cmdArraySize; i++)
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
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    assert (fileptr != NULL);
    assert (function.blockArray != NULL);
    assert (function.varArray != NULL);
    assert (function.name != NULL);

    fprintf (fileptr, "%s\n", function.name);

    fprintf (fileptr, "Variables:\n{\n");
    for (int i = 0; function.varArray[i].name != NULL; i++)
    {
        fprintf(fileptr, "\t%s\n", function.varArray[i].name);
    }
    fprintf (fileptr, "}\n");

    fprintf (fileptr, "blockArraySize = %d, blockArrayCapacity = %d\n", function.blockArraySize, function.blockArrayCapacity);
    fprintf (fileptr, "blocks: \n{\n");
    for (int i = 0; i < function.blockArraySize; i++)
    {
        fprintf (fileptr, "\t");
        dumpIRBlock(fileptr, function.blockArray[i]);
    }
    fprintf(fileptr, "}\n\n");

}

void dumpIR (const char* fileName, const BinaryTranslator* binTranslator)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen (fileName, "w");
    setvbuf(fileptr, NULL, _IONBF, 0);
    assert (fileptr != NULL);

    if (binTranslator->globalVars != NULL)
    {
        fprintf (fileptr, "Variables:\n");
        for (int i = 0; binTranslator->globalVars[i].name != NULL; i++)
        {
            fprintf (fileptr, "%s\n", binTranslator->globalVars[i].name);
        }
    }
    fprintf(fileptr, "funcArraySize = %d\n", binTranslator->funcArraySize);
    fprintf (fileptr, "Functions:\n");
    for (int i = 0; i < binTranslator->funcArraySize; i++)
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
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
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
    printf ("%s\n", __PRETTY_FUNCTION__);
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
static Var_bt* addVar (Var_bt* varArray, char* name, Location location, size_t pointer)
{
    assert (varArray != NULL);
    assert (name     != NULL);

    int i = 0;
    for (; varArray[i].name != NULL; i++) {};

    varArray[i].name = name;
    varArray[i].location = location;
    varArray[i].pointer = pointer;

    varArray[i + 1] = {};
    return &varArray[i];
}

static Var_bt* findVar (Var_bt* varArray, char* name)
{
    for (int i = 0; varArray[i].name != NULL; i++)
    {
        if (strcmp (varArray[i].name, name) == 0)
            return &varArray[i];
    }
    return NULL;
}

static Var_bt* addTempVar (Var_bt* varArray)
{
    static int NumberOfTempVars = 0;
    char* tempVarName = (char*) calloc (15, sizeof (char));
    sprintf(tempVarName, "temp%d", NumberOfTempVars);
    NumberOfTempVars += 1;

    return addVar(varArray, tempVarName, Stack, 0);
};

static Var_bt* parseVarToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    assert (node          != NULL);
    assert (binTranslator != NULL);
    assert (function      != NULL);

    Var_bt* var = findVar(function->varArray, node->var.varName);

    if (var == NULL)
        return addVar(function->varArray, node->var.varName, Stack, 0);

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
#define NumOP(num) createOpBt (Num_t, num);

static Cmd_bt* addCmd (Block_bt* block, OpCode_bt opCode, Op_bt* op1, Op_bt* op2, Op_bt* dest)
{
    assert (block->cmdArray != NULL);

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

static Block_bt* addBlock (Func_bt* function, int numOfCmd, char* name)
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
        addVar (function->varArray, node->left->var.varName, Stack, 1);
    }

    if (node->right)
        parseFuncParams (node->right, binTranslator, function);
}

static void parseFuncHead (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    assert(node != NULL);

    if (node->type == Func_t)
    {
         strcpy (function->name, node->Name);
    }

    if (node->right)
    {
        parseFuncParams (node->left, binTranslator, function);
    }
}

void parseFuncToIR (Node* node, BinaryTranslator* binTranslator)
{

    assert (node != nullptr);
    Func_bt* function = initFunction(countNumberOfVarsInFunc(node, 0) + 1, countNumberOfBlocks(node, 0) + 1); // +1 for NULL element

    if (node->left)
    {
        parseFuncHead (node->left, binTranslator, function);
        addBlock(function, countNumberOfCmdInFunc(node, 0), function->name);
    }
    else
        assert (0);

    if (node->right)
        parseStToIR   (node->right, binTranslator, function);
    else
        assert(0);

    binTranslator->funcArray[binTranslator->funcArraySize] = *function;
    binTranslator->funcArraySize += 1;
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
            printf ("asdffffffffffffffffddddddddddddd %d\n", node->opValue);
            Var_bt* tempVar = addTempVar (function->varArray);
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
    }
}
#undef CMD

#if 1
static void parseIfToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    static int numberOfIf = 0;
    char buf[15] = "";
    Block_bt* ifBlock    = NULL;
    Block_bt* elseBlock  = NULL;
    Block_bt* elderBlock = &function->blockArray[function->blockArraySize - 1];

    Op_bt* condition = parseExpToIR(node->left, binTranslator, function);

    sprintf(buf, "IF%d", numberOfIf);
    if (node->right)
    {
        if (strcmp (node->right->Name, "ELSE") == 0)
        {
            ifBlock = addBlock (function, countNumberOfCmdInFunc (node->right, 0), buf);
            parseStToIR (node->right->left, binTranslator, function);

            sprintf(buf, "ELSE%d", numberOfIf);

            elseBlock = addBlock(function,  countNumberOfCmdInFunc (node->right, 0), buf);
            parseStToIR (node->right->right, binTranslator, function);
        }
        else
        {
            ifBlock = addBlock (function, countNumberOfCmdInFunc (node->right, 0), buf);
            parseStToIR (node->right, binTranslator, function);
        }

        sprintf(buf, "MERGE%d", numberOfIf);
        Block_bt* mergeBlock = addBlock(function, 20, buf);

        addCmd (ifBlock, {OP_JMP, 0, 0, 0}, createOpBt(Pointer_t, {.block = mergeBlock}), NULL, NULL);

        if (elseBlock != NULL)
        {
            addCmd (elseBlock, {OP_JMP, 0, 0, 0}, createOpBt(Pointer_t, {.block = mergeBlock}), NULL, NULL);
            addCmd (elderBlock, {OP_IF, 0, 0, 0}, createOpBt(Pointer_t, {.block = ifBlock}), createOpBt(Pointer_t, {.block = elseBlock}), condition);
        }
        else
            addCmd (elderBlock, {OP_IF, 0, 0, 0}, createOpBt(Pointer_t, {.block = ifBlock}), createOpBt(Pointer_t, {.block = mergeBlock}), condition);

        numberOfIf += 1;
    }

}
#endif

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
                parseExpToIR (node->left, binTranslator, function);
                break;

            case Var_t:
                parseExpToIR (node->left, binTranslator, function);
                break;

            case Num_t:
                parseExpToIR (node->left, binTranslator, function);
                break;

            case Key_t:
                if (strcmp (node->left->Name, "ST") == 0)
                    parseStToIR (node->left, binTranslator, function);

                else if (strcmp (node->left->Name, "RET") == 0)
//                    parseStToIR (node->left, binTranslator, function);
                    addCmd(&function->blockArray[function->blockArraySize - 1], {(unsigned int) OP_RET, 0, 0, 0}, parseExpToIR(node->left->left, binTranslator, function),
                                        NULL, NULL);
                else if (strcmp (node->left->Name, "IF") == 0)
                    parseIfToIR (node->left, binTranslator, function);

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

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen(fileName, "r");
    assert (fileptr != NULL);
    Node* tree = getTreeFromStandart(fileName);

    treeDump(tree, "HEYY\n");

    binTranslator->funcArray = (Func_bt*) calloc (countNumberOfFunc(tree, 0) + 1, sizeof (Func_bt));

    parseProgToIR(tree, binTranslator);
    dumpIR("Dump.txt", binTranslator);
    firstIteration(binTranslator);
    dumpIRToAsm("asm.txt", binTranslator);
}

//----------------------------------------


