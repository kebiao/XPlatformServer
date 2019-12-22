#include "XSession.h"
#include "XServerApp.h"
#include "XPlayer.h"
#include "event/TcpSocket.h"
#include "event/NetworkInterface.h"
#include "log/XLog.h"
#include "protos/Commands.pb.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XSession::XSession(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher):
Session(sessionID, sock, pNetworkInterface, pEventDispatcher),
pPlayer()
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
	case CMD::Halls_RequestAllocClient:
	{
		CMD_Halls_RequestAllocClient packet;
		PARSE_PACKET();

		onRequestAllocClient(packet);
		break;
	}
	case CMD::Halls_Login:
	{
		CMD_Halls_Login packet;
		PARSE_PACKET();

		onLogin(requestorSessionID, packet);
		break;
	}
	case CMD::Halls_StartMatch:
	{
		CMD_Halls_StartMatch packet;
		PARSE_PACKET();

		onStartMatch(requestorSessionID, packet);
		break;
	}
	case CMD::RemoteDisconnected:
	{
		CMD_RemoteDisconnected packet;
		PARSE_PACKET();

		onRemoteDisconnected(requestorSessionID, packet);
		break;
	}
	case CMD::Halls_OnRequestCreateRoomCB:
	{
		CMD_Halls_OnRequestCreateRoomCB packet;
		PARSE_PACKET();

		onRequestCreateRoomCB(packet);
		break;
	}
	case CMD::Halls_OnRoomSrvGameOverReport:
	{
		CMD_Halls_OnRoomSrvGameOverReport packet;
		PARSE_PACKET();

		onRoomSrvGameOverReport(packet);
		break;
	}
	case CMD::Halls_OnQueryAccountCB:
	{
		CMD_Halls_OnQueryAccountCB packet;
		PARSE_PACKET();

		onQueryAccountCB(packet);
		break;
	}
	case CMD::Halls_OnQueryPlayerGameDataCB:
	{
		CMD_Halls_OnQueryPlayerGameDataCB packet;
		PARSE_PACKET();

		onQueryPlayerGameDataCB(packet);
		break;
	}
	case CMD::Halls_QueryPlayerGameData:
	{
		CMD_Halls_QueryPlayerGameData packet;
		PARSE_PACKET();

		onQueryPlayerGameData(requestorSessionID, packet);
		break;
	}
	case CMD::Halls_ListGames:
	{
		CMD_Halls_ListGames packet;
		PARSE_PACKET();

		onListGames(requestorSessionID, packet);
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
bool XSession::onForwardPacket(const CMD_ForwardPacket& packet)
{
	SessionID sessionID = packet.requestorid();
	std::string datas = packet.datas();

	PacketHeader& header = packetHeader();
	header.msgcmd = packet.msgcmd();
	header.msglen = datas.size();

	pPlayer = ((XServerApp&)XServerApp::getSingleton()).findSessionPlayer(sessionID);
	bool ret = onProcessPacket_(sessionID, (uint8*)(datas.data()), datas.size());
	pPlayer.reset();

	return ret;
}

//-------------------------------------------------------------------------------------
void XSession::onRemoteDisconnected(SessionID requestorSessionID, const CMD_RemoteDisconnected& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRemoteDisconnected(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRequestAllocClient(const CMD_Halls_RequestAllocClient& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestAllocClient(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onLogin(SessionID requestorSessionID, const CMD_Halls_Login& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionLogin(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onStartMatch(SessionID requestorSessionID, const CMD_Halls_StartMatch& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionStartMatch(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRequestCreateRoomCB(const CMD_Halls_OnRequestCreateRoomCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestCreateRoomCB(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRoomSrvGameOverReport(const CMD_Halls_OnRoomSrvGameOverReport& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRoomSrvGameOverReport(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onQueryAccountCB(const CMD_Halls_OnQueryAccountCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionQueryAccountCB(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onQueryPlayerGameDataCB(const CMD_Halls_OnQueryPlayerGameDataCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionQueryPlayerGameDataCB(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onQueryPlayerGameData(SessionID requestorSessionID, const CMD_Halls_QueryPlayerGameData& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionQueryPlayerGameData(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onListGames(SessionID requestorSessionID, const CMD_Halls_ListGames& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionListGames(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
}
