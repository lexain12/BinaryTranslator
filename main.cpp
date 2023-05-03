#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/DBFile.txt", &binTranslator);

    return 0;
}
