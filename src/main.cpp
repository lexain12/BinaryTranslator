#include "BinaryTranslator.h"
#include "translator.h"
#include "language/common.h"
#include "elfFileGen.h"

Configuration Config =
{
    .sizeOfScanf  = 174 + 16,
    .sizeOfPrintf   = 127 + 5*2,
};

void translateIRtoBin (BinaryTranslator* binTranslator)
{
    firstIteration (binTranslator);
    dumpIRToAsm("asm.txt", binTranslator);
    binTranslator->BT_ip = 0;

    dumpBTtable(binTranslator->nameTable);

    dumpIRToAsm ("asm.txt", binTranslator);
}

void printHelp ()
{
    printf ("Programm usage: ./<programm name> <fileWithTree> <outFileName>\n");
}

int main (int argc, char* argv[])
{
    if (argc != 3)
    {
        printHelp ();
    }
    else
    {
    BinaryTranslator binTranslator = {};

    parseTreeToIR(argv[1], &binTranslator);

    translateIRtoBin(&binTranslator);

    makeElfFile(argv[2], &binTranslator);
    }

    return 0;
}
