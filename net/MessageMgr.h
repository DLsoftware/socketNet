#ifndef __MESSAGEMGR_H__
#define __MESSAGEMGR_H__

#include "Bean.h"
#include <functional>
#include <map>

enum ConnectResultType
{
	CONNECT_ERROR = -1,
	CONNECT_TIMEOUT,
	CONNECT_SUCCESS
};

//连接结果回调
typedef std::function<void(ConnectResultType)> ccConnectResultCallback;

class CMessageMgr
{
public:
	CMessageMgr();
	~CMessageMgr();

	static CMessageMgr* getInstance();

	CBean* Recv();
	bool Connect(const char* cIP, int nPort, const ccConnectResultCallback& connectCall);
	bool Send(CBean* msg);
	void Close();
	void CallConnectResult(ConnectResultType type);
private:

	bool m_bIsConnected;
	int m_nMessageCount;

	ccConnectResultCallback m_cConnectResultCall;
};

#endif
