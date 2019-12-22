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
	case CMD::Machine_RequestCreateRoom:
	{
		CMD_Machine_RequestCreateRoom packet;
		PARSE_PACKET();

		onRequestCreateRoom(packet);
		break;
	}
	case CMD::Machine_RoomSrvReportAddr:
	{
		CMD_Machine_RoomSrvReportAddr packet;
		PARSE_PACKET();

		onRoomSrvReportAddr(packet);
		break;
	}
	case CMD::Machine_OnRoomSrvGameOverReport:
	{
		CMD_Machine_OnRoomSrvGameOverReport packet;
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
void XSession::onRequestCreateRoom(const CMD_Machine_RequestCreateRoom& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestCreateRoom(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRoomSrvReportAddr(const CMD_Machine_RoomSrvReportAddr& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRoomSrvReportAddr(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRoomSrvGameOverReport(const CMD_Machine_OnRoomSrvGameOverReport& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRoomSrvGameOverReport(this, packet);
}

//-------------------------------------------------------------------------------------
}
