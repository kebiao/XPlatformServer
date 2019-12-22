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
	virtual bool onForwardPacket(const CMD_ForwardPacket& packet) override;

	virtual void onSignup(SessionID requestorSessionID, const CMD_Login_Signup& packet);
	virtual void onSignin(SessionID requestorSessionID, const CMD_Login_Signin& packet);

	virtual void onSignupCB(const CMD_Login_OnSignupCB& packet);
	virtual void onSigninCB(const CMD_Login_OnSigninCB& packet);

	virtual void onRequestAllocClientCB(const CMD_Login_OnRequestAllocClientCB& packet);

	virtual void onRemoteDisconnected(SessionID requestorSessionID, const CMD_RemoteDisconnected& packet);
};

}

#endif // X_XSESSION_H
