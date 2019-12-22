#ifndef X_SERVER_APP_H
#define X_SERVER_APP_H

#include "common/common.h"
#include "common/singleton.h"
#include "server/XServerBase.h"

namespace XServer {

class XServerApp : public XServerBase
{
public:
	XServerApp();
	~XServerApp();
	
	virtual NetworkInterface* createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal) override;

	virtual bool initializeBegin() override;
	virtual bool inInitialize() override;
	virtual bool initializeEnd() override;

	virtual void onServerJoined(ServerInfo* pServerInfo) override;
	virtual void onServerExit(ServerInfo* pServerInfo) override;

	virtual void onSessionConnected(Session* pSession) override;

	void onSessionBindBackendSession(ServerType type, Session* pSession, uint64 hallsID);

protected:
	std::map<ServerAppID, int> mapBackendSessionNums_;
};

}

#endif // X_SERVER_APP_H
