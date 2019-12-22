#include "Table.h"
#include "Database.h"
#include "XServerApp.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Table::Table(Database* pDatabase, std::string db_tableName):
	pDatabase_(pDatabase),
	db_tableName_(db_tableName),
	collection_(NULL)
{
}

//-------------------------------------------------------------------------------------
Table::~Table()
{
	finalise();
}

//-------------------------------------------------------------------------------------
bool Table::initialize()
{
	bson_error_t  error;

	//如果存在则离开返回
	if (mongoc_database_has_collection(pDatabase_->pDatabase(), db_tableName_.c_str(), &error))
	{
		collection_ = mongoc_client_get_collection(pDatabase_->pMongoClient(), pDatabase_->db_name().c_str(), db_tableName_.c_str());
		return collection_ != NULL;
	}

	//不存在则创建
	return syncTable();
}

//-------------------------------------------------------------------------------------
void Table::finalise()
{
	if(collection_)
		mongoc_collection_destroy(collection_);
}

//-------------------------------------------------------------------------------------
bool Table::syncTableIndexs()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Table::syncTable()
{
	bson_error_t  error;

	bson_t options;
	bson_init(&options);

	collection_ = mongoc_database_create_collection(pDatabase_->pDatabase(), db_tableName_.c_str(), &options, &error);

	if (!collection_)
	{
		ERROR_MSG(fmt::format("Table::initialize: failed to mongoc_database_create_collection({}), error: {}\n", db_tableName_, error.message));
	}

	bson_destroy(&options);

	return collection_ != NULL && syncTableIndexs();
}

//-------------------------------------------------------------------------------------
bool Table::command(const bson_t *command, bson_t &reply)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::command: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	if (!mongoc_collection_command_simple(collection_, command, NULL, &reply,
		&error))
	{
		char *commandStr = bson_as_json(command, NULL);

		ERROR_MSG(fmt::format("{}::command: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::createIndex(const bson_t *keys, const mongoc_index_opt_t *opt)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::createIndex: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;
	bool r = mongoc_collection_create_index(collection_, keys, opt, &error);
	if (!r)
	{
		ERROR_MSG(fmt::format("{}::createIndex(): error: {}\n", db_tableName_, error.message));
	}

	return r;
}

//-------------------------------------------------------------------------------------
bool Table::dropIndex(const std::string& indexName)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::dropIndex: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t  error;
	bool r = mongoc_collection_drop_index(collection_, indexName.c_str(), &error);
	if (!r)
	{
		ERROR_MSG(fmt::format("{}::dropIndex(): error: {}\n", db_tableName_, error.message));
	}

	return r;
}

//-------------------------------------------------------------------------------------
bool Table::insert(const bson_t *doc)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::insert: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	if (!mongoc_collection_insert(collection_, MONGOC_INSERT_NONE, doc, NULL,
		&error))
	{
		char *commandStr = bson_as_json(doc, NULL);

		ERROR_MSG(fmt::format("{}::insert: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::query(const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
	unsigned int limit, unsigned int batch_size,
	const bson_t *fields)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::query: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	cursor = mongoc_collection_find(collection_, MONGOC_QUERY_NONE, skip, limit,
		batch_size, cond, fields, NULL);

	if (!cursor)
	{
		mongoc_cursor_error(cursor, &error);

		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("{}::query: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::update(const bson_t *cond, const bson_t *updatedoc)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::update: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	if (!mongoc_collection_update(collection_, MONGOC_UPDATE_MULTI_UPDATE, cond,
		updatedoc, NULL, &error)) 
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("{}::update: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::updateOrInsert(const bson_t *cond, const bson_t *updatedoc)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::updateOrInsert: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	if (!mongoc_collection_update(collection_, MONGOC_UPDATE_UPSERT, cond,
		updatedoc, NULL, &error))
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("{}::updateOrInsert: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::deleteOnce(const bson_t *cond)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::deleteOnce: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	if (!mongoc_collection_remove(collection_, MONGOC_REMOVE_SINGLE_REMOVE, cond,
		NULL, &error))
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("{}::deleteOnce: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::deleteAll(const bson_t *cond)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::deleteAll: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	if (!mongoc_collection_remove(collection_, MONGOC_REMOVE_NONE, cond,
		NULL, &error))
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("{}::deleteAll: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Table::count(const bson_t *cond, long long &count)
{
	if (!collection_)
	{
		ERROR_MSG(fmt::format("{}::count: collection_ == NULL\n", db_tableName_));
		return false;
	}

	bson_error_t error;

	count = mongoc_collection_count(collection_, MONGOC_QUERY_NONE, cond, 0, 0,
		NULL, &error);

	if (count < 0)
	{
		char *commandStr = bson_as_json(cond, NULL);

		ERROR_MSG(fmt::format("{}::count: failed to run command: {}, error: {}\n", db_tableName_, commandStr, error.message));

		bson_free(commandStr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
}
