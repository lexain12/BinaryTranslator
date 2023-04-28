#include <cstdio>
#include <cassert>

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

void parseTreeToIR (const char* fileName, BinaryTranslator* binTranslator)
{
    assert (fileName      != NULL);
    assert (binTranslator != NULL);

    FILE* fileptr = fopen(fileName, "r");
    assert (fileptr != NULL);
    Node* tree = getTreeFromStandart(fileName);

    treeDump(tree, "Here");

    FILE* DBFileptr = fopen("DBFILEOUT.txt", "w");
    treePrint(tree, DBFileptr);
}
