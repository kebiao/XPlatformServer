#include "XRobot.h"
#include "XSession.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"
#include "event/Timer.h"
#include "event/TcpSocket.h"
#include "event/Session.h"
#include "event/NetworkInterface.h"
#include "resmgr/ResMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XRobot::XRobot():
tickTimerEvent_(NULL),
heartbeatTickTimerEvent_(NULL),
pXServer_(NULL),
botState_(directory_connect),
botDoneState_(playing),
accountName_(),
playerName_(),
password_(),
tokenID_(),
hallsID_(),
clientDatas_(),
serverDatas_(),
serverIP_("localhost"),
serverPort_(0),
pSession_(NULL),
playerID_(0)
{
}

//-------------------------------------------------------------------------------------
XRobot::~XRobot()
{
	finalise();
}

//-------------------------------------------------------------------------------------
void XRobot::onProcessPacket(Session* pSession, const uint8* data, int32 size)
{
	const PacketHeader& header = pSession->packetHeader();
	if (header.msgcmd <= Client_Begin || header.msgcmd >= Client_End)
		assert(false);
	
	switch (header.msgcmd)
	{
	case CMD::Client_OnSignupCB:
	{
		CMD_Client_OnSignupCB packet;
		packet.ParseFromArray(data, header.msglen);

		onSignupCB(packet);
		break;
	}
	case CMD::Client_OnSigninCB:
	{
		CMD_Client_OnSigninCB packet;
		packet.ParseFromArray(data, header.msglen);

		onSigninCB(packet);
		break;
	}
	case CMD::Client_OnLoginCB:
	{
		CMD_Client_OnLoginCB packet;
		packet.ParseFromArray(data, header.msglen);

		onLoginCB(packet);
		break;
	}
	case CMD::Client_OnListServersCB:
	{
		CMD_Client_OnListServersCB packet;
		packet.ParseFromArray(data, header.msglen);

		onListServersCB(packet);
		break;
	}
	case CMD::Client_OnMatchingUpdate:
	{
		CMD_Client_OnMatchingUpdate packet;
		packet.ParseFromArray(data, header.msglen);

		onMatchingUpdate(packet);
		break;
	}
	case CMD::Client_OnEndMatch:
	{
		CMD_Client_OnEndMatch packet;
		packet.ParseFromArray(data, header.msglen);

		onEndMatch(packet);
		break;
	}
	case CMD::Client_OnGameOver:
	{
		CMD_Client_OnGameOver packet;
		packet.ParseFromArray(data, header.msglen);

		onGameOver(packet);
		break;
	}
	case CMD::Client_UpdatePlayerContext:
	{
		CMD_Client_UpdatePlayerContext packet;
		packet.ParseFromArray(data, header.msglen);

		onUpdatePlayerContext(packet);
		break;
	}
	case CMD::Client_OnListGamesCB:
	{
		CMD_Client_OnListGamesCB packet;
		packet.ParseFromArray(data, header.msglen);

		onListGamesCB(packet);
		break;
	}
	default:
	{
		ERROR_MSG(fmt::format("XSession::onProcessPacket(): unknown packet cmd: {}, msglen={}, sessionID={}, {}\n",
			CMD_Name((CMD)header.msgcmd), header.msglen, pSession->id(), pSession->pTcpSocket()->addr()));

		pSession->destroy();
		return;
		break;
	}
	};
}

//-------------------------------------------------------------------------------------
bool XRobot::initialize(XServerBase* pXServer)
{
	assert(tickTimerEvent_ == NULL);

	pXServer_ = pXServer;

	std::random_device r_device;
	std::mt19937 mt(r_device());

	accountName_ = fmt::format("robot_{}", (mt() + 1));
	password_ = "123456";
	clientDatas_ = "robot";

	tickTimerEvent_ = pXServer_->pTimer()->addTimer(TIME_SECONDS, -1, std::bind(&XRobot::onTick, this, std::placeholders::_1), NULL);
	heartbeatTickTimerEvent_ = pXServer_->pTimer()->addTimer(std::max<uint64>(TIME_SECONDS, ResMgr::getSingleton().serverConfig().heartbeatInterval), -1, std::bind(&XRobot::onHeartbeatTick, this, std::placeholders::_1), NULL);
	return tickTimerEvent_ != NULL && heartbeatTickTimerEvent_ != NULL;
}

