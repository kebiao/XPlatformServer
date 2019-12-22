#include "XServerApp.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "server/XServerBase.h"
#include "protos/Commands.pb.h"
#include "event/Session.h"
#include "event/TcpSocket.h"
#include "log/XLog.h"
#include "resmgr/ResMgr.h"
#include "common/threadpool.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XServerApp::XServerApp():
	createRoomContexts_()
{
}

//-------------------------------------------------------------------------------------
XServerApp::~XServerApp()
{
}

//-------------------------------------------------------------------------------------		
NetworkInterface* XServerApp::createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal)
{
	return new XNetworkInterface(pEventDispatcher, isInternal);
}

//-------------------------------------------------------------------------------------
bool XServerApp::initializeBegin()
{
	pServerMgr_->addInterestedServerType(ServerType::SERVER_TYPE_ROOMMGR);
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::initializeEnd()
{
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::finalise()
{
	for (auto& item : createRoomContexts_)
	{
		std::string cmd;
#if X_PLATFORM == PLATFORM_WIN32
		cmd = fmt::format("taskkill /F /PID {}", item.second.createResult.pid);
		system(cmd.c_str());
#else
		cmd = fmt::format("kill -9 {}", item.second.createResult.pid);
		system(cmd.c_str());
#endif
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::onUpdateServerInfoToSession(CMD_UpdateServerInfos& infos)
{
}

//-------------------------------------------------------------------------------------
bool XServerApp::addPendingCreateRoom(const CMD_Machine_RequestCreateRoom& packet)
{
	CreateRoomContext* pCreateRoomContext = findPendingCreateRoom(packet.roomid());
	if (pCreateRoomContext)
	{
		ERROR_MSG(fmt::format("XServerApp::addPendingCreateRoom(): alredy exist! hallsID={}, gameID={}, mode={}, roomID={}\n",
			packet.hallsid(), packet.gameid(), (int)packet.gamemode(), packet.roomid()));

		assert(false);
		return false;
	}

	CreateRoomContext& datas = createRoomContexts_[packet.roomid()];
	datas.requestInfos.CopyFrom(packet);
	datas.setState(CreateRoomContext::WaitCreate);

	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::removePendingCreateRoom(ObjectID roomID)
{
	CreateRoomContext* pCreateRoomContext = findPendingCreateRoom(roomID);
	if (!pCreateRoomContext)
	{
		ERROR_MSG(fmt::format("XServerApp::removePendingCreateRoom(): not found! roomID={}\n",
			roomID));

		return false;
	}

	createRoomContexts_.erase(roomID);
	return true;
}

//-------------------------------------------------------------------------------------
XServerApp::CreateRoomContext* XServerApp::findPendingCreateRoom(ObjectID roomID)
{
	auto iter = createRoomContexts_.find(roomID);
	if (iter == createRoomContexts_.end())
	{
		return NULL;
	}

	return &iter->second;
}

//-------------------------------------------------------------------------------------
bool XServerApp::startRoomServerProcess(CreateRoomContext* context)
{
	INFO_MSG(fmt::format("XServerApp::startRoomServerProcess(): hallsID={}, gameID={}, mode={}, roomID={}, exefile={} {}\n",
		context->requestInfos.hallsid(), context->requestInfos.gameid(), (int)context->requestInfos.gamemode(), context->requestInfos.roomid(), 
		context->requestInfos.exefile(), context->requestInfos.exeoptions()));

	context->setState(CreateRoomContext::Creating);

	CMD_Machine_RequestCreateRoom datas;
	datas.CopyFrom(context->requestInfos);

#if X_PLATFORM == PLATFORM_WIN32
	context->createResultFuture = pThreadPool()->enqueue([datas] {
		return XServerApp::startWindowsProcess(datas);
	});

#else
	context->createResultFuture = pThreadPool()->enqueue([datas] {
		return XServerApp::startLinuxProcess(datas);
	});

#endif

	return true;
}

//-------------------------------------------------------------------------------------
#if X_PLATFORM == PLATFORM_WIN32
#include <strsafe.h>
XServerApp::ThreadCreateRoomResult XServerApp::startWindowsProcess(CMD_Machine_RequestCreateRoom context)
{
	ThreadCreateRoomResult result;
	result.roomID = context.roomid();

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	wchar_t currdir[1024];
	GetCurrentDirectory(sizeof(currdir), currdir);

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	const int BUFSIZE = 65535;
	TCHAR chNewEnv[BUFSIZE];
	LPTSTR lpszCurrentVariable;

	lpszCurrentVariable = (LPTSTR)chNewEnv;

	WCHAR* pEnv = GetEnvironmentStrings();
	WCHAR* pCurrEnv = pEnv;

	while (*pCurrEnv != 0)
	{
		if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, (wchar_t*)pCurrEnv)))
		{
			result.sys_error = GetLastError();
			FreeEnvironmentStrings(pCurrEnv);
			result.errcode = CREATE_ROOM_PROCESS_FAILED;
			return result;
		}

		lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;
		pCurrEnv += wcslen(pCurrEnv) + 1;
	}

	FreeEnvironmentStrings(pEnv);

	std::string str = "games/" + context.exefile();
	str += ".exe";

	std::string exe_path = ResMgr::getSingleton().matchRes(str);

	// 用双引号把命令行括起来，以避免路径中存在空格，从而执行错误
	exe_path = "\"" + exe_path + "\"";

	// 増加参数
	exe_path += fmt::format(" -roomID={} ", context.roomid());
	exe_path += context.exeoptions();

	size_t outlen = 0;
	wchar_t* szCmdline = char2wchar(exe_path.c_str(), &outlen);

	wchar_t* szStr = char2wchar(fmt::format("roomID={}", context.roomid()).c_str(), &outlen);
	if(FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("gameID={}", context.gameid()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("gameModeID={}", context.gamemode()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("maxPlayerNum={}", context.maxplayernum()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("playerNum={}", context.players_size()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	for (int i = 0; i < context.players_size(); ++i)
	{
		lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

		free(szStr);

		const CMD_RoomPlayerInfo& roomPlayerInfo = context.players(i);
		szStr = char2wchar(fmt::format("player{}={}|&{}|&{}|&{}|&{}|&{}|&{}|&{}", i, 
			roomPlayerInfo.playerid(), 
			roomPlayerInfo.playername(), 
			roomPlayerInfo.playermodelid(), 
			roomPlayerInfo.clientaddr(),
			roomPlayerInfo.score(), 
			roomPlayerInfo.topscore(), 
			roomPlayerInfo.victory(), 
			roomPlayerInfo.defeat()).c_str(),
			&outlen);

		if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
		{
			result.sys_error = GetLastError();
			free(szStr);
			free(szCmdline);
			result.errcode = CREATE_ROOM_PROCESS_FAILED;
			return result;
		}
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("hallsID={}", context.hallsid()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("machineReportIP={}", "localhost").c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("roommgrIP={}", context.roommgrip()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("roommgrPort={}", context.roommgrport()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;

	free(szStr);
	szStr = char2wchar(fmt::format("machineReportPort={}", 
		((XServerApp&)XServerApp::getSingleton()).pInternalNetworkInterface()->getListenerPort()).c_str(), &outlen);

	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, szStr)))
	{
		result.sys_error = GetLastError();
		free(szStr);
		free(szCmdline);
		result.errcode = CREATE_ROOM_PROCESS_FAILED;

		return result;
	}

	free(szStr);

	//使环境变量内存块以NULL结尾  
	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1;
	*lpszCurrentVariable = (TCHAR)0;

	if (!CreateProcessW(NULL,   // No module name (use command line)
		szCmdline,      // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,    // No creation flags
		(LPVOID)chNewEnv,           // Use parent's environment block
		currdir,        // Use parent's starting directory
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		free(szCmdline);
		result.sys_error = GetLastError();
		result.errcode = CREATE_ROOM_PROCESS_FAILED;
		return result;
	}

	free(szCmdline);

	result.port = 0;
	result.pid = pi.dwProcessId;
	result.errcode = ServerError::OK;
	return result;
}
#else
//-------------------------------------------------------------------------------------
XServerApp::ThreadCreateRoomResult XServerApp::startLinuxProcess(CMD_Machine_RequestCreateRoom context)
{
	ThreadCreateRoomResult result1;
	uint16 childpid;

	if ((childpid = fork()) == 0)
	{
		std::string str = "rooms/" + context.exefile();

		std::string exe_path = ResMgr::getSingleton().matchRes(str);

		// 増加参数
		std::string soptions = fmt::format("-roomID={} ", context.roomid());
		soptions += context.exeoptions();
		std::vector< std::string > options;
		x_split(soptions, " ", options);

		assert(options.size() < 256);

		const char *argv[256];
		const char **pArgv = argv;
		
		*pArgv++ = exe_path.c_str();

		for(auto& item : options)
			*pArgv++ = item.c_str();

		*pArgv = NULL;

		// 关闭父类的socket
		//if(pInternalNetworkInterface())
		//	pInternalNetworkInterface()->finalise();

		//if (pExternalNetworkInterface())
		//	pExternalNetworkInterface()->finalise();

		//XServer::XLog::getSingleton().closeLogger();

		//int result = execv(exe_path.c_str(), (char * const *)argv);

		//if (result == -1)
		//{
		//	result.sys_error = errno;
		//}

		exit(1);
		return result1;
	}
	//else
	//	return childpid;

	return result1;
}
#endif

//-------------------------------------------------------------------------------------
void XServerApp::onCreateRoomServerProcessFinished(CreateRoomContext& context)
{
	CMD_Roommgr_OnRequestCreateRoomCB res_packet;
	res_packet.set_hallsid(context.requestInfos.hallsid());
	res_packet.set_roomid(context.requestInfos.roomid());

	if (context.state == CreateRoomContext::State::CreateTimeout)
	{
		ERROR_MSG(fmt::format("XServerApp::onCreateRoomServerProcessFinished(): start process is timeout! hallsID={}, gameID={}, mode={}, roomID={}, exefile={} {}\n",
			context.requestInfos.hallsid(), context.requestInfos.gameid(),
			(int)context.requestInfos.gamemode(), context.requestInfos.roomid(), 
			context.requestInfos.exefile(), context.requestInfos.exeoptions()));

		res_packet.set_errcode(ServerError::TIMEOUT);
	}
	else if (context.state == CreateRoomContext::State::WaitReport)
	{
		if (context.createResult.port > 0)
		{
			context.setState(CreateRoomContext::State::Finished);
			onCreateRoomServerProcessFinished(context);
			context.setState(CreateRoomContext::State::WaitGameOver);
		}

		return;
	}
	else
	{
		res_packet.set_ip(ResMgr::getSingleton().serverConfig().external_exposedIP);
		res_packet.set_port(context.createResult.port);
		res_packet.set_tokenid(context.requestInfos.tokenid());
		res_packet.set_errcode(context.createResult.errcode);

		DEBUG_MSG(fmt::format("XServerApp::onCreateRoomServerProcessFinished(): addr={}:{}, hallsID={}, gameID={}, mode={}, roomID={}, exefile={} {}, errcode={}, sys_error={}\n",
			res_packet.ip(), res_packet.port(), context.requestInfos.hallsid(), context.requestInfos.gameid(),
			(int)context.requestInfos.gamemode(), context.requestInfos.roomid(), ResMgr::getSingleton().matchRes("rooms/" + context.requestInfos.exefile()),
			context.requestInfos.exeoptions(), ServerError_Name(context.createResult.errcode), context.createResult.sys_error));
	}

	ServerInfo* pServerInfo = ((XServerApp&)XServerApp::getSingleton()).pServerMgr()->findServerOne(ServerType::SERVER_TYPE_ROOMMGR);
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::onCreateRoomServerProcessFinished(): create room is failed! not found roomMgr!\n"));

		res_packet.set_errcode(ServerError::SERVER_NOT_READY);
		return;
	}

	pServerInfo->pSession->sendPacket(CMD::Roommgr_OnRequestCreateRoomCB, res_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onTick(void* userargs)
{
	std::vector<ObjectID> dels;

	for (auto& item : createRoomContexts_)
	{
		if (item.second.state == CreateRoomContext::State::WaitCreate)
		{
			startRoomServerProcess(&item.second);
		}
		else if (item.second.state == CreateRoomContext::State::Creating)
		{
			std::chrono::microseconds span(1);

			if (item.second.createResultFuture.wait_for(span) != std::future_status::ready)
			{
				if (item.second.isCreateTimeout())
				{
					item.second.setState(CreateRoomContext::State::CreateTimeout);
					onCreateRoomServerProcessFinished(item.second);
					dels.push_back(item.first);

					killProcess(item.second.createResult.pid);
				}
			}
			else
			{
				item.second.createResult = item.second.createResultFuture.get();

				if(item.second.createResult.errcode == ServerError::OK &&
					item.second.createResult.port == 0)
					item.second.setState(CreateRoomContext::State::WaitReport);
				else
					item.second.setState(CreateRoomContext::State::Finished);

				onCreateRoomServerProcessFinished(item.second);
			}
		}
		else if (item.second.state == CreateRoomContext::State::WaitReport)
		{
			if (item.second.isCreateTimeout())
			{
				item.second.setState(CreateRoomContext::State::CreateTimeout);
				onCreateRoomServerProcessFinished(item.second);
				dels.push_back(item.first);
			}
			else
			{
				onCreateRoomServerProcessFinished(item.second);
			}
		}
		else if (item.second.state == CreateRoomContext::State::CreateTimeout)
		{
			onCreateRoomServerProcessFinished(item.second);
			dels.push_back(item.first);
		}
		else if (item.second.state == CreateRoomContext::State::Finished)
		{
			onCreateRoomServerProcessFinished(item.second);
			item.second.setState(CreateRoomContext::State::WaitGameOver);

			// checking process
			//dels.push_back(item.first);
		}
		else if (item.second.state == CreateRoomContext::State::WaitKillGameServer)
		{
			if (item.second.isKillTimeout())
			{
				dels.push_back(item.first);

				killProcess(item.second.createResult.pid);

				ERROR_MSG(fmt::format("XServerApp::onTick(): kill gameRoom! roomID={}! killProcess={}\n",
					item.second.requestInfos.roomid(), item.second.createResult.pid));
			}
		}
		else
		{
			if (item.second.isGameTimeout())
			{
				dels.push_back(item.first);

				killProcess(item.second.createResult.pid);

				ERROR_MSG(fmt::format("XServerApp::onTick(): gameRoom is timeout! roomID={}! killProcess={}\n",
					item.second.requestInfos.roomid(), item.second.createResult.pid));
			}
		}
	}

	for (auto& item : dels)
	{
		createRoomContexts_.erase(item);
	}
}

//-------------------------------------------------------------------------------------
bool XServerApp::killProcess(unsigned long pid)
{
	if (pid == 0)
		return false;

	std::string cmd;

#if X_PLATFORM == PLATFORM_WIN32
	cmd = fmt::format("taskkill /F /PID {}", pid);
	system(cmd.c_str());
	//killProcess(item.second.createResult.pid);
#else
	cmd = fmt::format("kill -9 {}", pid);
	system(cmd.c_str());
#endif

	/*
#if X_PLATFORM == PLATFORM_WIN32
	HANDLE hPrc;

	if (0 == pid)
		return false;

	hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (!TerminateProcess(hPrc, 0))
	{
		CloseHandle(hPrc);
		return false;
	}
	else
	{
		WaitForSingleObject(hPrc, 1000);
	}

	CloseHandle(hPrc);
#else

#endif
*/

	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestCreateRoom(Session* pSession, const CMD_Machine_RequestCreateRoom& packet)
{
	DEBUG_MSG(fmt::format("XServerApp::onSessionRequestCreateRoom(): hallsID={}, gameID={}, mode={}, roomID={}\n",
		packet.hallsid(), packet.gameid(), (int)packet.gamemode(), packet.roomid()));

	addPendingCreateRoom(packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRoomSrvReportAddr(Session* pSession, const CMD_Machine_RoomSrvReportAddr& packet)
{
	DEBUG_MSG(fmt::format("XServerApp::onSessionRoomSrvReportAddr(): hallsID={}, roomID={}, roomSrvAddr={}:{}\n",
		packet.hallsid(), packet.roomid(), packet.ip(), packet.port()));

	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRoomSrvReportAddr: roomsrv={}! error={}\n", 
			packet.roomid(), ServerError_Name(packet.errcode())));

		return;
	}

	CreateRoomContext* pCreateRoomContext = findPendingCreateRoom(packet.roomid());
	if (!pCreateRoomContext)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRoomSrvReportAddr(): not found! roomID={}\n",
			packet.roomid()));

		return;
	}

	pCreateRoomContext->createResult.port = packet.port();
	pCreateRoomContext->createResult.ip = ResMgr::getSingleton().serverConfig().external_exposedIP;

	onTick(NULL);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRoomSrvGameOverReport(Session* pSession, const CMD_Machine_OnRoomSrvGameOverReport& packet)
{
	ServerInfo* pServerInfo = pServerMgr()->findRoommgr();

	if (!pServerInfo)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRoomSrvGameOverReport(): create room is failed! not found roommgr!\n"));
		return;
	}

	CMD_Roommgr_OnRoomSrvGameOverReport res_packet;
	res_packet.set_roomid(packet.roomid());
	res_packet.set_errcode(packet.errcode());
	res_packet.set_hallsid(packet.hallsid());

	for (int i = 0; i < packet.playerdatas_size(); ++i)
	{
		CMD_Halls_PlayerGameData* pCMD_Halls_PlayerGameData = res_packet.add_playerdatas();
		pCMD_Halls_PlayerGameData->set_exp(0);
		pCMD_Halls_PlayerGameData->CopyFrom(packet.playerdatas(i));
	}

	pServerInfo->pSession->sendPacket(CMD::Roommgr_OnRoomSrvGameOverReport, res_packet);

	CreateRoomContext* pCreateRoomContext = findPendingCreateRoom(packet.roomid());
	if (!pCreateRoomContext)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRoomSrvGameOverReport(): not found! roomID={}\n",
			packet.roomid()));

		return;
	}

	pCreateRoomContext->setKillTime(60);

	DEBUG_MSG(fmt::format("XServerApp::onSessionRoomSrvGameOverReport(): gameRoom is gameover! roomID={}! killProcess={}\n",
		pCreateRoomContext->requestInfos.roomid(), pCreateRoomContext->createResult.pid));
}

//-------------------------------------------------------------------------------------
}
