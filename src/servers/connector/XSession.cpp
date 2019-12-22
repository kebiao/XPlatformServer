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
Session(sessionID, sock, pNetworkInterface, pEventDispatcher),
pBackendSession_(NULL)
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

	if (!isServer())
	{
		if (header_.msgcmd >= Login_Begin && header_.msgcmd <= Login_End)
		{
			// 第一次非公共协议通讯就直接绑定到具体app上
			if (!pBackendSession_ || pBackendSession_->appType() != ServerType::SERVER_TYPE_LOGIN)
			{
				onBindBackendServer(ServerType::SERVER_TYPE_LOGIN);
			}

			if (!pBackendSession_)
			{
				ERROR_MSG(fmt::format("XSession::onProcessPacket(): not found pBackendSession(login)!, id={}, {}, datasize={}\n",
					id(), addr(), header_.msglen));

				return false;
			}

			if (pBackendSession_->connected())
			{
				CMD_ForwardPacket packet;
				packet.set_requestorid(id());
				packet.set_datas(data, size);
				packet.set_msgcmd(header_.msgcmd);
				pBackendSession_->sendPacket(CMD::ForwardPacket, packet);
			}

			return true;
		}
		else if (header_.msgcmd >= Halls_Begin && header_.msgcmd <= Halls_End)
		{
			// 第一次非公共协议通讯就直接绑定到具体app上
			if (!pBackendSession_ || pBackendSession_->appType() != ServerType::SERVER_TYPE_HALLS)
			{
				ServerAppID hallsID = 0;

				// 对于登陆协议， 如果其中包含了hallsID那么需要绑定到指定hallsID上
				if (header_.msgcmd == CMD::Halls_Login)
				{
					CMD_Halls_Login packet;
					if (!packet.ParseFromArray(data, header_.msglen))
					{ 
						ERROR_MSG(fmt::format("XSession::onProcessPacket(): loginPacket parsing error! {}, size={}, sessionID={}, {}\n",
							CMD_Name((CMD)header_.msgcmd), header_.msglen, id(), pTcpSocket()->addr()));

						return false; 
					}

					hallsID = packet.hallsid();
				}
				
				onBindBackendServer(ServerType::SERVER_TYPE_HALLS, hallsID);
			}

			if (!pBackendSession_)
			{
				ERROR_MSG(fmt::format("XSession::onProcessPacket(): not found pBackendSession(halls)!, id={}, {}, datasize={}\n",
					id(), addr(), header_.msglen));

				return false;
			}

			if (pBackendSession_->connected())
			{
				CMD_ForwardPacket packet;
				packet.set_requestorid(id());
				packet.set_datas(data, size);
				packet.set_msgcmd(header_.msgcmd);
				pBackendSession_->sendPacket(CMD::ForwardPacket, packet);
			}

			return true;
		}
	}

	switch (header_.msgcmd)
	{
	default:
		{
			ERROR_MSG(fmt::format("XSession::onProcessPacket(): unknown packet cmd: {}, msglen={}, sessionID={}, {}\n",
				CMD_Name((CMD)packetHeader().msgcmd), packetHeader().msglen, id(), pTcpSocket()->addr()));

			return false;
			break;
		}
	};
}

//-------------------------------------------------------------------------------------
bool XSession::onForwardPacket(const CMD_ForwardPacket& packet)
{
	if (!isServer())
	{
		if (this->pBackendSession())
		{
			this->pBackendSession()->sendPacket(CMD::ForwardPacket, packet);
			return true;
		}

		ERROR_MSG(fmt::format("XSession::onProcessForwardPacket(): not found pBackendSession!, id={}, {}, datasize={}\n",
			id(), addr(), header_.msglen));

		return false;
	}

	// 如果是服务器内部session，说明需要将包转发给客户端
	SessionID sessionID = packet.requestorid();
	std::string datas = packet.datas();

	// 找到请求者的session， 将包投递给他解析
	Session* pSession = ((XServerApp&)XServerApp::getSingleton()).pExternalNetworkInterface()->findSession(sessionID);
	if (!pSession)
	{
		ERROR_MSG(fmt::format("Session::onForwardPacket(): not found requester({}), drop packet!", sessionID));

		return true;
	}

	assert(!pSession->isServer());

	PacketHeader& header = pSession->packetHeader();
	header.msgcmd = packet.msgcmd();
	header.msglen = datas.size();

	return pSession->sendPacket(header.msgcmd, (uint8*)(datas.data()), datas.size());
}

//-------------------------------------------------------------------------------------
void XSession::onBindBackendServer(ServerType type, uint64 hallsID)
{
	// 将会话绑定到后端App会话
	((XServerApp&)XServerApp::getSingleton()).onSessionBindBackendSession(type, this, hallsID);
}

//-------------------------------------------------------------------------------------
void XSession::onLoseBackendSession()
{
	pBackendSession(NULL);

	ERROR_MSG(fmt::format("XSession::onLoseBackendSession(): {}, addr={}!\n",
		id(), addr()));

	destroy();
}

//-------------------------------------------------------------------------------------
void XSession::onDisconnected()
{
	// client disconnect
	if (this->pBackendSession())
	{
		DEBUG_MSG(fmt::format("XSession::onDisconnected(): client disconnected! {}, addr={}\n",
			id(), addr()));

		CMD_RemoteDisconnected packet;
		pBackendSession_->forwardPacket(id(), CMD::RemoteDisconnected, packet);
	}
}

//-------------------------------------------------------------------------------------
}
