#ifndef X_XPLAYER_H
#define X_XPLAYER_H

#include "XObject.h"

namespace XServer {

class XPlayer : public XObject
{
public:
	enum State
	{
		WaitStart = 0,
		StartMatching = 1,
		Playing = 2
	};

public:
	XPlayer(ObjectID oid, SessionID requestorSessionID, Session* pSession, std::string accountName, uint64 tokenID);
	virtual ~XPlayer();

	ObjectID roomID() const {
		return roomID_;
	}

	void roomID(ObjectID v) {
		roomID_ = v;
	}

	virtual void onDestroyTimer(void* userargs);
	virtual void onDestroy() override;

	void onClientDisconnected(Session* pSession);

	void startMatch(GameID gameID, GameMode gameMode);

	State state() const {
		return state_;
	}

	void state(State v) {
		state_ = v;
	}

	void startGame();
	void endGame();

	void queryGameData(GameID gameID, GameMode gameMode);
	void saveGameData();
	void updateContextToClient();

	SessionID requestorSessionID() const {
		return requestorSessionID_;
	}

	const std::string& playerName() const {
		return playerName_;
	}

	void playerName(const std::string& name) {
		playerName_ = name;
	}

	const std::string& accountName() const {
		return accountName_;
	}

	const int modelID() const {
		return modelID_;
	}

	void modelID(int v) {
		modelID_ = v;
	}

	uint64 tokenID() const {
		return tokenID_;
	}

	void addExp(int v) {
		exp_ += v;
	}

	int exp() const {
		return exp_;
	}

	void exp(int v) {
		exp_ = v;
	}

	void addGold(int v) {
		gold_ += v;
	}

	int gold() const {
		return gold_;
	}

	void gold(int v) {
		gold_ = v;
	}

	void fillContextData(CMD_PlayerContext* pCMD_PlayerContext);

	bool hasGameData(GameID gameID, GameMode mode)
	{
		auto iter1 = topscore_.find(gameID);
		if (iter1 == topscore_.end())
		{
			return false;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			return false;
		}

		return true;
	}

	void addScore(GameID gameID, GameMode mode, int v, bool isUpdate = false) 
	{
		if (v > topscore(gameID, mode))
			setTopscore(gameID, mode, v);

		auto iter1 = score_.find(gameID);
		if (iter1 == score_.end())
		{
			std::map<GameMode, int>& map1 = score_[gameID];
			map1[mode] = v;
			return;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			iter1->second[mode] = v;
			return;
		}

		if(!isUpdate)
			iter2->second += v;
		else
			iter2->second = v;
	}

	int score(GameID gameID, GameMode mode) const {
		auto iter1 = score_.find(gameID);
		if (iter1 == score_.end())
		{
			return 0;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			return 0;
		}

		return iter2->second;
	}

	void setTopscore(GameID gameID, GameMode mode, int v) {
		auto iter1 = topscore_.find(gameID);
		if (iter1 == topscore_.end())
		{
			std::map<GameMode, int>& map1 = topscore_[gameID];
			map1[mode] = v;
			return;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			iter1->second[mode] = v;
			return;
		}

		iter2->second = v;
	}

	int topscore(GameID gameID, GameMode mode) const {
		auto iter1 = topscore_.find(gameID);
		if (iter1 == topscore_.end())
		{
			return 0;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			return 0;
		}

		return iter2->second;
	}

	int victory(GameID gameID, GameMode mode) const {
		auto iter1 = victory_.find(gameID);
		if (iter1 == victory_.end())
		{
			return 0;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			return 0;
		}

		return iter2->second;
	}

	void addVictory(GameID gameID, GameMode mode) {
		auto iter1 = victory_.find(gameID);
		if (iter1 == victory_.end())
		{
			std::map<GameMode, int>& map1 = victory_[gameID];
			map1[mode] = 1;
			return;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			iter1->second[mode] = 1;
			return;
		}

		iter2->second += 1;
	}

	void setVictory(GameID gameID, GameMode mode, int v) {
		auto iter1 = victory_.find(gameID);
		if (iter1 == victory_.end())
		{
			std::map<GameMode, int>& map1 = victory_[gameID];
			map1[mode] = v;
			return;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			iter1->second[mode] = v;
			return;
		}

		iter2->second = v;
	}

	int defeat(GameID gameID, GameMode mode) const {
		auto iter1 = defeat_.find(gameID);
		if (iter1 == defeat_.end())
		{
			return 0;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			return 0;
		}

		return iter2->second;
	}

	void addDefeat(GameID gameID, GameMode mode) {
		auto iter1 = defeat_.find(gameID);
		if (iter1 == defeat_.end())
		{
			std::map<GameMode, int>& map1 = defeat_[gameID];
			map1[mode] = 1;
			return;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			iter1->second[mode] = 1;
			return;
		}

		iter2->second += 1;
	}

	void setDefeat(GameID gameID, GameMode mode, int v) {
		auto iter1 = defeat_.find(gameID);
		if (iter1 == defeat_.end())
		{
			std::map<GameMode, int>& map1 = defeat_[gameID];
			map1[mode] = v;
			return;
		}

		auto iter2 = iter1->second.find(mode);
		if (iter2 == iter1->second.end())
		{
			iter1->second[mode] = v;
			return;
		}

		iter2->second += v;
	}

protected:
	State state_;

	ObjectID roomID_;

	SessionID requestorSessionID_;

	struct event * destroyTimerEvent_;

	std::string playerName_;
	std::string accountName_;

	int modelID_;

	uint64 tokenID_;

	int exp_;
	int gold_;

	typedef std::map<GameID, std::map<GameMode, int> > GAME_DATA;

	GAME_DATA score_;
	GAME_DATA topscore_;
	GAME_DATA victory_;
	GAME_DATA defeat_;
};

typedef std::shared_ptr<XPlayer> XPlayerPtr;

}

#endif // X_XPLAYER_H
