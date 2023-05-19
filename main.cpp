#include "BinaryTranslator.h"
#include "language/common.h"
#include "elfFileGen.h"

Configuration Config =
{
    .sizeOfScanf  = 72 + 16,
    .sizeOfPrintf   = 127 + 5*2,
};

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/full_faktorial.txt", &binTranslator);
    //startProg (&binTranslator);
    makeElfFile("src/MyFirstElf", &binTranslator);

    return 0;
}
