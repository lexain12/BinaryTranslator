#include <cstddef>
#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <elf.h>
#include <cassert>

#include "../language/common.h"
#include "../include/elfFileGen.h"
#include "../language/readerLib/functions.h"

#if defined(__LP64__)
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

static void writeELFHeader (FILE* fileptr)
{
    ElfW(Ehdr) header = {};
    header.e_ident[EI_MAG0]  = 0x7f;
    header.e_ident[EI_MAG1]  = 'E';
    header.e_ident[EI_MAG2]  = 'L';
    header.e_ident[EI_MAG3]  = 'F';
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA]  = ELFDATA2LSB;
    header.e_ident[EI_OSABI] = 0x00;
    header.e_ident[EI_VERSION] = 0x01;
    header.e_version           = 0x01;
    header.e_type            = ET_EXEC;
    header.e_machine         = 0x3E;
    header.e_entry           = 0x400078;
    header.e_phoff           = 0x40;
    header.e_shoff           = 0x0;
    header.e_ehsize          = 0x40;
    header.e_phentsize       = 0x38;
    header.e_phnum           = 0x01;

    fwrite(&header, sizeof (header), 1, fileptr);
}

static void writeELFPheader (FILE* fileptr, size_t sizeOfCode)
{
    ElfW(Phdr) textSection = {};
    size_t variableBufSize = 300;

    textSection.p_type = SHT_PROGBITS;
    textSection.p_flags = SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR;
    textSection.p_offset = 0x78;
    textSection.p_vaddr = 0x400078;
    textSection.p_filesz = sizeOfCode;
    textSection.p_memsz = sizeOfCode + variableBufSize;
    textSection.p_align = 0x1000;

    fwrite(&textSection, sizeof (textSection), 1, fileptr);
}
void giveRights (char* fileName)
{
    char cmdBuf[30] = "";
    sprintf(cmdBuf, "chmod +x %s", fileName);
    system (cmdBuf);
}

void linkMyPrintf (FILE* fileptr)
{
    FILE* printfPtr = fopen ("./bin/BinPrintf", "rb");
    size_t sizeOfFile = fileSize (printfPtr);

    unsigned char* buf = (unsigned char*) calloc(sizeOfFile, sizeof(unsigned char));
    assert (buf != nullptr);


    fread (buf, sizeof (unsigned char), sizeOfFile, printfPtr);
    fclose (printfPtr);
    fwrite (buf, sizeof (unsigned char), sizeOfFile, fileptr);

    free (buf);
}

void linkMyScanf (FILE* fileptr)
{
    FILE* scanfPtr = fopen ("./bin/BinScanf", "rb");
    size_t sizeOfFile = fileSize (scanfPtr);

    unsigned char* buf = (unsigned char*) calloc(sizeOfFile, sizeof(unsigned char));
    assert (buf != nullptr);

    fread (buf, sizeof (unsigned char), sizeOfFile, scanfPtr);
    fclose (scanfPtr);
    fwrite (buf, sizeof (unsigned char), sizeOfFile, fileptr);

    free (buf);
}


void makeElfFile (char* fileName, BinaryTranslator* binTranslator)
{
    FILE* fileptr = fopen (fileName, "wb");
    assert (fileptr != NULL);

    unsigned char BinPrint[] = {0x48, 0x31, 0xdb, 0x48, 0x31, 0xc9, 0x48, 0x89, 0xf8, 0x48, 0xc7, 0xc7, 0x0a, 0x00, 0x00, 0x00, 0x83, 0xf8, 0x00, 0x7d, 0x07, 0xf7, 0xd0, 0x48, 0xff, 0xc0, 0xb1, 0x01, 0x48, 0x31, 0xd2, 0x48, 0xf7, 0xf7, 0x48, 0x83, 0xc2, 0x30, 0x41, 0x88, 0x14, 0x1b, 0x48, 0xff, 0xc3,0x48, 0x83, 0xf8, 0x00, 0x75, 0xE9, 0x80, 0xf9, 0x01, 0x75, 0x08,0x41, 0xc6, 0x04,0x1b,0x2d, 0x48, 0xff, 0xc3, 0x4c, 0x89, 0xe7, 0x49, 0x8d, 0x74, 0x1b, 0xff, 0x48, 0x89, 0xda, 0xe8, 0x19, 0x00, 0x00, 0x00, 0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x4c, 0x89, 0xe6, 0x48, 0xc7, 0xc2, 0x05, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc3, 0x57, 0x8A, 0x06, 0x88, 0x07, 0x48, 0xff, 0xce, 0x48, 0xff, 0xc7, 0x48, 0xff, 0xca, 0x48, 0x83, 0xfa, 0x00, 0x75, 0xED, 0x58, 0xc3, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    unsigned char BinScanf[] = {0x41, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x50, 0x50, 0x41, 0x56, 0x57, 0x48, 0x31, 0xc0, 0x48, 0x31, 0xff, 0x4c, 0x89, 0xf6, 0x48, 0xc7, 0xc2, 0x14, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x5f, 0x41, 0x5e, 0x58, 0x41, 0x58, 0x41, 0x5d, 0x41, 0x5c, 0x41, 0x5b, 0x41, 0x56, 0x53, 0x50, 0xe8, 0x4c, 0x00, 0x00, 0x00, 0x48, 0x31, 0xc0, 0x48, 0x31, 0xd2, 0x4d, 0x31, 0xc0, 0x48, 0x31, 0xc9, 0x41, 0x8b, 0x06, 0x88, 0xc2, 0x31, 0xc0, 0x88, 0xd0, 0x83, 0xf8, 0x00, 0x83, 0xe8, 0x30, 0x83, 0xf8, 0x00, 0x7c, 0x24, 0x83, 0xf8, 0x09, 0x7f, 0x1f, 0xb5, 0x0a, 0x53, 0x48, 0x83, 0xfb, 0x00, 0x74, 0x08, 0xf6, 0xe5, 0x48, 0x83, 0xeb, 0x01, 0xeb, 0xf2, 0x5b, 0x48, 0x83, 0xeb, 0x01, 0x49, 0x01, 0xc0, 0x49, 0x83, 0xc6, 0x01, 0xeb, 0xc8, 0x4c, 0x89, 0x07, 0x58, 0x5b, 0x41, 0x5e, 0xc3, 0x48, 0x31, 0xdb, 0x41, 0x56, 0x41, 0x8b, 0x06, 0x88, 0xc2, 0x31, 0xc0, 0x88, 0xd0, 0x83, 0xf8, 0x00, 0x83, 0xe8, 0x30, 0x83, 0xf8, 0x00, 0x7c, 0x0f, 0x83, 0xf8, 0x09, 0x7f, 0x0a, 0x49, 0x83, 0xc6, 0x01, 0x48, 0x83, 0xc3, 0x01, 0xeb, 0xdd, 0x48, 0x83, 0xeb, 0x01, 0x41, 0x5e, 0xc3,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

    size_t sizeofProg = binTranslator->x86_arraySize+ sizeof (BinPrint) + sizeof (BinScanf) + 16 + 5*2;

    writeELFHeader(fileptr);
    writeELFPheader(fileptr, sizeofProg);
    fwrite(binTranslator->x86_array, sizeof (unsigned char), binTranslator->x86_arraySize, fileptr);
    linkMyPrintf(fileptr);
    linkMyScanf(fileptr);

    fclose(fileptr);
    giveRights(fileName);
}

