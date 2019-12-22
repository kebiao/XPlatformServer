#include "XServerApp.h"
#include "XObject.h"
#include "XPlayer.h"
#include "XSession.h"
#include "Room.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "server/ConnectorFinder.h"
#include "server/XServerBase.h"
#include "protos/Commands.pb.h"
#include "event/Session.h"
#include "log/XLog.h"
#include "resmgr/ResMgr.h"

#if X_PLATFORM == PLATFORM_WIN32
#pragma warning(disable : 4503)
#endif

namespace XServer {

//-------------------------------------------------------------------------------------
XServerApp::XServerApp():
	pendingAccounts_(),
	pConnectorFinder_(NULL),
	objects_(),
	rooms_ggpr_(),
	rooms_(),
	pendingMatchs_(),
	session2PlayerMapping_()
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
	pConnectorFinder_ = new ConnectorFinder(this);
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::finalise(void)
{
	pendingAccounts_.clear();
	pendingMatchs_.clear();
	objects_.clear();
	rooms_ggpr_.clear();
	rooms_.clear();

	SAFE_RELEASE(pConnectorFinder_);
	XServerBase::finalise();
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

	debugRooms();
}

//-------------------------------------------------------------------------------------
void XServerApp::debugRooms()
{
	std::string s = fmt::format("Status({}): rooms={}, players={}, pendingMatchs={}, pendingAccounts={}\n",
		id(), rooms_.size(), objects_.size(), pendingMatchs_.size(), pendingAccounts_.size());

	DEBUG_MSG(s);

	printf("%s", s.c_str());

	for (auto& item : rooms_)
	{
		DEBUG_MSG(item.second->c_str() + "\n");
		//printf("%s\n", item.second->c_str().c_str());
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::onUpdateServerInfoToSession(CMD_UpdateServerInfos& infos)
{
	infos.set_playernum(objects_.size());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionDisconnected(Session* pSession)
{
	XServerBase::onSessionDisconnected(pSession);

	std::vector<SessionID> dels;

	// connector disconnect
	for (auto& item : session2PlayerMapping_)
	{
		if (((XPlayer*)item.second.get())->pSession() == pSession)
		{
			((XPlayer*)item.second.get())->onClientDisconnected(pSession);
			dels.push_back(item.first);
		}
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRemoteDisconnected(SessionID requestorSessionID, Session* pSession, const CMD_RemoteDisconnected& packet)
{
	auto iter = session2PlayerMapping_.find(requestorSessionID);

	if (iter == session2PlayerMapping_.end())
		return;
	
	((XPlayer*)iter->second.get())->onClientDisconnected(pSession);
	session2PlayerMapping_.erase(iter);
}

//-------------------------------------------------------------------------------------
bool XServerApp::addObject(XObjectPtr obj)
{
	XObjectPtr fo = findObject(obj->id());

	if (fo != NULL)
		return false;

	objects_[obj->id()] = obj;
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::onDelObject(XObjectPtr obj)
{
	PendingMatch* pPendingMatch = findPendingMatch(obj->id());
	if (pPendingMatch)
	{
		DEBUG_MSG(fmt::format("XServerApp::onDelObject(): remove PendingMatch({})!\n",
			obj->id()));

		delPendingMatch(obj->id());
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
		ERROR_MSG(fmt::format("XServerApp::delPendingAccount(): not found {}!\n",
			accountName));

		return false;
	}

	pendingAccounts_.erase(accountName);
	return true;
}

//-------------------------------------------------------------------------------------
XServerApp::Rooms& XServerApp::findRooms(GameID gameID, GameMode gameMode, int playerMaxNum)
{
	auto GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter = rooms_ggpr_.find(gameID);
	if (GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter == rooms_ggpr_.end())
	{
		rooms_ggpr_[gameID] = GameModeMapping_PlayerMaxNumMappingRoom();
		rooms_ggpr_[gameID][gameMode] = PlayerMaxNumMapping_Room();
		rooms_ggpr_[gameID][gameMode][playerMaxNum] = Rooms();
		return rooms_ggpr_[gameID][gameMode][playerMaxNum];
	}

	auto GameModeMapping_PlayerMaxNumMappingRoom_iter = GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter->second.find(gameMode);
	if (GameModeMapping_PlayerMaxNumMappingRoom_iter == GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter->second.end())
	{
		GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter->second[gameMode] = PlayerMaxNumMapping_Room();
		GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter->second[gameMode][playerMaxNum] = Rooms();
		return GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom_iter->second[gameMode][playerMaxNum];
	}

	auto PlayerMaxNumMapping_Room_iter = GameModeMapping_PlayerMaxNumMappingRoom_iter->second.find(playerMaxNum);
	if (PlayerMaxNumMapping_Room_iter == GameModeMapping_PlayerMaxNumMappingRoom_iter->second.end())
	{
		GameModeMapping_PlayerMaxNumMappingRoom_iter->second[playerMaxNum] = Rooms();
		return GameModeMapping_PlayerMaxNumMappingRoom_iter->second[playerMaxNum];
	}

	return PlayerMaxNumMapping_Room_iter->second;
}

//-------------------------------------------------------------------------------------
void XServerApp::destroyRoom(ObjectID roomID)
{
	RoomPtr pRoom = findRoom(roomID);

	if (pRoom == NULL)
	{
		ERROR_MSG(fmt::format("XServerApp::destroyRoom(): not found {}!\n", roomID));
		return;
	}

	rooms_.erase(roomID);

	bool found = false;

	Rooms& rooms = findRooms(pRoom->gameID(), pRoom->gameMode(), 0);
	for (Rooms::iterator iter = rooms.begin(); iter != rooms.end(); ++iter)
	{
		if ((*iter)->id() == roomID)
		{
			found = true;
			rooms.erase(iter);
			break;
		}
	}

	if (!found)
	{
		ERROR_MSG(fmt::format("XServerApp::destroyRoom(): not found {}! findRooms()\n", roomID));
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestAllocClient(Session* pSession, const CMD_Halls_RequestAllocClient& packet)
{
	if (!pServerMgr_)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClient(): {} alloc failed! not found serverMgr!\n", packet.commitaccountname()));
		return;
	}

	ServerInfo* pServerInfo = pServerMgr_->findHallsmgr();
	if (!pServerInfo || !pServerInfo->pSession || !pServerInfo->pSession->connected())
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClient(): {} alloc failed! not found hallsmgr!\n", packet.commitaccountname()));
		return;
	}

	ServerInfo* pConnectorServerInfo = pConnectorFinder_->found();
	if (!pConnectorServerInfo || !pConnectorServerInfo->pSession || !pConnectorServerInfo->pSession->connected())
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestAllocClient(): {} alloc failed! not found connector!\n", packet.commitaccountname()));

		CMD_Hallsmgr_OnRequestAllocClientCB res_packet;
		res_packet.set_loginsessionid(packet.loginsessionid());
		res_packet.set_commitaccountname(packet.commitaccountname());
		res_packet.set_errcode(ServerError::SERVER_NOT_READY);
		pServerInfo->pSession->sendPacket(CMD::Hallsmgr_OnRequestAllocClientCB, res_packet);
		return;
	}

	std::random_device r_device;
	std::mt19937 mt(r_device());

	PendingAccount pendingAccount;
	pendingAccount.commitAccountName = packet.commitaccountname();
	pendingAccount.password = packet.password();
	pendingAccount.commitDatas = packet.datas();
	pendingAccount.foundObjectID = packet.foundobjectid();
	pendingAccount.tokenID = mt();

	if (!addPendingAccount(pendingAccount))
	{
		CMD_Hallsmgr_OnRequestAllocClientCB res_packet;
		res_packet.set_loginsessionid(packet.loginsessionid());
		res_packet.set_commitaccountname(packet.commitaccountname());
		res_packet.set_errcode(ServerError::FREQUENT_OPERATION);
		pServerInfo->pSession->sendPacket(CMD::Hallsmgr_OnRequestAllocClientCB, res_packet);
		return;
	}

	CMD_Hallsmgr_OnRequestAllocClientCB res_packet;
	res_packet.set_ip(pConnectorFinder_->found()->external_ip);
	res_packet.set_port(pConnectorFinder_->found()->external_port);
	res_packet.set_errcode(ServerError::OK);
	res_packet.set_loginsessionid(packet.loginsessionid());
	res_packet.set_commitaccountname(packet.commitaccountname());
	res_packet.set_tokenid(pendingAccount.tokenID);
	res_packet.set_hallsid(id());
	pServerInfo->pSession->sendPacket(CMD::Hallsmgr_OnRequestAllocClientCB, res_packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionLogin(SessionID requestorSessionID, Session* pSession, const CMD_Halls_Login& packet)
{
	CMD_Client_OnLoginCB res_packet;

	XObjectPtr pObj;

	if (packet.playerid() == 0)
	{
		PendingAccount* pendingAccount = findPendingAccount(packet.commitaccountname());
		if (!pendingAccount)
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionLogin(): not found {}!\n",
				packet.commitaccountname()));

			res_packet.set_errcode(ServerError::NOT_FOUND);
			pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
			return;
		}

		res_packet.set_datas(pendingAccount->backDatas);

		if (pendingAccount->tokenID == 0 || pendingAccount->tokenID != packet.tokenid())
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionLogin(): tokenID error! srvTokenID={}, commitTokenID={}, accountName={}\n",
				pendingAccount->tokenID, packet.tokenid(), packet.commitaccountname()));

			res_packet.set_errcode(ServerError::INVALID_TOKEN);
			pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
			return;
		}

		pendingAccount->requestorSessionID = requestorSessionID;
		pendingAccount->currentSessionID = pSession->id();

		if (!pServerMgr_)
		{
			res_packet.set_errcode(ServerError::SERVER_NOT_READY);
			pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
			return;
		}

		ServerInfo* pServerInfo = pServerMgr_->findDbmgr();
		if (!pServerInfo || !pServerInfo->pSession)
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionLogin(): {} login failed! not found dbmgr!\n", packet.commitaccountname()));

			res_packet.set_errcode(ServerError::SERVER_NOT_READY);
			pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
			return;
		}

		CMD_Dbmgr_QueryAccount req_packet;
		req_packet.set_commitaccountname(packet.commitaccountname());
		req_packet.set_password("");
		req_packet.set_datas(pendingAccount->commitDatas);
		req_packet.set_queryappid(id());
		req_packet.set_querytype(1);
		req_packet.set_accountid(uuid());
		pServerInfo->pSession->sendPacket(CMD::Dbmgr_QueryAccount, req_packet);
		return;
	}
	else
	{
		pObj = this->findObject(packet.playerid());

		if (!pObj)
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionLogin(): not found player! commitTokenID={}, accountName={}\n",
				 packet.tokenid(), packet.commitaccountname()));

