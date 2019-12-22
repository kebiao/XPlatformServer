#ifndef X_RESMGR_H
#define X_RESMGR_H

#include "common/common.h"
#include "common/singleton.h"

namespace XServer {

	struct GameModeConfig
	{
		std::string name;
		GameMode modeID;
		int playerMax;
		time_t gameTime;

		std::string serverExeFile;
		std::string serverExeOptions;
	};

	struct GameConfig
	{
		std::string name;
		GameID id;
		std::vector<GameModeConfig> gameModes;
		int type;

		std::string url_icon;
		std::string url_apk;
	};

	class ResMgr : public Singleton<ResMgr>
	{
	public:
		// 环境变量
		struct Env
		{
			std::string root_path;
			std::string res_path;
			std::string bin_path;
		};

		// 服务器配置
		struct ServerConfig
		{
			ServerConfig()
			{
				tickInterval = 10;
				heartbeatInterval = 10000;

				internal_SNDBUF = 0;
				internal_RCVBUF = 0;
				external_SNDBUF = 0;
				external_RCVBUF = 0;

				internal_ip = "localhost";
				internal_exposedIP = internal_ip;
				external_ip = "";
				external_exposedIP = external_ip;
				internal_port = 0;
				external_port = 0;

				debugPacket = false;
				netEncrypted = true;

				shutdownTick = 1000;
				shutdownTime = 60000;

				threads = 1;

				player_disconnected_lifetime = 1;

				db_name = "";
				db_username = "";
				db_password = "";
				db_host = "";
				db_port = 0;
			}

			uint32 tickInterval;

			uint32 heartbeatInterval;

			std::string internal_ip;
			std::string external_ip;
			std::string internal_exposedIP;
			std::string external_exposedIP;
			uint16 internal_port;
			uint16 external_port;

			uint32 internal_SNDBUF;
			uint32 internal_RCVBUF;

			uint32 external_SNDBUF;
			uint32 external_RCVBUF;

			std::vector<std::string> server_addresses;

			bool debugPacket;
			bool netEncrypted;

			int shutdownTick;
			int shutdownTime;

			int threads;

			int player_disconnected_lifetime;

			std::string db_name;
			std::string db_username;
			std::string db_password;
			std::string db_host;
			int db_port;
		};

public:
	ResMgr();
	~ResMgr();

	virtual bool initialize(const std::string& srvName);
	virtual void finalise();

	bool listPathRes(std::wstring path, const std::wstring& extendName, std::vector<std::wstring>& results, int recursiveNum = -1);
	std::string matchRes(const std::string& res);
	std::string matchPath(const std::string& path);

	FILE* openRes(std::string res, const char* mode = "r");

	const std::vector<std::string>& respaths() 
	{
		return respaths_;
	}

	const ServerConfig & serverConfig() const {
		return *pSrvcfg_;
	}

	const ServerConfig & findConfig(const std::string& srvName) {
		return srvcfgs_[srvName];
	}

	void print();

	GameConfig* findGameConfig(GameID id) {
		auto iter = gamecfgs_.find(id);
		if (iter != gamecfgs_.end())
			return &iter->second;

		return NULL;
	}

	std::map<GameID, GameConfig>& gameConfigs() {
		return gamecfgs_;
	}

	std::vector<GameConfig> findGameConfigs(int type) 
	{
		std::vector<GameConfig> cfgs;
		for (auto& item : gamecfgs_)
		{
			if (item.second.type == type)
				cfgs.push_back(item.second);
		}

		return cfgs;
	}

	const Env& env() const {
		return env_;
	}

private:
	bool loadConfigs(const std::string& srvName);
	bool loadGameConfigs();

protected:
	std::vector<std::string> respaths_;

	Env env_;

	std::tr1::unordered_map<std::string, ServerConfig> srvcfgs_;
	ServerConfig* pSrvcfg_;

	std::map<GameID, GameConfig> gamecfgs_;
};

	
}

#endif // X_RESMGR_H
