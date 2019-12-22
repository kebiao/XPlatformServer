#include "ConnectorFinder.h"
#include "ServerMgr.h"
#include "XServerBase.h"
#include "log/XLog.h"
#include "event/EventDispatcher.h"
#include "event/NetworkInterface.h"
#include "event/Timer.h"
#include "event/Session.h"
#include "event/TcpSocket.h"
#include "resmgr/ResMgr.h"
#include "protos/Commands.pb.h"

namespace XServer {

#define FINDER_TICK_TIME TIME_SECONDS

//-------------------------------------------------------------------------------------
ConnectorFinder::ConnectorFinder(XServerBase* pServer):
pXServer_(pServer),
timerEvent_(NULL),
connectorInfos_(),
found_(NULL),
attempt_(0)
{
	timerEvent_ = pXServer_->pTimer()->addTimer(FINDER_TICK_TIME, -1, std::bind(&ConnectorFinder::onFindingTick, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------
ConnectorFinder::~ConnectorFinder()
{
	if (timerEvent_)
	{
		pXServer_->pTimer()->delTimer(timerEvent_);
		timerEvent_ = NULL;
	}

	if (!found_)
	{
		ERROR_MSG(fmt::format("ConnectorFinder::~ConnectorFinder(): timeout! not found connector!\n"));

		if (pXServer_)
			pXServer_->shutDown(FINDER_TICK_TIME * 2/*�������timerEvent_*/);
	}
	else
	{
		ERROR_MSG(fmt::format("ConnectorFinder::~ConnectorFinder()\n"));
	}
}

//-------------------------------------------------------------------------------------	
void ConnectorFinder::onFindingTick(void* userargs)
{
	if (attempt_ == 0)
	{
		DEBUG_MSG(fmt::format("ConnectorFinder::onFindingTick: find connectors...\n"));
	}
	else if(attempt_ >= 100)
	{
		WARNING_MSG(fmt::format("ConnectorFinder::onFindingTick: find connectors..., attempt={}\n", attempt_));
	}

	++attempt_;

	if (!pXServer_ || pXServer_->isShutingdown())
	{
		return;
	}

	ServerMgr* pServerMgr = pXServer_->pServerMgr();
	if (!pServerMgr)
	{
		return;
	}

	pServerMgr->addInterestedServerType(ServerType::SERVER_TYPE_DIRECTORY);

	std::vector<ServerInfo*> connectorInfos = pServerMgr->findServer(ServerType::SERVER_TYPE_CONNECTOR);

	if (connectorInfos_.size() > 0 && connectorInfos_.size() == connectorInfos.size())
	{
		connectorInfos_ = connectorInfos;

		pXServer_->pTimer()->delTimer(timerEvent_);
		timerEvent_ = pXServer_->pTimer()->addTimer(FINDER_TICK_TIME, -1, std::bind(&ConnectorFinder::onTestingTick, this, std::placeholders::_1), NULL);
		attempt_ = 0;

		DEBUG_MSG(fmt::format("ConnectorFinder::onFindingTick: found connectors, start testing rtt! connsize={}\n", connectorInfos_.size()));
		return;
	}
	else
	{
		connectorInfos_ = connectorInfos;

		if (connectorInfos.size() == 0)
		{
			if(attempt_ > 60)
				WARNING_MSG(fmt::format("ConnectorFinder::onFindingTick: not found connectors, attempt={}\n", attempt_));
		}
	}

	ServerInfo* pServerInfo = pServerMgr->findDirectory();
	if (!pServerInfo || !pServerInfo->pSession || !pServerInfo->pSession->connected())
		return;

	// ���ٲ�ѯ2�Σ� �����һ�κͱ��β�ѯ��õ�connector����һ�����������connector�Ѿ�������ϣ���ʱ���Կ�ʼ��������ѡ����
	CMD_QueryServerInfos req_packet;
	// ����Ϊ0��ʾ�����Ʋ�ѯ
	req_packet.set_appid(0);
	req_packet.set_appgid(pXServer_->gid());
	req_packet.set_apptype((int32)ServerType::SERVER_TYPE_CONNECTOR);
	req_packet.set_maxnum(0);
	pServerInfo->pSession->sendPacket(CMD::QueryServerInfos, req_packet);
}

//-------------------------------------------------------------------------------------	
void ConnectorFinder::onTestingTick(void* userargs)
{
	if (++attempt_ >= 100)
	{
		WARNING_MSG(fmt::format("ConnectorFinder::onTestingTick: testing connectors, attempt={}...\n", attempt_));
	}

	if (!pXServer_ || pXServer_->isShutingdown())
	{
		return;
	}

	ServerMgr* pServerMgr = pXServer_->pServerMgr();
	if (!pServerMgr)
	{
		return;
	}

	bool alltested = true;

	for (auto& item : connectorInfos_)
	{
		if (!item->pSession)
		{
			item->pSession = pServerMgr->connectServer(item->internal_ip, item->internal_port, item->type);
			if (item->pSession)
				item->pSession->appID(item->id);

			alltested = false;
		}
		else
		{
			if (item->pSession->connected())
			{
				if (item->pSession->getRoundTripTime() < 0)
					alltested = false;

				item->pSession->ping();
			}
		}
	}

	// ���û�еõ����е�pingֵ����ô�����ȴ���һ�β���
	if (!alltested)
		return;

	// �õ�pingֵ֮�� �����С�ģ������ۺ��ж���������������ѡ��󶨵�����
	found_ = calcBestServer();
	if (!found_)
		return;

	for (auto& item : connectorInfos_)
	{
		DEBUG_MSG(fmt::format("   ==> appID={}, addr={}:{}, rtt={}, playerNum={}, sessionNum={}, childSrvs={}\n",
			item->id, item->internal_ip, item->internal_port, item->pSession->getRoundTripTime(), item->playerNum, item->sessionNum, item->child_srvs.size()));
	}

	DEBUG_MSG(fmt::format("ConnectorFinder::onTestTick(): bind to connector(appID={}, addr={}:{})!\n",
		found_->id, found_->internal_ip, found_->internal_port));

	bool ifConnectAll = false;

	if(!ifConnectAll)
	{
		pServerMgr->addServer(*found_);
		pServerMgr->addInterestedServerID(found_->id);
	}
	else
	{
		for (auto& item : connectorInfos_)
		{
			pServerMgr->addServer(*item);
			pServerMgr->addInterestedServerID(item->id);
		}
	}

	pServerMgr->removeInterestedServerType(ServerType::SERVER_TYPE_DIRECTORY);
	
	pXServer_->pTimer()->delTimer(timerEvent_);
	timerEvent_ = pXServer_->pTimer()->addTimer(FINDER_TICK_TIME, -1, std::bind(&ConnectorFinder::onCheckingTick, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------	
ServerInfo* ConnectorFinder::calcBestServer()
{
	if (connectorInfos_.size() == 0)
		return NULL;

	ServerInfo* pServerInfo = NULL;
	
	// �Ƚ���С�Ĳ��ҳ���
	for (auto& item : connectorInfos_)
	{
		if (!pServerInfo || item->pSession->getRoundTripTime() < pServerInfo->pSession->getRoundTripTime())
		{
			pServerInfo = item;
		}
	}

	std::vector<ServerInfo*> groups_;

	// Ȼ����rtt��Χֵ���Ƶ��γ�һ��
	for (auto& item : connectorInfos_)
	{
		if (item->pSession->getRoundTripTime() - pServerInfo->pSession->getRoundTripTime() <= ResMgr::getSingleton().serverConfig().tickInterval)
		{
			groups_.push_back(item);
		}
	}

	// �ٴ�һ���а���sessionNum���϶�һ����С��
	for (auto& item : connectorInfos_)
	{
		if (item->sessionNum < pServerInfo->sessionNum)
		{
			pServerInfo = item;
		}
	}

	assert(pServerInfo);
	return pServerInfo;
}

//-------------------------------------------------------------------------------------	
void ConnectorFinder::onCheckingTick(void* userargs)
{
	if (!pXServer_ || pXServer_->isShutingdown())
	{
		return;
	}

	ServerMgr* pServerMgr = pXServer_->pServerMgr();
	if (!pServerMgr)
	{
		return;
	}

	if (!pServerMgr->findConnector())
	{
		ERROR_MSG(fmt::format("ConnectorFinder::onTestTick(): lose connector, start finding...\n"));
		attempt_ = 0;
		found_ = NULL;
		pXServer_->pTimer()->delTimer(timerEvent_);
		timerEvent_ = pXServer_->pTimer()->addTimer(FINDER_TICK_TIME, -1, std::bind(&ConnectorFinder::onFindingTick, this, std::placeholders::_1), NULL);
	}
}

//-------------------------------------------------------------------------------------
}
