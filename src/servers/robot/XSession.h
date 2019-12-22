#ifndef X_XSESSION_H
#define X_XSESSION_H

#include "event/Session.h"

namespace XServer {

class XRobot;

class XSession : public Session
{
public:
	XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher);
	virtual ~XSession();

	void pXRobot(XRobot* v) {
		pXRobot_ = v;
	}

protected:
	virtual bool onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size) override;
	virtual void onHelloCB(const CMD_HelloCB& packet) override;

	virtual void onConnected() override;
	virtual void onDisconnected() override;

	XRobot* pXRobot_;

};

}

#endif // X_XSESSION_H
