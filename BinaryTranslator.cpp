#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "BinaryTranslator.h"
#include "language/common.h"
#include "./language/readerLib/functions.h"

const char* OpArray[] = {"NoOP", "HLT"};

static void dumpIRFuncion (FILE* fileptr, const Func_bt function)
{
    assert (fileptr != NULL);
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
        fprintf (fileptr, "%s\n", OpArray[function.cmdArray[i].opCode.operation]);
    }
    fprintf(fileptr, "}\n");

}

void dumpIR (const char* fileName, const BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen (fileName, "w");
    assert (fileptr != NULL);

    fprintf (fileptr, "Variables:\n");
    for (int i = 0; binTranslator->globalVars[i].name != NULL; i++)
    {
        fprintf (fileptr, "%s\n", binTranslator->globalVars[i].name);
    }

    fprintf (fileptr, "Functions:\n");
    for (int i = 0; binTranslator->funcArray[i].name != NULL; i++)
    {
        dumpIRFuncion(fileptr, binTranslator->funcArray[i]);
    }
}

static Func_bt* initFunction (size_t numOfVars, size_t numOfCmd)
{
    Func_bt* function = (Func_bt*) calloc (1, sizeof (*function));
    assert (function != NULL);

    Var_bt* varArray = (Var_bt*) calloc (1, sizeof (*varArray));
    assert (varArray != NULL);
    varArray[0] = {};

    Cmd_bt* cmdArray = (Cmd_bt*) calloc (1, sizeof (*cmdArray));
    assert (cmdArray != NULL);
    cmdArray[0] = {};

    function->varArray = NULL;
    function->cmdArray = NULL;
    function->name     = NULL;

    return function;
}
static size_t countNumberOfCmdInFunc (Node* node, size_t numOfCmd)
{
    assert (node != NULL);

    if (node->type == OP_t)
        numOfCmd += 1;

    if (node->type == Key_t)
        numOfCmd += 1;

    if (node->left)
        numOfCmd += countNumberOfCmdInFunc(node->left, numOfCmd);

    if (node->right)
        numOfCmd += countNumberOfCmdInFunc(node->right, numOfCmd);

    return numOfCmd;
}

static size_t countNumberOfVarsInFunc (Node* node, size_t numOfVars)
{
    assert (node != NULL);

    if (node->type == OP_t)
        numOfVars += 1;
    if (node->type == Var_t)
        numOfVars += 1;

    if (node->left)
        numOfVars += countNumberOfVarsInFunc(node->left, numOfVars);

    if (node->right)
        numOfVars += countNumberOfVarsInFunc(node->right, numOfVars);

    return numOfVars;
}

static void addVar (Var_bt* varArray, char* name, Location location, size_t pointer)
{
    assert (varArray != NULL);
    assert (name     != NULL);

    int i = 0;
    for (; varArray[i].name != NULL; i++) {};

    varArray[i].name = name;
    varArray[i].location = location;
    varArray[i].pointer = pointer;

    varArray[i + 1] = {};
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
}

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen(fileName, "r");
    assert (fileptr != NULL);
    Node* tree = getTreeFromStandart(fileName);

}
