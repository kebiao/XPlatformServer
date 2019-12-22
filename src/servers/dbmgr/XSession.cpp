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
	case CMD::Dbmgr_WriteAccount:
	{
		CMD_Dbmgr_WriteAccount packet;
		PARSE_PACKET();

		onWriteAccount(packet);
		break;
	}
	case CMD::Dbmgr_QueryAccount:
	{
		CMD_Dbmgr_QueryAccount packet;
		PARSE_PACKET();

		onQueryAccount(packet);
		break;
	}
	case CMD::Dbmgr_UpdateAccountData:
	{
		CMD_Dbmgr_UpdateAccountData packet;
		PARSE_PACKET();

		onUpdateAccountData(packet);
		break;
	}
	case CMD::Dbmgr_WritePlayerGameData:
	{
		CMD_Dbmgr_WritePlayerGameData packet;
		PARSE_PACKET();

		onWritePlayerGameData(packet);
		break;
	}
	case CMD::Dbmgr_QueryPlayerGameData:
	{
		CMD_Dbmgr_QueryPlayerGameData packet;
		PARSE_PACKET();

		onQueryPlayerGameData(packet);
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
void XSession::onWriteAccount(const CMD_Dbmgr_WriteAccount& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionWriteAccount(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onQueryAccount(const CMD_Dbmgr_QueryAccount& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionQueryAccount(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onUpdateAccountData(const CMD_Dbmgr_UpdateAccountData& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionUpdateAccountData(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onWritePlayerGameData(const CMD_Dbmgr_WritePlayerGameData& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionWritePlayerGameData(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onQueryPlayerGameData(const CMD_Dbmgr_QueryPlayerGameData& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionQueryPlayerGameData(this, packet);
}

//-------------------------------------------------------------------------------------
}
