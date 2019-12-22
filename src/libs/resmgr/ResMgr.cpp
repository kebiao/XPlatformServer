#include "ResMgr.h"
#include "log/XLog.h"
#include "iniparser.h"

namespace XServer {

X_SINGLETON_INIT(ResMgr);

ResMgr g_ResMgr;

//-------------------------------------------------------------------------------------
ResMgr::ResMgr():
respaths_(),
env_(),
srvcfgs_(),
pSrvcfg_(NULL)
{
}

//-------------------------------------------------------------------------------------
ResMgr::~ResMgr()
{
}

//-------------------------------------------------------------------------------------		
bool ResMgr::initialize(const std::string& srvName)
{
	env_.root_path = getenv("X_ROOT") == NULL ? "" : getenv("X_ROOT");
	env_.res_path = getenv("X_RES") == NULL ? "" : getenv("X_RES");
	env_.bin_path = getenv("X_BIN") == NULL ? "" : getenv("X_BIN");

	respaths_.push_back(env_.res_path);

	return loadConfigs(srvName);
}

//-------------------------------------------------------------------------------------
void ResMgr::finalise(void)
{
}

//-------------------------------------------------------------------------------------
void ResMgr::print()
{
	INFO_MSG(fmt::format("Resmgr::initialize: X_ROOT={0}\n", env_.root_path));
	INFO_MSG(fmt::format("Resmgr::initialize: X_RES={0}\n", env_.res_path));
	INFO_MSG(fmt::format("Resmgr::initialize: X_BIN={0}\n", env_.bin_path));

#if KBE_PLATFORM == PLATFORM_WIN32
	printf("%s", fmt::format("X_ROOT = {0}\n", env_.root_path).c_str());
	printf("%s", fmt::format("X_RES = {0}\n", env_.res_path).c_str());
	printf("%s", fmt::format("X_BIN = {0}\n", env_.bin_path).c_str());
	printf("\n");
#endif
}

//-------------------------------------------------------------------------------------
bool ResMgr::loadGameConfigs()
{
	std::string path = matchPath("games");

	if (path == "games")
	{
		ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found: {}\n", "res/game/"));
		return false;
	}

	std::vector<std::wstring> gameconfigs;
	wchar_t* wstr = char2wchar(path.c_str());
	bool success = listPathRes(wstr, L"ini", gameconfigs, 2);
	free(wstr);

	if (!success)
		return success;

	for (auto& item : gameconfigs)
	{
		char* cpath = wchar2char(item.c_str());
		char pathstr[512];
		strcpy(pathstr, cpath);
		free(cpath);

		std::string config = pathstr;
		x_replace(config, "\\", "/");
		x_replace(config, "//", "/");

		std::vector<std::string> s;
		x_split(config, "/", s);

		if (s.size() < 2)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): cannot open file: {}\n", config));
			continue;
		}

		std::string gameName = s[s.size() - 2];
		config = s[s.size() - 1];

