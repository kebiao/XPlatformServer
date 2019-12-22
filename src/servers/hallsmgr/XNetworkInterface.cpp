#include "XNetworkInterface.h"
#include "XSession.h"
#include "log/XLog.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XNetworkInterface::XNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternalNetwork):
NetworkInterface(pEventDispatcher, isInternalNetwork)
{
}

//-------------------------------------------------------------------------------------
XNetworkInterface::~XNetworkInterface()
{
}

//-------------------------------------------------------------------------------------
Session* XNetworkInterface::createSession_(SessionID sessionID, evutil_socket_t sock)
{
	return new XSession(sessionID, sock, this, this->pEventDispatcher());
}

//-------------------------------------------------------------------------------------
}
