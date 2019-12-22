#include "XPlayer.h"
#include "Room.h"
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
XPlayer::XPlayer(ObjectID oid, SessionID requestorSessionID, Session* pSession, std::string accountName, uint64 tokenID):
XObject(oid, pSession),
state_(WaitStart),
roomID_(OBJECT_ID_INVALID),
requestorSessionID_(requestorSessionID),
destroyTimerEvent_(NULL),
playerName_(accountName),
accountName_(accountName),
modelID_(0),
tokenID_(tokenID),
exp_(0),
gold_(0),
score_(),
topscore_(),
victory_(),
defeat_()
{
}

//-------------------------------------------------------------------------------------
XPlayer::~XPlayer()
{
	if (destroyTimerEvent_)
		((XServerApp&)XServerApp::getSingleton()).pTimer()->delTimer(destroyTimerEvent_);
}

//-------------------------------------------------------------------------------------
void XPlayer::onClientDisconnected(Session* pSession)
{
	XObject::onClientDisconnected(pSession);

	if (destroyTimerEvent_)
		((XServerApp&)XServerApp::getSingleton()).pTimer()->delTimer(destroyTimerEvent_);

	destroyTimerEvent_ = ((XServerApp&)XServerApp::getSingleton()).pTimer()->addTimer(
		TIME_SECONDS * ResMgr::getSingleton().serverConfig().player_disconnected_lifetime, -1,
		std::bind(&XPlayer::onDestroyTimer, this, std::placeholders::_1), NULL);
}

//-------------------------------------------------------------------------------------
void XPlayer::onDestroyTimer(void* userargs)
{
	destroy();
}

//-------------------------------------------------------------------------------------
void XPlayer::onDestroy()
{
	if (roomID_ > 0)
	{
		RoomPtr room = ((XServerApp&)XServerApp::getSingleton()).findRoom(roomID_);
		if (room != NULL)
		{
			room->leave(id());
		}
	}
}

//-------------------------------------------------------------------------------------
void XPlayer::fillContextData(CMD_PlayerContext* pCMD_PlayerContext)
{
	pCMD_PlayerContext->set_playerid(id());
	pCMD_PlayerContext->set_playername(playerName());
	pCMD_PlayerContext->set_exp(exp());
	pCMD_PlayerContext->set_playermodelid(modelID());

	Room* pRoom = ((XServerApp&)XServerApp::getSingleton()).findRoom(roomID()).get();
	if (pRoom)
	{
		pRoom->fillContextData(pCMD_PlayerContext);
		pCMD_PlayerContext->set_playergametopscore(topscore(pCMD_PlayerContext->gameid(), pCMD_PlayerContext->gamemode()));
	}
	else
	{
		pCMD_PlayerContext->set_roomid(0);
	}
}

//-------------------------------------------------------------------------------------
void XPlayer::startMatch(GameID gameID, GameMode gameMode)
{
	if (!pSession_)
	{
		ERROR_MSG(fmt::format("XPlayer::startMatch(): {}, pSession == NULL!\n", id_));
		return;
	}

	if (state_ != WaitStart)
	{
		ERROR_MSG(fmt::format("XPlayer::startMatch(): {}, state({}) != WaitStart!\n", id_, state_));
		return;
	}

	state_ = StartMatching;

	((XServerApp&)XServerApp::getSingleton()).startMatch(((XServerApp&)XServerApp::getSingleton()).findObject(this->id()), gameID, gameMode);
}

//-------------------------------------------------------------------------------------
void XPlayer::startGame()
{
	state_ = Playing;

	Room* pRoom = ((XServerApp&)XServerApp::getSingleton()).findRoom(roomID()).get();
	assert(pRoom);

	bool hasData = hasGameData(pRoom->gameID(), pRoom->gameMode());

	DEBUG_MSG(fmt::format("XPlayer::startGame(): {}, hasGameData={}\n", id_, hasData));

	if (!hasData)
	{
		queryGameData(pRoom->gameID(), pRoom->gameMode());
	}
}

//-------------------------------------------------------------------------------------
void XPlayer::endGame()
{
	state_ = WaitStart;
	
	DEBUG_MSG(fmt::format("XPlayer::endGame(): {}\n", id_));
	saveGameData();
	roomID_ = 0;
}

//-------------------------------------------------------------------------------------
void XPlayer::queryGameData(GameID gameID, GameMode gameMode)
{
	DEBUG_MSG(fmt::format("XPlayer::queryGameData(): {}, GameID={}, GameMode={}\n", id_, gameID, (int)gameMode));

	ServerInfo* pServerInfo = ((XServerApp&)XServerApp::getSingleton()).pServerMgr()->findDbmgr();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XPlayer::queryGameData({}): not found dbmgr!\n", id_));
		return;
	}

	CMD_Dbmgr_QueryPlayerGameData req_packet;
	req_packet.set_playerid(id());
	req_packet.set_gameid(gameID);
	req_packet.set_gamemode(gameMode);

	pServerInfo->pSession->sendPacket(CMD::Dbmgr_QueryPlayerGameData, req_packet);
}

//-------------------------------------------------------------------------------------
void XPlayer::saveGameData()
{
	DEBUG_MSG(fmt::format("XPlayer::saveGameData(): {}\n", id_));

	Room* pRoom = ((XServerApp&)XServerApp::getSingleton()).findRoom(roomID()).get();
	assert(pRoom);

	ServerInfo* pServerInfo = ((XServerApp&)XServerApp::getSingleton()).pServerMgr()->findDbmgr();
	if (!pServerInfo || !pServerInfo->pSession)
	{
		ERROR_MSG(fmt::format("XPlayer::saveGameData({}): not found dbmgr!\n", id_));
		return;
	}

	CMD_Dbmgr_WritePlayerGameData req_packet;
	req_packet.set_playerid(id());
	req_packet.set_gameid(pRoom->gameID());
	req_packet.set_gamemode(pRoom->gameMode());
	req_packet.set_playerid(id());
	req_packet.set_score(score(pRoom->gameID(), pRoom->gameMode()));
	req_packet.set_topscore(topscore(pRoom->gameID(), pRoom->gameMode()));
	req_packet.set_victory(victory(pRoom->gameID(), pRoom->gameMode()));
	req_packet.set_defeat(defeat(pRoom->gameID(), pRoom->gameMode()));

	pServerInfo->pSession->sendPacket(CMD::Dbmgr_WritePlayerGameData, req_packet);
}

//-------------------------------------------------------------------------------------
void XPlayer::updateContextToClient()
{
	if (!pSession())
		return;

	DEBUG_MSG(fmt::format("XPlayer::updateContextToClient(): {}\n", id_));

	CMD_Client_UpdatePlayerContext res_packet;
	CMD_PlayerContext* pCMD_PlayerContext = res_packet.mutable_playercontext();
	fillContextData(pCMD_PlayerContext);

	pSession()->forwardPacket(requestorSessionID(), CMD::Client_UpdatePlayerContext, res_packet);
}

//-------------------------------------------------------------------------------------
}
