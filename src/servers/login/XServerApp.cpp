#include "XServerApp.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "server/ConnectorFinder.h"
#include "server/XServerBase.h"
#include "protos/Commands.pb.h"
#include "event/Session.h"
#include "log/XLog.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XServerApp::XServerApp():
	pendingAccounts_(),
	pConnectorFinder_(NULL)
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
	pServerMgr_->addInterestedServerType(ServerType::SERVER_TYPE_DBMGR);
	pServerMgr_->addInterestedServerType(ServerType::SERVER_TYPE_HALLSMGR);
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
	pConnectorFinder_ = new ConnectorFinder(this);
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::finalise(void)
{
	SAFE_RELEASE(pConnectorFinder_);
	XServerBase::finalise();
}

//-------------------------------------------------------------------------------------
void XServerApp::onServerJoined(ServerInfo* pServerInfo)
{
	XServerBase::onServerJoined(pServerInfo);

	if (!pServerMgr_)
		return;
}

//-------------------------------------------------------------------------------------
void XServerApp::onServerExit(ServerInfo* pServerInfo)
{
	XServerBase::onServerExit(pServerInfo);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRemoteDisconnected(SessionID requestorSessionID, Session* pSession, const CMD_RemoteDisconnected& packet)
{

}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionQueryServerInfosCB(Session* pSession, const CMD_QueryServerInfosCB& packet)
{
	XServerBase::onSessionQueryServerInfosCB(pSession, packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onHeartbeatTick(void* userargs)
{
	XServerBase::onHeartbeatTick(userargs);

	while (true)
	{
		auto i = pendingAccounts_.size();
		for (auto& item : pendingAccounts_)
		{
			if (item.second.isTimeout())
			{
				WARNING_MSG(fmt::format("XServerApp::onHeartbeatTick(): pendingAccount: {} timeout!\n",
					item.second.commitAccountName));

				delPendingAccount(item.second.commitAccountName);
				break;
			}
		}

		if (i == pendingAccounts_.size())
			break;
	}
}

//-------------------------------------------------------------------------------------
XServerApp::PendingAccount* XServerApp::findPendingAccount(const std::string accountName)
{
	auto iter = pendingAccounts_.find(accountName);
	if (iter == pendingAccounts_.end())
		return NULL;

	return &iter->second;
}

//-------------------------------------------------------------------------------------
bool XServerApp::addPendingAccount(const PendingAccount& pendingAccount)
{
	PendingAccount* pPendingAccount = findPendingAccount(pendingAccount.commitAccountName);
	if (pPendingAccount)
	{
		ERROR_MSG(fmt::format("XServerApp::addPendingAccount(): {} already exist!\n", 
			pendingAccount.commitAccountName));

		return false;
	}

	pendingAccounts_[pendingAccount.commitAccountName] = pendingAccount;
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::delPendingAccount(const std::string accountName)
{
	PendingAccount* pPendingAccount = findPendingAccount(accountName);
	if (!pPendingAccount)
	{
		ERROR_MSG(fmt::format("XServerApp::addPendingAccount(): not found {}!\n",
			accountName));

		return false;
	}

	pendingAccounts_.erase(accountName);
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::sendFailed(SessionID sessionID, SessionID requestorSessionID, const std::string& commitAccountName, ServerError err, bool isSignup)
{
	Session* pSession = this->pInternalNetworkInterface()->findSession(sessionID);
	if (!pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::sendFailed(): not found session({})!\n", sessionID));
		return;
	}

	if (isSignup)
	{
		CMD_Client_OnSignupCB res_packet;
		res_packet.set_errcode(err);
		pSession->forwardPacket(requestorSessionID, CMD::Client_OnSignupCB, res_packet);
	}
	else
	{
		CMD_Client_OnSigninCB res_packet;
		res_packet.set_errcode(err);
		pSession->forwardPacket(requestorSessionID, CMD::Client_OnSigninCB, res_packet);
	}

	if(findPendingAccount(commitAccountName))
		delPendingAccount(commitAccountName);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionSignup(SessionID requestorSessionID, Session* pSession, const CMD_Login_Signup& packet)
{
	signup(requestorSessionID, pSession, packet.commitaccountname(), packet.password(), packet.datas());
}

//-------------------------------------------------------------------------------------
void XServerApp::signup(SessionID requestorSessionID, Session* pSession, 
	const std::string& commitAccountName, const std::string& password, const std::string& datas)
{
	if (!pServerMgr_)
	{
		sendFailed(pSession->id(), requestorSessionID, commitAccountName, ServerError::SERVER_NOT_READY, true);
		return;
	}

	ServerInfo* pServerInfo = pServerMgr_->findDbmgr();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::signup(): {} signup failed! not found dbmgr!\n", commitAccountName));

		sendFailed(pSession->id(), requestorSessionID, commitAccountName, ServerError::SERVER_NOT_READY, true);
		return;
	}

	PendingAccount pendingAccount;
	pendingAccount.commitAccountName = commitAccountName;
	pendingAccount.password = password;
	pendingAccount.commitDatas = datas;
	pendingAccount.currentSessionID = pSession->id();
	pendingAccount.requestorSessionID = requestorSessionID;

	if (!addPendingAccount(pendingAccount))
	{
		sendFailed(pSession->id(), requestorSessionID, commitAccountName, ServerError::FREQUENT_OPERATION, true);
		return;
	}

	CMD_Dbmgr_WriteAccount req_packet;
	req_packet.set_appid(id());
	req_packet.set_commitaccountname(commitAccountName);
	req_packet.set_password(password);
	req_packet.set_datas(datas);
	pServerInfo->pSession->sendPacket(CMD::Dbmgr_WriteAccount, req_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionSignupCB(Session* pSession, const CMD_Login_OnSignupCB& packet)
{
	PendingAccount* pendingAccount = findPendingAccount(packet.commitaccountname());

	if (!pendingAccount)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionSignupCB(): not found pendingAccount({})!\n", packet.commitaccountname()));
		return;
	}

	pendingAccount->updateTime();
	pendingAccount->backDatas = packet.datas();
	
	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionSignupCB(): signup error({})!, accountName={}\n", ServerError_Name(packet.errcode()), pendingAccount->commitAccountName));
	}

	Session* pConnectorSession = this->pInternalNetworkInterface()->findSession(pendingAccount->currentSessionID);
	if (!pConnectorSession)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionSignupCB(): not found connectorSession({})!\n", pendingAccount->currentSessionID));

		delPendingAccount(packet.commitaccountname());
		return;
	}

	CMD_Client_OnSignupCB res_packet;
	res_packet.set_datas(pendingAccount->backDatas);
	res_packet.set_errcode(packet.errcode());
	pConnectorSession->forwardPacket(pendingAccount->requestorSessionID, CMD::Client_OnSignupCB, res_packet);

	delPendingAccount(packet.commitaccountname());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionSignin(SessionID requestorSessionID, Session* pSession, const CMD_Login_Signin& packet)
{
	signin(requestorSessionID, pSession, packet.commitaccountname(), packet.password(), packet.datas());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionSigninCB(Session* pSession, const CMD_Login_OnSigninCB& packet)
{
	PendingAccount* pendingAccount = findPendingAccount(packet.commitaccountname());

	if (!pendingAccount)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionSigninCB(): not found pendingAccount({})!\n", packet.commitaccountname()));
		return;
	}

	if (packet.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionSigninCB(): error({})!, accountName={}\n", ServerError_Name(packet.errcode()), pendingAccount->commitAccountName));

		Session* pConnectorSession = pInternalNetworkInterface()->findSession(pendingAccount->currentSessionID);

		if (!pConnectorSession || !pConnectorSession->connected())
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionSigninCB(): ot found connectorSession({})!\n",
				pendingAccount->currentSessionID));

			delPendingAccount(packet.commitaccountname());
			return;
		}

		sendFailed(pConnectorSession->id(), pendingAccount->requestorSessionID, packet.commitaccountname(), packet.errcode(), false);
		return;
	}

	pendingAccount->updateTime();
	pendingAccount->backDatas = packet.datas();

	// 去请求hallsmgr分配一个合适的halls
	ServerInfo* pServerInfo = pServerMgr_->findHallsmgr();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionSigninCB(): {} signin failed! not found hallsmgr!\n", packet.commitaccountname()));
		return;
	}

	CMD_Hallsmgr_RequestAllocClient req_packet;
	req_packet.set_commitaccountname(packet.commitaccountname());
	req_packet.set_password(pendingAccount->password);
	req_packet.set_datas(pendingAccount->backDatas);
	req_packet.set_foundappid(packet.foundappid());
	req_packet.set_foundobjectid(packet.foundobjectid());
	res_packet.set_hallsid(packet.hallsid());
	pServerInfo->pSession->sendPacket(CMD::Hallsmgr_RequestAllocClient, req_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::signin(SessionID requestorSessionID, Session* pSession, 
	const std::string& commitAccountName, const std::string& password, const std::string& datas)
{
	if (!pServerMgr_)
	{
		sendFailed(pSession->id(), requestorSessionID, commitAccountName, ServerError::SERVER_NOT_READY, false);
		return;
	}

	ServerInfo* pServerInfo = pServerMgr_->findDbmgr();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::signin(): {} login failed! not found dbmgr!\n", commitAccountName));

		sendFailed(pSession->id(), requestorSessionID, commitAccountName, ServerError::SERVER_NOT_READY, false);
		return;
	}

	PendingAccount pendingAccount;
	pendingAccount.commitAccountName = commitAccountName;
	pendingAccount.password = password;
	pendingAccount.commitDatas = datas;
	pendingAccount.currentSessionID = pSession->id();
	pendingAccount.requestorSessionID = requestorSessionID;

	if (!addPendingAccount(pendingAccount))
	{
		sendFailed(pSession->id(), requestorSessionID, commitAccountName, ServerError::FREQUENT_OPERATION, false);
		return;
	}

	CMD_Dbmgr_QueryAccount req_packet;
	req_packet.set_commitaccountname(commitAccountName);
	req_packet.set_password(password);
	req_packet.set_datas(datas);
	req_packet.set_queryappid(id());
	req_packet.set_querytype(1);
	req_packet.set_accountid(0);
	pServerInfo->pSession->sendPacket(CMD::Dbmgr_QueryAccount, req_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestAllocClientCB(Session* pSession, const CMD_Login_OnRequestAllocClientCB& packet)
{
	PendingAccount* pendingAccount = findPendingAccount(packet.commitaccountname());

	if (!pendingAccount)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClientCB(): not found pendingAccount({})!\n", packet.commitaccountname()));
		delPendingAccount(packet.commitaccountname());

		return;
	}

	Session* pConnectorSession = pInternalNetworkInterface()->findSession(pendingAccount->currentSessionID);

	if (!pConnectorSession || !pConnectorSession->connected())
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClientCB(): {} alloc failed! not found connectorSession({})!\n",
			packet.commitaccountname(), pendingAccount->currentSessionID));

		delPendingAccount(packet.commitaccountname());
		return;
	}

	if (packet.errcode() == ServerError::OK)
	{
		DEBUG_MSG(fmt::format("XServerApp::onSessionRequestAllocClientCB(): {} alloc to {}:{}!\n",
			packet.commitaccountname(), packet.ip(), packet.port()));
	}
	else
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClientCB(): {} alloc failed! error({})!\n",
			packet.commitaccountname(), ServerError_Name(packet.errcode())));

		delPendingAccount(packet.commitaccountname());
	}

	CMD_Client_OnSigninCB res_packet;
	res_packet.set_addr(packet.ip());
	res_packet.set_port(packet.port());
	res_packet.set_errcode(packet.errcode());
	res_packet.set_datas(pendingAccount->backDatas);
	res_packet.set_tokenid(packet.tokenid());
	pConnectorSession->forwardPacket(pendingAccount->requestorSessionID, CMD::Client_OnSigninCB, res_packet);
	delPendingAccount(packet.commitaccountname());
}

//-------------------------------------------------------------------------------------
}
