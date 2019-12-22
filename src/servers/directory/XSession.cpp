#include "XSession.h"
#include "XServerApp.h"
#include "event/TcpSocket.h"
#include "log/XLog.h"
#include "protos/Commands.pb.h"
#include "server/XServerBase.h"

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
	if (header_.msgcmd <= Directory_Begin || header_.msgcmd >= Directory_End)
	{
		ERROR_MSG(fmt::format("XSession::onProcessPacket(): invalid packet cmd: {}, msgcmd={}, sessionID={}, {}\n",
			packetHeader().msglen, CMD_Name((CMD)packetHeader().msgcmd), id(), pTcpSocket()->addr()));

		return true;
	}

	switch (header_.msgcmd)
	{
	case CMD::Directory_ListServers:
		{
			CMD_Directory_ListServers packet;
			PARSE_PACKET();

			onListServers(packet);
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
void XSession::onListServers(const CMD_Directory_ListServers& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionListServers(this, packet);
}

//-------------------------------------------------------------------------------------
}