			res_packet.set_errcode(ServerError::OBJECT_NOT_FOUND);
			pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
			return;
		}

		if (((XPlayer*)pObj.get())->tokenID() != packet.tokenid())
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionLogin(): player tokenID error! srvTokenID={}, commitTokenID={}, accountName={}\n",
				((XPlayer*)pObj.get())->tokenID(), packet.tokenid(), packet.commitaccountname()));

			res_packet.set_errcode(ServerError::INVALID_TOKEN);
			pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
			return;
		}

		res_packet.set_datas("");
	}

	DEBUG_MSG(fmt::format("XServerApp::onSessionLogin(): success! accountName={}, token={}!\n", packet.commitaccountname(), packet.tokenid()));

	// current gameData
	res_packet.set_errcode(ServerError::OK);

	CMD_PlayerContext* pCMD_PlayerContext = res_packet.mutable_playercontext();
	((XPlayer*)pObj.get())->fillContextData(pCMD_PlayerContext);

	pSession->forwardPacket(requestorSessionID, CMD::Client_OnLoginCB, res_packet);

	session2PlayerMapping_[requestorSessionID] = pObj;
	addObject(pObj);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionQueryAccountCB(Session* pSession, const CMD_Halls_OnQueryAccountCB& packet)
{
	CMD_Client_OnLoginCB res_packet;

	PendingAccount* pendingAccount = findPendingAccount(packet.commitaccountname());
	if (!pendingAccount)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionQueryAccountCB(): not found {}!\n",
			packet.commitaccountname()));

		return;
	}

	Session* pConnectorSession = pInternalNetworkInterface()->findSession(pendingAccount->currentSessionID);
	if (!pConnectorSession)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionQueryAccountCB(): not found Connector! accountName={}, currentSessionID={}\n",
			packet.commitaccountname(), pendingAccount->currentSessionID));

		res_packet.set_errcode(ServerError::SERVER_NOT_READY);
		pConnectorSession->forwardPacket(pendingAccount->requestorSessionID, CMD::Client_OnLoginCB, res_packet);
		delPendingAccount(packet.commitaccountname());
		return;
	}

	// 登陆
	if (packet.querytype() == 1)
	{
		XObjectPtr pObj = XPlayerPtr(new XPlayer(packet.foundobjectid(), pendingAccount->requestorSessionID, pConnectorSession, pendingAccount->commitAccountName, pendingAccount->tokenID));

		DEBUG_MSG(fmt::format("XServerApp::onSessionQueryAccountCB(): success! accountName={}, token={}!\n", packet.commitaccountname(), pendingAccount->tokenID));

		XPlayer* pXPlayer = ((XPlayer*)pObj.get());

		if (ServerError::OBJECT_NOT_FOUND == (ServerError)packet.errcode())
		{
			ERROR_MSG(fmt::format("XServerApp::onSessionQueryAccountCB(): not found account! accountName={}, currentSessionID={}\n",
				packet.commitaccountname(), pendingAccount->currentSessionID));

			res_packet.set_errcode(packet.errcode());
			pConnectorSession->forwardPacket(pendingAccount->requestorSessionID, CMD::Client_OnLoginCB, res_packet);
			delPendingAccount(packet.commitaccountname());
		}
		else
		{
		//	pXPlayer->playerName(packet.playername());
		//	pXPlayer->modelID(packet.modelid());
			pXPlayer->exp(packet.exp());
			pXPlayer->gold(packet.gold());

			// current gameData
			res_packet.set_errcode(ServerError::OK);

			CMD_PlayerContext* pCMD_PlayerContext = res_packet.mutable_playercontext();
			((XPlayer*)pObj.get())->fillContextData(pCMD_PlayerContext);

			pConnectorSession->forwardPacket(pendingAccount->requestorSessionID, CMD::Client_OnLoginCB, res_packet);

			session2PlayerMapping_[pendingAccount->requestorSessionID] = pObj;
			addObject(pObj);
		}
	}

	delPendingAccount(packet.commitaccountname());
}

