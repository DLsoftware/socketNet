#include "GameSocket.h"
#include "MessageMgr.h"
#include <assert.h>

static CGameSocket *s_pGameSocket = nullptr;

CGameSocket::CGameSocket()
{
}


CGameSocket::~CGameSocket()
{
}

CGameSocket* CGameSocket::getInstance()
{
	if (s_pGameSocket == nullptr)
		s_pGameSocket = new CGameSocket();
	return s_pGameSocket;
}

bool CGameSocket::Create(const char* pszServerIP, int nServerPort, int nBlockSec /*= BLOCKSECONDS*/, bool bKeepAlive /*= false*/)
{
	if (pszServerIP == nullptr)
		return false;

#ifdef WIN32// WIN32 下 开启socket服务
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 0);
	int ret = WSAStartup(version, &wsaData);
	if (ret != 0)
		return false;
#endif

	//创建套接字
	m_sockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sockClient == INVALID_SOCKET)
	{
		closeSocket();
		return false;
	}

#ifdef WIN32
	DWORD nMode = 1;
	int nRes = ioctlsocket(m_sockClient, FIONBIO, &nMode);
#else//linux
	fcntl(m_sockClient, F_SETFL, 0_NONBLOCK);
#endif // WIN32  启用 非阻塞模式

	sockaddr_in addr_in;
	memset(&addr_in, 0, sizeof(addr_in));
	//协议族 TCP/IP
	addr_in.sin_family = AF_INET;
	//端口
	addr_in.sin_port = htons(nServerPort);

	//IP
	unsigned long serveraddr = inet_addr(pszServerIP);
	if (serveraddr != INVALID_SOCKET)
		addr_in.sin_addr.s_addr = serveraddr;
	else
	{
#ifdef WIN32
		struct hostent *host = gethostbyname(pszServerIP);
		if (host == NULL)
		{
			closeSocket();
			return false;
		}

		addr_in.sin_addr = *(struct in_addr*)(host->h_addr_list[0]);
#else
		closeSocket();
		return false;
#endif // WIN32
	}

	//连接
	if (connect(m_sockClient, (sockaddr*)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
	{
		if (hasError())
		{
			closeSocket();
			return false;
		}
		else//等待连接
		{
			timeval timeout;
			timeout.tv_sec = nBlockSec;
			timeout.tv_usec = 0;
			fd_set readset, writeset;
			FD_ZERO(&writeset);
			FD_ZERO(&readset);
			FD_SET(m_sockClient, &writeset);
			FD_SET(m_sockClient, &readset);

			int ret = select(m_sockClient+1, &readset, &writeset, NULL, &timeout);//监听套接的可读和可写条件
			if (ret <= 0)
			{
				closeSocket();
				return false;
			}
			else
			{
				//可读可写 进一步判断
				if (FD_ISSET(m_sockClient, &readset) && FD_ISSET(m_sockClient, &writeset))
				{
					char error = 0;
					int len = sizeof(error);
					if (getsockopt(m_sockClient, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
					{
						closeSocket();
						return false;
					}

					if (error != 0)//如果error不为0， 则表示链接到此没有建立完成
					{
						closeSocket();
						return false;
					}

					//如果error为0，则说明链接建立完成
				}

				//可写不可读 连接成功
				if (!FD_ISSET(m_sockClient, &readset) && FD_ISSET(m_sockClient, &writeset))
				{
					
				}
				else
				{
					return false;
				}
			}
		}
	}

	//小数据发送等待，减轻网络负载
	int delayOn = 0;
	int keepAliveOn = bKeepAlive ? 1 : 0;
#ifdef WIN32  
	setsockopt(m_sockClient, SOL_SOCKET, SO_KEEPALIVE, (const char *)&keepAliveOn, sizeof(keepAliveOn));
	setsockopt(m_sockClient, IPPROTO_TCP, TCP_NODELAY, (const char *)&delayOn, sizeof(delayOn));
#else  
	setsockopt(socket_handle, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAliveOn sizeof(keepAliveOn));
	setsockopt(socket_handle, IPPROTO_TCP, TCP_NODELAY, (void *)&delayOn, sizeof(delayOn));
#endif 

	m_nOutBufLen = 0;
	m_nInBufLen = 0;
	m_nInBufStart = 0;
	memset(m_outBuf, 0, sizeof(m_outBuf));
	memset(m_inBuf, 0, sizeof(m_inBuf));

	return true;
}

bool CGameSocket::SendMsg(BYTE* pBuf, int nSize)
{
	if (pBuf == NULL || nSize <= 0)
		return false;

	if (m_sockClient == INVALID_SOCKET)
		return false;

	// 检查消息包长度
	int nPackSize = 0;
	nPackSize = nSize;
	// 检测BUFFER溢出
	if (m_nOutBufLen + nSize > OUTBUFSIZE)
	{
		// 立即发送OUTBUF中的数据，以清空OUTBUF
		Flush();
		if (m_nOutBufLen + nSize > OUTBUFSIZE)
		{
			// 出错了
			Destory();
			return false;
		}
	}
	// 数据添加到BUF尾
	memcpy(m_outBuf + m_nOutBufLen, pBuf, nSize);
	m_nOutBufLen += nSize;
	Flush();
	return true;
}

bool CGameSocket::RecvMsg(BYTE* pBuf, int& nSize, int& nCompressFlag)
{
	if (pBuf == NULL || nSize <= 0)
		return false;

	if (m_sockClient == INVALID_SOCKET)
		return false;

	if (m_nInBufLen < 4)
	{
		if (!recvFromSocket() || m_nInBufLen < 4)
			return false;
	}

	// 计算要拷贝的消息的大小（一个消息，大小为整个消息的前32位），因为环形缓冲区，所以要分开计算 
	// 服务器传过来的是大端存储的
	int nPackSize = ((unsigned char)m_inBuf[m_nInBufStart] << 24)
		+ ((unsigned char)m_inBuf[(m_nInBufStart + 1) % INBUFSIZE] << 16)
		+ ((unsigned char)m_inBuf[(m_nInBufStart + 2) % INBUFSIZE] << 8)
		+ ((unsigned char)m_inBuf[(m_nInBufStart + 3) % INBUFSIZE]); // 注意字节序，高位+低位 

	// 记录消息大小的数据长度要算进去
	int nHeadSize = 4;

	//是否压缩 包长度符号位表示是否压缩
	nCompressFlag = (nPackSize & 0x80000000) == 0x80000000;
	nPackSize = nPackSize & 0x7fffffff;

	assert(nPackSize > 0 && (nPackSize + nHeadSize) <= MAXMSGSIZE);
	

	// 检查消息是否完整(如果将要拷贝的消息大于此时缓冲区数据长度，需要再次请求接收剩余数据) 
	if (nPackSize > m_nInBufLen)
	{
		// 如果没有请求成功   或者    依然无法获取到完整的数据包  则返回，直到取得完整包 
		if (!recvFromSocket() || nPackSize > m_nInBufLen)
		{ // 这个m_nInbufLen已更新 
			return false;
		}
	}

	// 复制出一个消息 
	if (m_nInBufStart + nHeadSize + nPackSize > INBUFSIZE)//消息有回卷（被拆成两份在环形缓冲区的头尾） 
	{
		BYTE tmpBuf[INBUFSIZE] = {0};
		// 先拷贝环形缓冲区末尾的数据 
		int copylen = INBUFSIZE - m_nInBufStart;
		memcpy(tmpBuf, m_inBuf + m_nInBufStart, copylen);

		// 再拷贝环形缓冲区头部的剩余部分
		memcpy(tmpBuf + copylen, m_inBuf, nPackSize + nHeadSize - copylen);

		//去掉包长度字段
		memcpy(pBuf, tmpBuf + nHeadSize, nPackSize);

		nSize = nPackSize;
	}
	else
	{
		// 消息没有回卷，可以一次拷贝出去 
		memcpy(pBuf, m_inBuf + m_nInBufStart + nHeadSize, nPackSize);
		nSize = nPackSize;
	}

	// 重新计算环形缓冲区头部位置 
	m_nInBufStart = (m_nInBufStart + nHeadSize + nPackSize) % INBUFSIZE;
	m_nInBufLen -= (nHeadSize + nPackSize); // 消息头也要算进去 

	assert(m_nInBufLen >= 0);
	if (m_nInBufLen == 0)
		memset(m_inBuf, 0, sizeof(m_inBuf));

	return  true;
}

bool CGameSocket::Flush()
{
	if (m_sockClient == INVALID_SOCKET)
		return false;

	if (m_nOutBufLen <= 0)
		return true;

	int nOutSize = send(m_sockClient, m_outBuf, m_nOutBufLen, 0);
	//CCLOG("send size: %d", nOutSize);
	if (nOutSize > 0)
	{
		if (m_nOutBufLen - nOutSize > 0)
		{
			memcpy(m_outBuf, m_outBuf + nOutSize, m_nOutBufLen - nOutSize);
		}
		m_nOutBufLen -= nOutSize;

		assert(m_nOutBufLen >= 0);

		return Flush();
	}
	else
	{
		if (hasError())
		{
			Destory();
			return false;
		}

		return true;
	}
}

// 从缓存中读取尽可能多的数据
bool CGameSocket::recvFromSocket()
{
	if (m_nInBufLen >= INBUFSIZE || m_sockClient == INVALID_SOCKET)
		return false;

	//计算 到环形缓冲区尾部的 剩余空间
	/*
	m_nInBufStart			       (m_nInBufStart+m_nInBufLen)
	begin----|--------------------------------|---------end
	
	(m_nInBufStart+m_nInBufLen)			m_nInBufStart        
	begin----|--------------------------------|---------end
	
	*/
	int nSaveLen = 0;							    
	if (m_nInBufStart + m_nInBufLen < INBUFSIZE)
		nSaveLen = INBUFSIZE - (m_nInBufStart + m_nInBufLen);
	else
		nSaveLen = INBUFSIZE - m_nInBufLen;

	// 缓冲区数据的末尾 即 剩余空间的起始位置
	int nSavePos = 0;
	nSavePos = (m_nInBufStart + m_nInBufLen) % INBUFSIZE;

	//从 套接字 copy 数据
	int nInLen = recv(m_sockClient, m_inBuf + nSavePos, nSaveLen, 0);
	
	//nInLen == 0 表示对端的socket已正常关闭
	//nInLen < 0 由于是非阻塞的模式,所以当errno为EAGAIN时,表示当前缓冲区已无数据可读 交给 hasError 处理
	if (nInLen == 0 || (nInLen < 0 && hasError()))
	{
		Destory();
		return false;
	}

	if (nInLen > 0)
	{
		//CCLOG("receive size: %d", nInLen);

		m_nInBufLen += nInLen;
		assert(m_nInBufLen <= INBUFSIZE);

		//nInLen == nSaveLen 可能还有数据没copy完
		//m_nInBufLen < INBUFSIZE	出现这种情况  说明环形缓冲区 头部 还有一块空闲空间
		if (nInLen == nSaveLen && m_nInBufLen < INBUFSIZE)
		{
			int nSaveLen = INBUFSIZE - m_nInBufLen;
			int nSavePos = (m_nInBufStart + m_nInBufLen) % INBUFSIZE;
			nInLen = recv(m_sockClient, m_inBuf + nSavePos, nSaveLen, 0);
			if (nInLen == 0 || (nInLen < 0 && hasError()))
			{
				Destory();
				return false;
			}

			if (nInLen > 0)
			{
				m_nInBufLen += nInLen;
				assert(m_nInBufLen <= INBUFSIZE);
			}
		}
		else if (nInLen < nSaveLen)//数据拷贝完了
		{
			return true;
		}
	}

	return true;
}

bool CGameSocket::hasError()
{
#ifdef WIN32
	int nErr = WSAGetLastError();
	if (nErr != WSAEWOULDBLOCK)
#else
	int nErr = errno;
	if (nErr != EINPROGRESS && nErr != EAGAIN)
#endif
	{
		return true;
	}

	return false;
}

void CGameSocket::closeSocket()
{
#ifdef WIN32
	closesocket(m_sockClient);
	WSACleanup();
#else
	close(m_sockClient);
#endif
	m_sockClient = INVALID_SOCKET;
	CMessageMgr::getInstance()->CallConnectResult(CONNECT_ERROR);
}

void CGameSocket::Destory()
{
	// 关闭
	struct linger so_linger;
	so_linger.l_onoff = 1;
	so_linger.l_linger = 500;
	int ret = setsockopt(m_sockClient, SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof(so_linger));

	closeSocket();
	m_nInBufLen = 0;
	m_nInBufStart = 0;
	m_nOutBufLen = 0;

	memset(m_outBuf, 0, sizeof(m_outBuf));
	memset(m_inBuf, 0, sizeof(m_inBuf));
}
