#ifndef SectionX86_64
#define SectionX86_64
#include <string>
#include <vector>
#include "flags.h"
#include "elf.h"
#include "binaryData.h"
#include "sectionHeader.h"
#include <memory>

class StringTable;
class BinaryReader;
using namespace std;

class Section: public SectionHeader{
    friend class X86Parser;
public:
    Section( const BinaryReader& headerPos, 
             const BinaryReader &strings );
    Section (string header, StringTable* );
    ~Section ();
    Elf64_Xword GetFlags() ;

    // Methods
    string GetLinkFlags();
    string WriteLinkHeader();
    string WriteLinkData();

    void WriteRawData(BinaryWriter &writer) const;
    void WriteRawData(BinaryWriter &&writer) const {
        WriteRawData(writer);
    }

    bool IsLInkSection();
    string Name() { return name; }

    shared_ptr<Data> GetData() { return data; }

    // The caller is repsonsible for destruction
    static Section* MakeNewStringTable( StringTable &tab, StringTable *sectionNames, string name);



protected:
    static const Flags& TypeFlags();
    void SetFlags();
    static Flags::Mask Flags_SHF_WRITE;
    static Flags::Mask Flags_SHF_ALLOC;
    static Flags::Mask Flags_SHF_EXECINSTR;
private:
    Section ();
    shared_ptr<Data> data;
    StringTable *stringTable;
    string name;
    Flags sh_flags;
    /* data */
};

#endif
