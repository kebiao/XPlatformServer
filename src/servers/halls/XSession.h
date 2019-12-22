#ifndef X_XSESSION_H
#define X_XSESSION_H

#include "event/Session.h"

namespace XServer {

class XObject;
typedef std::shared_ptr<XObject> XObjectPtr;

class XSession : public Session
{
public:
	XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher);
	virtual ~XSession();

protected:
	virtual bool onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size) override;
	virtual bool onForwardPacket(const CMD_ForwardPacket& packet) override;

	virtual void onRemoteDisconnected(SessionID requestorSessionID, const CMD_RemoteDisconnected& packet);

	virtual void onRequestAllocClient(const CMD_Halls_RequestAllocClient& packet);
	virtual void onLogin(SessionID requestorSessionID, const CMD_Halls_Login& packet);
	virtual void onStartMatch(SessionID requestorSessionID, const CMD_Halls_StartMatch& packet);
	
	virtual void onRequestCreateRoomCB(const CMD_Halls_OnRequestCreateRoomCB& packet);
	virtual void onRoomSrvGameOverReport(const CMD_Halls_OnRoomSrvGameOverReport& packet);

	virtual void onQueryAccountCB(const CMD_Halls_OnQueryAccountCB& packet);
	virtual void onQueryPlayerGameDataCB(const CMD_Halls_OnQueryPlayerGameDataCB& packet);
	virtual void onQueryPlayerGameData(SessionID requestorSessionID, const CMD_Halls_QueryPlayerGameData& packet);

	virtual void onListGames(SessionID requestorSessionID, const CMD_Halls_ListGames& packet);
	
public:
	XObjectPtr pPlayer;
};

}

#endif // X_XSESSION_H
