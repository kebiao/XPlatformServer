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
	virtual void onRequestAllocClient(const CMD_Hallsmgr_RequestAllocClient& packet);
	virtual void onRequestAllocClientCB(const CMD_Hallsmgr_OnRequestAllocClientCB& packet);
};

}

#endif // X_XSESSION_H
