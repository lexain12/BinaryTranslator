#ifndef TRANSLATOR
#define TRANSLATOR

#include "./BinaryTranslator.h"

#define Dumpx86Buf(binTranslator, start, end) \
    printf ("Called from %s\n", __PRETTY_FUNCTION__);\
    dumpx86Buf(binTranslator, start, end);

#define SimpleCMD(name) writeCmdIntoArray( binTranslator, {.code = name, .size = SIZE_##name});

#define BYTE(offset) offset * 8

void dumpx86Buf (BinaryTranslator* binTranslator, size_t start, size_t end);

#endif
