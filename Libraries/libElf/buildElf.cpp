#include "buildElf.h"
#include "programHeader.h"
#include <cstring>
#include <iostream>



ElfFile::ElfFile(ElfContent& data) 
     : file(1024) , 
       dataSectionStart(file),
       sectionHeadersStart(file),
       header(ElfHeaderX86_64::NewObjectFile())
{
    InitialiseHeader(data);
    InitialiseFile(data);
      
    // Process the program headers
    BinaryWriter dataWritePos =  ProcessProgHeaders( data);

    // Write any additional data, not required at run time
    sectionHeadersStart = (long)WriteUnloadedDataSections(data, dataWritePos);
    header.SectionTableStart() = (long)sectionHeadersStart;


    WriteSectionHeaders(data);

    // Finally write the header
    file.Writer().Write(&header,header.Size());

    // clean up any extra data
    file.Resize( (long)sectionHeadersStart + 
                 header.Sections() * header.SectionHeaderSize() );
}

void ElfFile::InitialiseFile(ElfContent& data) {
    long sectionDataLength = 0;
    
    dataSectionStart.Offset() = header.Size();
                         
    // Reserve space for data, plus room for alignment 
    // ( the idea is to allocate too much and resize back down )

    // guess the length of the sectionDataLength
    for( auto section : data.sections)
        sectionDataLength += section->Size();
    // allow room for alignments of loadable segments
    sectionDataLength += 16 * header.ProgramHeaders();

    file.Resize(  (long) dataSectionStart
                + sectionDataLength
                + header.SectionHeaderSize() * header.Sections());
    file.Fill(0,'\0',file.Size());
}

void ElfFile::InitialiseHeader(ElfContent &data) {
    
    header.ProgramHeaders() = data.progHeaders.size();
    header.Sections()  = data.sections.size();
    
    // If there is no load table, progheader start is defined to be 0 by 
    // the elf standard
    if ( header.ProgramHeaders() == 0 ) {
        header.ProgramHeadersStart() = 0;
    } else {
        header.ProgramHeadersStart() = header.Size();
    }

    dataWritten.resize(header.Sections());
    for ( int sec=0; sec<dataWritten.size(); sec++ ) {
        dataWritten[sec] = false;
    }
}

BinaryWriter ElfFile::ProcessProgHeaders(ElfContent &data ) {


    // we need a local copy to sort
    vector<ProgramHeader *> codeHeaders(data.progHeaders);

    if (codeHeaders.size() == 0 ) {
        // Some elf files have no prog. headers
        return dataSectionStart;
    }

    auto lt = [] (ProgramHeader* lhs, ProgramHeader* rhs) -> bool {
                  int lrank = lhs->RawHeader().FileRank();
                  int rrank = rhs->RawHeader().FileRank();
                  if ( lrank == rrank ) {
                      return lhs->Address() < rhs->Address();
                  } else {
                      return lrank < rrank;
                  }
                  //Can't get here
              };
    sort(codeHeaders.begin(),codeHeaders.end(),lt);

    BinaryWriter dataPos = dataSectionStart;
    WriteProgHeaders(data,codeHeaders,dataPos);

    return dataPos;
}

void ElfFile::WriteProgHeaders ( 
                     ElfContent& data,
                     vector<ProgramHeader *>& headers,
                     BinaryWriter&  dataPos)
{
    BinaryWriter headerPos = dataPos;
    BinaryWriter dataEnd = dataPos;

    auto it = headers.begin();
    ProgramHeader* ph;

    // First handle the program-headers header
    ph = *it;
    ph->DataStart() = dataPos;
    ph->SetFileSize(ph->Size() * header.ProgramHeaders());
    headerPos << ph->RawHeader();
    
    // Now iterate through the remaing sections
    dataPos = ph->DataStart() + ph->FileSize();
    for (it++; it != headers.end(); ++it) {
        ph = *it;

        // We need to align the program segment: (see comment in .h)
        // Calculate boundrary location
        if ( ph->Alignment() != 0 && !ph->IsLoadableSegment()) {
            dataEnd.Offset() = dataPos.NextBoundrary( ph->Alignment()) 
                             + (ph->Address() % ph->Alignment());
        }
        // move to the boundrary
        dataPos.FillTo(dataEnd);
        dataPos.Offset() = dataEnd;

        // write the section data
        dataEnd.Offset() = WriteDataSections( data, *ph, dataPos);

        // Set the file position in the program header
        if ( ph->IsLoadableSegment() && ph->DataStart() == 0 ) {
            // The first loadable segment load the first block of the file
        } else {
            ph->DataStart() = dataPos;
        }
        ph->SetFileSize(dataEnd - dataPos);
        headerPos << ph->RawHeader();

        dataPos.Offset() += ph->SizeInMemory();
    }
}

