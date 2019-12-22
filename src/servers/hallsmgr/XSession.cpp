#include "XSession.h"
#include "XServerApp.h"
#include "event/TcpSocket.h"
#include "event/NetworkInterface.h"
#include "log/XLog.h"
#include "protos/Commands.pb.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XSession::XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher):
Session(sessionID, sock, pNetworkInterface, pEventDispatcher)
{
}

//-------------------------------------------------------------------------------------
XSession::~XSession()
{
}

//-------------------------------------------------------------------------------------
bool XSession::onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size)
{
	if (header_.msgcmd >= CMD_MAX)
	{
		ERROR_MSG(fmt::format("XSession::onProcessPacket(): invalid packet cmd: {}, msgcmd={}, sessionID={}, {}\n",
			packetHeader().msglen, packetHeader().msgcmd, id(), pTcpSocket()->addr()));

		return true;
	}

	switch (header_.msgcmd)
	{
	case CMD::Hallsmgr_RequestAllocClient:
	{
		CMD_Hallsmgr_RequestAllocClient packet;
		PARSE_PACKET();

		onRequestAllocClient(packet);
		break;
	}
	case CMD::Hallsmgr_OnRequestAllocClientCB:
	{
		CMD_Hallsmgr_OnRequestAllocClientCB packet;
		PARSE_PACKET();

		onRequestAllocClientCB(packet);
		break;
	}
	default:
	{
		ERROR_MSG(fmt::format("XSession::onProcessPacket(): unknown packet cmd: {}, msglen={}, sessionID={}, {}\n",
			CMD_Name((CMD)packetHeader().msgcmd), packetHeader().msglen, id(), pTcpSocket()->addr()));

		return false;
		break;
	}
	};

	return true;
}

//-------------------------------------------------------------------------------------
void XSession::onRequestAllocClient(const CMD_Hallsmgr_RequestAllocClient& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestAllocClient(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRequestAllocClientCB(const CMD_Hallsmgr_OnRequestAllocClientCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestAllocClientCB(this, packet);
}

//-------------------------------------------------------------------------------------
}
