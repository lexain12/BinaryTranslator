#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "BinaryTranslator.h"
#include "language/common.h"
#include "./language/readerLib/functions.h"

extern const char* FullOpArray[];

static void dumpIRFuncion (FILE* fileptr, const Func_bt function)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    assert (fileptr != NULL);
    assert (function.cmdArray != NULL);
    assert (function.varArray != NULL);
    assert (function.name != NULL);

    fprintf (fileptr, "%s\n", function.name);

    fprintf (fileptr, "Variables:\n{\n");
    for (int i = 0; function.varArray[i].name != NULL; i++)
    {
        fprintf(fileptr, "%s\n", function.varArray[i].name);
    }
    fprintf (fileptr, "}\n");

    fprintf (fileptr, "cmd:\n{\n");
    for (int i = 0; function.cmdArray[i].opCode.operation != 0; i++)
    {
        fprintf (fileptr, "%s ", FullOpArray[function.cmdArray[i].opCode.operation]);
        if (function.cmdArray[i].operator1->var)
        {
            if (function.cmdArray[i].dest->var->name)
                fprintf(fileptr, " %s", function.cmdArray[i].operator1->var->name);
        }
        else
            fprintf(fileptr, " %d",function.cmdArray[i].operator1->num);

        if (function.cmdArray[i].operator2->var)
        {
            if (function.cmdArray[i].dest->var->name)
                fprintf(fileptr, " %s", function.cmdArray[i].operator2->var->name);
        }
        else
            fprintf(fileptr, " %d",function.cmdArray[i].operator2->num);

        if (function.cmdArray[i].dest->var)
            if (function.cmdArray[i].dest->var->name)
                fprintf(fileptr, " %s\n", function.cmdArray[i].dest->var->name);

    }
    fprintf(fileptr, "}\n");

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
    fprintf (fileptr, "Functions:\n");
    for (int i = 0; binTranslator->funcArray[i].name != NULL; i++)
    {
        dumpIRFuncion(fileptr, binTranslator->funcArray[i]);
    }
}

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

static Op_bt* createOpBt (Type type, Var_bt* varPointer, int num)
{
    Op_bt* opPointer = (Op_bt*) calloc (1, sizeof (*opPointer));

    opPointer->type = type;
    opPointer->var  = varPointer;
    opPointer->num  = num;

    return opPointer;
}
#define NumOP(num) createOpBt (Num_t, NULL, num);

static Cmd_bt* addCmd (Func_bt* function, OpCode_bt opCode, Op_bt* op1, Op_bt* op2, Op_bt* dest, char* name)
{
    assert (function->cmdArray != NULL);

    int i = 0;
    for (; function->cmdArray[i].opCode.operation != 0; i++) {};

    if (i >= function->cmdArraySize - 2)
    {
        function->cmdArray = (Cmd_bt*) realloc(function->cmdArray, function->cmdArraySize * 2 * sizeof (*function->cmdArray));
        assert (function->cmdArray != NULL);
        function->cmdArraySize *= 2;
    }

    function->cmdArray[i] = {opCode, op1, op2, dest};

    return &(function->cmdArray[i]);
}

static Func_bt* initFunction (size_t numOfVars, size_t numOfCmd)
{
    Func_bt* function = (Func_bt*) calloc (1, sizeof (*function));
    assert (function != NULL);

    Var_bt* varArray = (Var_bt*) calloc (numOfVars, sizeof (*varArray));
    assert (varArray != NULL);
    varArray[0] = {};

    Cmd_bt* cmdArray = (Cmd_bt*) calloc (numOfCmd, sizeof (*cmdArray));
    assert (cmdArray != NULL);
    cmdArray[0] = {};

    function->varArray     = varArray;
    function->cmdArray     = cmdArray;
    function->name         = NULL;
    function->varArraySize = numOfVars;
    function->cmdArraySize = numOfCmd;
    printf("%lu\n", numOfVars);
    printf("%lu\n", numOfCmd);


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
         function->name = node->Name;
    }

    if (node->right)
    {
        parseFuncParams (node->left, binTranslator, function);
    }
}
#define CMD2op(opCode) addCmd(function, {(unsigned int) opCode, 0, 0, 0}, \
        parseExpToIR(node->left, binTranslator, function),                        \
        parseExpToIR(node->right, binTranslator, function),                       \
        tempOp);
#define CMD1op(opCode, direction) addCmd (function, {(unsigned int) opCode, 0, 0, 0}, \
        parseExpToIR (direction, binTranslator, function), NULL, tempOp);

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
            Op_bt*  tempOp  = createOpBt(Var_t, tempVar, 0);

            switch (node->opValue)
            {
                case OP_ADD:
                    CMD2op(OP_ADD);
                    return createOpBt(Var_t, tempVar, 0);
                    break;

                case OP_SUB:
                    CMD2op(OP_SUB);
                    return createOpBt(Var_t, tempVar, 0);
                    break;

                case OP_MUL:
                    CMD2op(OP_MUL);
                    return createOpBt(Var_t, tempVar, 0);
                    break;

                case OP_DIV:
                    CMD2op(OP_DIV);
                    return createOpBt(Var_t, tempVar, 0);
                    break;

                case OP_EQ:
                    addCmd (function, {(unsigned int) OP_EQ, 0, 0, 0},
                        parseExpToIR (node->right, binTranslator, function), NULL, parseExpToIR(node->left, binTranslator, function));


                default:
                    assert (0);
            }
            break;
        }

        case Var_t:
            return createOpBt(Var_t, parseVarToIR(node, binTranslator, function), 0);
            break;

        case Num_t:
            return NumOP(node->numValue);
            break;
    }
}
#undef CMD

static void parseIfToIR (Node* node, BinaryTranslator* binTranslator, Func_bt* function)
{
    Op_bt* condition = parseExpToIR(node->left, binTranslator, function);

    if (node->right)
    {
        if (strcmp (node->right->Name, "ELSE") == 0)
        {
        }
        else
        {
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
                    parseStToIR (node->left, binTranslator, function);
//                    addCmd(function, {(unsigned int) OP_RET, 0, 0, 0}, parseExpToIR(node->left, binTranslator, function),
//                                        NULL, createOpBt(Var_t, addTempVar(function->varArray), 0));
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

void parseFuncToIR (Node* node, BinaryTranslator* binTranslator)
{

    assert (node != nullptr);
    Func_bt* function = initFunction(countNumberOfVarsInFunc(node, 0) + 1, countNumberOfCmdInFunc(node, 0) + 1); // +1 for NULL element

    if (node->left)
    {
        parseFuncHead (node->left, binTranslator, function);
    }
    else
        assert (0);

    if (node->right)
        parseStToIR   (node->right, binTranslator, function);
    else
        assert(0);

    binTranslator->funcArray[0] = *function;
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

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen(fileName, "r");
    assert (fileptr != NULL);
    Node* tree = getTreeFromStandart(fileName);

    treeDump(tree, "HEYY\n");

    binTranslator->funcArray = (Func_bt*) calloc (countNumberOfFunc(tree, 0) + 1, sizeof (Func_bt));

    parseFuncToIR(tree->left, binTranslator);
    dumpIR("Dump.txt", binTranslator);
}
