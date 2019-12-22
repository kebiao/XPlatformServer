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
	case CMD::Login_Signup:
	{
		CMD_Login_Signup packet;
		PARSE_PACKET();

		onSignup(requestorSessionID, packet);
		break;
	}
	case CMD::Login_OnSignupCB:
	{
		CMD_Login_OnSignupCB packet;
		PARSE_PACKET();

		onSignupCB(packet);
		break;
	}
	case CMD::Login_Signin:
	{
		CMD_Login_Signin packet;
		PARSE_PACKET();

		onSignin(requestorSessionID, packet);
		break;
	}
	case CMD::Login_OnSigninCB:
	{
		CMD_Login_OnSigninCB packet;
		PARSE_PACKET();

		onSigninCB(packet);
		break;
	}
	case CMD::Login_OnRequestAllocClientCB:
	{
		CMD_Login_OnRequestAllocClientCB packet;
		PARSE_PACKET();

		onRequestAllocClientCB(packet);
		break;
	}
	case CMD::RemoteDisconnected:
	{
		CMD_RemoteDisconnected packet;
		PARSE_PACKET();

		onRemoteDisconnected(requestorSessionID, packet);
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

	return onProcessPacket_(sessionID, (uint8*)(datas.data()), datas.size());
}

//-------------------------------------------------------------------------------------
void XSession::onRemoteDisconnected(SessionID requestorSessionID, const CMD_RemoteDisconnected& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRemoteDisconnected(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onSignup(SessionID requestorSessionID, const CMD_Login_Signup& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionSignup(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onSignupCB(const CMD_Login_OnSignupCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionSignupCB(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onSignin(SessionID requestorSessionID, const CMD_Login_Signin& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionSignin(requestorSessionID, this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onSigninCB(const CMD_Login_OnSigninCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionSigninCB(this, packet);
}

//-------------------------------------------------------------------------------------
void XSession::onRequestAllocClientCB(const CMD_Login_OnRequestAllocClientCB& packet)
{
	((XServerApp&)XServerApp::getSingleton()).onSessionRequestAllocClientCB(this, packet);
}

//-------------------------------------------------------------------------------------
}
