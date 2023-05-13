#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/CALL_TEST.txt", &binTranslator);

    return 0;
}
