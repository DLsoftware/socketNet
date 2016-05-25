#ifndef __CBEAN_H__
#define __CBEAN_H__

#include <string>
#include "net_config.h"

using namespace std;

#define BUFFSIZE_INC 1024

class CBean
{
public:
	CBean();
	virtual ~CBean();

	virtual bool Writing();
	virtual bool Reading();
	virtual bool Read(BYTE* buf, INT bufSize);

	virtual void writeInt(INT val);
	virtual void writeString(const char* str);
	virtual void writeLong(string str);
	virtual void writeShort(short val);
	virtual void writeByte(BYTE val);

	virtual int				readInt();
	virtual string			readString();
	virtual string			readLong();
	virtual short			readShort();
	virtual BYTE			readByte();

	void Release();
private:
	bool CheckBuffSize(INT, INT);

public:
	char *m_pBuffer;
	int m_bufferSize;
	int m_capacitySize;
};

#endif
