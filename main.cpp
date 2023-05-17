#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/OUT_TEST.txt", &binTranslator);
    startProg (&binTranslator);

    return 0;
}
