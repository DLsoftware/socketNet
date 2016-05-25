#include "MessageMgr.h"
#include "GameSocket.h"
#include <iostream>
#include "zlib.h"

using namespace cocos2d;

static CMessageMgr* s_messageMgr = nullptr;


CMessageMgr::CMessageMgr()
{
	m_bIsConnected = false;
	m_nMessageCount = 0;
	m_cConnectResultCall = nullptr;
}


CMessageMgr::~CMessageMgr()
{

}

CMessageMgr* CMessageMgr::getInstance()
{
	if (s_messageMgr == nullptr)
	{
		s_messageMgr = new CMessageMgr();
	}
		
	return s_messageMgr;
}

bool CMessageMgr::Connect(const char* cIP, int nPort, const ccConnectResultCallback& connectCall)
{
	if (!m_bIsConnected)
	{
		m_bIsConnected = CGameSocket::getInstance()->Create(cIP, nPort);
		m_cConnectResultCall = connectCall;
	}
	return m_bIsConnected;
}

bool CMessageMgr::Send(CBean* msg)
{
	if (!m_bIsConnected)
		return false;

	BYTE sendBuffer[MAXMSGSIZE];
	memset(sendBuffer, 0, MAXMSGSIZE);
	
	//数据包 = ｛包体长度字段(int) + 校验码字段(int) + 消息体(data)｝
	//包体长度 = 校验码字段长度 + 消息体长度
	INT bagSize = sizeof(INT)+msg->m_bufferSize;
	INT nMsglen = __SWAP32(bagSize);
	memcpy(sendBuffer, &nMsglen, sizeof(INT));

	//校验码
	INT code = m_nMessageCount;
	code = __SWAP32(code);
	memcpy(sendBuffer + sizeof(INT), &code, sizeof(INT));

	//消息体
	memcpy(sendBuffer + sizeof(INT)*2, msg->m_pBuffer, msg->m_bufferSize);

	//加上 包体长度字段的长度
	CGameSocket::getInstance()->SendMsg(sendBuffer, bagSize + sizeof(INT));

	delete msg;
	msg = nullptr;

	m_nMessageCount++;
	return true;
}

void CMessageMgr::Close()
{
	if (m_bIsConnected)
	{
		CGameSocket::getInstance()->Destory();
		m_bIsConnected = false;
		m_nMessageCount = 0;
	}
}

CBean* CMessageMgr::Recv()
{
	BYTE msgBuffer[MAXMSGSIZE] = { 0 };
	INT nSize = sizeof(msgBuffer);
	INT nCompressFlag = 0;
	if (!CGameSocket::getInstance()->RecvMsg(msgBuffer, nSize, nCompressFlag))
		return nullptr;

	//INT nMessageID = __SWAP32(*(INT*)(msgBuffer));

	CBean* bean = new CBean();

	// 消息是压缩过的
	if (nCompressFlag == 1)
	{
		BYTE *pUnCompress = NULL;
		INT nLen = ZipUtils::ccInflateMemory(msgBuffer + sizeof(INT), nSize, &pUnCompress);
		if (nLen != 0)
		{
			assert(nLen <= MAXMSGSIZE);

			memcpy(msgBuffer + sizeof(INT), pUnCompress, nLen);
			bean->Read(msgBuffer, nLen + sizeof(INT));
			free(pUnCompress);
		}
	}
	else
	{
		bean->Read(msgBuffer, nSize);
	}

	return bean;
}

void CMessageMgr::CallConnectResult(ConnectResultType type)
{
	if (m_cConnectResultCall)
	{
		m_cConnectResultCall(type);
	}
}

