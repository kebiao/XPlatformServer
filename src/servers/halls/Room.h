#ifndef X_ROOM_H
#define X_ROOM_H

#include "common/common.h"
#include "protos/Commands.pb.h"

namespace XServer {

class Session;
struct GameConfig;
struct GameModeConfig;
class XObject;
class XPlayer;
class Room;

typedef std::shared_ptr<Room> RoomPtr;
typedef std::shared_ptr<XObject> XObjectPtr;
typedef std::vector<RoomPtr> Rooms;

class Room
{
public:
	enum State
	{
		WaitStart = 0,
		CreatingRoomServer = 1,
		Playing = 2,
		GameOver = 3
	};

	static bool sortFunc(RoomPtr& v1, RoomPtr& v2)
	{
		return v1->expectedPlayerNum() < v2->expectedPlayerNum();
	}

public:
	Room(GameID gameID, GameMode mode, ObjectID oid, Rooms& roomsPool);
	virtual ~Room();

	ObjectID id() const {
		return id_;
	}

	GameID gameID() const;
	GameMode gameMode() const;
	std::string& name() const;

	int gameMaxPlayerNum() const;
	float gameTime() const;

	State state() const {
		return state_;
	}

	void state(State v) {
		state_ = v;
	}

	bool isWaitStart() const {
		return state_ == WaitStart;
	}

	bool isPlaying() const {
		return state_ == Playing;
	}

	bool isCreatingRoomServer() const {
		return state_ == CreatingRoomServer;
	}

	bool isGameOver() const {
		return state_ == GameOver;
	}

	bool isDestroyed() const {
		return isDestroyed_;
	}

	void destroy();
	virtual void onDestroy();

	int expectedPlayerNum();

	void enter(XObjectPtr pObj);
	void leave(XObjectPtr pObj);
	void leave(ObjectID oid);

	int numPlayers() const {
		return players_.size();
	}

	void startGame();
	void endGame();

	void createRoomServer();
	void onCreateRoomServerCB(const CMD_Halls_OnRequestCreateRoomCB& results);
	void onRoomSrvGameOverReport(const CMD_Halls_OnRoomSrvGameOverReport& packet);

	std::string c_str();

	void broadcastPlayerMessages(int32 cmd, const ::google::protobuf::Message& packet);

	virtual void onTick(void* userargs);

	std::string roomServerIP() const {
		return roomServerIP_;
	}

	uint16 roomServerPort() const {
		return roomServerPort_;
	}

	ObjectID roomServerTokenID() const {
		return roomServerTokenID_;
	}

	void fillContextData(CMD_PlayerContext* pCMD_PlayerContext);

protected:
	ObjectID id_;

	GameConfig* pGameConfig_;
	GameModeConfig* pGameModeConfig_;

	State state_;

	std::map<ObjectID, XObjectPtr> players_;

	Rooms& roomsPool_;

	bool isDestroyed_;

	time_t roomCreateTime_;

	struct event * tickTimerEvent_;

	std::string roomServerIP_;
	uint16 roomServerPort_;
	ObjectID roomServerTokenID_;
};

typedef std::shared_ptr<Room> RoomPtr;

}

#endif // X_ROOM_H
