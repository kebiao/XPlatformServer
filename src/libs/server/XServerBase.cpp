#include "XServerBase.h"
#include "ServerMgr.h"
#include "log/XLog.h"
#include "event/EventDispatcher.h"
#include "event/NetworkInterface.h"
#include "event/Session.h"
#include "event/Timer.h"
#include "resmgr/ResMgr.h"
#include "common/threadpool.h"
#include <google/protobuf/stubs/common.h>

namespace XServer {

X_SINGLETON_INIT(XServerBase);

//-------------------------------------------------------------------------------------
XServerBase::XServerBase():
	pEventDispatcher_(NULL),
	pInternalNetworkInterface_(NULL),
	pExternalNetworkInterface_(NULL),
	signals_(),
	id_(0),
	gid_(0),
	type_(ServerType::SERVER_TYPE_UNKNOWN),
	name_(),
	state_(SERVER_STATE_INIT),
	pServerMgr_(NULL),
	pTimer_(NULL),
	shuttingdownTimerEvent_(NULL),
	shutdownExpiredTimerEvent_(NULL),
	tickTimerEvent_(NULL),
	heartbeatTickTimerEvent_(NULL),
	pThreadPool_(NULL)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
}

//-------------------------------------------------------------------------------------
XServerBase::~XServerBase()
{
	SAFE_RELEASE(pEventDispatcher_);
}

//-------------------------------------------------------------------------------------	
void XServerBase::shutDown(time_t shutdowntime)
{
	if (!isRunning())
		return;

	INFO_MSG(fmt::format("XServerBase::shutDown(): shutdowntime={}\n", shutdowntime));

	shuttingdownTimerEvent_ = pTimer()->addTimer(ResMgr::getSingleton().serverConfig().shutdownTick, -1, std::bind(&XServerBase::onShuttingdownTick, this, std::placeholders::_1), NULL);
	shutdownExpiredTimerEvent_ = pTimer()->addTimer(shutdowntime, -1, std::bind(&XServerBase::onShutdownExpiredTick, this, std::placeholders::_1), NULL);

	state_ = SERVER_STATE_SHUTDOWN_WAITING;

	onShutdownBegin();
	onShutdown(true);
}

//-------------------------------------------------------------------------------------
void XServerBase::onShutdownBegin()
{
	INFO_MSG(fmt::format("XServerBase::onShutdownBegin()\n"));
}

//-------------------------------------------------------------------------------------
void XServerBase::onShutdown(bool first)
{
	INFO_MSG(fmt::format("XServerBase::onShutdown(): num={}\n", first));
	state_ = SERVER_STATE_SHUTDOWN_WAITING;
}

//-------------------------------------------------------------------------------------
void XServerBase::onShutdownEnd()
{
	INFO_MSG(fmt::format("XServerBase::onShutdownEnd()\n"));
	state_ = SERVER_STATE_SHUTDOWN_OVER;

	finalise();

	if (pEventDispatcher())
	{
		pEventDispatcher()->exitDispatch();
	}
}

//-------------------------------------------------------------------------------------
void XServerBase::onShuttingdownTick(void* userargs)
{
	onShutdown(false);
}

//-------------------------------------------------------------------------------------
void XServerBase::onShutdownExpiredTick(void* userargs)
{
	onShutdownEnd();
}

//-------------------------------------------------------------------------------------
bool XServerBase::loadResources()
{
	if (!ResMgr::getSingleton().initialize(typeName()))
		return false;

	return true;
}

//-------------------------------------------------------------------------------------		
bool XServerBase::installSignals()
{
	INFO_MSG(fmt::format("XServerBase::installSignals()\n"));
	struct event * signal_event = NULL;

	signal_event = pEventDispatcher_->add_watch_signal(SIGINT, signal_cb, (void*)this);
	if (!signal_event)
		return false;

	signals_.push_back(signal_event);

#if X_PLATFORM != PLATFORM_WIN32
	signal_event = pEventDispatcher_->add_watch_signal(SIGPIPE, signal_cb, (void*)this);
	if (!signal_event)
		return false;

	signals_.push_back(signal_event);

	signal_event = pEventDispatcher_->add_watch_signal(SIGHUP, signal_cb, (void*)this);
	if (!signal_event)
		return false;

	signals_.push_back(signal_event);

	signal_event = pEventDispatcher_->add_watch_signal(SIGQUIT, signal_cb, (void*)this);
	if (!signal_event)
		return false;

	signals_.push_back(signal_event);
#endif
	return true;
}

