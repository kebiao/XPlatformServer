#ifndef X_XROBOT_H
#define X_XROBOT_H

#include "event/Session.h"

namespace XServer {

class XServerBase;
class Session;

class XRobot
{
public:
	enum State 
	{
		directory_connect = 0,
		directory_hello = 1,
		directory_list_servers = 2,
		connect_login_connector = 3,
		login_connector_hello = 4,
		login_signup = 5,
		login_signin = 6,
		connect_halls_connector = 7,
		halls_connector_hello = 8,
		halls_login = 9,
		halls_start_matching = 10,
		playing = 11,
		logout = 12,
	};

public:
	XRobot();
	virtual ~XRobot();

	bool initialize(XServerBase* pXServer);
	void finalise();

	virtual void onProcessPacket(Session* pSession, const uint8* data, int32 size);
	
	virtual void onTick(void* userargs);
	virtual void onHeartbeatTick(void* userargs);

	virtual void onStateConnectDirectory();
	virtual void onStateDirectoryHello();
	virtual void onStateListServers();
	virtual void onStateConnectLoginConnector();
	virtual void onStateLoginConnectorHello();
	virtual void onStateConnectHallsConnector();
	virtual void onStateHallsConnectorHello();
	virtual void onStateHallsLogin();
	virtual void onStateSignup();
	virtual void onStateSignin();
	virtual void onStatePlaying();
	virtual void onStateStartMatching();
	virtual void onStateLogout();

	virtual void onHelloCB(const CMD_HelloCB& packet);

	virtual void onListServersCB(const CMD_Client_OnListServersCB& packet);
	

	virtual void onSignupCB(const CMD_Client_OnSignupCB& packet);
	virtual void onSigninCB(const CMD_Client_OnSigninCB& packet);
	virtual void onLoginCB(const CMD_Client_OnLoginCB& packet);

	virtual void onEndMatch(const CMD_Client_OnEndMatch& packet);
	virtual void onMatchingUpdate(const CMD_Client_OnMatchingUpdate& packet);
	virtual void onGameOver(const CMD_Client_OnGameOver& packet);

	virtual void onUpdatePlayerContext(const CMD_Client_UpdatePlayerContext& packet);
	
	virtual void onListGamesCB(const CMD_Client_OnListGamesCB& packet);
	
	State state() const {
		return botState_;
	}

	virtual void onConnected(Session* pSession);
	virtual void onDisconnected(Session* pSession);

protected:
	struct event * tickTimerEvent_;
	struct event * heartbeatTickTimerEvent_;

	XServerBase* pXServer_;

	State botState_;
	State botDoneState_;

	std::string accountName_;
	std::string playerName_;
	std::string password_;
	uint64 tokenID_;
	uint64 hallsID_;

	std::string clientDatas_;
	std::string serverDatas_;

	std::string serverIP_;
	uint16 serverPort_;
	Session* pSession_;

	ObjectID playerID_;

	CMD_PlayerContext context_;
};

}

#endif // X_XROBOT_H
