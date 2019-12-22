#ifndef X_XNETWORKINTERFACE_H
#define X_XNETWORKINTERFACE_H

#include "event/NetworkInterface.h"

namespace XServer {

class XNetworkInterface : public NetworkInterface
{
public:
	XNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternalNetwork);
	virtual ~XNetworkInterface();

	virtual Session* createSession_(SessionID sessionID, evutil_socket_t sock) override;

protected:
};

}

#endif // X_XNETWORKINTERFACE_H