		if (config != gameName + ".ini")
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): config({}) not is \n", config, (gameName + ".ini")));
			continue;
		}

		dictionary* ini = iniparser_load(pathstr);
		if (ini == NULL)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): cannot parse file: {}\n", pathstr));
			return false;
		}

		GameConfig cfg;

		std::string ini_gameName = iniparser_getstring(ini, "game:name", "");
		if (ini_gameName != gameName)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): [game:name] not is {}, curr={}, config={}\n", gameName, ini_gameName, pathstr));
			iniparser_freedict(ini);
			continue;
		}

		cfg.name = ini_gameName;

		uint64 ini_gameID = iniparser_getlongint(ini, "game:id", -1);
		if (-1 == ini_gameID)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [game:id], config={}\n", pathstr));
			iniparser_freedict(ini);
			continue;
		}

		cfg.id = ini_gameID;

		int ini_type = iniparser_getint(ini, "game:type", -1);
		if (-1 == ini_type)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [game:type], config={}\n", pathstr));
			iniparser_freedict(ini);
			continue;
		}

		cfg.type = ini_type;

		std::string ini_url_icon = iniparser_getstring(ini, "game:url_icon", "");
		if (ini_url_icon.size() == 0)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [game:url_icon], config={}\n", pathstr));
			iniparser_freedict(ini);
			continue;
		}

		cfg.url_icon = ini_url_icon;

		std::string ini_url_apk = iniparser_getstring(ini, "game:url_apk", "");
		if (ini_url_apk.size() == 0)
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [game:url_apk], config={}\n", pathstr));
			iniparser_freedict(ini);
			continue;
		}

		cfg.url_apk = ini_url_apk;

		for (int modeID = 1; modeID < 256; ++modeID)
		{
			int n = iniparser_getsecnkeys(ini, fmt::format("gamemode{}", modeID).c_str());
			if (n == 0)
				break;

			GameModeConfig mode;
			mode.modeID = modeID;
			
			std::string keyName = fmt::format("gamemode{}:name", modeID);
			std::string ini_modelName = iniparser_getstring(ini, keyName.c_str(), "");
			if (ini_url_apk.size() == 0)
			{
				ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [{}], config={}\n", keyName, pathstr));
				break;
			}

			mode.name = ini_modelName;

			keyName = fmt::format("gamemode{}:serverExeFile", modeID);
			std::string ini_serverExeFile = iniparser_getstring(ini, keyName.c_str(), "");
			if (ini_serverExeFile.size() == 0)
			{
				ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [{}], config={}\n", keyName, pathstr));
				break;
			}

			mode.serverExeFile = ini_serverExeFile;

			keyName = fmt::format("gamemode{}:serverExeOptions", modeID);
			std::string ini_serverExeOptions = iniparser_getstring(ini, keyName.c_str(), "");
			if (ini_serverExeOptions.size() == 0)
			{
				ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [{}], config={}\n", keyName, pathstr));
				break;
			}

			mode.serverExeOptions = ini_serverExeOptions;

			keyName = fmt::format("gamemode{}:playerMax", modeID);
			int ini_playerMax = iniparser_getint(ini, keyName.c_str(), -1);
			if (-1 == ini_playerMax)
			{
				ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [{}], config={}\n", keyName, pathstr));
				break;
			}

			mode.playerMax = ini_playerMax;

			keyName = fmt::format("gamemode{}:gameTime", modeID);
			int ini_gameTime = iniparser_getint(ini, keyName.c_str(), -1);
			if (-1 == ini_gameTime)
			{
				ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not found [{}], config={}\n", keyName, pathstr));
				break;
			}

			mode.gameTime = ini_gameTime;

			cfg.gameModes.push_back(mode);
		}

		iniparser_freedict(ini);

		if (cfg.gameModes.size() > 0)
		{
			gamecfgs_[cfg.id] = cfg;
		}
		else
		{
			ERROR_MSG(fmt::format("ResMgr::loadGameConfigs(): not gameMode, config={}\n", pathstr));
		}
	}

	INFO_MSG(fmt::format("ResMgr::loadGameConfigs(): game={}\n", gamecfgs_.size()));

#if X_PLATFORM == PLATFORM_WIN32
	printf("ResMgr::loadGameConfigs(): game=%d\n", (int)gamecfgs_.size());
#endif

	return true;
}

