#ifndef X_SERVER_APP_H
#define X_SERVER_APP_H

#include "common/common.h"
#include "server/XServerBase.h"
#include "mongoc.h"

namespace XServer {

class Database;

class XServerApp : public XServerBase
{
public:
	XServerApp();
	~XServerApp();
	
	virtual NetworkInterface* createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal) override;

	virtual bool initializeBegin() override;
	virtual bool inInitialize() override;
	virtual bool initializeEnd() override;
	virtual void finalise() override;

	bool init_db();

	void onSessionWriteAccount(Session* pSession, const CMD_Dbmgr_WriteAccount& packet);
	void onSessionQueryAccount(Session* pSession, const CMD_Dbmgr_QueryAccount& packet);
	void onSessionUpdateAccountData(Session* pSession, const CMD_Dbmgr_UpdateAccountData& packet);

	void onSessionWritePlayerGameData(Session* pSession, const CMD_Dbmgr_WritePlayerGameData& packet);
	void onSessionQueryPlayerGameData(Session* pSession, const CMD_Dbmgr_QueryPlayerGameData& packet);

	bool command(const std::string& db_tableName, const bson_t *command, bson_t &reply, const std::string& databaseName = "default");

	virtual bool insert(const std::string& db_tableName, const bson_t *doc, const std::string& databaseName = "default");

	virtual bool query(const std::string& db_tableName, const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
		unsigned int limit, unsigned int batch_size,
		const bson_t *fields, const std::string& databaseName = "default");

	virtual bool update(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc, const std::string& databaseName = "default");
	virtual bool updateOrInsert(const std::string& db_tableName, const bson_t *cond, const bson_t *updatedoc, const std::string& databaseName = "default");

	virtual bool deleteOnce(const std::string& db_tableName, const bson_t *cond, const std::string& databaseName = "default");
	virtual bool deleteAll(const std::string& db_tableName, const bson_t *cond, const std::string& databaseName = "default");

	virtual bool count(const std::string& db_tableName, const bson_t *cond, long long &count, const std::string& databaseName = "default");

	Database* findDefaultDB()
	{
		static Database* db = NULL;

		if (!db)
		{
			auto iter = databases_.find("default");

			if (iter == databases_.end())
				return NULL;

			db = iter->second;
		}

		return db;
	}

protected:
	std::tr1::unordered_map<std::string, Database*> databases_;

};

}

#endif // X_SERVER_APP_H