//-------------------------------------------------------------------------------------	
void XServerBase::signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	XServerBase* pXServerBase = (XServerBase*)user_data;
	pXServerBase->onSignalled((int)sig);
}

//-------------------------------------------------------------------------------------
void XServerBase::onSignalled(int sigNum)
{
	INFO_MSG(fmt::format("XServerBase::onSignalled(): sigNum={}.\n", sigNum));

	if (SIGINT == sigNum || SIGQUIT == sigNum)
	{
		printf("XServerBase::signal_cb(): Caught an interrupt signal; exiting cleanly in 1 seconds.\n");
		INFO_MSG(fmt::format("XServerBase::signal_cb(): Caught an interrupt signal; exiting cleanly in 1 seconds.\n"));
		shutDown(TIME_SECONDS);
		return;
	}
	else if (SIGHUP == sigNum)
	{
		shutDown(TIME_SECONDS);
		return;
	}
}

//-------------------------------------------------------------------------------------		
NetworkInterface* XServerBase::createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal)
{
	return new NetworkInterface(pEventDispatcher, isInternal);
}

//-------------------------------------------------------------------------------------		
bool XServerBase::initialize(ServerAppID appID, ServerType appType, ServerGroupID appGID, const std::string& appName)
{
	assert(id_ == 0 && type_ == ServerType::SERVER_TYPE_UNKNOWN);

	if (appID == 0)
	{
		std::random_device r_device;
		std::mt19937 mt(r_device());
		appID = mt() + 1;
	}

	id_ = appID;
	type_ = appType;
	gid_ = appGID;
	name_ = appName;

	setenv("SERVER_APPID", fmt::format("{}", id_).c_str(), 1);
	setenv("SERVER_APPGID", fmt::format("{}", gid_).c_str(), 1);
	setenv("SERVER_TYPE", ServerType2Name[(int)type_], 1);

	if (!loadResources())
		return false;

	if (!XLog::getSingleton().initialize(fmt::format("server/log4cxx_configs/{}.properties", 
		ServerType2Name[(int)appType])))
		return false;

	INFO_MSG(fmt::format("\n\n-------------------------------------------------------------------------\n"));
	INFO_MSG(fmt::format("XServerBase::initialize()...\n"));
	INFO_MSG(fmt::format("XServerBase::initialize(): id={}\n", id_));
	INFO_MSG(fmt::format("XServerBase::initialize(): gid={}\n", gid_));
	INFO_MSG(fmt::format("XServerBase::initialize(): name={}\n", name_));
	INFO_MSG(fmt::format("XServerBase::initialize(): pool_threads={}\n", ResMgr::getSingleton().serverConfig().threads));

#if X_PLATFORM == PLATFORM_WIN32
	printf(fmt::format("XServerBase::initialize(): id={}\n", id_).c_str());
	printf(fmt::format("XServerBase::initialize(): gid={}\n", gid_).c_str());
	printf(fmt::format("XServerBase::initialize(): name={}\n", name_).c_str());
	printf(fmt::format("XServerBase::initialize(): pool_threads={}\n", ResMgr::getSingleton().serverConfig().threads).c_str());
#endif

	ResMgr::getSingleton().print();

	assert(!pThreadPool_);
	pThreadPool_ = new ThreadPool(ResMgr::getSingleton().serverConfig().threads);

	pEventDispatcher_ = new EventDispatcher();
	if (!pEventDispatcher_->initialize())
	{
		ERROR_MSG(fmt::format("XServerBase::initialize(): pEventDispatcher initialization error!\n"));
		return false;
	}

	pInternalNetworkInterface_ = createNetworkInterface(pEventDispatcher_, true);
	if (!pInternalNetworkInterface_->initialize(ResMgr::getSingleton().serverConfig().internal_ip, ResMgr::getSingleton().serverConfig().internal_port))
	{
		ERROR_MSG(fmt::format("XServerBase::initialize(): InternalNetworkInterface initialization error!\n"));
		return false;
	}

	if (ResMgr::getSingleton().serverConfig().external_ip.size() > 0)
	{
		pExternalNetworkInterface_ = createNetworkInterface(pEventDispatcher_, false);
		if (!pExternalNetworkInterface_->initialize(ResMgr::getSingleton().serverConfig().external_ip, ResMgr::getSingleton().serverConfig().external_port))
		{
			ERROR_MSG(fmt::format("XServerBase::initialize(): ExternalNetworkInterface initialization error!\n"));
			return false;
		}
	}

	pTimer_ = new Timer(pEventDispatcher_);
	tickTimerEvent_ = pTimer_->addTimer(std::max<uint64>(1, ResMgr::getSingleton().serverConfig().tickInterval), -1, std::bind(&XServerBase::onTick, this, std::placeholders::_1), NULL);
	heartbeatTickTimerEvent_ = pTimer_->addTimer(std::max<uint64>(TIME_SECONDS, ResMgr::getSingleton().serverConfig().heartbeatInterval), -1, std::bind(&XServerBase::onHeartbeatTick, this, std::placeholders::_1), NULL);

	pServerMgr_ = new ServerMgr(this);

	if(!installSignals())
		return false;
	
	if(!initializeBegin())
		return false;
	
	if(!inInitialize())
		return false;

	return initializeEnd();
}

