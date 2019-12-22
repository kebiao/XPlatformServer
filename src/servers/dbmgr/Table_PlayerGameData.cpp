#include "Table_PlayerGameData.h"
#include "Database.h"
#include "XServerApp.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"
#include "event/Session.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Table_PlayerGameData::Table_PlayerGameData(Database* pDatabase):
	Table(pDatabase, "playerGameData")
{
}

//-------------------------------------------------------------------------------------
Table_PlayerGameData::~Table_PlayerGameData()
{
}

//-------------------------------------------------------------------------------------
bool Table_PlayerGameData::initialize()
{
	return Table::initialize();
}

//-------------------------------------------------------------------------------------
void Table_PlayerGameData::finalise()
{
	Table::finalise();
}

//-------------------------------------------------------------------------------------
bool Table_PlayerGameData::syncTableIndexs()
{
	bson_t keys;
	mongoc_index_opt_t opt;

	bson_init(&keys);
	mongoc_index_opt_init(&opt);

	opt.unique = true;
	bson_append_int32(&keys, "playerID", -1, 1);
	bson_append_int32(&keys, "gameID", -1, 1);
	bson_append_int32(&keys, "gameMode", -1, 1);
	bool ret = createIndex(&keys, &opt);

	bson_destroy(&keys);

	return ret;
}

//-------------------------------------------------------------------------------------
bool Table_PlayerGameData::syncTable()
{
	return Table::syncTable();
}

//-------------------------------------------------------------------------------------
void Table_PlayerGameData::writePlayerGameData(Session* pSession, uint64 gameID, uint32 gameMode, uint64 playerID, int32 score, int32 topscore, int32 victory, int32 defeat)
{
		bson_t doc;
		bson_init(&doc);
		BSON_APPEND_INT64(&doc, "playerID", playerID);
		BSON_APPEND_INT64(&doc, "gameID", gameID);
		BSON_APPEND_INT32(&doc, "gameMode", gameMode);
		BSON_APPEND_INT32(&doc, "score", score);
		BSON_APPEND_INT32(&doc, "topscore", topscore);
		BSON_APPEND_INT32(&doc, "victory", victory);
		BSON_APPEND_INT32(&doc, "defeat", defeat);

		bson_t cond;
		bson_init(&cond);
		BSON_APPEND_INT64(&cond, "playerID", playerID);
		BSON_APPEND_INT64(&cond, "gameID", gameID);
		BSON_APPEND_INT32(&cond, "gameMode", gameMode);

		bool result = updateOrInsert(&cond, &doc);
		bson_destroy(&doc);
		bson_destroy(&cond);

		DEBUG_MSG(fmt::format("Table_PlayerGameData::writePlayerGameData(): playerID={}, gameID={}, gameMode={}, score={}, topscore={}, victory={}, defeat={}!\n",
			playerID, gameID, gameMode, score, topscore, victory, defeat));
}

//-------------------------------------------------------------------------------------
void Table_PlayerGameData::queryPlayerGameData(Session* pSession, uint64 gameID, uint32 gameMode, uint64 playerID)
{
	int32 score = 0;
	int32 topscore = 0;
	int32 victory = 0; 
	int32 defeat = 0;

	ServerError resultCode = ServerError::OK;

	mongoc_cursor_t *cursor = NULL;

	bson_t cond;
	bson_init(&cond);
	BSON_APPEND_INT64(&cond, "playerID", playerID);
	BSON_APPEND_INT64(&cond, "gameID", gameID);
	BSON_APPEND_INT32(&cond, "gameMode", gameMode);

	bool result = query(&cond, cursor, 0, 0, 0, NULL);

	bson_destroy(&cond);

	if (result)
	{
		const bson_t *doc = NULL;
		bson_error_t  error;
		while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &doc)) {
			break;
		}

		if (mongoc_cursor_error(cursor, &error))
		{
			ERROR_MSG(fmt::format("Table_PlayerGameData::queryPlayerGameData(): An error occurred: {}\n", error.message));
			resultCode = ServerError::OBJECT_NOT_FOUND;
		}
		else
		{
			if (doc == NULL)
			{
				resultCode = ServerError::OBJECT_NOT_FOUND;
			}
			else
			{
				bson_iter_t iter;
				bson_iter_init(&iter, doc);

				if (bson_iter_find(&iter, "score"))
				{
					score = bson_iter_int32(&iter);
				}
				else if (bson_iter_find(&iter, "topscore"))
				{
					topscore = bson_iter_int32(&iter);
				}
				else if (bson_iter_find(&iter, "victory"))
				{
					victory = bson_iter_int32(&iter);
				}
				else if (bson_iter_find(&iter, "defeat"))
				{
					defeat = bson_iter_int32(&iter);
				}
			}
		}
	}
	else
	{
		resultCode = ServerError::OBJECT_NOT_FOUND;
	}

	if (cursor)
		mongoc_cursor_destroy(cursor);

	CMD_Halls_OnQueryPlayerGameDataCB res_packet;
	res_packet.set_playerid(playerID);
	res_packet.set_score(score);
	res_packet.set_errcode(resultCode);
	res_packet.set_topscore(topscore);
	res_packet.set_gameid(gameID);
	res_packet.set_gamemode(gameMode);
	res_packet.set_victory(victory);
	res_packet.set_defeat(defeat);

	pSession->sendPacket(CMD::Halls_OnQueryPlayerGameDataCB, res_packet);
}

//-------------------------------------------------------------------------------------
}
