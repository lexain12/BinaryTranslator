#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/full_faktorial.txt", &binTranslator);
    startProg (&binTranslator);

    return 0;
}