//-------------------------------------------------------------------------------------
void XRobot::finalise()
{
	if(pSession_)
		pSession_->destroy();

	if (tickTimerEvent_ && pXServer_->pTimer())
	{
		pXServer_->pTimer()->delTimer(tickTimerEvent_);
		tickTimerEvent_ = NULL;
	}

	if (heartbeatTickTimerEvent_ && pXServer_->pTimer())
	{
		pXServer_->pTimer()->delTimer(heartbeatTickTimerEvent_);
		heartbeatTickTimerEvent_ = NULL;
	}
}

//-------------------------------------------------------------------------------------
void XRobot::onHeartbeatTick(void* userargs)
{
	if (XServerBase::getSingleton().pExternalNetworkInterface())
		XServerBase::getSingleton().pExternalNetworkInterface()->checkSessions();

	if (XServerBase::getSingleton().pInternalNetworkInterface())
		XServerBase::getSingleton().pInternalNetworkInterface()->checkSessions();

	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Heartbeat req_packet;
	req_packet.set_appid(pXServer_->id());
	pSession_->sendPacket(CMD::Heartbeat, req_packet);
}

//-------------------------------------------------------------------------------------
void XRobot::onTick(void* userargs)
{
	if (botDoneState_ != botState_)
	{
		switch (botState_)
		{
		case directory_connect:
			onStateConnectDirectory();
			break;
		case directory_hello:
			onStateDirectoryHello();
			break;
		case directory_list_servers:
			onStateListServers();
			break;
		case connect_login_connector:
			onStateConnectLoginConnector();
			break;
		case login_connector_hello:
			onStateLoginConnectorHello();
			break;
		case login_signup:
			onStateSignup();
			break;
		case login_signin:
			onStateSignin();
			break;
		case connect_halls_connector:
			onStateConnectHallsConnector();
			break;
		case halls_connector_hello:
			onStateHallsConnectorHello();
			break;
		case halls_login:
			onStateHallsLogin();
			break;
		case halls_start_matching:
			onStateStartMatching();
			break;
		case playing:
			onStatePlaying();
			break;
		case logout:
			onStateLogout();
			break;
		default:
			assert(false);
			break;
		};

		botDoneState_ = botState_;
	}
}

//-------------------------------------------------------------------------------------
void XRobot::onConnected(Session* pSession)
{
	DEBUG_MSG(fmt::format("XRobot::onConnected({})\n", pSession->id()));

	if(botState_ == directory_connect)
		botState_ = directory_hello;
	else if (botState_ == connect_login_connector)
		botState_ = login_connector_hello;
	else if (botState_ == connect_halls_connector)
		botState_ = halls_connector_hello;
	else
		assert(false);
}

//-------------------------------------------------------------------------------------
void XRobot::onDisconnected(Session* pSession)
{
	DEBUG_MSG(fmt::format("XRobot::onDisconnected({})\n", pSession->id()));

	if(pSession == pSession_)
		pSession_ = NULL;
}

