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

	void onSessionRequestCreateRoom(Session* pSession, const CMD_Roommgr_RequestCreateRoom& packet);
	void onSessionRequestCreateRoomCB(Session* pSession, const CMD_Roommgr_OnRequestCreateRoomCB& packet);
	void onSessionRoomSrvGameOverReport(Session* pSession, const CMD_Roommgr_OnRoomSrvGameOverReport& packet);

	ServerInfo *findBestMachines();

protected:

};

}

#endif // X_SERVER_APP_H
