#ifndef __GAMESOCKET_H__
#define __GAMESOCKET_H__

#include "cocos2d.h"

#define MAXMSGSIZE		2 * 1024		// 消息最大长度
#define BLOCKSECONDS	5				// INIT函数阻塞时间
#define INBUFSIZE		( 2 * 1024 )	// 接收数据的缓存大小
#define OUTBUFSIZE		( 1024 )	// 发送数据缓存

class CGameSocket
{
public:
	CGameSocket();
	~CGameSocket();

	static CGameSocket* getInstance();

	bool Create(const char* pszServerIP, int nServerPort, int nBlockSec = BLOCKSECONDS, bool bKeepAlive = false);

	bool SendMsg(BYTE* pBuf, int nSize);
	bool RecvMsg(BYTE* pBuf, int& nSize, int& nCompressFlag);
	bool Flush();

	void Destory();
private:
	bool recvFromSocket();
	bool hasError();
	void closeSocket();

	SOCKET		m_sockClient;
	char		m_outBuf[OUTBUFSIZE];
	int			m_nOutBufLen;

	//环形缓冲区
	char	m_inBuf[INBUFSIZE];
	//环形缓冲区数据长度
	int		m_nInBufLen;
	//环形缓冲区数据开始处
	int		m_nInBufStart;
};

#endif
