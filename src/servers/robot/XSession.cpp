#include "XSession.h"
#include "XRobot.h"
#include "XServerApp.h"
#include "log/XLog.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XSession::XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher):
Session(sessionID, sock, pNetworkInterface, pEventDispatcher),
pXRobot_(NULL)
{
}

//-------------------------------------------------------------------------------------
XSession::~XSession()
{
}

//-------------------------------------------------------------------------------------
bool XSession::onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size)
{
	Session::onProcessPacket(requestorSessionID, data, size);
	pXRobot_->onProcessPacket(this, data, size);

	return true;
}

//-------------------------------------------------------------------------------------
void XSession::onHelloCB(const CMD_HelloCB& packet)
{
	Session::onHelloCB(packet);
	
	pXRobot_->onHelloCB(packet);
}

//-------------------------------------------------------------------------------------
void XSession::onConnected()
{
	pXRobot_->onConnected(this);
}

//-------------------------------------------------------------------------------------
void XSession::onDisconnected()
{
	pXRobot_->onDisconnected(this);
}

//-------------------------------------------------------------------------------------
}
