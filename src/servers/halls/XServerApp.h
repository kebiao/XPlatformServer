#ifndef X_SERVER_APP_H
#define X_SERVER_APP_H

#include "common/common.h"
#include "common/singleton.h"
#include "server/XServerBase.h"

namespace XServer {

class ConnectorFinder;
class XObject;
class XPlayer;
class Room;

typedef std::shared_ptr<XObject> XObjectPtr;
typedef std::shared_ptr<Room> RoomPtr;

class XServerApp : public XServerBase
{
public:
	typedef std::map<ObjectID, XObjectPtr> Objects;
	typedef std::vector<RoomPtr> Rooms;
	typedef std::map<int /* PlayerMaxNum */, Rooms/*rooms*/ > PlayerMaxNumMapping_Room;
	typedef std::map<GameMode /* gameMode */, PlayerMaxNumMapping_Room> GameModeMapping_PlayerMaxNumMappingRoom;
	typedef std::map<GameID, GameModeMapping_PlayerMaxNumMappingRoom> GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom;

	struct PendingAccount
	{
		PendingAccount()
		{
			requestorSessionID = SESSION_ID_INVALID;
			currentSessionID = SESSION_ID_INVALID;
			commitAccountName = "";
			tokenID = 0;
			password = "";
			commitDatas = "";
			backDatas = "";
			foundObjectID = OBJECT_ID_INVALID;
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
		uint64 tokenID;
		std::string password;
		std::string commitDatas;
		std::string backDatas;
		time_t lastTime;

		// 如果有存在的ID != OBJECT_ID_INVALID则表明该对象已经在当前进程创建
		ObjectID foundObjectID;
	};

	struct PendingMatch {
		XObjectPtr pObj;
		GameID gameID;
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

	XObjectPtr findObject(ObjectID id) {
		auto iter = objects_.find(id);
		if (iter != objects_.end())
			return iter->second;

		return XObjectPtr();
	}

	bool addObject(XObjectPtr obj);

	void onDelObject(XObjectPtr obj);

	bool delObject(ObjectID id) {
		auto iter = objects_.find(id);
		if (iter != objects_.end())
		{
			onDelObject(iter->second);
			objects_.erase(iter);
			return true;
		}

		return false;
	}

	PendingAccount* findPendingAccount(const std::string accountName);
	bool addPendingAccount(const PendingAccount& pendingAccount);
	bool delPendingAccount(const std::string accountName);

	PendingMatch* findPendingMatch(ObjectID oid);
	bool addPendingMatch(const PendingMatch& pendingMatch);
	bool delPendingMatch(ObjectID oid);

	virtual void onSessionDisconnected(Session* pSession) override;
	void onSessionRemoteDisconnected(SessionID requestorSessionID, Session* pSession, const CMD_RemoteDisconnected& packet);

	void onSessionRequestAllocClient(Session* pSession, const CMD_Halls_RequestAllocClient& packet);
	void onSessionLogin(SessionID requestorSessionID, Session* pSession, const CMD_Halls_Login& packet);
	void onSessionStartMatch(SessionID requestorSessionID, Session* pSession, const CMD_Halls_StartMatch& packet);
	void onSessionRequestCreateRoomCB(Session* pSession, const CMD_Halls_OnRequestCreateRoomCB& packet);
	void onSessionRoomSrvGameOverReport(Session* pSession, const CMD_Halls_OnRoomSrvGameOverReport& packet);
	void onSessionQueryAccountCB(Session* pSession, const CMD_Halls_OnQueryAccountCB& packet);
	void onSessionQueryPlayerGameDataCB(Session* pSession, const CMD_Halls_OnQueryPlayerGameDataCB& packet);
	void onSessionQueryPlayerGameData(SessionID requestorSessionID, Session* pSession, const CMD_Halls_QueryPlayerGameData& packet);
	void onSessionListGames(SessionID requestorSessionID, Session* pSession, const CMD_Halls_ListGames& packet);

	virtual void onUpdateServerInfoToSession(CMD_UpdateServerInfos& infos) override;

	void startMatch(XObjectPtr pObj, GameID gameID, GameMode gameMode);
	void startMatch_(XObjectPtr pObj, GameID gameID, GameMode gameMode, bool notFoundCreateRoom);

	RoomPtr findRoom(ObjectID roomID) {
		auto iter = rooms_.find(roomID);
		if (iter == rooms_.end())
			return RoomPtr();

		return iter->second;
	}

	Rooms& findRooms(GameID gameID, GameMode gameMode, int playerMaxNum);

	void movePendingMatchObjectsToRoom(RoomPtr room);

	XObjectPtr findSessionPlayer(SessionID sessionID) {
		auto iter = session2PlayerMapping_.find(sessionID);

		if (iter == session2PlayerMapping_.end())
			return XObjectPtr();

		return iter->second;
	}

	void debugRooms();

	void destroyRoom(ObjectID roomID);

protected:
	std::tr1::unordered_map<std::string, PendingAccount> pendingAccounts_;
	ConnectorFinder* pConnectorFinder_;

	Objects objects_;

	// GameID, gameMode, PlayerMaxNum, rooms
	GameIDMapping_GameModeMapping_PlayerMaxNumMappingRoom rooms_ggpr_;

	std::map<ObjectID, RoomPtr> rooms_;

	std::map<ObjectID, PendingMatch> pendingMatchs_;

	std::map<SessionID, XObjectPtr> session2PlayerMapping_;
};

}

#endif // X_SERVER_APP_H
