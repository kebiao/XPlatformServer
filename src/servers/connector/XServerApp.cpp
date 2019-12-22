#include "XServerApp.h"
#include "XSession.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "protos/Commands.pb.h"
#include "event/Session.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XServerApp::XServerApp():
mapBackendSessionNums_()
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
	pServerMgr_->addInterestedServerType(ServerType::SERVER_TYPE_DIRECTORY);
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
void XServerApp::onServerJoined(ServerInfo* pServerInfo)
{
	XServerBase::onServerJoined(pServerInfo);

	if (!pServerMgr_)
		return;

	// �ҵ�Ŀ¼��
	ServerInfo* pDirServerInfo = pServerMgr_->findDirectory();
	if (!pDirServerInfo || !pDirServerInfo->pSession || !pDirServerInfo->pSession->connected() 
		|| !pServerInfo || !pServerInfo->pSession || !pServerInfo->pSession->connected())
		return;

	// ֻ���з��������뵽�ý��̣��ͽ����µ�״̬���¹�ȥ
	updateServerInfoToSession(pDirServerInfo->pSession);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionConnected(Session* pSession)
{
	XServerBase::onSessionConnected(pSession);
}

//-------------------------------------------------------------------------------------
void XServerApp::onServerExit(ServerInfo* pServerInfo)
{
	XServerBase::onServerExit(pServerInfo);

	if (this->pInternalNetworkInterface())
	{
		NetworkInterface::SessionMap& sessions = pInternalNetworkInterface()->sessions();
		for (auto& item : sessions)
		{
			if (((XSession*)item.second)->pBackendSession() == pServerInfo->pSession)
			{
				((XSession*)item.second)->onLoseBackendSession();
			}
		}
	}

	if (this->pExternalNetworkInterface())
	{
		NetworkInterface::SessionMap& sessions = pExternalNetworkInterface()->sessions();
		for (auto& item : sessions)
		{
			if (((XSession*)item.second)->pBackendSession() == pServerInfo->pSession)
			{
				((XSession*)item.second)->onLoseBackendSession();
			}
		}
	}

	if (!pServerMgr_)
		return;

	// �ҵ�Ŀ¼��
	ServerInfo* pDirServerInfo = pServerMgr_->findDirectory();
	if (!pServerInfo || !pServerInfo->pSession || !pServerInfo->pSession->connected())
		return;

	// ֻ���з��������뵽�ý��̣��ͽ����µ�״̬���¹�ȥ
	updateServerInfoToSession(pServerInfo->pSession);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionBindBackendSession(ServerType type, Session* pSession, uint64 hallsID)
{
	// ���Ự�󶨵����App�Ự
	if (!pServerMgr_)
		return;

	ServerInfo* bestInfo = NULL;

	if (hallsID > 0)
	{
		bestInfo = pServerMgr_->findServer(hallsID);

		if (!bestInfo || bestInfo->type != ServerType::SERVER_TYPE_HALLS)
			bestInfo = NULL;
	}
	else
	{
		std::vector<ServerInfo*> infos = pServerMgr_->findServer(type);

		std::vector<ServerInfo*> founds;

		// �ɲ���һ���Ķ�̬�����㷨�󶨵����Ľ�С��app��
		for (auto& item : infos)
		{
			auto iter = mapBackendSessionNums_.find(item->id);
			if (iter == mapBackendSessionNums_.end())
			{
				mapBackendSessionNums_[item->id] = 0;
			}

			if (!item->pSession || !item->pSession->connected())
				continue;

			founds.push_back(item);
		}

		for (auto& item : founds)
		{
			if (!bestInfo || mapBackendSessionNums_[item->id] < mapBackendSessionNums_[bestInfo->id])
			{
				bestInfo = item;
			}
		}
	}

	if (!bestInfo)
		return;

	mapBackendSessionNums_[bestInfo->id] += 1;

	((XSession*)pSession)->pBackendSession(bestInfo->pSession);
}

//-------------------------------------------------------------------------------------
}
