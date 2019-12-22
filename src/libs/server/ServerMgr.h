#ifndef X_SERVER_MGR_H
#define X_SERVER_MGR_H

#include "common/common.h"
#include "protos/Commands.pb.h"

namespace XServer {

class XServerBase;
class Session;

struct ServerInfo
{
	ServerInfo()
	{
		id = SERVER_APP_ID_INVALID;
		gid = SERVER_GROUP_ID_INVALID;
		name = "unknown";
		pSession = NULL;
		internal_ip = "";
		internal_port = 0;
		external_ip = "";
		external_port = 0;
		type = ServerType::SERVER_TYPE_UNKNOWN;
		load = 0.f;
		playerNum = 0;
		sessionNum = 0;
		sent_hello = false;
		reconnectionNum = 0;
	}

	ServerAppID id;
	ServerGroupID gid;
	ServerType type;
	std::string name;

	Session* pSession;
	std::string internal_ip;
	std::string external_ip;
	uint16 internal_port;
	uint16 external_port;
	float load;
	int playerNum;
	int sessionNum;
	bool sent_hello;
	int reconnectionNum;
	std::map<ServerAppID, ServerInfo> child_srvs;
};

class ServerMgr
{
public:
	ServerMgr(XServerBase* pServer);
	ServerMgr(const std::vector<ServerType>& srvTypes, XServerBase* pServer);
	virtual ~ServerMgr();

	bool addServer(const ServerInfo& info);
	bool delServer(ServerAppID id);

	ServerInfo* findServer(ServerAppID id);
	ServerInfo* findServer(Session* pSession);
	std::vector<ServerInfo*> findServer(ServerType srvType, ServerAppID id = 0, int maxNum = 0);
	ServerInfo* findServerOne(ServerType srvType, ServerAppID id = 0);

	ServerInfo* findDirectory();
	ServerInfo* findHallsmgr();
	ServerInfo* findRoommgr();
	ServerInfo* findDbmgr();
	ServerInfo* findConnector();

	void addInterestedServerType(ServerType type);
	void removeInterestedServerType(ServerType type);

	void addInterestedServerID(ServerAppID id);
	void removeInterestedServerID(ServerAppID id);

	void onTick(void* userargs);
	void onHeartbeatTick();

	Session* connectServer(std::string ip, uint16 port, ServerType type);

	void onSessionConnected(Session* pSession);
	void onSessionDisconnected(Session* pSession);
	void onSessionHello(Session* pSession, const CMD_Hello& packet);
	void onSessionHelloCB(Session* pSession, const CMD_HelloCB& packet);

	void dumpToProtobuf(CMD_UpdateServerInfos& infos);

protected:
	std::map<ServerAppID, ServerInfo> srv_infos_;

	std::vector<ServerType> interestedServerTypes_;
	std::vector<ServerAppID> interestedServerIDs_;

	XServerBase* pXServer_;

	struct event * timerEvent_;
};

}

#endif // X_SERVER_MGR_H
