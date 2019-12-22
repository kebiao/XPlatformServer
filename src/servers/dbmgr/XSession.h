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

	virtual void onWriteAccount(const CMD_Dbmgr_WriteAccount& packet);
	virtual void onQueryAccount(const CMD_Dbmgr_QueryAccount& packet);
	virtual void onUpdateAccountData(const CMD_Dbmgr_UpdateAccountData& packet);
	virtual void onWritePlayerGameData(const CMD_Dbmgr_WritePlayerGameData& packet);
	virtual void onQueryPlayerGameData(const CMD_Dbmgr_QueryPlayerGameData& packet);
};

}

#endif // X_XSESSION_H
