#ifndef X_DATABASE_TABLE_H
#define X_DATABASE_TABLE_H

#include "common/common.h"
#include "mongoc.h"

namespace XServer {

class Database;
class Session;

class Table
{
public:
	Table(Database* pDatabase, std::string db_tableName);
	virtual ~Table();

	virtual bool initialize();
	virtual void finalise();
	
	virtual bool command(const bson_t *command, bson_t &reply);

	bool createIndex(const bson_t *keys, const mongoc_index_opt_t *opt);
	bool dropIndex(const std::string& indexName);

	virtual bool insert(const bson_t *doc);

	virtual bool query(const bson_t *cond, mongoc_cursor_t *&cursor, unsigned int skip,
		unsigned int limit, unsigned int batch_size,
		const bson_t *fields);

	virtual bool update(const bson_t *cond, const bson_t *updatedoc);
	virtual bool updateOrInsert(const bson_t *cond, const bson_t *updatedoc);

	virtual bool deleteOnce(const bson_t *cond);
	virtual bool deleteAll(const bson_t *cond);

	virtual bool count(const bson_t *cond, long long &count);

	virtual bool syncTableIndexs();
	virtual bool syncTable();

	mongoc_collection_t *collection() {
		return collection_;
	}

	const std::string& db_tableName() const {
		return db_tableName_;
	}

	Database* pDatabase() const {
		return pDatabase_;
	}

protected:
	Database* pDatabase_;
	std::string db_tableName_;
	mongoc_collection_t *collection_;
};

}

#endif // X_DATABASE_TABLE_H