//-------------------------------------------------------------------------------------
bool ResMgr::loadConfigs(const std::string& srvName)
{
	dictionary* ini;
	std::string ini_name = matchRes("server/server.ini");

	ini = iniparser_load(ini_name.c_str());
	if (ini == NULL) 
	{
		fprintf(stderr, "ResMgr::loadConfigs(): cannot parse file: %s\n", ini_name.c_str());
		ERROR_MSG(fmt::format("ResMgr::loadConfigs(): cannot parse file: {}\n", ini_name));
		return false;
	}

	for (int srvType = (int)ServerType::SERVER_TYPE_UNKNOWN; srvType < (int)ServerType::SERVER_TYPE_MAX; ++srvType)
	{
		ServerConfig srvcfg;
		std::string currSrvName = ServerType2Name[srvType];

		// common
		int tickInterval = iniparser_getint(ini, "common:tickInterval", -1);
		if (-1 != tickInterval)
			srvcfg.tickInterval = tickInterval;

		int heartbeatInterval = iniparser_getint(ini, "common:heartbeatInterval", -1);
		if (-1 != heartbeatInterval)
			srvcfg.heartbeatInterval = heartbeatInterval;

		int internal_SNDBUF = iniparser_getint(ini, "common:internal_SNDBUF", -1);
		if (-1 != internal_SNDBUF)
			srvcfg.internal_SNDBUF = internal_SNDBUF;

		int internal_RCVBUF = iniparser_getint(ini, "common:internal_RCVBUF", -1);
		if (-1 != internal_RCVBUF)
			srvcfg.internal_RCVBUF = internal_RCVBUF;

		int external_SNDBUF = iniparser_getint(ini, "common:external_SNDBUF", -1);
		if (-1 != external_SNDBUF)
			srvcfg.external_SNDBUF = external_SNDBUF;

		int external_RCVBUF = iniparser_getint(ini, "common:external_RCVBUF", -1);
		if (-1 != external_RCVBUF)
			srvcfg.external_RCVBUF = external_RCVBUF;

		std::string internal_ip = iniparser_getstring(ini, "common:internal_ip", "");
		if (internal_ip.size() > 0)
			srvcfg.internal_ip = internal_ip;

		std::string internal_exposedIP = iniparser_getstring(ini, "common:internal_exposedIP", "");
		if (internal_exposedIP.size() > 0)
			srvcfg.internal_exposedIP = internal_exposedIP;

		int internal_port = iniparser_getint(ini, "common:internal_port", -1);
		if (-1 != internal_port)
			srvcfg.internal_port = internal_port;

		std::string external_ip = iniparser_getstring(ini, "common:external_ip", "");
		if (external_ip.size() > 0)
			srvcfg.external_ip = external_ip;

		std::string external_exposedIP = iniparser_getstring(ini, "common:external_exposedIP", "");
		if (external_exposedIP.size() > 0)
			srvcfg.external_exposedIP = external_exposedIP;
		
		int external_port = iniparser_getint(ini, "common:external_port", -1);
		if (-1 != external_port)
			srvcfg.external_port = external_port;

		int debugPacket = iniparser_getint(ini, "common:debugPacket", -1);
		if (-1 != debugPacket)
			srvcfg.debugPacket = debugPacket > 0;

		int netEncrypted = iniparser_getint(ini, "common:netEncrypted", -1);
		if (-1 != netEncrypted)
			srvcfg.netEncrypted = netEncrypted > 0;

		int shutdownTime = iniparser_getint(ini, "common:shutdownTime", -1);
		if (-1 != shutdownTime)
			srvcfg.shutdownTime = shutdownTime;
		
		int shutdownTick = iniparser_getint(ini, "common:shutdownTick", -1);
		if (-1 != shutdownTick)
			srvcfg.shutdownTick = shutdownTick;

		int threads = iniparser_getint(ini, "common:threads", -1);
		if (-1 != threads)
			srvcfg.threads = threads;

		// app configuration
		tickInterval = iniparser_getint(ini, fmt::format("{}:tickInterval", currSrvName).c_str(), -1);
		if (-1 != tickInterval)
			srvcfg.tickInterval = tickInterval;

		heartbeatInterval = iniparser_getint(ini, fmt::format("{}:heartbeatInterval", currSrvName).c_str(), -1);
		if (-1 != heartbeatInterval)
			srvcfg.heartbeatInterval = heartbeatInterval;

		internal_SNDBUF = iniparser_getint(ini, fmt::format("{}:internal_SNDBUF", currSrvName).c_str(), -1);
		if (-1 != internal_SNDBUF)
			srvcfg.internal_SNDBUF = internal_SNDBUF;

		internal_RCVBUF = iniparser_getint(ini, fmt::format("{}:internal_RCVBUF", currSrvName).c_str(), -1);
		if (-1 != internal_RCVBUF)
			srvcfg.internal_RCVBUF = internal_RCVBUF;

		external_SNDBUF = iniparser_getint(ini, fmt::format("{}:external_SNDBUF", currSrvName).c_str(), -1);
		if (-1 != external_SNDBUF)
			srvcfg.external_SNDBUF = external_SNDBUF;

		external_RCVBUF = iniparser_getint(ini, fmt::format("{}:external_RCVBUF", currSrvName).c_str(), -1);
		if (-1 != external_RCVBUF)
			srvcfg.external_RCVBUF = external_RCVBUF;

		internal_ip = iniparser_getstring(ini, fmt::format("{}:internal_ip", currSrvName).c_str(), "");
		if (internal_ip.size() > 0)
			srvcfg.internal_ip = internal_ip;

		internal_exposedIP = iniparser_getstring(ini, fmt::format("{}:internal_exposedIP", currSrvName).c_str(), "");
		if (internal_exposedIP.size() > 0)
			srvcfg.internal_exposedIP = internal_exposedIP;

		internal_port = iniparser_getint(ini, fmt::format("{}:internal_port", currSrvName).c_str(), -1);
		if (-1 != internal_port)
			srvcfg.internal_port = internal_port;

		external_ip = iniparser_getstring(ini, fmt::format("{}:external_ip", currSrvName).c_str(), "");
		if (external_ip.size() > 0)
			srvcfg.external_ip = external_ip;

		external_exposedIP = iniparser_getstring(ini, fmt::format("{}:external_exposedIP", currSrvName).c_str(), "");
		if (external_exposedIP.size() > 0)
			srvcfg.external_exposedIP = external_exposedIP;
		
		external_port = iniparser_getint(ini, fmt::format("{}:external_port", currSrvName).c_str(), -1);
		if (-1 != external_port)
			srvcfg.external_port = external_port;

		debugPacket = iniparser_getint(ini, fmt::format("{}:debugPacket", currSrvName).c_str(), -1);
		if (-1 != debugPacket)
			srvcfg.debugPacket = debugPacket > 0;

		netEncrypted = iniparser_getint(ini, fmt::format("{}:netEncrypted", currSrvName).c_str(), -1);
		if (-1 != netEncrypted)
			srvcfg.netEncrypted = netEncrypted > 0;
		
		shutdownTick = iniparser_getint(ini, fmt::format("{}:shutdownTick", currSrvName).c_str(), -1);
		if (-1 != shutdownTick)
			srvcfg.shutdownTick = shutdownTick;

		shutdownTime = iniparser_getint(ini, fmt::format("{}:shutdownTime", currSrvName).c_str(), -1);
		if (-1 != shutdownTime)
			srvcfg.shutdownTime = shutdownTime;

		threads = iniparser_getint(ini, fmt::format("{}:threads", currSrvName).c_str(), -1);
		if (-1 != threads)
			srvcfg.threads = threads;

		if ((ServerType)srvType == ServerType::SERVER_TYPE_HALLS)
		{
			int player_disconnected_lifetime = iniparser_getint(ini, fmt::format("{}:player_disconnected_lifetime", currSrvName).c_str(), -1);
			if (-1 != player_disconnected_lifetime)
				srvcfg.player_disconnected_lifetime = player_disconnected_lifetime;
		}
		else if ((ServerType)srvType == ServerType::SERVER_TYPE_DBMGR)
		{
			std::string db_name = iniparser_getstring(ini, fmt::format("{}:db_name", currSrvName).c_str(), "");
			if (db_name.size() > 0)
				srvcfg.db_name = db_name;

			std::string db_username = iniparser_getstring(ini, fmt::format("{}:db_username", currSrvName).c_str(), "");
			if (db_username.size() > 0)
				srvcfg.db_username = db_username;

			std::string db_password = iniparser_getstring(ini, fmt::format("{}:db_password", currSrvName).c_str(), "");
			if (db_password.size() > 0)
				srvcfg.db_password = db_password;

			std::string db_host = iniparser_getstring(ini, fmt::format("{}:db_host", currSrvName).c_str(), "");
			if (db_host.size() > 0)
				srvcfg.db_host = db_host;

			int db_port = iniparser_getint(ini, fmt::format("{}:db_port", currSrvName).c_str(), -1);
			if (-1 != db_port)
				srvcfg.db_port = db_port;
		}

		if (srvcfg.external_exposedIP.size() == 0)
			srvcfg.external_exposedIP = srvcfg.external_ip;

		if (srvcfg.internal_exposedIP.size() == 0)
			srvcfg.internal_exposedIP = srvcfg.internal_ip;
		
		srvcfgs_[currSrvName] = srvcfg;
	}

	pSrvcfg_ = &srvcfgs_[srvName];

	// ∂¡»°µÿ÷∑≥ÿ
	int n = iniparser_getsecnkeys(ini, "server_addresses");
	const char* keys[1024];
	iniparser_getseckeys(ini, "server_addresses", keys);

	for (int i = 0; i < n; ++i)
	{
		std::string addr = keys[i];
		x_replace(addr, "server_addresses:", "");
		pSrvcfg_->server_addresses.push_back(addr);
	}

	iniparser_freedict(ini);

	return loadGameConfigs();
}