//-------------------------------------------------------------------------------------
void XRobot::onHelloCB(const CMD_HelloCB& packet)
{
	if (botState_ == directory_hello)
		botState_ = directory_list_servers;
	else if (botState_ == login_connector_hello)
		botState_ = login_signup;
	else if (botState_ == halls_connector_hello)
		botState_ = halls_login;
	else
		assert(false);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateConnectDirectory()
{
	if(pSession_)
		pSession_->destroy();

	const ResMgr::ServerConfig& cfg = ResMgr::getSingleton().findConfig("directory");

	DEBUG_MSG(fmt::format("XRobot::onStateConnectDirectory: connect {}:{}...\n", cfg.external_ip, cfg.external_port));

	pSession_ = XServerBase::getSingleton().pServerMgr()->connectServer(cfg.external_ip, cfg.external_port, ServerType::SERVER_TYPE_DIRECTORY);

	if (pSession_)
		((XSession*)pSession_)->pXRobot(this);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateDirectoryHello()
{
	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Hello req_packet;
	req_packet.set_version(XPLATFORMSERVER_VERSION);
	req_packet.set_appid(0);
	req_packet.set_apptype((int32)ServerType::SERVER_TYPE_ROBOT);
	pSession_->sendPacket(CMD::Hello, req_packet);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateListServers()
{
	CMD_Directory_ListServers req_packet;
	pSession_->sendPacket(CMD::Directory_ListServers, req_packet);
	DEBUG_MSG(fmt::format("XRobot::onStateListServers: list ...\n"));
}

//-------------------------------------------------------------------------------------
void XRobot::onListServersCB(const CMD_Client_OnListServersCB& packet)
{
	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XRobot::onListServersCB: error={}\n", ServerError_Name(packet.errcode())));
		botState_ = directory_connect;
		botDoneState_ = playing;
		return;
	}

	for (int i = 0; i < packet.srvs_size(); ++i)
	{
		const CMD_ListServersInfo& info = packet.srvs(i);

		DEBUG_MSG(fmt::format("XRobot::onListServersCB: appID={}, appGID={}, name={}, addr={}:{}\n", 
			info.id(), info.groupid(), info.name(), info.addr(), info.port()));

		serverIP_ = info.addr();
		serverPort_ = info.port();
	}

	botState_ = connect_login_connector;
}

//-------------------------------------------------------------------------------------
void XRobot::onStateConnectLoginConnector()
{
	pSession_->destroy();

	DEBUG_MSG(fmt::format("XRobot::onStateConnectLoginConnector: connect {}:{}...\n", serverIP_, serverPort_));
	pSession_ = XServerBase::getSingleton().pServerMgr()->connectServer(serverIP_, serverPort_, ServerType::SERVER_TYPE_CONNECTOR);

	if (pSession_)
		((XSession*)pSession_)->pXRobot(this);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateLoginConnectorHello()
{
	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Hello req_packet;
	req_packet.set_version(XPLATFORMSERVER_VERSION);
	req_packet.set_appid(0);
	req_packet.set_apptype((int32)ServerType::SERVER_TYPE_ROBOT);
	pSession_->sendPacket(CMD::Hello, req_packet);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateConnectHallsConnector()
{
	if (!pSession_ || !pSession_->connected())
		return;

	pSession_->destroy();

	DEBUG_MSG(fmt::format("XRobot::onStateConnectHallsConnector: connect {}:{}...\n", serverIP_, serverPort_));
	pSession_ = XServerBase::getSingleton().pServerMgr()->connectServer(serverIP_, serverPort_, ServerType::SERVER_TYPE_CONNECTOR);

	if (pSession_)
		((XSession*)pSession_)->pXRobot(this);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateHallsConnectorHello()
{
	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Hello req_packet;
	req_packet.set_version(XPLATFORMSERVER_VERSION);
	req_packet.set_appid(0);
	req_packet.set_apptype((int32)ServerType::SERVER_TYPE_ROBOT);
	pSession_->sendPacket(CMD::Hello, req_packet);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateHallsLogin()
{
	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Halls_Login req_packet;
	req_packet.set_commitaccountname(accountName_);
	req_packet.set_tokenid(tokenID_);
	req_packet.set_hallsid(hallsID_);
	pSession_->sendPacket(CMD::Halls_Login, req_packet);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateSignup()
{
	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Login_Signup req_packet;
	req_packet.set_commitaccountname(accountName_);
	req_packet.set_password(password_);
	req_packet.set_datas(clientDatas_);
	pSession_->sendPacket(CMD::Login_Signup, req_packet);

	DEBUG_MSG(fmt::format("XRobot::onStateSignup: request signup({}) ...\n", accountName_));
}

//-------------------------------------------------------------------------------------
void XRobot::onSignupCB(const CMD_Client_OnSignupCB& packet)
{
	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XRobot::onSignupCB: signup failed! error={}\n", ServerError_Name(packet.errcode())));
		return;
	}

	serverDatas_ = packet.datas();
	botState_ = login_signin;

	DEBUG_MSG(fmt::format("XRobot::onSignupCB: signup({}) success!\n", accountName_));
}

//-------------------------------------------------------------------------------------
void XRobot::onStateSignin()
{
	if (!pSession_ || !pSession_->connected())
		return;

	CMD_Login_Signin req_packet;
	req_packet.set_commitaccountname(accountName_);
	req_packet.set_password(password_);
	req_packet.set_datas(clientDatas_);
	pSession_->sendPacket(CMD::Login_Signin, req_packet);

	DEBUG_MSG(fmt::format("XRobot::onStateSignin: request signin({}) ...\n", accountName_));
}

//-------------------------------------------------------------------------------------
void XRobot::onSigninCB(const CMD_Client_OnSigninCB& packet)
{
	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XRobot::onSigninCB: signin failed! error={}\n", ServerError_Name(packet.errcode())));
		return;
	}

	serverDatas_ = packet.datas();
	tokenID_ = packet.tokenid();
	serverIP_ = packet.addr();
	serverPort_ = packet.port();
	hallsID_ = packet.hallsid();
	botState_ = connect_halls_connector;

	DEBUG_MSG(fmt::format("XRobot::onSigninCB: signin({}) success! tokenID={}, halls_addr={}:{}\n", 
		accountName_, tokenID_, serverIP_, serverPort_));
}

//-------------------------------------------------------------------------------------
void XRobot::onLoginCB(const CMD_Client_OnLoginCB& packet)
{
	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XRobot::onLoginCB: login halls failed! error={}\n", ServerError_Name(packet.errcode())));
		return;
	}

	serverDatas_ = packet.datas();

	botState_ = halls_start_matching;
	playerID_ = packet.playercontext().playerid();
	playerName_ = packet.playercontext().playername();

	context_.CopyFrom(packet.playercontext());
	DEBUG_MSG(fmt::format("XRobot::onLoginCB: login halls({}) success! playerID={}\n", playerName_, playerID_));

	CMD_Halls_ListGames req_packet;
	req_packet.set_page(1);
	req_packet.set_type(1);
	req_packet.set_maxnum(100);
	pSession_->sendPacket(CMD::Halls_ListGames, req_packet);
}

//-------------------------------------------------------------------------------------
void XRobot::onStateStartMatching()
{
	if (!pSession_ || !pSession_->connected())
		return;

	std::random_device r_device;
	std::uniform_int_distribution<int> dis(0, 3);

	GameID gameID = 1;
	int gameMode = dis(r_device);

	CMD_Halls_StartMatch req_packet;
	req_packet.set_gameid(gameID);
	req_packet.set_gamemode(gameMode);
	pSession_->sendPacket(CMD::Halls_StartMatch, req_packet);

	DEBUG_MSG(fmt::format("XRobot::onStateStartMatching: {} request matching(gameID={}, gameMode={}) ...\n", 
		accountName_, req_packet.gameid(), req_packet.gamemode()));
}

//-------------------------------------------------------------------------------------
void XRobot::onMatchingUpdate(const CMD_Client_OnMatchingUpdate& packet)
{
	DEBUG_MSG(fmt::format("XRobot::onMatchingUpdate({}): name={}, id={}, modelID={}, enter={}!\n", 
		accountName_, packet.name(), packet.id(), packet.modelid(), packet.enter()));
}

//-------------------------------------------------------------------------------------
void XRobot::onEndMatch(const CMD_Client_OnEndMatch& packet)
{
	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XRobot::onEndMatch: error={}\n", ServerError_Name(packet.errcode())));

		if(packet.errcode() == ServerError::MATCH_FAILED)
			botState_ = halls_start_matching;

		return;
	}

	botState_ = playing;

	DEBUG_MSG(fmt::format("XRobot::onEndMatch({}): gameServerAddr={}:{}, gameServerTokenID={}, roomID={}, gameID={}, gameMode={}, gameTime={}, players={}!\n",
		accountName_, packet.gameserverip(), packet.gameserverport(), packet.gameservertokenid(), packet.roomid(), packet.gameid(), packet.gamemode(), packet.gametime(), packet.gameplayersize()));
}

//-------------------------------------------------------------------------------------
void XRobot::onGameOver(const CMD_Client_OnGameOver& packet)
{
	ERROR_MSG(fmt::format("XRobot::onGameOver: error={}\n", ServerError_Name(packet.errcode())));
	botState_ = halls_start_matching;
}

//-------------------------------------------------------------------------------------
void XRobot::onStatePlaying()
{
	if (!pSession_ || !pSession_->connected())
		return;

	DEBUG_MSG(fmt::format("XRobot::onStatePlaying: playing({})!\n", accountName_));
}

//-------------------------------------------------------------------------------------
void XRobot::onStateLogout()
{
	if (!pSession_ || !pSession_->connected())
		return;

}

//-------------------------------------------------------------------------------------
void XRobot::onUpdatePlayerContext(const CMD_Client_UpdatePlayerContext& packet)
{
	DEBUG_MSG(fmt::format("XRobot::onUpdatePlayerContext: playing({})!\n", accountName_));
}

//-------------------------------------------------------------------------------------
void XRobot::onListGamesCB(const CMD_Client_OnListGamesCB& packet)
{
	DEBUG_MSG(fmt::format("XRobot::onListGamesCB: playing({}), totalGameNum={}!\n", accountName_, packet.totalgamesnum()));
}

//-------------------------------------------------------------------------------------
}
