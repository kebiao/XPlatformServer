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
	case CMD::Roommgr_RequestCreateRoom:
	{
		CMD_Roommgr_RequestCreateRoom packet;
		PARSE_PACKET();

		onRequestCreateRoom(packet);
		break;
	}
	case CMD::Roommgr_OnRequestCreateRoomCB:
	{
		CMD_Roommgr_OnRequestCreateRoomCB packet;
		PARSE_PACKET();

		onRequestCreateRoomCB(packet);
		break;
	}
	case CMD::Roommgr_OnRoomSrvGameOverReport:
	{
		CMD_Roommgr_OnRoomSrvGameOverReport packet;
		PARSE_PACKET();

		onRoomSrvGameOverReport(packet);
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
void XSession::onRequestCreateRoom(const CMD_Roommgr_RequestCreateRoom& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestCreateRoom(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRequestCreateRoomCB(const CMD_Roommgr_OnRequestCreateRoomCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestCreateRoomCB(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRoomSrvGameOverReport(const CMD_Roommgr_OnRoomSrvGameOverReport& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRoomSrvGameOverReport(this, packet);
}

//-------------------------------------------------------------------------------------
}
