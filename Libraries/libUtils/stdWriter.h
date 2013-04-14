#ifndef STD_WRITER_H
#define STD_WRITER_H
#include <fstream>
#include <string>
#include "binaryWriter.h"

using namespace std;

class StdWriter: public virtual FileLikeWriter {
public:
    StdWriter (ostream &f);
    virtual void Write(long offset, const void *src, long size);
    virtual void Flush();
    virtual void Put(long offset, unsigned char c);
    virtual void Fill(long offset, unsigned char c, long count);

private:
    ostream &file;
};

class OFStreamWriter: public ofstream, public StdWriter {
public:
	OFStreamWriter(const char *fname);
	OFStreamWriter(const OFStreamWriter& rhs);
	string Fname() { return fileName;}
	virtual ~OFStreamWriter(){}
private:
    string fileName;
};


#endif