//-------------------------------------------------------------------------------------
bool ResMgr::listPathRes(std::wstring path, const std::wstring& extendName, std::vector<std::wstring>& results, int recursiveNum)
{
	if (recursiveNum == 0)
		return true;

	if(path.size() == 0)
	{
		ERROR_MSG("ResMgr::listPathRes: open dir error!\n");
		return false;
	}

	if(path[path.size() - 1] != L'\\' && path[path.size() - 1] != L'/')
		path += L"/";

	std::vector<std::wstring> extendNames;
	x_split<wchar_t>(extendName, L'|', extendNames);

#if X_PLATFORM != PLATFORM_WIN32
	struct dirent *filename;
	DIR *dir;

    char* cpath = wchar2char(path.c_str());
    char pathstr[MAX_PATH];
    strcpy(pathstr, cpath);
    free(cpath);

	dir = opendir(pathstr);
	if(dir == NULL)
	{
		ERROR_MSG(fmt::format("ResMgr::listPathRes: open dir [{}] error!\n", pathstr));
		return false;
	}

	while((filename = readdir(dir)) != NULL)
	{
		if(strcmp(filename->d_name, ".") == 0 || strcmp(filename->d_name, "..") == 0)
			continue;

		struct stat s;
		char pathstrtmp[MAX_PATH];
		sprintf(pathstrtmp,"%s%s",pathstr, filename->d_name);
		lstat(pathstrtmp, &s);

		if(S_ISDIR(s.st_mode))
		{
			wchar_t* wstr = char2wchar(pathstrtmp);
			listPathRes(wstr, extendName, results, recursiveNum - 1);
			free(wstr);
		}
		else
		{
			wchar_t* wstr = char2wchar(filename->d_name);

			if(extendName.size() == 0 || extendName == L"*" || extendName == L"*.*")
			{
				results.push_back(path + wstr);
			}
			else
			{
				if(extendNames.size() > 0)
				{
					std::vector<std::wstring> vec;
					x_split<wchar_t>(wstr, L'.', vec);

					for(size_t ext = 0; ext < extendNames.size(); ++ext)
					{
						if(extendNames[ext].size() > 0 && vec.size() > 1 && vec[vec.size() - 1] == extendNames[ext])
						{
							results.push_back(path + wstr);
						}
					}
				}
				else
				{
					results.push_back(path + wstr);
				}
			}

			free(wstr);
		}
	}

	closedir(dir);

#else
	wchar_t szFind[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	wcscpy(szFind, path.c_str());
	wcscat(szFind, L"*");
	
	HANDLE hFind = FindFirstFile(szFind, &FindFileData);
	if(INVALID_HANDLE_VALUE == hFind)
	{
		char* cstr = wchar2char(path.c_str());
		ERROR_MSG(fmt::format("ResMgr::listPathRes: open dir [{}] error!\n", cstr));
		free(cstr);
		return false;
	}

	while(TRUE)
	{
		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(FindFileData.cFileName[0] != L'.')
			{
				wcscpy(szFind, path.c_str());
				wcscat(szFind, L"");
				wcscat(szFind, FindFileData.cFileName);
				listPathRes(szFind, extendName, results, recursiveNum - 1);
			}
		}
		else
		{
			if(extendName.size() == 0 || extendName == L"*" || extendName == L"*.*")
			{
				results.push_back(path + FindFileData.cFileName);
			}
			else
			{
				if(extendNames.size() > 0)
				{
					std::vector<std::wstring> vec;
					x_split<wchar_t>(FindFileData.cFileName, L'.', vec);

					for(size_t ext = 0; ext < extendNames.size(); ++ext)
					{
						if(extendNames[ext].size() > 0 && vec.size() > 1 && vec[vec.size() - 1] == extendNames[ext])
						{
							results.push_back(path + FindFileData.cFileName);
						}
					}
				}
				else
				{
					results.push_back(path + FindFileData.cFileName);
				}
			}
		}

		if(!FindNextFile(hFind, &FindFileData))
			break;
	}

	FindClose(hFind);

#endif

	return true;
}

