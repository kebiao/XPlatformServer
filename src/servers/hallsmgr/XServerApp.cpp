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
XServerApp::XServerApp()
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
ServerInfo* XServerApp::findBestServer()
{
	// 根据客户端连接的connector的ip搜寻同一局域网下的挂载有halls的connector的ip（因为这个ip是经过客户端筛选出响应最快的地址）
	// 如果找不到可用的地址，则随机选一个

	std::vector<ServerInfo*> founds = ((XServerApp&)XServerApp::getSingleton()).pServerMgr()->findServer(ServerType::SERVER_TYPE_HALLS);
	if (founds.size() == 0)
	{
		return NULL;
	}

	ServerInfo *pServerInfo = NULL;

	for (auto& item : founds)
	{
		if (!item->pSession || !item->pSession->connected())
			continue;

		if (!pServerInfo || item->load < pServerInfo->load)
			pServerInfo = item;
	}

	return pServerInfo;
}

//-------------------------------------------------------------------------------------
void XServerApp::sendFailed(Session* pSession, const std::string& commitAccountname, CMD cmd, ServerError err)
{
	if (cmd == CMD::Hallsmgr_RequestAllocClient)
	{
		CMD_Login_OnRequestAllocClientCB res_packet;
		res_packet.set_commitaccountname(commitAccountname);
		res_packet.set_errcode(err);
		pSession->sendPacket(CMD::Login_OnRequestAllocClientCB, res_packet);
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestAllocClient(Session* pSession, const CMD_Hallsmgr_RequestAllocClient& packet)
{
	if (!pServerMgr_)
	{
		sendFailed(pSession, packet.commitaccountname(), CMD::Hallsmgr_RequestAllocClient, ServerError::SERVER_NOT_READY);
		return;
	}

	ServerAppID foundAppID = packet.foundappid();

	ServerInfo* pServerInfo = NULL;
	
	if(foundAppID != SERVER_APP_ID_INVALID)
		pServerInfo = pServerMgr_->findServerOne(ServerType::SERVER_TYPE_HALLS, foundAppID);
	else
		pServerInfo = findBestServer();

	if (!pServerInfo || !pServerInfo->pSession || !pServerInfo->pSession->connected())
	{
		sendFailed(pSession, packet.commitaccountname(), CMD::Hallsmgr_RequestAllocClient, ServerError::SERVER_NOT_READY);
		return;
	}

	pServerInfo->load += 1;
	pServerInfo->playerNum += 1;

	CMD_Halls_RequestAllocClient req_packet;
	req_packet.set_commitaccountname(packet.commitaccountname());
	req_packet.set_password(packet.password());
	req_packet.set_datas(packet.datas());
	req_packet.set_foundobjectid(packet.foundobjectid());
	req_packet.set_loginsessionid(pSession->id());
	pServerInfo->pSession->sendPacket(CMD::Halls_RequestAllocClient, req_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestAllocClientCB(Session* pSession, const CMD_Hallsmgr_OnRequestAllocClientCB& packet)
{
	Session* pLoginSession = pInternalNetworkInterface()->findSession(packet.loginsessionid());

	if (!pLoginSession || !pLoginSession->connected())
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClientCB(): {} alloc failed! not found loginSession({})!\n", 
			packet.commitaccountname(), packet.loginsessionid()));

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
	}

	CMD_Login_OnRequestAllocClientCB res_packet;
	res_packet.set_ip(packet.ip());
	res_packet.set_port(packet.port());
	res_packet.set_errcode(packet.errcode());
	res_packet.set_commitaccountname(packet.commitaccountname());
	res_packet.set_tokenid(packet.tokenid());
	res_packet.set_hallsid(packet.hallsid());
	pLoginSession->sendPacket(CMD::Login_OnRequestAllocClientCB, res_packet);
}

//-------------------------------------------------------------------------------------
}