//-------------------------------------------------------------------------------------
void XServerBase::finalise(void)
{
	INFO_MSG(fmt::format("XServerBase::finalise()\n"));

	SAFE_RELEASE(pThreadPool_);

	if(shuttingdownTimerEvent_)
		pTimer()->delTimer(shuttingdownTimerEvent_);

	if (shutdownExpiredTimerEvent_)
		pTimer()->delTimer(shutdownExpiredTimerEvent_);
	
	if(pExternalNetworkInterface_)
		pExternalNetworkInterface_->finalise();

	if (pInternalNetworkInterface_)
		pInternalNetworkInterface_->finalise();

	for (auto& item : signals_)
	{
		pEventDispatcher_->del_watch_signal(item);
	}

	signals_.clear();

	SAFE_RELEASE(pExternalNetworkInterface_);
	SAFE_RELEASE(pInternalNetworkInterface_);
	SAFE_RELEASE(pServerMgr_);

	if (tickTimerEvent_)
	{
		pTimer()->delTimer(tickTimerEvent_);
		tickTimerEvent_ = NULL;
	}

	if (heartbeatTickTimerEvent_)
	{
		pTimer()->delTimer(heartbeatTickTimerEvent_);
		heartbeatTickTimerEvent_ = NULL;
	}

	SAFE_RELEASE(pTimer_);

	if(pEventDispatcher_)
		pEventDispatcher_->finalise();

	ResMgr::getSingleton().finalise();
	XLog::getSingleton().finalise();
}

//-------------------------------------------------------------------------------------
bool XServerBase::run(void)
{
	if (!pEventDispatcher_)
		return false;

	INFO_MSG(fmt::format("--- running! ---\n"));

#if X_PLATFORM == PLATFORM_WIN32
	printf(fmt::format("--- running! ---\n").c_str());
#endif

	state_ = SERVER_STATE_RUNNING;
	return pEventDispatcher_->dispatch();
}

//-------------------------------------------------------------------------------------
size_t XServerBase::sessionNum() const
{
	size_t num = 0;

	if (pInternalNetworkInterface_)
		num += pInternalNetworkInterface_->sessionNum();

	if (pExternalNetworkInterface_)
		num += pExternalNetworkInterface_->sessionNum();

	return num;
}

//-------------------------------------------------------------------------------------
void XServerBase::onTick(void* userargs)
{
	//INFO_MSG(fmt::format("XServerBase::onTick()\n"));
}

//-------------------------------------------------------------------------------------
void XServerBase::onHeartbeatTick(void* userargs)
{
	//INFO_MSG(fmt::format("XServerBase::onHeartbeatTick()\n"));

	if(pExternalNetworkInterface_)
		pExternalNetworkInterface_->checkSessions();

	if (pInternalNetworkInterface_)
		pInternalNetworkInterface_->checkSessions();

	if(pServerMgr_)
		pServerMgr_->onHeartbeatTick();
}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionConnected(Session* pSession)
{
	if (pSession->isServer())
	{
		if (pServerMgr_)
			pServerMgr_->onSessionConnected(pSession);
	}
}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionDisconnected(Session* pSession)
{
	if (pSession->isServer())
	{
		if (pServerMgr_)
			pServerMgr_->onSessionDisconnected(pSession);
	}
}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionHello(Session* pSession, const CMD_Hello& packet)
{
	if (pSession->isServer())
	{
		if (pServerMgr_)
			pServerMgr_->onSessionHello(pSession, packet);
	}
}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionHelloCB(Session* pSession, const CMD_HelloCB& packet)
{
	if (pSession->isServer())
	{
		if (pServerMgr_)
			pServerMgr_->onSessionHelloCB(pSession, packet);
	}
}

