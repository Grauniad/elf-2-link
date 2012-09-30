#include <string>
#include <vector>
#include "flags.h"
#include "elf.h"
#include "elfReader.h"
using namespace std;
#ifndef SectionX86_64
   #define SectionX86_64
#endif

class StringTable;

class Section {
    friend class X86Parser;
public:
    Section (ElfReader &reader, int offset, int stable);
    Section (string header, StringTable* );
    ~Section ();
    Elf64_Xword GetFlags();

    // Data
    size_t Size() {return sizeof(Elf64_Shdr);}
    Elf64_Addr& Address() { return elfHeader.sh_addr; }
    bool Writeable() { return elfHeader.sh_flags & SHF_WRITE; }
    bool Allocate() { return elfHeader.sh_flags &  SHF_ALLOC; }
    bool Executable(){ return elfHeader.sh_flags &SHF_EXECINSTR; }
    Elf64_Off& DataStart() { return elfHeader.sh_offset; }
    uint64_t& DataSize() { return elfHeader.sh_size; }



private:
    char * data;
    Elf64_Shdr elfHeader;
    StringTable *stringTable;
    Flags sh_flags;
    string name;
    /* data */
};

