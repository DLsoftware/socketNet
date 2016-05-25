#include "Bean.h"

CBean::CBean()
{
	m_bufferSize = 0;
	m_capacitySize = 0;
	m_pBuffer = (char*)malloc(BUFFSIZE_INC);
	memset(m_pBuffer, 0, BUFFSIZE_INC);
	m_capacitySize += BUFFSIZE_INC;
}


CBean::~CBean()
{
	m_bufferSize = 0;
	free(m_pBuffer);
}

//检测buff长度是否足够，动态增长
bool CBean::CheckBuffSize(INT curBufSize, INT addLen)
{
	if (m_bufferSize + addLen > curBufSize)
	{
		if (CheckBuffSize(BUFFSIZE_INC + curBufSize, addLen))
		{
			char* tmpCache = (char*)malloc(BUFFSIZE_INC + curBufSize);
			memcpy(tmpCache, m_pBuffer, curBufSize);
			free(m_pBuffer);
			m_pBuffer = tmpCache;
			m_capacitySize = BUFFSIZE_INC + curBufSize;
		}
		return false;
	}

	return true;
}

bool CBean::Writing(void)
{
	return true;
}

bool CBean::Reading(void)
{
	return true;
}

bool CBean::Read(BYTE* buf, INT bufSize)
{
	CheckBuffSize(m_capacitySize, bufSize);
	memcpy(m_pBuffer, buf, bufSize);
	bool ret = Reading();
	return ret;
}

void CBean::writeInt(INT val)
{
	CheckBuffSize(m_capacitySize, sizeof(INT));
	INT bigVal = __SWAP32(val);
	memcpy(m_pBuffer + m_bufferSize, &bigVal, sizeof(INT));
	m_bufferSize += sizeof(INT);
}

void CBean::writeString(const char* str)
{
	if (str == nullptr)
	{
		CheckBuffSize(m_capacitySize, sizeof(INT));
		memcpy(m_pBuffer + m_bufferSize, 0, sizeof(INT));
		m_bufferSize += sizeof(INT);
	}
	else
	{
		INT strLen = strlen(str);
		INT bigVal = __SWAP32(strLen);
		CheckBuffSize(m_capacitySize, strLen + sizeof(INT));

		memcpy(m_pBuffer + m_bufferSize, &bigVal, sizeof(INT));
		m_bufferSize += sizeof(INT);
		memcpy(m_pBuffer + m_bufferSize, str, strLen);
		m_bufferSize += strLen;
	}
}

void CBean::writeLong(string str)
{
	QWORD val = 0;

	if (str != "")
	{
		val = stoull(str);
	}

	CheckBuffSize(m_capacitySize, sizeof(QWORD));
	QWORD bigVal = __SWAP64(val);
	memcpy(m_pBuffer + m_bufferSize, &bigVal, sizeof(QWORD));
	m_bufferSize += sizeof(QWORD);
}

void CBean::writeShort(short val)
{
	CheckBuffSize(m_capacitySize, sizeof(short));
	short bigVal = __SWAP16(val);
	memcpy(m_pBuffer + m_bufferSize, &bigVal, sizeof(short));
	m_bufferSize += sizeof(short);
}

void CBean::writeByte(BYTE val)
{
	CheckBuffSize(m_capacitySize, sizeof(BYTE));
	memcpy(m_pBuffer + m_bufferSize, &val, sizeof(BYTE));
	m_bufferSize += sizeof(BYTE);
}

int CBean::readInt(void)
{
	INT ret = 0;
	memcpy(&ret, (m_pBuffer + m_bufferSize), sizeof(INT));
	m_bufferSize += sizeof(INT);
	ret = __SWAP32(ret);
	return ret;
}

std::string CBean::readString(void)
{
	INT nLen = readInt();
	if (nLen == 0)
		return "";

	char *ret = new char[nLen + 1];
	memcpy(ret, (m_pBuffer + m_bufferSize), nLen);
	m_bufferSize += nLen;
	ret[nLen] = '\0';
	return ret;
}

string CBean::readLong(void)
{
	QWORD  ret = 0;
	memcpy(&ret, (m_pBuffer + m_bufferSize), sizeof(QWORD));
	m_bufferSize += sizeof(QWORD);
	ret = __SWAP64(ret);
	
	return to_string(ret);
}

short CBean::readShort(void)
{
	short ret = 0;
	memcpy(&ret, (m_pBuffer + m_bufferSize), sizeof(short));
	m_bufferSize += sizeof(short);
	ret = __SWAP16(ret);
	return ret;
}

BYTE CBean::readByte(void)
{
	BYTE ret = 0;
	memcpy(&ret, (m_pBuffer + m_bufferSize), sizeof(BYTE));
	m_bufferSize += sizeof(BYTE);
	return ret;
}

void CBean::Release()
{
	m_bufferSize = 0;
	free(m_pBuffer);
}
