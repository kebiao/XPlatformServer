#ifndef X_DATABASE_H
#define X_DATABASE_H

#include "common/common.h"
#include "mongoc.h"

namespace XServer {

class Table;
class Table_Accounts;
class Table_Onlines;
class Table_PlayerGameData;

class Database
{
public:
	Database();
	virtual ~Database();

	virtual bool initialize(std::string db_host, int db_port, std::string db_username, std::string db_password, std::string db_name);
	virtual bool initTables();

	virtual void finalise();
	
	virtual bool command(const std::string& db_tableName, const bson_t *command, bson_t &reply);

	virtual bool insert(const std::string& db_tableName, const bson_t *doc);

	virtual bool query(const std::string& db_tableName, const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
		unsigned int limit, unsigned int batch_size,
		const bson_t *fields);

	virtual bool update(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc);
	virtual bool updateOrInsert(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc);

	virtual bool deleteOnce(const std::string& db_tableName, const bson_t *cond);
	virtual bool deleteAll(const std::string& db_tableName, const bson_t *cond);

	virtual bool count(const std::string& db_tableName, const bson_t *cond, long long &count);

	mongoc_client_t* pMongoClient() { return pMongoClient_; }
	mongoc_database_t* pDatabase() { return pDatabase_; }
	
	const std::string& db_name() const {
		return db_name_;
	}

	Table* findTable(const std::string& db_tableName)
	{
		auto iter = tables_.find(db_tableName);
		if (iter == tables_.end())
		{
			return NULL;
		}

		return iter->second;
	}

	Table_Accounts* findAccountTable()
	{
		static Table_Accounts* pTable = NULL;
		
		if(!pTable)
			pTable = (Table_Accounts*)findTable("accounts");

		return pTable;
	}

	Table_Onlines* findOnlinesTable()
	{
		static Table_Onlines* pTable = NULL;

		if (!pTable)
			pTable = (Table_Onlines*)findTable("onlines");

		return pTable;
	}

	Table_PlayerGameData* findPlayerGameDataTable()
	{
		static Table_PlayerGameData* pTable = NULL;

		if (!pTable)
			pTable = (Table_PlayerGameData*)findTable("playerGameData");

		return pTable;
	}

protected:
	std::string db_host_;
	int db_port_;
	std::string db_username_;
	std::string db_password_;
	std::string db_name_;
	
	mongoc_client_t *pMongoClient_;
	mongoc_database_t *pDatabase_;

	std::tr1::unordered_map<std::string, Table*> tables_;
};

}

#endif // X_DATABASE_H
