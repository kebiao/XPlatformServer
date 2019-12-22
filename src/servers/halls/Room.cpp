#include "Room.h"
#include "XObject.h"
#include "XPlayer.h"
#include "XServerApp.h"
#include "event/TcpSocket.h"
#include "event/NetworkInterface.h"
#include "event/Session.h"
#include "event/Timer.h"
#include "log/XLog.h"
#include "protos/Commands.pb.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"
#include "resmgr/ResMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Room::Room(GameID gameID, GameMode mode, ObjectID oid, Rooms& roomsPool):
id_(oid),
pGameConfig_(0),
pGameModeConfig_(0),
state_(WaitStart),
players_(),
roomsPool_(roomsPool),
isDestroyed_(false),
roomCreateTime_(getTimeStamp()),
tickTimerEvent_(NULL),
roomServerIP_(),
roomServerPort_(),
roomServerTokenID_(0)
{
	DEBUG_MSG(fmt::format("new Room(): gameID={}, mode={}, roomID={}\n", 
		gameID, (int)mode, id_));

	pGameConfig_ = ResMgr::getSingleton().findGameConfig(gameID);
	assert(pGameConfig_);
	pGameModeConfig_ = &pGameConfig_->gameModes[mode - 1];
	assert(pGameModeConfig_);

	tickTimerEvent_ = ((XServerApp&)XServerApp::getSingleton()).pTimer()->addTimer(TIME_SECONDS, -1,
		std::bind(&Room::onTick, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------
Room::~Room()
{
	DEBUG_MSG(fmt::format("Room::~Room(): {}\n", 
		c_str()));

	if (tickTimerEvent_)
		((XServerApp&)XServerApp::getSingleton()).pTimer()->delTimer(tickTimerEvent_);
}

//-------------------------------------------------------------------------------------
std::string Room::c_str()
{
	std::string strplayers;

	for (auto& item : players_)
		strplayers += fmt::format("{}, ", item.second->id());

	return fmt::format("roomID={}, gameID={}, mode={}, state={}, players=[{}]", 
		id(), gameID(), (int)gameMode(), state(), strplayers);
}

//-------------------------------------------------------------------------------------
GameID Room::gameID() const 
{
	return pGameConfig_->id;
}

//-------------------------------------------------------------------------------------
GameMode Room::gameMode() const
{
	return pGameModeConfig_->modeID;
}

//-------------------------------------------------------------------------------------
std::string& Room::name() const
{
	return pGameConfig_->name;
}

//-------------------------------------------------------------------------------------
int Room::gameMaxPlayerNum() const
{
	return pGameModeConfig_->playerMax;
}

//-------------------------------------------------------------------------------------
float Room::gameTime() const
{
	return pGameModeConfig_->gameTime;
}

//-------------------------------------------------------------------------------------
int Room::expectedPlayerNum()
{
	if (!isWaitStart())
		return 999999;

	int num = pGameModeConfig_->playerMax - players_.size();
	if (num < 0)
		num = 0;

	return num;
}

//-------------------------------------------------------------------------------------
void Room::destroy()
{
	if (isDestroyed_)
		return;

	isDestroyed_ = true;

	DEBUG_MSG(fmt::format("Room::destroy(): gameID={}, mode={}, roomID={}, players={}\n", 
		gameID(), (int)gameMode(), id_, players_.size()));

	onDestroy();

	((XServerApp&)XServerApp::getSingleton()).destroyRoom(id());
}

//-------------------------------------------------------------------------------------
void Room::onDestroy()
{
	for (auto& item : players_)
	{
		((XPlayer*)item.second.get())->endGame();
	}
}

//-------------------------------------------------------------------------------------
void Room::fillContextData(CMD_PlayerContext* pCMD_PlayerContext)
{
	pCMD_PlayerContext->set_roomid(id());
	pCMD_PlayerContext->set_gamemode(gameMode());
	pCMD_PlayerContext->set_gameid(gameID());
	pCMD_PlayerContext->set_gameserverip(roomServerIP());
	pCMD_PlayerContext->set_gameserverport(roomServerPort());
	pCMD_PlayerContext->set_gameservertokenid(roomServerTokenID());
	pCMD_PlayerContext->set_gamemaxplayernum(gameMaxPlayerNum());
	pCMD_PlayerContext->set_gametime(gameTime());

	for (auto& item : players_)
	{
		CMD_RoomPlayerInfo* pCMD_RoomPlayerInfo = pCMD_PlayerContext->add_gameplayers();
		XPlayer* pXPlayer = (XPlayer*)item.second.get();

		pCMD_RoomPlayerInfo->set_playerid(pXPlayer->id());
		pCMD_RoomPlayerInfo->set_playername(pXPlayer->playerName());
		pCMD_RoomPlayerInfo->set_playermodelid(pXPlayer->modelID());
		pCMD_RoomPlayerInfo->set_clientaddr("");
		pCMD_RoomPlayerInfo->set_exp(pXPlayer->exp());
		pCMD_RoomPlayerInfo->set_score(0);
		pCMD_RoomPlayerInfo->set_topscore(pXPlayer->topscore(gameID(), GameMode()));
		pCMD_RoomPlayerInfo->set_victory(pXPlayer->victory(gameID(), GameMode()));
		pCMD_RoomPlayerInfo->set_defeat(pXPlayer->defeat(gameID(), GameMode()));
	}
}

//-------------------------------------------------------------------------------------
void Room::onTick(void* userargs)
{
	if (isWaitStart())
	{
		if (players_.size() == 0)
			destroy();
	}
	else
	{
		time_t diff = getTimeStamp() - roomCreateTime_;
		if ((pGameModeConfig_->gameTime * TIME_SECONDS) + (TIME_SECONDS * 60) < diff)
		{
			ERROR_MSG(fmt::format("Room::onTick(): game timeout! roomID={}, gameID={}, mode={}\n",
				id(), gameID(), (int)gameMode()));

			endGame();
		}
	}
}

//-------------------------------------------------------------------------------------
void Room::enter(XObjectPtr pObj)
{
	DEBUG_MSG(fmt::format("Room::enter(): gameID={}, mode={}, roomID={}, player={}\n", 
		gameID(), (int)gameMode(), id_, pObj->id()));

	players_[pObj->id()] = pObj;

	XPlayer* pPlayer = ((XPlayer*)pObj.get());

	pPlayer->roomID(id());
	
	CMD_Client_OnMatchingUpdate res_packet;
	res_packet.set_name(pPlayer->playerName());
	res_packet.set_id(pPlayer->id());
	res_packet.set_modelid(pPlayer->modelID());
	res_packet.set_enter(true);
	broadcastPlayerMessages(CMD::Client_OnMatchingUpdate, res_packet);

	for (auto& item : players_)
	{
		XPlayer* pOtherPlayer = ((XPlayer*)item.second.get());

		if (pPlayer == pOtherPlayer)
			continue;

		CMD_Client_OnMatchingUpdate res_packet;
		res_packet.set_name(pOtherPlayer->playerName());
		res_packet.set_id(pOtherPlayer->id());
		res_packet.set_modelid(pOtherPlayer->modelID());
		res_packet.set_enter(true);

		Session* pSession = pPlayer->pSession();
		if (pSession)
			pSession->forwardPacket(pPlayer->requestorSessionID(), CMD::Client_OnMatchingUpdate, res_packet);
	}

	startGame();

	std::sort(roomsPool_.begin(), roomsPool_.end(), sortFunc);
}

//-------------------------------------------------------------------------------------
void Room::leave(XObjectPtr pObj)
{
	leave(pObj->id());
}

//-------------------------------------------------------------------------------------
void Room::leave(ObjectID oid)
{
	DEBUG_MSG(fmt::format("Room::leave(): gameID={}, mode={}, roomID={}, player={}\n",
		gameID(), (int)gameMode(), id_, oid));

	auto iter = players_.find(oid);
	if (iter == players_.end())
	{
		ERROR_MSG(fmt::format("Room::leave(): not found player! gameID={}, mode={}, roomID={}, player={}\n",
			gameID(), (int)gameMode(), id_, oid));

		return;
	}
	
	((XPlayer*)iter->second.get())->roomID(OBJECT_ID_INVALID);
	players_.erase(oid);

	std::sort(roomsPool_.begin(), roomsPool_.end(), sortFunc);
}

//-------------------------------------------------------------------------------------
void Room::startGame()
{
	if (!isWaitStart() && !isCreatingRoomServer())
		return;

	roomCreateTime_ = getTimeStamp();

	if (isWaitStart())
	{
		if (expectedPlayerNum() > 0)
			return;

		createRoomServer();
		return;
	}
	
	state_ = Playing;

	DEBUG_MSG(fmt::format("Room::startGame(): gameID={}, mode={}, roomID={}\n", 
		gameID(), (int)gameMode(), id_));

	for (auto& item : players_)
	{
		((XPlayer*)item.second.get())->startGame();
	}
}

//-------------------------------------------------------------------------------------
void Room::broadcastPlayerMessages(int32 cmd, const ::google::protobuf::Message& packet)
{
	for (auto& item : players_)
	{
		Session* pSession = item.second->pSession();
		if (pSession)
			pSession->forwardPacket(((XPlayer*)item.second.get())->requestorSessionID(), cmd, packet);
	}
}

//-------------------------------------------------------------------------------------
void Room::createRoomServer()
{
	DEBUG_MSG(fmt::format("Room::createRoomServer(): gameID={}, mode={}, roomID={}\n", 
		gameID(), (int)gameMode(), id_));

	state_ = CreatingRoomServer;

	ServerInfo* pServerInfo = ((XServerApp&)XServerApp::getSingleton()).pServerMgr()->findRoommgr();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XServerApp::createRoomServer(): create room is failed! not found roommgr!\n"));

		CMD_Client_OnEndMatch res_packet;
		res_packet.set_errcode(ServerError::SERVER_NOT_READY);
		broadcastPlayerMessages(CMD::Client_OnEndMatch, res_packet);
		destroy();
		return;
	}

	CMD_Roommgr_RequestCreateRoom req_packet;
	req_packet.set_exefile(pGameModeConfig_->serverExeFile);
	req_packet.set_exeoptions(pGameModeConfig_->serverExeOptions);
	req_packet.set_gameid(gameID());
	req_packet.set_gamemode(gameMode());
	req_packet.set_gametime(pGameModeConfig_->gameTime);
	req_packet.set_hallsid(((XServerApp&)XServerApp::getSingleton()).id());
	req_packet.set_maxplayernum(pGameModeConfig_->playerMax);
	req_packet.set_roomid(id());

	for (auto& item : players_)
	{
		XPlayer* pXPlayer = ((XPlayer*)item.second.get());

		CMD_RoomPlayerInfo* pCMD_RoomPlayerInfo = req_packet.add_players();
		Session* pSession = item.second->pSession();
		if (pSession)
		{
			pCMD_RoomPlayerInfo->set_clientaddr(pSession->getIP());
		}

		pCMD_RoomPlayerInfo->set_playerid(pXPlayer->id());
		pCMD_RoomPlayerInfo->set_playername(pXPlayer->playerName());
		pCMD_RoomPlayerInfo->set_playermodelid(pXPlayer->modelID());
		pCMD_RoomPlayerInfo->set_exp(pXPlayer->exp());
		pCMD_RoomPlayerInfo->set_score(pXPlayer->score(gameID(), GameMode()));
		pCMD_RoomPlayerInfo->set_topscore(pXPlayer->topscore(gameID(), GameMode()));
		pCMD_RoomPlayerInfo->set_victory(pXPlayer->victory(gameID(), GameMode()));
		pCMD_RoomPlayerInfo->set_defeat(pXPlayer->defeat(gameID(), GameMode()));
	}
	
	pServerInfo->pSession->sendPacket(CMD::Roommgr_RequestCreateRoom, req_packet);
}

//-------------------------------------------------------------------------------------
void Room::onCreateRoomServerCB(const CMD_Halls_OnRequestCreateRoomCB& results)
{
	CMD_Client_OnEndMatch res_packet;
	res_packet.set_errcode(results.errcode());
	res_packet.set_gameserverip(results.ip());
	res_packet.set_gameserverport(results.port());
	res_packet.set_gameid(gameID());
	res_packet.set_gamemode(gameMode());
	res_packet.set_gameservertokenid(results.tokenid());
	res_packet.set_roomid(id());
	res_packet.set_gameplayersize(players_.size());
	res_packet.set_gametime(gameTime());

	for (auto& item : players_)
	{
		XPlayer* pXPlayer = (XPlayer*)item.second.get();
		res_packet.set_topscore(pXPlayer->topscore(gameID(), gameMode()));
		res_packet.set_victory(pXPlayer->victory(gameID(), gameMode()));
		res_packet.set_defeat(pXPlayer->defeat(gameID(), gameMode()));

		Session* pSession = pXPlayer->pSession();
		if (pSession)
			pSession->forwardPacket(pXPlayer->requestorSessionID(), CMD::Client_OnEndMatch, res_packet);
	}

	if (results.errcode() != ServerError::OK)
	{
		ERROR_MSG(fmt::format("Room::onCreateRoomServerCB: ceate room is failed! error={}\n", 
			ServerError_Name(results.errcode())));

		destroy();
		return;
	}

	DEBUG_MSG(fmt::format("Room::onCreateRoomServerCB(): create room({}:{}, token:{}) is success! gameID={}, mode={}, roomID={}\n", 
		results.ip(), results.port(), results.tokenid(), gameID(), (int)gameMode(), id_));

	roomCreateTime_ = getTimeStamp();

	roomServerIP_ = results.ip();
	roomServerPort_ = results.port();
	roomServerTokenID_ = results.tokenid();

	startGame();
}

//-------------------------------------------------------------------------------------
void Room::onRoomSrvGameOverReport(const CMD_Halls_OnRoomSrvGameOverReport& packet)
{
	DEBUG_MSG(fmt::format("Room::onRoomSrvGameOverReport(): gameID={}, mode={}, roomID={}\n",
		gameID(), (int)gameMode(), id_));

	for (int i = 0; i < packet.playerdatas_size(); ++i)
	{
		const CMD_Halls_PlayerGameData& playerData = packet.playerdatas(i);

		DEBUG_MSG(fmt::format("Room::onRoomSrvGameOverReport(): playerID={}, exp={}, score={}, victory={}\n",
			playerData.id(), playerData.exp(), playerData.score(), playerData.victory()));

		auto iter = players_.find(playerData.id());
		if (iter == players_.end())
		{
			ERROR_MSG(fmt::format("Room::onRoomSrvGameOverReport(): not found player={}\n",
				playerData.id()));
		}
		else
		{
			bool victory = playerData.victory();

			if (victory)
				((XPlayer*)iter->second.get())->addVictory(gameID(), gameMode());
			else
				((XPlayer*)iter->second.get())->addDefeat(gameID(), gameMode());

			((XPlayer*)iter->second.get())->addScore(gameID(), gameMode(), playerData.score());
			((XPlayer*)iter->second.get())->addExp(playerData.exp());
		}
	}
}

//-------------------------------------------------------------------------------------
void Room::endGame()
{
	DEBUG_MSG(fmt::format("Room::endGame(): gameID={}, mode={}, roomID={}, state={}\n", 
		gameID(), (int)gameMode(), id_, state_));

	if (state_ == Playing)
	{
		CMD_Client_OnGameOver res_packet;
		res_packet.set_errcode(ServerError::OK);
		res_packet.set_roomid(id());
		res_packet.set_gameid(gameID());
		res_packet.set_gamemode(gameMode());

		broadcastPlayerMessages(CMD::Client_OnGameOver, res_packet);
	}

	state_ = GameOver;

	destroy();
}

//-------------------------------------------------------------------------------------
}