//-------------------------------------------------------------------------------------
XServerApp::PendingMatch* XServerApp::findPendingMatch(ObjectID oid)
{
	auto iter = pendingMatchs_.find(oid);
	if (iter == pendingMatchs_.end())
		return NULL;

	return &iter->second;
}

//-------------------------------------------------------------------------------------
bool XServerApp::addPendingMatch(const PendingMatch& pendingMatch)
{
	PendingMatch* pPendingMatch = findPendingMatch(pendingMatch.pObj->id());
	if (pPendingMatch)
	{
		ERROR_MSG(fmt::format("XServerApp::addPendingMatch(): {} already exist!\n",
			pendingMatch.pObj->id()));

		return false;
	}

	pendingMatchs_[pendingMatch.pObj->id()] = pendingMatch;
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::delPendingMatch(ObjectID oid)
{
	PendingMatch* pPendingMatch = findPendingMatch(oid);
	if (!pPendingMatch)
	{
		ERROR_MSG(fmt::format("XServerApp::delPendingMatch(): not found {}!\n",
			oid));

		return false;
	}

	pendingMatchs_.erase(oid);
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionStartMatch(SessionID requestorSessionID, Session* pSession, const CMD_Halls_StartMatch& packet)
{
	XPlayer* pObj = ((XPlayer*)((XSession*)pSession)->pPlayer.get());

	if (pObj == NULL)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionStartMatch(): not found objectID! requestorSessionID={}\n",
			requestorSessionID));

		CMD_Client_OnEndMatch res_packet;
		res_packet.set_errcode(ServerError::OBJECT_NOT_FOUND);
		pSession->forwardPacket(requestorSessionID, CMD::Client_OnEndMatch, res_packet);
		return;
	}

	if (pObj->state() != XPlayer::WaitStart)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionStartMatch(): {}, state({}) != WaitStart!\n", pObj->id(), pObj->state()));

		CMD_Client_OnEndMatch res_packet;

		if(pObj->state() == XPlayer::StartMatching)
			res_packet.set_errcode(ServerError::FREQUENT_OPERATION);
		else
			res_packet.set_errcode(ServerError::PLAYING);

		pSession->forwardPacket(requestorSessionID, CMD::Client_OnEndMatch, res_packet);
		return;
	}

	pObj->startMatch(packet.gameid(), packet.gamemode());
}

