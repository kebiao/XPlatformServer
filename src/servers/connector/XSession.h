#ifndef X_XSESSION_H
#define X_XSESSION_H

#include "event/Session.h"

namespace XServer {

class XSession : public Session
{
public:
	XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher);
	virtual ~XSession();

	Session* pBackendSession() {
		return pBackendSession_;
	}

	void pBackendSession(Session* pSession) {
		pBackendSession_ = pSession;
	}

	void onLoseBackendSession();

protected:
	virtual bool onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size) override;
	virtual bool onForwardPacket(const CMD_ForwardPacket& packet) override;

	virtual void onDisconnected() override;

	void onBindBackendServer(ServerType type, uint64 hallsID = 0);

	// 绑定的后端应用session， 例如：login、halls
	Session* pBackendSession_;

};

}

#endif // X_XSESSION_H
