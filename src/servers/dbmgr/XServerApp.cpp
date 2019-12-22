#include "XServerApp.h"
#include "Database.h"
#include "Table_Accounts.h"
#include "Table_Onlines.h"
#include "Table_PlayerGameData.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "server/XServerBase.h"
#include "protos/Commands.pb.h"
#include "event/Session.h"
#include "event/TcpSocket.h"
#include "log/XLog.h"
#include "resmgr/ResMgr.h"
#include "common/threadpool.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XServerApp::XServerApp():
	databases_()
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
	return init_db();
}

//-------------------------------------------------------------------------------------
bool XServerApp::init_db()
{
	mongoc_init();

	Database* pDB = new Database();

	if (!pDB->initialize(ResMgr::getSingleton().serverConfig().db_host, ResMgr::getSingleton().serverConfig().db_port, 
		ResMgr::getSingleton().serverConfig().db_username, ResMgr::getSingleton().serverConfig().db_password, 
		ResMgr::getSingleton().serverConfig().db_name))
		return false;

	databases_["default"] = pDB;

	return pDB != NULL;
}

//-------------------------------------------------------------------------------------
void XServerApp::finalise(void)
{
	for (auto& item : databases_)
	{
		item.second->finalise();
		delete item.second;
	}

	databases_.clear();

	mongoc_cleanup();

	XServerBase::finalise();
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionWriteAccount(Session* pSession, const CMD_Dbmgr_WriteAccount& packet)
{
	findDefaultDB()->findAccountTable()->writeAccount(pSession, packet.commitaccountname(), packet.password(), packet.datas(), "", 1, 0, 0);
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionQueryAccount(Session* pSession, const CMD_Dbmgr_QueryAccount& packet)
{
	findDefaultDB()->findAccountTable()->queryAccount(pSession, packet.commitaccountname(), packet.password(), packet.queryappid(), packet.querytype(), packet.accountid());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionUpdateAccountData(Session* pSession, const CMD_Dbmgr_UpdateAccountData& packet)
{
	findDefaultDB()->findAccountTable()->updateAccountData(pSession, packet.commitaccountname(), packet.playername(), packet.modelid(), packet.exp(), packet.gold());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionWritePlayerGameData(Session* pSession, const CMD_Dbmgr_WritePlayerGameData& packet)
{
	findDefaultDB()->findPlayerGameDataTable()->writePlayerGameData(pSession, packet.gameid(), packet.gamemode(), packet.playerid(), packet.score(), packet.topscore(), packet.victory(), packet.defeat());
}

//-------------------------------------------------------------------------------------
void XServerApp::onSessionQueryPlayerGameData(Session* pSession, const CMD_Dbmgr_QueryPlayerGameData& packet)
{
	findDefaultDB()->findPlayerGameDataTable()->queryPlayerGameData(pSession, packet.gameid(), packet.gamemode(), packet.playerid());
}

//-------------------------------------------------------------------------------------
bool XServerApp::command(const std::string& db_tableName, const bson_t *command, bson_t &reply, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(command, NULL);

		ERROR_MSG(fmt::format("XServerApp::command(): not found database: {}, command: {}\n",
			databaseName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->command(db_tableName, command, reply);
}

//-------------------------------------------------------------------------------------
bool XServerApp::insert(const std::string& db_tableName, const bson_t *doc, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(doc, NULL);

		ERROR_MSG(fmt::format("XServerApp::insert(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->insert(db_tableName, doc);
}

//-------------------------------------------------------------------------------------
bool XServerApp::query(const std::string& db_tableName, const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
	unsigned int limit, unsigned int batch_size,
	const bson_t *fields, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("XServerApp::query(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->query(db_tableName, cond, cursor, skip, limit, batch_size, fields);
}

//-------------------------------------------------------------------------------------
bool XServerApp::update(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("XServerApp::update(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->update(db_tableName, cond, updatedoc);
}

//-------------------------------------------------------------------------------------
bool XServerApp::updateOrInsert(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("XServerApp::updateOrInsert(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->updateOrInsert(db_tableName, cond, updatedoc);
}

//-------------------------------------------------------------------------------------
bool XServerApp::deleteOnce(const std::string& db_tableName, const bson_t *cond, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("XServerApp::deleteOnce(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->deleteOnce(db_tableName, cond);
}

//-------------------------------------------------------------------------------------
bool XServerApp::deleteAll(const std::string& db_tableName, const bson_t *cond, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("XServerApp::deleteAll(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->deleteAll(db_tableName, cond);
}

//-------------------------------------------------------------------------------------
bool XServerApp::count(const std::string& db_tableName, const bson_t *cond, long long &count, const std::string& databaseName)
{
	auto iter = databases_.find(databaseName);

	if (iter == databases_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("XServerApp::count(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->count(db_tableName, cond, count);
}

//-------------------------------------------------------------------------------------
}
