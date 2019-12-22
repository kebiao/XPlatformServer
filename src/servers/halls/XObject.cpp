#include "XObject.h"
#include "XServerApp.h"
#include "event/TcpSocket.h"
#include "event/NetworkInterface.h"
#include "event/Session.h"
#include "log/XLog.h"
#include "protos/Commands.pb.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XObject::XObject(ObjectID oid, Session* pSession):
id_(oid),
pSession_(pSession),
isDestroyed_(false)
{
	DEBUG_MSG(fmt::format("new XObject(): {}\n", id_));
}

//-------------------------------------------------------------------------------------
XObject::~XObject()
{
	DEBUG_MSG(fmt::format("XObject::~XObject(): {}\n", id_));
}

//-------------------------------------------------------------------------------------
void XObject::onClientConnected(Session* pSession)
{
	pSession_ = pSession;

	DEBUG_MSG(fmt::format("XObject::onClientConnected(): {}\n", id_));
}

//-------------------------------------------------------------------------------------
void XObject::onClientDisconnected(Session* pSession)
{
	DEBUG_MSG(fmt::format("XObject::onClientDisconnected(): {}\n", id_));

	pSession_ = NULL;
}

//-------------------------------------------------------------------------------------
void XObject::destroy()
{
	if (isDestroyed_)
		return;

	isDestroyed_ = true;

	DEBUG_MSG(fmt::format("XObject::destroy(): {}\n", id_));

	onDestroy();
	((XServerApp&)XServerApp::getSingleton()).delObject(id());
}

//-------------------------------------------------------------------------------------
void XObject::onDestroy()
{
}

//-------------------------------------------------------------------------------------
}