//-------------------------------------------------------------------------------------
void XServerApp::startMatch(XObjectPtr pObj, GameID gameID, GameMode gameMode)
{
	DEBUG_MSG(fmt::format("XServerApp::startMatch(): {}, gameID={}, gameMode={}\n",
		pObj->id(), gameID, (int)gameMode));

	GameConfig* cfg = ResMgr::getSingleton().findGameConfig(gameID);

	if (!cfg)
	{
		ERROR_MSG(fmt::format("XServerApp::startMatch(): not found gameID={}! player={}\n",
			gameID, pObj->id()));

		return;
	}

	if (gameMode > cfg->gameModes.size())
	{
		ERROR_MSG(fmt::format("XServerApp::startMatch(): not found gameMode={}! gameID={}, player={}\n",
			gameMode, gameID, pObj->id()));

		return;
	}

	if (gameMode != 0)
	{
		startMatch_(pObj, gameID, gameMode, true);
	}
	else
	{
		/*
		if (gameMode == 0)
		{
		std::random_device r_device;
		std::uniform_int_distribution<int> dis(1, 3);
		gameMode = dis(r_device);
		}
		*/
		std::vector<int> randMode;
		for (int i = 0; i < cfg->gameModes.size(); ++i)
		{
			randMode.push_back(cfg->gameModes[i].modeID);
		}

		std::random_shuffle(randMode.begin(), randMode.end());

		for (auto& item : randMode)
		{
			startMatch_(pObj, gameID, item, false);
			if (((XPlayer*)pObj.get())->roomID() != OBJECT_ID_INVALID)
				break;
		}

		// not found room
		if (((XPlayer*)pObj.get())->roomID() == OBJECT_ID_INVALID)
		{
			// add to pendings
			PendingMatch pendingMatch;
			pendingMatch.gameID = gameID;
			pendingMatch.pObj = pObj;

			addPendingMatch(pendingMatch);
		}
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::startMatch_(XObjectPtr pObj, GameID gameID, GameMode gameMode, bool notFoundCreateRoom)
{
	GameConfig* cfg = ResMgr::getSingleton().findGameConfig(gameID);
	int playerMax = cfg->gameModes[gameMode - 1].playerMax;

	// 1: find from local-rooms
	// 2: not found in local-rooms, find local-randommatch players
	// 2: not found in local, find from remote halls1~N
	Rooms& rooms = findRooms(gameID, gameMode, 0);
	RoomPtr foundRoom;

	// sorted!
	if(rooms.size() > 0 && rooms[0]->isWaitStart())
		foundRoom = rooms[0];

	if (foundRoom)
	{
		foundRoom->enter(pObj);
		movePendingMatchObjectsToRoom(foundRoom);
		return;
	}

	if (notFoundCreateRoom)
	{
		foundRoom = RoomPtr(new Room(gameID, gameMode, uuid(), rooms));
		rooms.push_back(foundRoom);
		rooms_[foundRoom->id()] = foundRoom;
		foundRoom->enter(pObj);
		movePendingMatchObjectsToRoom(foundRoom);
		return;
	}

	DEBUG_MSG(fmt::format("XServerApp::startMatch_(): not found room, try find pendingMatchs, gameMode={}! gameID={}, pendingMatchs={}, player={}\n",
		(int)gameMode, gameID, pendingMatchs_.size(), pObj->id()));

	// create a room of empty
	foundRoom = RoomPtr(new Room(gameID, gameMode, uuid(), rooms));

	// Trying to join the room
	movePendingMatchObjectsToRoom(foundRoom);

	if (foundRoom->numPlayers() > 0)
	{
		foundRoom->enter(pObj);
		rooms.push_back(foundRoom);
		rooms_[foundRoom->id()] = foundRoom;
	}
	else
	{
		DEBUG_MSG(fmt::format("XServerApp::startMatch_(): not found room, gameMode={}! gameID={}, pendingMatchs={}, player={}\n",
			(int)gameMode, gameID, pendingMatchs_.size(), pObj->id()));
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::movePendingMatchObjectsToRoom(RoomPtr room)
{
	GameID gameID = room->gameID();
	GameMode gameMode = room->gameMode();

	int expectedPlayerNum = room->expectedPlayerNum();
	if (expectedPlayerNum <= 0)
	{
		return;
	}

	std::vector<PendingMatch*> founds;

	for (auto& item : pendingMatchs_)
	{
		if (item.second.gameID == gameID)
		{
			founds.push_back(&item.second);

			if (expectedPlayerNum <= founds.size())
				break;
		}
	}

	if ((expectedPlayerNum - 1/*requestor*/) > founds.size())
	{
		return;
	}

	for (auto& item : founds)
	{
		room->enter(item->pObj);
		pendingMatchs_.erase(item->pObj->id());
	}
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRequestCreateRoomCB(Session* pSession, const CMD_Halls_OnRequestCreateRoomCB& packet)
{
	RoomPtr room = findRoom(packet.roomid());

	if (room == NULL)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRequestCreateRoomCB(): not found room={}!\n",
			packet.roomid()));

		return;
	}

	room->onCreateRoomServerCB(packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionRoomSrvGameOverReport(Session* pSession, const CMD_Halls_OnRoomSrvGameOverReport& packet)
{
	RoomPtr room = findRoom(packet.roomid());

	if (room == NULL)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionRoomSrvGameOverReport(): not found room={}!\n",
			packet.roomid()));

		return;
	}

	room->onRoomSrvGameOverReport(packet);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionQueryPlayerGameDataCB(Session* pSession, const CMD_Halls_OnQueryPlayerGameDataCB& packet)
{
	XObjectPtr pObj = findObject(packet.playerid());

	if (!pObj)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionQueryPlayerGameDataCB(): not found player! playerID={}\n",
			packet.playerid()));

		return;
	}

	DEBUG_MSG(fmt::format("XPlayer::queryGameData(): player={}, GameID={}, GameMode={}, score={}, topscore={}, victory={}, defeat={}\n", 
		packet.playerid(), packet.gameid(), packet.gamemode(), packet.score(), packet.topscore(), packet.victory(), packet.defeat()));

	XPlayer* pXPlayer = ((XPlayer*)(pObj.get()));

	pXPlayer->addScore(packet.gameid(), packet.gamemode(), packet.score(), true);
	pXPlayer->setTopscore(packet.gameid(), packet.gamemode(), packet.topscore());
	pXPlayer->setDefeat(packet.gameid(), packet.gamemode(), packet.defeat());
	pXPlayer->setVictory(packet.gameid(), packet.gamemode(), packet.victory());
	pXPlayer->updateContextToClient();
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionQueryPlayerGameData(SessionID requestorSessionID, Session* pSession, const CMD_Halls_QueryPlayerGameData& packet)
{
	XPlayer* pObj = ((XPlayer*)((XSession*)pSession)->pPlayer.get());

	if (pObj == NULL)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionQueryPlayerGameData(): not found objectID! requestorSessionID={}\n",
			requestorSessionID));

		//CMD_Client_OnEndMatch res_packet;
		//res_packet.set_errcode(ServerError::OBJECT_NOT_FOUND);
		//pSession->forwardPacket(requestorSessionID, CMD::Client_OnEndMatch, res_packet);
		return;
	}

	DEBUG_MSG(fmt::format("XServerApp::onSessionQueryPlayerGameData(): player={}\n", pObj->id()));
	pObj->queryGameData(packet.gameid(), packet.gamemode());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionListGames(SessionID requestorSessionID, Session* pSession, const CMD_Halls_ListGames& packet)
{
	XPlayer* pObj = ((XPlayer*)((XSession*)pSession)->pPlayer.get());

	CMD_Client_OnListGamesCB res_packet;

	if (pObj == NULL)
	{
		ERROR_MSG(fmt::format("XServerApp::onSessionListGames(): not found objectID! requestorSessionID={}\n",
			requestorSessionID));

		res_packet.set_page(0);
		res_packet.set_type(0);
		pSession->forwardPacket(requestorSessionID, CMD::Client_OnListGamesCB, res_packet);
		return;
	}

	DEBUG_MSG(fmt::format("XServerApp::onSessionListGames(): player={}\n", pObj->id()));

	uint32 page = packet.page();
	uint32 type = packet.type();
	uint32 maxNum = packet.maxnum();

	res_packet.set_page(page);
	res_packet.set_type(type);

	std::map<GameID, GameConfig>& configs = ResMgr::getSingleton().gameConfigs();

	res_packet.set_totalgamesnum(configs.size());

	for (auto& item : configs)
	{
		GameConfig& cfg = item.second;
		CMD_GameInfos* gameInfos = res_packet.add_gameinfos();

		gameInfos->set_gameid(cfg.id);
		gameInfos->set_gamename(cfg.name);
		gameInfos->set_type(cfg.type);
		gameInfos->set_url_apk(cfg.url_apk);
		gameInfos->set_url_icon(cfg.url_icon);

		for (auto& mode : cfg.gameModes)
		{
			CMD_GameModeInfos* pCMD_GameModeInfos = gameInfos->add_gamemodes();
			pCMD_GameModeInfos->set_gamemodeid(mode.modeID);
			pCMD_GameModeInfos->set_gamemodename(mode.name);
			pCMD_GameModeInfos->set_gametime(mode.gameTime);
			pCMD_GameModeInfos->set_playermax(mode.playerMax);
		}
	}

	pSession->forwardPacket(requestorSessionID, CMD::Client_OnListGamesCB, res_packet);
}

//-------------------------------------------------------------------------------------
}
