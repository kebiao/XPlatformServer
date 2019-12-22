#include "ServerMgr.h"
#include "XServerBase.h"
#include "log/XLog.h"
#include "event/EventDispatcher.h"
#include "event/NetworkInterface.h"
#include "event/Timer.h"
#include "event/Session.h"
#include "event/TcpSocket.h"
#include "resmgr/ResMgr.h"
#include "protos/Commands.pb.h"

namespace XServer {

//-------------------------------------------------------------------------------------
ServerMgr::ServerMgr(XServerBase* pServer):
srv_infos_(),
interestedServerTypes_(),
interestedServerIDs_(),
pXServer_(pServer),
timerEvent_(NULL)
{
	timerEvent_ = pXServer_->pTimer()->addTimer(TIME_SECONDS, -1, std::bind(&ServerMgr::onTick, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------
ServerMgr::ServerMgr(const std::vector<ServerType>& srvTypes, XServerBase* pServer):
srv_infos_(),
interestedServerTypes_(srvTypes),
interestedServerIDs_(),
pXServer_(pServer),
timerEvent_(NULL)
{
	timerEvent_ = pXServer_->pTimer()->addTimer(TIME_SECONDS, -1, std::bind(&ServerMgr::onTick, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------
ServerMgr::~ServerMgr()
{
	if(timerEvent_)
		pXServer_->pTimer()->delTimer(timerEvent_);
}

//-------------------------------------------------------------------------------------	
bool ServerMgr::addServer(const ServerInfo& info)
{
	if (srv_infos_.find(info.id) != srv_infos_.end())
		return false;

	if (info.id == pXServer_->id())
		return false;

	srv_infos_[info.id] = info;
	return true;
}

//-------------------------------------------------------------------------------------
bool ServerMgr::delServer(ServerAppID id)
{
	if (srv_infos_.find(id) != srv_infos_.end())
		return false;

	srv_infos_.erase(id);
	return true;
}

//-------------------------------------------------------------------------------------
ServerInfo* ServerMgr::findServer(ServerAppID id)
{
	std::map<ServerAppID, ServerInfo>::iterator finfoIter = srv_infos_.find(id);

	if (finfoIter  == srv_infos_.end())
		return NULL;

	return &finfoIter->second;
}

//-------------------------------------------------------------------------------------
ServerInfo* ServerMgr::findServer(Session* pSession)
{
	for (auto& item : srv_infos_)
	{
		ServerInfo& info = item.second;

		if (info.pSession == pSession)
		{
			return &info;
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------------------	
std::vector<ServerInfo*> ServerMgr::findServer(ServerType srvType, ServerAppID id, int maxNum)
{
	std::vector<ServerInfo*> ret;

	for (auto& item : srv_infos_)
	{
		ServerInfo& info = item.second;

		if (info.type == srvType)
		{
			if (id > 0 && id != info.id)
				continue;

			ret.push_back(&info);

			if (maxNum > 0 && ret.size() >= maxNum)
				break;
		}
	}

	return ret;
}

//-------------------------------------------------------------------------------------	
ServerInfo* ServerMgr::findServerOne(ServerType srvType, ServerAppID id)
{
	std::vector<ServerInfo*> found = findServer(srvType, id, 1);
	if (found.size() == 0)
		return NULL;

	return found[0];
}

//-------------------------------------------------------------------------------------	
ServerInfo* ServerMgr::findDirectory()
{
	return findServerOne(ServerType::SERVER_TYPE_DIRECTORY, 0);
}

//-------------------------------------------------------------------------------------	
ServerInfo* ServerMgr::findHallsmgr()
{
	return findServerOne(ServerType::SERVER_TYPE_HALLSMGR, 0);
}

//-------------------------------------------------------------------------------------	
ServerInfo* ServerMgr::findRoommgr()
{
	return findServerOne(ServerType::SERVER_TYPE_ROOMMGR, 0);
}

//-------------------------------------------------------------------------------------	
ServerInfo* ServerMgr::findDbmgr()
{
	return findServerOne(ServerType::SERVER_TYPE_DBMGR, 0);
}

//-------------------------------------------------------------------------------------	
ServerInfo* ServerMgr::findConnector()
{
	return findServerOne(ServerType::SERVER_TYPE_CONNECTOR, 0);
}

//-------------------------------------------------------------------------------------	
Session* ServerMgr::connectServer(std::string ip, uint16 port, ServerType type)
{
	//INFO_MSG(fmt::format("ServerMgr::connectServer(): {}:{}...\n", ip, port));

	NetworkInterface* pNetworkInterface = pXServer_->pInternalNetworkInterface();
	EventDispatcher* pEventDispatcher = pNetworkInterface->pEventDispatcher();

	if (ip == "0.0.0.0")
		ip = "localhost";

	struct hostent *host;
	if ((host = gethostbyname(ip.c_str())) == NULL)
	{
		ERROR_MSG(fmt::format("ServerMgr::connectServer(): gethostbyname({}) error!\n", ip));
		return NULL;
	}

	socket_t sock = -1;
	Session* pSession = pNetworkInterface->createSession(sock);

	pSession->appType(type);
	
	if (!pSession->pTcpSocket()->isGood())
	{
		ERROR_MSG(fmt::format("ServerMgr::connectServer(): socket create error!\n"));
		delete pSession;
		return NULL;
	}

	pSession->isServer(true);
	pNetworkInterface->addSession(pSession->id(), pSession);

	if (!pSession->initialize())
	{
		pNetworkInterface->removeSession(pSession->id());
		delete pSession;
		return NULL;
	}

	pSession->appType(type);

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr = *((struct in_addr *)host->h_addr);
	serverAddr.sin_port = htons(port);

	int err = bufferevent_socket_connect(pSession->pTcpSocket()->getBufEvt(), (sockaddr *)&serverAddr, sizeof(serverAddr));
	if (err != 0)
	{
		ERROR_MSG(fmt::format("ServerMgr::connectServer(): connect error! type={}, addr={}:{}\n", 
			ServerType2Name[(int)type], ip, port));

		// 此处不应该销毁， 因为bufferevent_socket_connect会触发事件，Session内部会销毁
		//delete pSession;

		return NULL;
	}

	pNetworkInterface->addSession(pSession->id(), pSession);
	return pSession;
}

//-------------------------------------------------------------------------------------	
void ServerMgr::onTick(void* userargs)
{
	for (auto& item : interestedServerTypes_)
	{
		ServerInfo* srvInfos = findServerOne(item);
		if (srvInfos == NULL)
		{
			INFO_MSG(fmt::format("ServerMgr::onTick(): finding {} ...\n",
				ServerType2Name[(int)item]));

			const ResMgr::ServerConfig& serverConfig = ResMgr::getSingleton().findConfig(ServerType2Name[(int)item]);

			ServerInfo info;
			info.internal_ip = serverConfig.internal_ip;
			info.internal_port = serverConfig.internal_port;
			info.id = 0;
			info.type = item;
			info.pSession = connectServer(serverConfig.internal_ip, serverConfig.internal_port, item);

			if(info.pSession)
				addServer(info);

			return;
		}
		else
		{
			// 等待获取对方进程信息
			if (srvInfos->id == 0)
			{
				if (srvInfos->pSession && srvInfos->pSession->connected() && !srvInfos->sent_hello)
				{
					srvInfos->sent_hello = true;

					CMD_Hello req_packet;
					req_packet.set_version(XPLATFORMSERVER_VERSION);
					req_packet.set_appid(pXServer_->id());
					req_packet.set_apptype((int32)pXServer_->type());
					req_packet.set_appgid((int32)pXServer_->gid());
					req_packet.set_name(pXServer_->name());
					srvInfos->pSession->sendPacket(CMD::Hello, req_packet);
				}

				return;
			}
		}
	}

	for (auto& item : interestedServerIDs_)
	{
		ServerInfo* srvInfos = findServer(item);
		if (srvInfos == NULL)
		{
			ERROR_MSG(fmt::format("ServerMgr::onTick(): not found interestedServerID={}\n",
				item));

			removeInterestedServerID(item);
			return;
		}

		if (srvInfos->pSession)
		{
			if (srvInfos->pSession->connected() && !srvInfos->sent_hello)
			{
				srvInfos->sent_hello = true;

				CMD_Hello req_packet;
				req_packet.set_version(XPLATFORMSERVER_VERSION);
				req_packet.set_appid(pXServer_->id());
				req_packet.set_apptype((int32)pXServer_->type());
				req_packet.set_appgid((int32)pXServer_->gid());
				req_packet.set_name(pXServer_->name());
				srvInfos->pSession->sendPacket(CMD::Hello, req_packet);
			}
		}
		else
		{
			++srvInfos->reconnectionNum;

			if (srvInfos->reconnectionNum > 10)
			{
				removeInterestedServerID(srvInfos->id);
				delServer(srvInfos->id);
				return;
			}

			ERROR_MSG(fmt::format("ServerMgr::onTick(): lose connection, interestedServerID={}, type={}! try reconnection({})...\n",
				item, ServerType2Name[(int)srvInfos->type], srvInfos->reconnectionNum));

			srvInfos->pSession = connectServer(srvInfos->internal_ip, srvInfos->internal_port, srvInfos->type);
		}
	}
}

//-------------------------------------------------------------------------------------	
void ServerMgr::onHeartbeatTick()
{
	if (!pXServer_)
		return;

	for (auto& item : srv_infos_)
	{
		ServerInfo& info = item.second;
		
		if (!info.pSession)
			continue;

		CMD_Heartbeat req_packet;
		req_packet.set_appid(pXServer_->id());
		info.pSession->sendPacket(CMD::Heartbeat, req_packet);
	}
}

//-------------------------------------------------------------------------------------
void ServerMgr::onSessionConnected(Session* pSession)
{
}

//-------------------------------------------------------------------------------------
void ServerMgr::onSessionDisconnected(Session* pSession)
{
	if (!pXServer_)
		return;

	for (auto& item : srv_infos_)
	{
		ServerInfo& info = item.second;

		if (info.pSession && info.pSession == pSession)
		{
			pXServer_->onServerExit(&info);
			srv_infos_.erase(info.id);
			return;
		}
	}
}

//-------------------------------------------------------------------------------------
void ServerMgr::onSessionHello(Session* pSession, const CMD_Hello& packet)
{
	ServerInfo info;
	info.internal_ip = pSession->getIP();
	info.internal_port = pSession->getPort();
	info.id = packet.appid();
	info.gid = packet.appgid();
	info.name = packet.name();
	info.type = (ServerType)packet.apptype();
	info.pSession = pSession;

	if (info.pSession)
		addServer(info);

	pXServer_->onServerJoined(&info);
}

//-------------------------------------------------------------------------------------
void ServerMgr::onSessionHelloCB(Session* pSession, const CMD_HelloCB& packet)
{
	for (auto& item : srv_infos_)
	{
		ServerInfo info = item.second;

		if (info.pSession && info.pSession == pSession)
		{
			info.id = packet.appid();
			info.gid = packet.appgid();
			info.name = packet.name();
			pSession->appID(info.id);

			srv_infos_[info.id] = info;

			if (item.first == 0 && info.id > 0)
			{
				srv_infos_.erase(item.first);
			}

			pXServer_->onServerJoined(&info);
			return;
		}
		else
		{
			//ERROR_MSG(fmt::format("ServerMgr::onSessionHelloCB(): appid exist!, appID={}, addr={}!\n",
			//	packet.appid(), pSession->addr()));

			//assert(false);
		}
	}
}

//-------------------------------------------------------------------------------------
void ServerMgr::dumpToProtobuf(CMD_UpdateServerInfos& infos)
{
	for (auto& item : srv_infos_)
	{
		ServerInfo info = item.second;

		CMD_UpdateServerInfos* pCMD_UpdateServerInfos = infos.add_child_srvs();
		pCMD_UpdateServerInfos->set_appid(info.id);
		pCMD_UpdateServerInfos->set_appgid(info.gid);
		pCMD_UpdateServerInfos->set_name(info.name);
		pCMD_UpdateServerInfos->set_apptype((int32)info.type);
		pCMD_UpdateServerInfos->set_load(info.load);
		pCMD_UpdateServerInfos->set_playernum(info.playerNum);
		pCMD_UpdateServerInfos->set_sessionnum(info.sessionNum);
		pCMD_UpdateServerInfos->set_internal_ip(info.internal_ip);
		pCMD_UpdateServerInfos->set_internal_port(info.internal_port);
		pCMD_UpdateServerInfos->set_external_ip(info.external_ip);
		pCMD_UpdateServerInfos->set_external_port(info.external_port);
	}
}

//-------------------------------------------------------------------------------------
void ServerMgr::addInterestedServerType(ServerType type)
{
	auto fiter = std::find(interestedServerTypes_.begin(), interestedServerTypes_.end(), type);
	if (fiter != interestedServerTypes_.end())
		return;

	INFO_MSG(fmt::format("ServerMgr::addInterestedServerType(): {}\n",
		ServerType2Name[(int)type]));

	interestedServerTypes_.push_back(type);
}

//-------------------------------------------------------------------------------------
void ServerMgr::removeInterestedServerType(ServerType type)
{
	auto fiter = std::find(interestedServerTypes_.begin(), interestedServerTypes_.end(), type);

	if (fiter != interestedServerTypes_.end())
	{
		interestedServerTypes_.erase(fiter);

		INFO_MSG(fmt::format("ServerMgr::removeInterestedServerType(): {}\n",
			ServerType2Name[(int)type]));
	}

	std::vector<ServerAppID> dels;

	for (auto& item : srv_infos_)
	{
		ServerInfo& info = item.second;

		if (type != info.type)
			continue;

		dels.push_back(info.id);
	}

	for (auto& item : dels)
	{
		ServerInfo* pServerInfo = findServer(item);
		if (!pServerInfo)
			continue;

		if (pServerInfo->pSession)
		{
			pServerInfo->pSession->destroy();
		}

		delServer(item);
	}
}

//-------------------------------------------------------------------------------------
void ServerMgr::addInterestedServerID(ServerAppID id)
{
	auto fiter = std::find(interestedServerIDs_.begin(), interestedServerIDs_.end(), id);

	if (fiter != interestedServerIDs_.end())
		return;

	INFO_MSG(fmt::format("ServerMgr::addInterestedServerID(): {}, id={}\n",
		ServerType2Name[(int)srv_infos_[id].type], id));

	interestedServerIDs_.push_back(id);
}

//-------------------------------------------------------------------------------------
void ServerMgr::removeInterestedServerID(ServerAppID id)
{
	auto fiter = std::find(interestedServerIDs_.begin(), interestedServerIDs_.end(), id);

	if (fiter != interestedServerIDs_.end())
	{
		interestedServerIDs_.erase(fiter);

		INFO_MSG(fmt::format("ServerMgr::removeInterestedServerID(): id={}\n", id));
	}
}

//-------------------------------------------------------------------------------------
}