BinaryWriter ElfFile::WriteUnloadedDataSections( ElfContent& data,
                                                 BinaryWriter& dataWritePos ) 
{
    // Pick up any sections that don't have program headers
    //  - This may be all in a .o file
    for ( int i =0; i< header.Sections(); i++ ) {
        Section& sec = *(data.sections[i]);
        if (   !dataWritten[i] 
            && sec.HasFileData() 
            && !sec.IsRelocTable())
        {
            if ( sec.IsNull() ) {
                sec.DataStart() = 0;
            } else {
                // Align start position
                if ( sec.Alignment() != 0 )  {
                    dataWritePos.Offset() = 
                        dataWritePos.NextBoundrary(sec.Alignment());
                }

                sec.DataStart() = dataWritePos;
                sec.WriteRawData(dataWritePos);
            }
            dataWritePos = (long)dataWritePos + sec.DataSize();
            dataWritten[i] = true;
        }
    }

    // The relocation tables go on the end:
    for ( int i =0; i< header.Sections(); i++ ) {
        Section& sec = *(data.sections[i]);
        if (   !dataWritten[i] && sec.IsRelocTable())
        {
            // Align start position
            if ( sec.Alignment() != 0 )  {
                dataWritePos.Offset() = 
                    dataWritePos.NextBoundrary(sec.Alignment());
            }

            sec.DataStart() = dataWritePos;
            sec.WriteRawData(dataWritePos);
            dataWritePos = (long)dataWritePos + sec.DataSize();
            dataWritten[i] = true;
        }
    }

    return dataWritePos;
}


BinaryWriter ElfFile::WriteDataSections( ElfContent &data, 
                                         ProgramHeader& prog,
                                         BinaryWriter& writer) {
    BinaryWriter end = writer;
    BinaryWriter writePos = writer;
    Section * section;
    for ( auto sname: prog.SectionNames() ) {
        section = data.sections[data.sectionMap[sname]];
        if ( prog.Address() != 0 ) {
            if ( section->DataSize() == 0 ) { 
                // don't care
                continue;
            }
            if ( section->Address() == 0 ) {
                string error = "FATAL ERROR: A section in a ";
                error += "loadable segment MUST specify a load ";
                error += "address. Segment table is currupt, I ";
                error += "cannot continue";
                error += "( " + sname + " )";
                throw error;
            }
            writePos = (long)writer + section->Address() 
                                    - prog.Address();

            section->DataStart() = writePos;
            section->WriteRawData(writePos);
            if (end <=  section->DataSize() + writePos)
                 end =  section->DataSize() + writePos;

        } else {
            writePos = (long)end;
            section->DataStart() = writePos;
            section->WriteRawData(writePos);
            // These are not loadable, we have no responsibility to 
            // align them
            end = section->DataSize() + writePos;
        }
    }
    writePos = (long)end;
    return writePos;
}

bool ElfFile::IsSpecialSection(Section& s) {
    bool special = false;
    special |= s.Name() == ".shstrtab";
    special |= s.Name() == ".symtab";
    special |= s.Name() == ".strtab";
    return special;
}

void ElfFile::WriteSectionHeaders(ElfContent &data ) {
    auto writer = sectionHeadersStart;
    long idx = 0;
    // Write the standard header sections
    for ( Section* sec : data.sections ) {
        if ( ! IsSpecialSection( *sec ) ) {
            writer << (Elf64_Shdr) *sec;
            ++idx;
        }
    }

    WriteSpecial(data, ".shstrtab" ,idx, writer);
    WriteSpecial(data, ".symtab", idx, writer);
    WriteSpecial(data, ".strtab", idx, writer);
}

void ElfFile::WriteSpecial(ElfContent& data, string name,
                                             long& idx,
                                             BinaryWriter& writer) {
    if ( name == ".shstrtab" )  {
        this->header.StringTableIndex() = idx;
    }
    auto loc = data.sectionMap.find(name);
    if ( loc != data.sectionMap.end() ) {
        const Elf64_Shdr& sec = *data.sections[loc->second];
        writer << sec;
        ++idx;
    }
}

void ElfFile::WriteToFile(BinaryWriter& w) {
    this->file.Reader().Read(w,this->file.Size());
}
