#include "BinaryTranslator.h"

int main ()
{
    BinaryTranslator binTranslator = {};

    parseTreeToIR("./tests/faktorial.txt", &binTranslator);

    return 0;
}
