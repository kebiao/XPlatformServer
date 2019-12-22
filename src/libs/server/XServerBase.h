#ifndef X_SERVER_BASE_H
#define X_SERVER_BASE_H

#include "common/common.h"
#include "common/singleton.h"
#include "protos/ServerCommon.pb.h"
#include "protos/Commands.pb.h"

namespace XServer {

class EventDispatcher;
class NetworkInterface;
class ServerMgr;
class Timer;
class Session;
struct ServerInfo;
class ThreadPool;

class XServerBase : public Singleton<XServerBase>
{
public:
	enum ServerState
	{
		SERVER_STATE_INIT = 0,
		SERVER_STATE_RUNNING = 1,
		SERVER_STATE_PAUSE = 2,
		SERVER_STATE_SHUTDOWN_WAITING = 3,
		SERVER_STATE_SHUTDOWN_OVER = 4,
	};

public:
	XServerBase();
	~XServerBase();

	virtual bool initialize(ServerAppID appID, ServerType appType, ServerGroupID appGID, const std::string& appName);
	virtual bool initializeBegin() { return true; }
	virtual bool inInitialize() { return true; }
	virtual bool initializeEnd() {return true; }
	virtual void finalise();
	virtual bool run();

	virtual bool installSignals();
	virtual void onSignalled(int sigNum);
	static void signal_cb(evutil_socket_t sig, short events, void *user_data);

	virtual bool loadResources();

	virtual void shutDown(time_t shutdowntime);
	virtual void onShutdownBegin();
	virtual void onShutdown(bool first);
	virtual void onShutdownEnd();
	
	EventDispatcher* pEventDispatcher() {
		return pEventDispatcher_;
	}

	virtual NetworkInterface* createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal);

	NetworkInterface* pInternalNetworkInterface() {
		return pInternalNetworkInterface_;
	}

	NetworkInterface* pExternalNetworkInterface() {
		return pExternalNetworkInterface_;
	}

	ThreadPool* pThreadPool() {
		return pThreadPool_;
	}

	size_t sessionNum() const;

	bool isRunning() const {
		return state_ == SERVER_STATE_RUNNING;
	}

	bool isShutingdown() const {
		return state_ == SERVER_STATE_SHUTDOWN_WAITING || state_ == SERVER_STATE_SHUTDOWN_OVER;
	}

	bool isShutdown() const {
		return state_ == SERVER_STATE_SHUTDOWN_OVER;
	}

	ServerState state() const {
		return state_;
	}

	virtual void onTick(void* userargs);
	virtual void onHeartbeatTick(void* userargs);
	virtual void onShuttingdownTick(void* userargs);
	virtual void onShutdownExpiredTick(void* userargs);

	ServerAppID id() const {
		return id_;
	}

	ServerGroupID gid() const {
		return gid_;
	}

	ServerType type() const {
		return type_;
	}

	const char* typeName() const {
		return ServerType2Name[(int)type_];
	}

	const std::string& name() const {
		return name_;
	}

	ServerMgr* pServerMgr() const {
		return pServerMgr_;
	}

	virtual void updateServerInfoToSession(Session* pSession);
	virtual void onUpdateServerInfoToSession(CMD_UpdateServerInfos& infos);

	virtual void onSessionDisconnected(Session* pSession);
	virtual void onSessionConnected(Session* pSession);
	virtual void onSessionHello(Session* pSession, const CMD_Hello& packet);
	virtual void onSessionHelloCB(Session* pSession, const CMD_HelloCB& packet);

	Timer* pTimer() {
		return pTimer_;
	}

	virtual void onServerJoined(ServerInfo* pServerInfo);
	virtual void onServerExit(ServerInfo* pServerInfo);

	virtual void onSessionUpdateServerInfos(Session* pSession, const CMD_UpdateServerInfos& packet);
	virtual void onSessionQueryServerInfos(Session* pSession, const CMD_QueryServerInfos& packet);
	virtual void onSessionQueryServerInfosCB(Session* pSession, const CMD_QueryServerInfosCB& packet);

protected:
	EventDispatcher* pEventDispatcher_;
	NetworkInterface* pInternalNetworkInterface_;
	NetworkInterface* pExternalNetworkInterface_;

	std::vector<struct event *> signals_;

	// 服务器唯一ID
	ServerAppID id_;
	ServerGroupID gid_;
	ServerType type_;
	std::string name_;

	ServerState state_;

	// 其他服务器
	ServerMgr* pServerMgr_;

	Timer* pTimer_;

	struct event * shuttingdownTimerEvent_;
	struct event * shutdownExpiredTimerEvent_;

	struct event * tickTimerEvent_;
	struct event * heartbeatTickTimerEvent_;

	ThreadPool* pThreadPool_;
};

}

#endif // X_SERVER_BASE_H
