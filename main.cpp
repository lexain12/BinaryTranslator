#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/DBfileHard.txt", &binTranslator);

    return 0;
}