//-------------------------------------------------------------------------------------
void XServerBase::onServerJoined(ServerInfo* pServerInfo)
{
	INFO_MSG(fmt::format("XServerBase::onServerJoined(): {}, appID={}, appGID={}, appName={}, addr={}!\n",
		ServerType2Name[(int)pServerInfo->type], pServerInfo->id, pServerInfo->gid, 
		pServerInfo->name, pServerInfo->pSession->addr()));
}

//-------------------------------------------------------------------------------------
void XServerBase::onServerExit(ServerInfo* pServerInfo)
{
	INFO_MSG(fmt::format("XServerBase::onServerExit(): {}, appID={}, appGID={}, appName={}, addr={}!\n", 
		ServerType2Name[(int)pServerInfo->type], pServerInfo->id, pServerInfo->gid, 
		pServerInfo->name, pServerInfo->pSession->addr()));
}

//-------------------------------------------------------------------------------------
void XServerBase::updateServerInfoToSession(Session* pSession)
{
	CMD_UpdateServerInfos req_packet;
	req_packet.set_appid(id());
	req_packet.set_appgid(gid());
	req_packet.set_name(name());
	req_packet.set_apptype((int32)type());
	req_packet.set_load(0.f);
	req_packet.set_playernum(0);
	req_packet.set_sessionnum(sessionNum());
	req_packet.set_internal_ip(ResMgr::getSingleton().serverConfig().internal_exposedIP);
	req_packet.set_internal_port(pInternalNetworkInterface_->getListenerPort());
	req_packet.set_external_ip(ResMgr::getSingleton().serverConfig().external_exposedIP);
	req_packet.set_external_port(pExternalNetworkInterface_->getListenerPort());

	pServerMgr_->dumpToProtobuf(req_packet);

	onUpdateServerInfoToSession(req_packet);
	pSession->sendPacket(CMD::UpdateServerInfos, req_packet);
}

