#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/FUNC_twoFunctions.txt", &binTranslator);

    return 0;
}
