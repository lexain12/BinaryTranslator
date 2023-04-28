#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./language/DBFile.txt", &binTranslator);

    return 0;
}