//-------------------------------------------------------------------------------------
void XServerBase::onUpdateServerInfoToSession(CMD_UpdateServerInfos& infos)
{

}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionUpdateServerInfos(Session* pSession, const CMD_UpdateServerInfos& packet)
{
	if (!pServerMgr_ || packet.appid() == id())
		return;

	ServerInfo* pServerInfo = pServerMgr_->findServer(packet.appid());
	if (!pServerInfo)
		return;

	pServerInfo->id = packet.appid();
	pServerInfo->gid = packet.appgid();
	pServerInfo->name = packet.name();
	pServerInfo->type = (ServerType)packet.apptype();
	pServerInfo->internal_ip = packet.internal_ip();
	pServerInfo->internal_port = packet.internal_port();
	pServerInfo->external_ip = packet.external_ip();
	pServerInfo->external_port = packet.external_port();
	pServerInfo->load = packet.load();
	pServerInfo->playerNum = packet.playernum();
	pServerInfo->sessionNum = packet.sessionnum();

	INFO_MSG(fmt::format("XServerBase::onUpdateServerInfos(): {}, appID={}, appGID={}, appName={}, internal_addr={}, external_addr={}:{}, playerNum={}, sessionNum={}!\n",
		ServerType2Name[(int)pServerInfo->type], pServerInfo->id, pServerInfo->gid, pServerInfo->name, 
		pServerInfo->pSession->addr(), pServerInfo->external_ip, pServerInfo->external_port, pServerInfo->playerNum, pServerInfo->sessionNum));

	pServerInfo->child_srvs.clear();
	for (int i = 0; i < packet.child_srvs_size(); ++i)
	{
		const CMD_UpdateServerInfos& infos = packet.child_srvs(i);

		if (infos.appid() == id())
			continue;

		ServerInfo serverInfo;
		serverInfo.id = infos.appid();
		serverInfo.gid = packet.appgid();
		serverInfo.name = packet.name();
		serverInfo.type = (ServerType)infos.apptype();
		serverInfo.internal_ip = infos.internal_ip();
		serverInfo.internal_port = infos.internal_port();
		serverInfo.external_ip = infos.external_ip();
		serverInfo.external_port = infos.external_port();
		serverInfo.load = infos.load();
		serverInfo.playerNum = infos.playernum();
		serverInfo.sessionNum = infos.sessionnum();

		pServerInfo->child_srvs[serverInfo.id] = serverInfo;

		INFO_MSG(fmt::format("   ==> child_srvs[{}], appID={}, appGID={}, appName={}, internal_addr={}:{}, external_addr={}:{}, playerNum={}, sessionNum={}\n",
			ServerType2Name[(int)serverInfo.type], serverInfo.id, serverInfo.gid, 
			serverInfo.name, serverInfo.internal_ip, serverInfo.internal_port, serverInfo.external_ip, serverInfo.external_port,
			serverInfo.playerNum, serverInfo.sessionNum));
	}
}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionQueryServerInfos(Session* pSession, const CMD_QueryServerInfos& packet)
{
	if (!pServerMgr_ || packet.appid() == id())
		return;

	CMD_QueryServerInfosCB res_packet;

	if (!pServerMgr_)
	{
		pSession->sendPacket(CMD::QueryServerInfosCB, res_packet);
		return;
	}

	std::vector<ServerInfo*> results = pServerMgr_->findServer((ServerType)packet.apptype(), packet.appid(), packet.maxnum());
	for (auto& item : results)
	{
		if (packet.appgid() > 0 && packet.appgid() != item->gid)
			continue;

		CMD_UpdateServerInfos* pUpdateServerInfos = res_packet.add_srvs();
		pUpdateServerInfos->set_appid(item->id);
		pUpdateServerInfos->set_appgid(item->gid);
		pUpdateServerInfos->set_name(item->name);
		pUpdateServerInfos->set_apptype((int32)item->type);
		pUpdateServerInfos->set_internal_ip(item->internal_ip);
		pUpdateServerInfos->set_internal_port(item->internal_port);
		pUpdateServerInfos->set_external_ip(item->external_ip);
		pUpdateServerInfos->set_external_port(item->external_port);
		pUpdateServerInfos->set_load(item->load);
		pUpdateServerInfos->set_playernum(item->playerNum);
		pUpdateServerInfos->set_sessionnum(item->sessionNum);
	}

	pSession->sendPacket(CMD::QueryServerInfosCB, res_packet);
}

//-------------------------------------------------------------------------------------
void XServerBase::onSessionQueryServerInfosCB(Session* pSession, const CMD_QueryServerInfosCB& packet)
{
	for (int i = 0; i < packet.srvs_size(); ++i)
	{
		const CMD_UpdateServerInfos& infos = packet.srvs(i);

		if (infos.appid() == id())
			continue;

		ServerInfo serverInfo;
		
		ServerInfo* pServerInfo = pServerMgr_->findServer(infos.appid());
		bool newadd = pServerInfo == NULL;

		if(newadd)
			pServerInfo = &serverInfo;

		pServerInfo->id = infos.appid();
		pServerInfo->gid = infos.appgid();
		pServerInfo->name = infos.name();
		pServerInfo->type = (ServerType)infos.apptype();
		pServerInfo->external_ip = infos.external_ip();
		pServerInfo->external_port = infos.external_port();
		pServerInfo->internal_ip = infos.internal_ip();
		pServerInfo->internal_port = infos.internal_port();
		pServerInfo->load = infos.load();
		pServerInfo->playerNum = infos.playernum();
		pServerInfo->sessionNum = infos.sessionnum();
		
		if (newadd)
			pServerMgr_->addServer(*pServerInfo);

		//DEBUG_MSG(fmt::format("XServerBase::onSessionQueryServerInfosCB: srvs[{}], appID={}, appGID={}, appName={}, internal_addr={}:{}\n",
		//	ServerType2Name[(int)pServerInfo->type], pServerInfo->id, pServerInfo->gid, pServerInfo->name, pServerInfo->ip, pServerInfo->port));
	}
}

//-------------------------------------------------------------------------------------
}
