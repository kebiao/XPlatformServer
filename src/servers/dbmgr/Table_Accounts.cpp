#include "Table_Accounts.h"
#include "Database.h"
#include "XServerApp.h"
#include "Table_Onlines.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"
#include "event/Session.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Table_Accounts::Table_Accounts(Database* pDatabase):
	Table(pDatabase, "accounts")
{
}

//-------------------------------------------------------------------------------------
Table_Accounts::~Table_Accounts()
{
}

//-------------------------------------------------------------------------------------
bool Table_Accounts::initialize()
{
	return Table::initialize();
}

//-------------------------------------------------------------------------------------
void Table_Accounts::finalise()
{
	Table::finalise();
}

//-------------------------------------------------------------------------------------
bool Table_Accounts::syncTableIndexs()
{
	bson_t keys;
	mongoc_index_opt_t opt;

	bson_init(&keys);
	mongoc_index_opt_init(&opt);

	opt.unique = true;
	bson_append_int32(&keys, "name", -1, 1);
	bool ret = createIndex(&keys, &opt);

	bson_destroy(&keys);

	return ret;
}

//-------------------------------------------------------------------------------------
void Table_Accounts::writeAccount(Session* pSession, const std::string& commitAccountName, const std::string& password, const std::string& datas,
	const std::string& playerName, int modelID, int exp, int gold)
{
	if (pSession->appType() == ServerType::SERVER_TYPE_LOGIN)
	{
		bson_t doc;
		bson_init(&doc);
		bson_append_utf8(&doc, "name", (int)strlen("name"), commitAccountName.c_str(), commitAccountName.size());
		bson_append_utf8(&doc, "password", (int)strlen("password"), password.c_str(), password.size());
		bson_append_utf8(&doc, "datas", (int)strlen("datas"), datas.data(), datas.size());
		bson_append_utf8(&doc, "playerName", (int)strlen("playerName"), playerName.data(), playerName.size());
		BSON_APPEND_INT32(&doc, "modelID", modelID);
		BSON_APPEND_INT32(&doc, "exp", exp);
		BSON_APPEND_INT32(&doc, "gold", gold);

		bool result = insert(&doc);
		bson_destroy(&doc);

		CMD_Login_OnSignupCB res_packet;
		res_packet.set_commitaccountname(commitAccountName);
		res_packet.set_realaccountname(commitAccountName);
		res_packet.set_datas(datas);

		if (result)
			res_packet.set_errcode(ServerError::OK);
		else
			res_packet.set_errcode(ServerError::CREATE_ERROR);

		pSession->sendPacket(CMD::Login_OnSignupCB, res_packet);
		return;
	}

	ERROR_MSG(fmt::format("Table_Accounts::writeAccount(): no support appType({}), commitAccountName={}!\n",
		pSession->typeName(), commitAccountName));
}

//-------------------------------------------------------------------------------------
void Table_Accounts::queryAccount(Session* pSession, const std::string& commitAccountName, const std::string& password, ServerAppID queryAppID, int queryType, ObjectID objectID)
{
	ServerError resultCode = ServerError::OK;

	std::string getPlayerName = "";
	int32 getExp = 0;
	int32 getModelID = 0;
	int32 getGold = 0;
	std::string getDatas = "";

	mongoc_cursor_t *cursor = NULL;

	bson_t cond;
	bson_init(&cond);
	bson_append_utf8(&cond, "name", (int)strlen("name"), commitAccountName.c_str(), commitAccountName.size());

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
			ERROR_MSG(fmt::format("XServerApp::queryAccount(): An error occurred: {}\n", error.message));
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

				if (bson_iter_find(&iter, "password"))
				{
					if (pSession->appType() == ServerType::SERVER_TYPE_LOGIN)
					{
						uint32_t len = 0;
						const char* passwordBuf = bson_iter_utf8(&iter, &len);
						std::string getPassword = std::string(passwordBuf, len);

						if (getPassword != password)
						{
							resultCode = ServerError::AUTH_FAILED;
						}
					}
				}
				else if (bson_iter_find(&iter, "datas"))
				{
					uint32_t len = 0;
					const char* datasBuf = bson_iter_utf8(&iter, &len);
					getDatas = std::string(datasBuf, len);
				}
				else if (bson_iter_find(&iter, "playerName"))
				{
					uint32_t len = 0;
					const char* playerNameBuf = bson_iter_utf8(&iter, &len);
					getPlayerName = std::string(playerNameBuf, len);
				}
				else if (bson_iter_find(&iter, "modelID"))
				{
					getModelID = bson_iter_int32(&iter);
				}
				else if (bson_iter_find(&iter, "exp"))
				{
					getExp = bson_iter_int32(&iter);
				}
				else if (bson_iter_find(&iter, "gold"))
				{
					getGold = bson_iter_int32(&iter);
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

	if (pSession->appType() == ServerType::SERVER_TYPE_LOGIN)
	{
		CMD_Login_OnSigninCB res_packet;
		res_packet.set_commitaccountname(commitAccountName);
		res_packet.set_realaccountname(commitAccountName);
		res_packet.set_datas(getDatas);
		res_packet.set_errcode(resultCode);

		ObjectID oid;
		ServerAppID appID;

		this->pDatabase()->findOnlinesTable()->queryLog(commitAccountName, oid, appID);

		res_packet.set_foundappid(appID);
		res_packet.set_foundobjectid(oid);

		pSession->sendPacket(CMD::Login_OnSigninCB, res_packet);
	}
	else if (pSession->appType() == ServerType::SERVER_TYPE_HALLS)
	{
		this->pDatabase()->findOnlinesTable()->writeLog(commitAccountName, objectID, queryAppID);

		CMD_Halls_OnQueryAccountCB res_packet;
		res_packet.set_commitaccountname(commitAccountName);
		res_packet.set_datas(getDatas);
		res_packet.set_errcode(resultCode);
		res_packet.set_foundobjectid(objectID);
		res_packet.set_playername(getPlayerName);
		res_packet.set_exp(getExp);
		res_packet.set_gold(getGold);
		res_packet.set_modelid(getModelID);
		res_packet.set_querytype(queryType);

		pSession->sendPacket(CMD::Halls_OnQueryAccountCB, res_packet);
	}
	else
	{
		ERROR_MSG(fmt::format("Table_Accounts::queryAccount(): no support appType({}), commitAccountName={}!\n",
			pSession->typeName(), commitAccountName));
	}
}

//-------------------------------------------------------------------------------------
void Table_Accounts::updateAccountData(Session* pSession, const std::string& commitAccountName, const std::string& playerName, int modelID, int exp, int gold)
{
	bson_t doc;
	bson_init(&doc);

	bson_append_utf8(&doc, "playerName", (int)strlen("playerName"), playerName.data(), playerName.size());
	BSON_APPEND_INT32(&doc, "modelID", modelID);
	BSON_APPEND_INT32(&doc, "exp", exp);
	BSON_APPEND_INT32(&doc, "gold", gold);

	bson_t cond;
	bson_init(&cond);
	bson_append_utf8(&cond, "name", (int)strlen("name"), commitAccountName.c_str(), commitAccountName.size());

	bool result = update(&cond, &doc);
	bson_destroy(&doc);
	bson_destroy(&cond);

	DEBUG_MSG(fmt::format("Table_Accounts::updateAccountData(): AccountName={}, playerName={}, modelID={}, exp={}, gold={}!\n",
		commitAccountName, playerName, modelID, exp, gold));
}

//-------------------------------------------------------------------------------------
}
