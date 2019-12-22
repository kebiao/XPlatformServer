#ifndef X_XSESSION_H
#define X_XSESSION_H

#include "event/Session.h"

namespace XServer {

class XSession : public Session
{
public:
	XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher);
	virtual ~XSession();

protected:
	virtual bool onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size) override;

	virtual void onRequestCreateRoom(const CMD_Roommgr_RequestCreateRoom& packet);
	virtual void onRequestCreateRoomCB(const CMD_Roommgr_OnRequestCreateRoomCB& packet);
	virtual void onRoomSrvGameOverReport(const CMD_Roommgr_OnRoomSrvGameOverReport& packet);
};

}

#endif // X_XSESSION_H
