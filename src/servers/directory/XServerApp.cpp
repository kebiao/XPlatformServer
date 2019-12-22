#include "XServerApp.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "event/Session.h"

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
void XServerApp::onSessionListServers(Session* pSession, const CMD_Directory_ListServers& packet)
{
	CMD_Client_OnListServersCB res_packet;

	if (!pServerMgr())
	{
		res_packet.set_errcode(ServerError::NOT_FOUND);
		pSession->sendPacket(CMD::Client_OnListServersCB, res_packet);
		return;
	}

	std::vector<ServerInfo*> foundConnectorInfos;

	std::vector<ServerInfo*> connectorInfos = pServerMgr()->findServer(ServerType::SERVER_TYPE_CONNECTOR);
	for (auto& item : connectorInfos)
	{
		if (item->child_srvs.size() == 0 || !item->pSession || !item->pSession->connected())
			continue;


		for (auto& child_item : item->child_srvs)
		{
			ServerInfo& serverInfo = child_item.second;
			if (serverInfo.type != ServerType::SERVER_TYPE_LOGIN)
				continue;

			foundConnectorInfos.push_back(item);
			break;
		}
	}

	// 此处可以通过计算, 同一网段的地址，将连接数动态均衡的分配给connector
	for (auto& item : foundConnectorInfos)
	{
		CMD_ListServersInfo* pCMD_ListServersInfo = res_packet.add_srvs();
		pCMD_ListServersInfo->set_addr(item->external_ip);
		pCMD_ListServersInfo->set_port(item->external_port);
		pCMD_ListServersInfo->set_groupid(item->gid);
		pCMD_ListServersInfo->set_id(id());
		pCMD_ListServersInfo->set_name(item->name);
	}

	if (res_packet.srvs_size() == 0)
	{
		res_packet.set_errcode(ServerError::NOT_FOUND);
	}

	pSession->sendPacket(CMD::Client_OnListServersCB, res_packet);
}

//-------------------------------------------------------------------------------------
}
