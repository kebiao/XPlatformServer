#include "Session.h"
#include "EventDispatcher.h"
#include "NetworkInterface.h"
#include "TcpSocket.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "resmgr/ResMgr.h"
#include "event/Timer.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Session::Session(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher):
id_(sessionID),
pNetworkInterface_(pNetworkInterface),
pEventDispatcher_(pEventDispatcher),
pTcpSocket_(new TcpSocket(pEventDispatcher, sock)),
headerRcved_(false),
header_(),
isServer_(false),
connected_(false),
lastReceivedTime_(0),
appID_(0),
appType_(ServerType::SERVER_TYPE_UNKNOWN),
rtt_(-1),
destroyTimerEvent_(NULL)
{
}

//-------------------------------------------------------------------------------------
Session::~Session()
{
	finalise();
}

//-------------------------------------------------------------------------------------
EventDispatcher* Session::pEventDispatcher()
{
	return pEventDispatcher_;
}

//-------------------------------------------------------------------------------------		
bool Session::initialize()
{
	if (!pTcpSocket_ || !pTcpSocket_->isGood())
	{
		ERROR_MSG(fmt::format("Session::initialize(): TcpSocket error!\n"));
		return false;
	}

	if (!pTcpSocket_->setcb(recvCallback, sendCallback, eventCallback, this))
	{
		ERROR_MSG(fmt::format("Session::initialize(): set callbacks failed!\n"));
		return false;
	}

	if(!pTcpSocket_->enable(EV_READ | EV_WRITE))
	{
		ERROR_MSG(fmt::format("Session::initialize(): enable events[EV_READ | EV_WRITE] error!\n"));
		return false;
	}

	lastReceivedTime_ = getTimeStamp();

	appID_ = 0;
	appType_ = ServerType::SERVER_TYPE_UNKNOWN;

	return true;
}

//-------------------------------------------------------------------------------------
void Session::finalise(void)
{
	id_ = SESSION_ID_INVALID;
	pNetworkInterface_ = NULL;
	pEventDispatcher_ = NULL;

	SAFE_RELEASE(pTcpSocket_);

	if (destroyTimerEvent_)
	{
		XServerBase::getSingleton().pTimer()->delTimer(destroyTimerEvent_);
		destroyTimerEvent_ = NULL;
	}
}

//-------------------------------------------------------------------------------------
std::string Session::getIP()
{
	if (!pTcpSocket_)
		return "";

	return pTcpSocket_->getIP();
}

//-------------------------------------------------------------------------------------
uint16 Session::getPort()
{
	if (!pTcpSocket_)
		return 0;

	return pTcpSocket_->getPort();
}

//-------------------------------------------------------------------------------------
void Session::onDestroyTimer(void* userargs)
{
	XServerBase::getSingleton().pTimer()->delTimer(destroyTimerEvent_);
	destroyTimerEvent_ = NULL;

	close();
}

