#include "Database.h"
#include "Table.h"
#include "Table_Accounts.h"
#include "Table_Onlines.h"
#include "Table_PlayerGameData.h"
#include "XServerApp.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Database::Database():
	db_host_(),
	db_port_(0),
	db_username_(),
	db_password_(),
	db_name_(),
	pMongoClient_(NULL),
	pDatabase_(NULL),
	tables_()
{
}

//-------------------------------------------------------------------------------------
Database::~Database()
{
	finalise();
}

//-------------------------------------------------------------------------------------
bool Database::initialize(std::string db_host, int db_port, std::string db_username, std::string db_password, std::string db_name)
{
	db_host_ = db_host;
	db_port_ = db_port;
	db_username_ = db_username;
	db_password_ = db_password;
	db_name_ = db_name;

	std::string debuglog = fmt::format("Database::initialize(): db_name={}, db_username={}, db_addr={}:{}\n",
		db_name, db_username, db_host, db_port);

	DEBUG_MSG(debuglog);

#if X_PLATFORM == PLATFORM_WIN32
	printf("%s", debuglog.c_str());
#endif

	std::string urlstr = fmt::format("mongodb://{}:{}@{}{}/?authSource=admin", db_username_,
		db_password_, db_host_,
		(db_port_ > 0 ? fmt::format(":{}", db_port_): "")).c_str();

	// 建立与mongodb的连接，uristr表示mongodb的URI地址，例如：mongodb://127.0.0.1:8888/
	pMongoClient_ = mongoc_client_new(urlstr.c_str());

	if (!pMongoClient_)
	{
		debuglog = fmt::format("Database::initialize(): connect to mangoserver error! url={}\n",
			urlstr);

		ERROR_MSG(debuglog);

#if X_PLATFORM == PLATFORM_WIN32
		printf("%s", debuglog.c_str());
#endif
		return false;
	}

	pDatabase_ = mongoc_client_get_database(pMongoClient_, db_name_.c_str());
	if (!pDatabase_)
	{
		debuglog = fmt::format("Database::initialize(): get database({}) error!\n",
			db_name_.c_str());

		ERROR_MSG(debuglog);

#if X_PLATFORM == PLATFORM_WIN32
		printf("%s", debuglog.c_str());
#endif
		return false;
	}

	bson_t *command = BCON_NEW("ping", BCON_INT32(1));

	bson_t reply;
	bson_error_t error;
	bool retval = mongoc_client_command_simple(pMongoClient_, "admin", command, NULL, &reply, &error);

	if (!retval) 
	{
		bson_destroy(command);
		ERROR_MSG(fmt::format("Database::initialize(): ping error: {}\n", error.message));
		return false;
	}

	bson_destroy(command);
	bson_destroy(&reply);

	return retval && initTables();
}

//-------------------------------------------------------------------------------------
bool Database::initTables()
{
	Table* pTable = new Table_Accounts(this);

	if (!pTable->initialize())
		return false;

	tables_[pTable->db_tableName()] = pTable;

	pTable = new Table_Onlines(this);

	if (!pTable->initialize())
		return false;

	tables_[pTable->db_tableName()] = pTable;
	
	pTable = new Table_PlayerGameData(this);

	if (!pTable->initialize())
		return false;

	tables_[pTable->db_tableName()] = pTable;
	return pTable != NULL;
}

//-------------------------------------------------------------------------------------
void Database::finalise()
{
	for (auto& item : tables_)
	{
		item.second->finalise();
		delete item.second;
	}

	tables_.clear();

	if (pMongoClient())
	{
		if(pDatabase_)
			mongoc_database_destroy(pDatabase_);

		mongoc_client_destroy(pMongoClient_);

		pMongoClient_ = NULL;
		pDatabase_ = NULL;
	}
}

//-------------------------------------------------------------------------------------
bool Database::command(const std::string& db_tableName, const bson_t *command, bson_t &reply)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(command, NULL);

		ERROR_MSG(fmt::format("Database::command(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->command(command, reply);
}

//-------------------------------------------------------------------------------------
bool Database::insert(const std::string& db_tableName, const bson_t *doc)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(doc, NULL);

		ERROR_MSG(fmt::format("Database::insert(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->insert(doc);
}

//-------------------------------------------------------------------------------------
bool Database::query(const std::string& db_tableName, const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
	unsigned int limit, unsigned int batch_size,
	const bson_t *fields)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("Database::query(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->query(cond, cursor, skip, limit, batch_size, fields);
}

//-------------------------------------------------------------------------------------
bool Database::update(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("Database::update(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->update(cond, updatedoc);
}

//-------------------------------------------------------------------------------------
bool Database::updateOrInsert(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("Database::updateOrInsert(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->updateOrInsert(cond, updatedoc);
}

//-------------------------------------------------------------------------------------
bool Database::deleteOnce(const std::string& db_tableName, const bson_t *cond)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("Database::deleteOnce(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->deleteOnce(cond);
}

//-------------------------------------------------------------------------------------
bool Database::deleteAll(const std::string& db_tableName, const bson_t *cond)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("Database::deleteAll(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->deleteAll(cond);
}

//-------------------------------------------------------------------------------------
bool Database::count(const std::string& db_tableName, const bson_t *cond, long long &count)
{
	auto iter = tables_.find(db_tableName);

	if (iter == tables_.end())
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("Database::count(): not found table: {}, command: {}\n",
			db_tableName, commandStr));

		bson_free(commandStr);
		return false;
	}

	return iter->second->count(cond, count);
}

//-------------------------------------------------------------------------------------
}
