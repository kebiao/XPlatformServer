#ifndef X_SERVER_APP_H
#define X_SERVER_APP_H

#include "common/common.h"
#include "common/singleton.h"
#include "server/XServerBase.h"

namespace XServer {

class ConnectorFinder;

class XServerApp : public XServerBase
{
	struct PendingAccount
	{
		PendingAccount()
		{
			requestorSessionID = SESSION_ID_INVALID;
			currentSessionID = SESSION_ID_INVALID;
			commitAccountName = "";
			password = "";
			commitDatas = "";
			backDatas = "";
			updateTime();
		}

		bool isTimeout()
		{
			time_t diff = getTimeStamp() - lastTime;
			return 15 * TIME_SECONDS < diff;
		}

		void updateTime()
		{
			lastTime = getTimeStamp();
		}

		SessionID requestorSessionID;
		SessionID currentSessionID;
		std::string commitAccountName; 
		std::string password;
		std::string commitDatas;
		std::string backDatas;
		time_t lastTime;
	};

public:
	XServerApp();
	~XServerApp();
	
	virtual NetworkInterface* createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal) override;

	virtual bool initializeBegin() override;
	virtual bool inInitialize() override;
	virtual bool initializeEnd() override;
	virtual void finalise() override;

	virtual void onHeartbeatTick(void* userargs) override;

	virtual void onServerJoined(ServerInfo* pServerInfo) override;
	virtual void onServerExit(ServerInfo* pServerInfo) override;

	virtual void onSessionQueryServerInfosCB(Session* pSession, const CMD_QueryServerInfosCB& packet) override;

	void signup(SessionID requestorSessionID, Session* pSession, 
		const std::string& commitAccountName, const std::string& password, const std::string& datas);

	void signin(SessionID requestorSessionID, Session* pSession, 
		const std::string& commitAccountName, const std::string& password, const std::string& datas);

	void onSessionSignup(SessionID requestorSessionID, Session* pSession, const CMD_Login_Signup& packet);
	void onSessionSignin(SessionID requestorSessionID, Session* pSession, const CMD_Login_Signin& packet);

	void onSessionSignupCB(Session* pSession, const CMD_Login_OnSignupCB& packet);
	void onSessionSigninCB(Session* pSession, const CMD_Login_OnSigninCB& packet);

	void onSessionRemoteDisconnected(SessionID requestorSessionID, Session* pSession, const CMD_RemoteDisconnected& packet);

	PendingAccount* findPendingAccount(const std::string accountName);
	bool addPendingAccount(const PendingAccount& pendingAccount);
	bool delPendingAccount(const std::string accountName);

	void sendFailed(SessionID sessionID, SessionID requestorSessionID, const std::string& commitAccountName, ServerError err, bool isSignup);

	void onSessionRequestAllocClientCB(Session* pSession, const CMD_Login_OnRequestAllocClientCB& packet);

protected:
	std::tr1::unordered_map<std::string, PendingAccount> pendingAccounts_;
	ConnectorFinder* pConnectorFinder_;

};

}

#endif // X_SERVER_APP_H
