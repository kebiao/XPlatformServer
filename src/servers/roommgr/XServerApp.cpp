#include "XServerApp.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
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
ServerInfo *XServerApp::findBestMachines()
{
	std::vector<ServerInfo*> founds = ((XServerApp&)XServerApp::getSingleton()).pServerMgr()->findServer(ServerType::SERVER_TYPE_MACHINE);
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
void XServerApp::onSessionRequestCreateRoom(Session* pSession, const CMD_Roommgr_RequestCreateRoom& packet)
{
	DEBUG_MSG(fmt::format("XServerApp::onSessionRequestCreateRoom(): hallsID={}, gameID={}, mode={}, roomID={}\n", 
		packet.hallsid(), packet.gameid(), (int)packet.gamemode(), packet.roomid()));

	ServerInfo *pServerInfo = findBestMachines();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestCreateRoom(): create room is failed! not found machine!\n"));

		CMD_Halls_OnRequestCreateRoomCB res_packet;
		res_packet.set_errcode(ServerError::SERVER_NOT_READY);
		pSession->sendPacket(CMD::Halls_OnRequestCreateRoomCB, res_packet);
		return;
	}

	pServerInfo->load += 1;

	CMD_Machine_RequestCreateRoom req_packet;
	req_packet.set_exefile(packet.exefile());
	req_packet.set_exeoptions(packet.exeoptions());
	req_packet.set_gameid(packet.gameid());
	req_packet.set_gamemode(packet.gamemode());
	req_packet.set_gametime(packet.gametime());
	req_packet.set_hallsid(packet.hallsid());
	req_packet.set_maxplayernum(packet.maxplayernum());
	req_packet.set_roomid(packet.roomid());
	req_packet.set_roommgrip(pInternalNetworkInterface()->getListenerIP());
	req_packet.set_roommgrport(pInternalNetworkInterface()->getListenerPort());

	std::random_device r_device;
	std::mt19937 mt(r_device());
	req_packet.set_tokenid(mt());

	for (int i = 0; i < packet.players_size(); ++i)
	{
		CMD_RoomPlayerInfo* pCMD_RoomPlayerInfo = req_packet.add_players();
		pCMD_RoomPlayerInfo->CopyFrom(packet.players(i));
	}

	pServerInfo->pSession->sendPacket(CMD::Machine_RequestCreateRoom, req_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestCreateRoomCB(Session* pSession, const CMD_Roommgr_OnRequestCreateRoomCB& packet)
{
	ServerInfo* pServerInfo = pServerMgr()->findServer(packet.hallsid());

	if (!pServerInfo)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestCreateRoom(): create room is failed! not found halls({})!\n", packet.hallsid()));

		CMD_Halls_OnRequestCreateRoomCB res_packet;
		res_packet.set_errcode(ServerError::SERVER_NOT_READY);
		pSession->sendPacket(CMD::Halls_OnRequestCreateRoomCB, res_packet);
		return;
	}

	CMD_Halls_OnRequestCreateRoomCB res_packet;
	res_packet.set_roomid(packet.roomid());
	res_packet.set_errcode(packet.errcode());
	res_packet.set_ip(packet.ip());
	res_packet.set_port(packet.port());
	res_packet.set_tokenid(packet.tokenid());
	pServerInfo->pSession->sendPacket(CMD::Halls_OnRequestCreateRoomCB, res_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRoomSrvGameOverReport(Session* pSession, const CMD_Roommgr_OnRoomSrvGameOverReport& packet)
{
	ServerInfo* pMachineServerInfo = pServerMgr()->findServer(pSession);
	pMachineServerInfo->load -= 1;

	ServerInfo* pServerInfo = pServerMgr()->findServer(packet.hallsid());

	if (!pServerInfo)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRoomSrvGameOverReport(): create room is failed! not found halls({})!\n", packet.hallsid()));
		return;
	}

	CMD_Halls_OnRoomSrvGameOverReport res_packet;
	res_packet.set_roomid(packet.roomid());
	res_packet.set_errcode(packet.errcode());
	
	for (int i = 0; i < packet.playerdatas_size(); ++i)
	{
		CMD_Halls_PlayerGameData* pCMD_Halls_PlayerGameData = res_packet.add_playerdatas();
		pCMD_Halls_PlayerGameData->CopyFrom(packet.playerdatas(i));
	}

	pServerInfo->pSession->sendPacket(CMD::Halls_OnRoomSrvGameOverReport, res_packet);
}

//-------------------------------------------------------------------------------------
}