//-------------------------------------------------------------------------------------
void Session::destroy()
{
	if (destroyTimerEvent_)
		return;

	destroyTimerEvent_ = XServerBase::getSingleton().pTimer()->addTimer(TIME_SECONDS * 0.1f, -1, 
		std::bind(&Session::onDestroyTimer, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------
bool Session::send(const uint8 *data, uint32 size)
{
	if (!pTcpSocket_)
		return false;

	return pTcpSocket_->send(data, size);
}

//-------------------------------------------------------------------------------------
bool Session::sendPacket(int32 cmd, const uint8 *data, uint32 size)
{
	assert(size <= PACKET_LENGTH_MAX);

	if (ResMgr::getSingleton().serverConfig().netEncrypted)
	{
		return encryptSend(cmd, data, size);
	}

	PacketHeader header;
	header.msglen = (uint16)size;
	header.msgcmd = cmd;
	header.encode();

	if (!send((const uint8*)&header, sizeof(header)))
		return false;

	return send(data, size);
}

//-------------------------------------------------------------------------------------
bool Session::sendPacket(int32 cmd, const ::google::protobuf::Message& packet)
{
	std::string datas;
	packet.SerializeToString(&datas);

	if (ResMgr::getSingleton().serverConfig().debugPacket)
	{
		std::string cmdName = CMD_Name((CMD)cmd);

		DEBUG_MSG(fmt::format("Session::sendPacket(): appType={}, cmd={} to :{}, size={}.\ndatas={}\n",
			typeName(), cmdName, pTcpSocket()->addr(), datas.size(), packet.DebugString()));
	}

	return sendPacket(cmd, (uint8*)datas.data(), datas.size());
}

//-------------------------------------------------------------------------------------
bool Session::forwardPacket(SessionID requestorSessionID, int32 cmd, const ::google::protobuf::Message& packet)
{
	std::string datas;
	packet.SerializeToString(&datas);

	CMD_ForwardPacket forwardPacket;
	forwardPacket.set_requestorid(requestorSessionID);
	forwardPacket.set_datas(datas);
	forwardPacket.set_msgcmd(cmd);
	return sendPacket(CMD::ForwardPacket, forwardPacket);
}

//-------------------------------------------------------------------------------------
bool Session::encryptSend(int32 cmd, const uint8 *data, uint32 size)
{
	uint8 buf[PACKET_LENGTH_MAX + 1];

	PacketHeader header;
	header.msglen = size;
	header.msgcmd = cmd;
	header.encode();

	int offset = sizeof(header);
	memcpy(buf, (const uint8*)&header, offset);
	memcpy(buf + offset, data, size);

	return send(buf, offset + size);
}

//-------------------------------------------------------------------------------------
bool Session::decryptSend(const uint8 *data, uint32 size)
{
	return true;
}

//-------------------------------------------------------------------------------------
void Session::onRecv()
{
	//INFO_MSG(fmt::format("Session::onRecv()\n"));

	while (true)
	{
		if (isDestroyed())
		{
			close();
			return;
		}

		if (!headerRcved_)
		{
			if (sizeof(headerRcved_) > pTcpSocket_->getRecvBufferLength())
				return;

			pTcpSocket_->recv((uint8*)&header_, sizeof(header_));

			header_.decode();

			if (header_.msglen > PACKET_LENGTH_MAX)
			{
				ERROR_MSG(fmt::format("Session::onRecv(): invalid packet length: {}, msgcmd={}, isServer={}, sessionID={}, {}\n",
					header_.msglen, header_.msgcmd, isServer(), id(), pTcpSocket_->addr()));

				close();
				return;
			}

			headerRcved_ = true;
		}

		// 等待接收到足够长度
		if (header_.msglen > pTcpSocket_->getRecvBufferLength())
			return;

		//DEBUG_MSG(fmt::format("Session::onRecv(): length={}, cmd={}, isServer={}, sessionID={}, {}\n", 
		//	header_.msglen, header_.msgcmd, isServer(), id(), pTcpSocket_->addr()));

		lastReceivedTime_ = getTimeStamp();

		uint8 data[PACKET_LENGTH_MAX + 1];
		if (!pTcpSocket()->recv(data, header_.msglen))
		{
			ERROR_MSG(fmt::format("Session::onRecv(): recv error! msglen={}, msgcmd={}, sessionID={}, {}\n",
				packetHeader().msglen, packetHeader().msgcmd, id(), pTcpSocket()->addr()));

			return;
		}

		// 将包解开，并分发执行
		if (!onProcessPacket_(id(), data, header_.msglen))
		{
			if(!isServer())
				close();
				
			return;
		}

		headerRcved_ = false;
	}
}

//-------------------------------------------------------------------------------------
bool Session::onProcessPacket_(SessionID requestorSessionID, uint8 * data, uint32_t size)
{
	if (ResMgr::getSingleton().serverConfig().netEncrypted)
	{
		decryptSend((const uint8*)data, header_.msglen);
	}

	if (header_.msgcmd > CMD_MAX)
	{
		ERROR_MSG(fmt::format("Session::onProcessPacket_(): cmd error! msgcmd={}, sessionID={}, {}\n",
			packetHeader().msgcmd, id(), pTcpSocket()->addr()));

		return true;
	}

	if (ResMgr::getSingleton().serverConfig().debugPacket)
	{
		DEBUG_MSG(fmt::format("Session::onProcessPacket_(): {}, size={}, sessionID={}, {}\n",
			CMD_Name((CMD)header_.msgcmd), header_.msglen, id(), pTcpSocket()->addr()));
	}

	switch (header_.msgcmd)
	{
		case CMD::Hello:
		{
			CMD_Hello packet;
			PARSE_PACKET();

			onHello(packet);
			break;
		}
		case CMD::HelloCB:
		{
			CMD_HelloCB packet;
			PARSE_PACKET();

			onHelloCB(packet);
			break;
		}
		case CMD::Heartbeat:
		{
			CMD_Heartbeat packet;
			PARSE_PACKET();

			onHeartbeat(packet);
			break;
		}
		case CMD::HeartbeatCB:
		{
			CMD_HeartbeatCB packet;
			PARSE_PACKET();

			onHeartbeatCB(packet);
			break;
		}
		case CMD::Version_Not_Match:
		{
			CMD_Version_Not_Match packet;
			PARSE_PACKET();

			onVersionNotMatch(packet);
			break;
		}
		case CMD::UpdateServerInfos:
		{
			CMD_UpdateServerInfos packet;
			PARSE_PACKET();

			onUpdateServerInfos(packet);
			break;
		}
		case CMD::QueryServerInfos:
		{
			CMD_QueryServerInfos packet;
			PARSE_PACKET();

			onQueryServerInfos(packet);
			break;
		}
		case CMD::QueryServerInfosCB:
		{
			CMD_QueryServerInfosCB packet;
			PARSE_PACKET();

			onQueryServerInfosCB(packet);
			break;
		}
		case CMD::Ping:
		{
			CMD_Ping packet;
			PARSE_PACKET();

			onPing(packet);
			break;
		}
		case CMD::Pong:
		{
			CMD_Pong packet;
			PARSE_PACKET();

			onPong(packet);
			break;
		}
		case CMD::ForwardPacket:
		{
			CMD_ForwardPacket packet;
			PARSE_PACKET();

			return onForwardPacket(packet);
			break;
		}
		default:
		{
			return onProcessPacket(requestorSessionID, (const uint8*)&data[0], header_.msglen);
			break;
		}
	};

	return true;
}

//-------------------------------------------------------------------------------------
void Session::onHello(const CMD_Hello& packet)
{
	if (packet.version() != XPLATFORMSERVER_VERSION)
	{
		CMD_Version_Not_Match res_packet;
		res_packet.set_appid(XServerBase::getSingleton().id());
		res_packet.set_apptype((int32)XServerBase::getSingleton().type());
		res_packet.set_version(XPLATFORMSERVER_VERSION);
		sendPacket(CMD::Version_Not_Match, res_packet);

		close();
	}

	CMD_HelloCB res_packet;
	res_packet.set_appid(XServerBase::getSingleton().id());
	res_packet.set_apptype((int32)XServerBase::getSingleton().type());
	res_packet.set_version(XPLATFORMSERVER_VERSION);
	sendPacket(CMD::HelloCB, res_packet);

	appType((ServerType)packet.apptype());

	if(!isServer() && (appType() != ServerType::SERVER_TYPE_CLIENT || appType() != ServerType::SERVER_TYPE_ROBOT))
		appType(ServerType::SERVER_TYPE_CLIENT);

	XServerBase::getSingleton().onSessionHello(this, packet);
}

//-------------------------------------------------------------------------------------
void Session::onHelloCB(const CMD_HelloCB& packet)
{	
	appID(packet.appid());
	appType((ServerType)packet.apptype());

	XServerBase::getSingleton().onSessionHelloCB(this, packet);
}

//-------------------------------------------------------------------------------------
void Session::onVersionNotMatch(const CMD_Version_Not_Match& packet)
{
	ERROR_MSG(fmt::format("Session::onVersionNotMatch(): currVersion={} != {}(Version:{}, id:{})\n", 
		XPLATFORMSERVER_VERSION, ServerType2Name[(int)packet.apptype()], packet.version(), packet.appid()));
}

//-------------------------------------------------------------------------------------
void Session::onHeartbeat(const CMD_Heartbeat& packet)
{
	CMD_HeartbeatCB res_packet;
	res_packet.set_appid(XServerBase::getSingleton().id());
	sendPacket(CMD::HeartbeatCB, res_packet);
}

//-------------------------------------------------------------------------------------
void Session::onHeartbeatCB(const CMD_HeartbeatCB& packet)
{
}

//-------------------------------------------------------------------------------------
void Session::onUpdateServerInfos(const CMD_UpdateServerInfos& packet)
{
	XServerBase::getSingleton().onSessionUpdateServerInfos(this, packet);
}

//-------------------------------------------------------------------------------------
void Session::onQueryServerInfos(const CMD_QueryServerInfos& packet)
{
	XServerBase::getSingleton().onSessionQueryServerInfos(this, packet);
}

//-------------------------------------------------------------------------------------
void Session::onQueryServerInfosCB(const CMD_QueryServerInfosCB& packet)
{
	XServerBase::getSingleton().onSessionQueryServerInfosCB(this, packet);
}

//-------------------------------------------------------------------------------------
void Session::onSent()
{
	//INFO_MSG(fmt::format("Session::onSent()\n"));
}

//-------------------------------------------------------------------------------------
void Session::close()
{
	DEBUG_MSG(fmt::format("Session::close(): type={}, isServer={}, sessionID={}, {}\n",
		ServerType2Name[(int)appType_], isServer(), id(), pTcpSocket_->addr()));

	connected_ = false;
	onDisconnected();
	XServerBase::getSingleton().onSessionDisconnected(this);

	pTcpSocket_->close();
	pNetworkInterface_->removeSession(id());
}

//-------------------------------------------------------------------------------------
void Session::onConnected()
{
}

//-------------------------------------------------------------------------------------
void Session::onDisconnected()
{
}

//-------------------------------------------------------------------------------------
void Session::handleEvent(short events)
{
	if (events & BEV_EVENT_EOF) {
		INFO_MSG(fmt::format("Session::handleEvent(): Connection closed. type={}, isServer={}, sessionID={}, {}\n", 
			ServerType2Name[(int)appType_], isServer(), id(), pTcpSocket_->addr()));

	}
	else if (events & BEV_EVENT_ERROR) {
		ERROR_MSG(fmt::format("Session::handleEvent(): Got an error on the connection: {}, type={}, isServer={}, sessionID={}, {}\n", 
			strerror(errno), ServerType2Name[(int)appType_], isServer(), id(), pTcpSocket_->addr()));
	}
	else if (events & BEV_EVENT_CONNECTED)
	{
		//INFO_MSG(fmt::format("Session::handleEvent(): connected! type={}, isServer={}, sessionID={}, {}\n",
		//	ServerType2Name[(int)appType_], isServer(), id(), pTcpSocket_->addr()));

		connected_ = true;
		onConnected();
		XServerBase::getSingleton().onSessionConnected(this);
		return;
	}

	close();
}

//-------------------------------------------------------------------------------------
void Session::recvCallback(struct bufferevent *bev, void *data)
{
	Session *pSession = (Session *)data;
	pSession->onRecv();
}

//-------------------------------------------------------------------------------------
void Session::sendCallback(struct bufferevent *bev, void *data)
{
	Session *pSession = (Session *)data;
	pSession->onSent();
}

//-------------------------------------------------------------------------------------
void Session::eventCallback(struct bufferevent *bev, short events, void *data)
{
	Session *pSession = (Session *)data;
	pSession->handleEvent(events);
}

//-------------------------------------------------------------------------------------
std::string Session::addr()
{
	if (!pTcpSocket_)
		return "SocketNull";

	return pTcpSocket_->addr();
}

//-------------------------------------------------------------------------------------
bool Session::isTimeout()
{
	time_t diff = getTimeStamp() - lastReceivedTime_;
	time_t timeout_time = (ResMgr::getSingleton().serverConfig().heartbeatInterval * 2);
	return timeout_time > 0 && timeout_time < diff;
}

//-------------------------------------------------------------------------------------
time_t Session::ping()
{
	time_t t = getTimeStamp();
	CMD_Ping req_packet;
	req_packet.set_time(t);
	sendPacket(CMD::Ping, req_packet);

	//DEBUG_MSG(fmt::format("Session::ping(): id={}, appid={}, time={}\n", id(), this->appID(), t));
	return t;
}

//-------------------------------------------------------------------------------------
void Session::onPing(const CMD_Ping& packet)
{
	CMD_Pong res_packet;
	res_packet.set_time(packet.time());
	sendPacket(CMD::Pong, res_packet);

	//DEBUG_MSG(fmt::format("Session::onPing(): id={}, appid={}, time={}\n", id(), this->appID(), packet.time()));
}

//-------------------------------------------------------------------------------------
void Session::onPong(const CMD_Pong& packet)
{
	rtt_ = getTimeStamp() - packet.time();

	//DEBUG_MSG(fmt::format("Session::onPong(): id={}, appid={}, time={}, oldtime={}, rtt={}\n", 
	//	id(), this->appID(), getTimeStamp(), packet.time(), rtt_));
}

//-------------------------------------------------------------------------------------
bool Session::onForwardPacket(const CMD_ForwardPacket& packet)
{
	ERROR_MSG(fmt::format("XSession::onForwardPacket(): Not implemented!, id={}, {}, datasize={}\n",
		id(), addr(), header_.msglen));

	return true;
}

//-------------------------------------------------------------------------------------
}