//-------------------------------------------------------------------------------------
std::string ResMgr::matchRes(const std::string& res)
{
	std::vector<std::string>::iterator iter = respaths_.begin();

	for (; iter != respaths_.end(); ++iter)
	{
		std::string fpath = ((*iter) + res);

		x_replace(fpath, "\\", "/");
		x_replace(fpath, "//", "/");

		FILE * f = fopen(fpath.c_str(), "r");
		if (f != NULL)
		{
			fclose(f);
			return fpath;
		}
	}

	return res;
}

//-------------------------------------------------------------------------------------
std::string ResMgr::matchPath(const std::string& path)
{
	std::vector<std::string>::iterator iter = respaths_.begin();

	std::string npath = path;
	x_replace(npath, "\\", "/");
	x_replace(npath, "//", "/");

	for (; iter != respaths_.end(); ++iter)
	{
		std::string fpath = ((*iter) + npath);

		x_replace(fpath, "\\", "/");
		x_replace(fpath, "//", "/");

		if (!access(fpath.c_str(), 0))
		{
			return fpath;
		}
	}

	return "";
}

//-------------------------------------------------------------------------------------
FILE* ResMgr::openRes(std::string res, const char* mode)
{
	FILE * f = fopen(matchRes(res).c_str(), mode);
	if (f != NULL)
	{
		return f;
	}

	return NULL;
}

//-------------------------------------------------------------------------------------
}
