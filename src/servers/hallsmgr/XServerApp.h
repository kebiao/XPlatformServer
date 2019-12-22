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

	ServerInfo* findBestServer();

	void sendFailed(Session* pSession, const std::string& commitAccountname, CMD cmd, ServerError err);

	void onSessionRequestAllocClient(Session* pSession, const CMD_Hallsmgr_RequestAllocClient& packet);
	void onSessionRequestAllocClientCB(Session* pSession, const CMD_Hallsmgr_OnRequestAllocClientCB& packet);

protected:

};

}

#endif // X_SERVER_APP_H
