#ifndef ProgramX86_64
#define ProgramX86_64

#include <string>
#include <vector>
#include "flags.h"
#include "elf.h"
using namespace std;


class Section;
class BinaryReader;

class RawProgramHeader: public Elf64_Phdr {
public:
    RawProgramHeader(const Elf64_Phdr& other);
    RawProgramHeader() = default;

    // attributes
    size_t Size() const { return sizeof(Elf64_Phdr);}
    Elf64_Off& DataStart() { return p_offset; }
    Elf64_Xword& Alignment() { return p_align; }
    Elf64_Xword& FileSize() { return p_filesz; }
    Elf64_Xword& SizeInMemory() { return p_memsz; }
    Elf64_Addr& Address() { return p_vaddr; }

    // Constant access
    const Elf64_Off& DataStart() const { return p_offset; }
    const Elf64_Xword& Alignment() const { return p_align; }
    const Elf64_Xword& FileSize() const { return p_filesz; }
    const Elf64_Xword& SizeInMemory() const { return p_memsz; }
    const Elf64_Addr& Address() const { return p_vaddr; }

    // Check Flags
    const bool IsNull() const { return p_type == PT_NULL;}
    const bool IsLoadableSegment() const { return p_type == PT_LOAD;}
    const bool IsDynamicLinking() const { return p_type == PT_DYNAMIC;}
    const bool IsInterpreter() const { return p_type == PT_INTERP;}
    const bool IsNote() const { return p_type == PT_NOTE;}
    const bool IsProgHeaders() const { return p_type == PT_PHDR; }

    /* Entry for the prog headers themselves! */
    const bool IsProgramHeaders() const { return p_type == PT_PHDR;}

    const bool IsThreadLocalStorage() const { return p_type == PT_TLS;}
    const bool GNUIsExceptionFrame() const { return p_type == PT_GNU_EH_FRAME;}
    const bool GNUIsExecutable() const { return p_type == PT_GNU_STACK;}
    const bool GNUIsReadOnlyAfterReloc() const { return p_type == PT_GNU_RELRO;}

    const bool IsExecutable() const { return p_flags & PF_X; }
    const bool IsReadable() const { return p_flags & PF_R; }
    const bool IsWriteable() const { return p_flags & PF_W; }

    // Update Flags
    void AddExecutable() { p_flags |= PF_X;}
    void AddReadable() { p_flags |= PF_R;}
    void AddWriteable() { p_flags |= PF_W;}
    void RemoveExecutable() { p_flags &= ~PF_X; }
    void RemoveReadable() { p_flags &= ~PF_R; }
    void RemoveWriteable() { p_flags &= ~PF_W; }

    string Describe() const;
    int FileRank() const;
};

class ProgramHeader: protected RawProgramHeader {
public:
    typedef std::vector<Section*> SECTION_ARRAY;

    /**
     * C'tor
     */
    ProgramHeader ( BinaryReader&, const SECTION_ARRAY&);

    /**
     * Handle moved binary readers...by redirecting to the std c'tor
     */
    ProgramHeader ( BinaryReader&& r, const SECTION_ARRAY& s)
        : ProgramHeader(r,s){};

    virtual ~ProgramHeader (){};


    //expose utility functions from the base
    using RawProgramHeader::Size;
    using RawProgramHeader::DataStart;
    using RawProgramHeader::Alignment;
    using RawProgramHeader::Address;
    using RawProgramHeader::IsNull;
    using RawProgramHeader::IsExecutable;
    using RawProgramHeader::IsLoadableSegment;
    using RawProgramHeader::IsProgramHeaders;
    using RawProgramHeader::IsReadable;
    using RawProgramHeader::IsWriteable;
    using RawProgramHeader::AddExecutable;
    using RawProgramHeader::AddReadable;
    using RawProgramHeader::AddWriteable;
    using RawProgramHeader::RemoveExecutable;
    using RawProgramHeader::RemoveReadable;
    using RawProgramHeader::RemoveWriteable;
    using RawProgramHeader::FileSize;
    // Constant Access
    inline const Elf64_Xword& SizeInMemory() const { 
        return RawProgramHeader::SizeInMemory(); 
    }
    

    // Methods
    void InitialiseFlags();
    const RawProgramHeader& RawHeader() const { return *this;}
      

    const std::vector<string>& SectionNames(){return sectionNames;}

    // Calculated values
    Elf64_Off  DataEnd() const { return DataStart() + FileSize(); }
    Elf64_Addr AddrEnd() const { return Address() + SizeInMemory(); }

    // Don't loose track of implicit memory when re-sizing in
    // the file
    const Elf64_Off GetAdditionalMemory() const; 
    void SetAdditionalMemory(const Elf64_Off& space);
    inline void AddAdditionalMemory(const Elf64_Off& space) {
        SetAdditionalMemory(space + GetAdditionalMemory());
    }

protected:
    /*
     * Calculate size in file, by looking at the size taken up by all sections
     * within the file..
     */
    Elf64_Off CalculateFileSize(const SECTION_ARRAY& sections) const;

    static const Flags& TypeFlags();

    static Flags::Mask Flags_Executable;
    static Flags::Mask Flags_Writeable;
    static Flags::Mask Flags_Readable;

private:
    std::vector<string> sectionNames;
    Flags flags;
};


#endif

