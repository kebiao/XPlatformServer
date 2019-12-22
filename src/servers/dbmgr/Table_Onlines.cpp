#include "Table_Onlines.h"
#include "Database.h"
#include "XServerApp.h"
#include "log/XLog.h"
#include "server/XServerBase.h"
#include "server/ServerMgr.h"
#include "event/Session.h"

namespace XServer {

//-------------------------------------------------------------------------------------
Table_Onlines::Table_Onlines(Database* pDatabase):
	Table(pDatabase, "onlines")
{
}

//-------------------------------------------------------------------------------------
Table_Onlines::~Table_Onlines()
{
}

//-------------------------------------------------------------------------------------
bool Table_Onlines::initialize()
{
	return Table::initialize();
}

//-------------------------------------------------------------------------------------
void Table_Onlines::finalise()
{
	Table::finalise();
}

//-------------------------------------------------------------------------------------
bool Table_Onlines::syncTableIndexs()
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
bool Table_Onlines::syncTable()
{
	bson_t options;
	bson_init(&options);
	deleteAll(&options);
	
	return Table::syncTable();
}

//-------------------------------------------------------------------------------------
bool Table_Onlines::writeLog(const std::string& name, uint64 objectID, ServerAppID appID)
{
	bson_t doc;
	bson_init(&doc);
	bson_append_utf8(&doc, "name", (int)strlen("name"), name.data(), name.size());
	BSON_APPEND_INT64(&doc, "objectID", objectID);
	BSON_APPEND_INT64(&doc, "appID", appID);

	bool result = insert(&doc);
	bson_destroy(&doc);
	return result;
}

//-------------------------------------------------------------------------------------
bool Table_Onlines::queryLog(const std::string& name, uint64& objectID, ServerAppID& appID)
{
	objectID = 0;
	appID = 0;

	mongoc_cursor_t *cursor = NULL;

	bson_t cond;
	bson_init(&cond);
	bson_append_utf8(&cond, "name", (int)strlen("name"), name.c_str(), name.size());

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
			ERROR_MSG(fmt::format("XServerApp::queryLog(): An error occurred: {}\n", error.message));

			if (cursor)
				mongoc_cursor_destroy(cursor);

			return false;
		}
		else
		{
			if (doc == NULL)
			{
				if (cursor)
					mongoc_cursor_destroy(cursor);

				return false;
			}
			else
			{
				bson_iter_t iter;
				bson_iter_init(&iter, doc);

				if (bson_iter_find(&iter, "objectID"))
				{
					objectID = bson_iter_int64(&iter);
				}
				else if (bson_iter_find(&iter, "appID"))
				{
					appID = bson_iter_int64(&iter);
				}
			}
		}
	}
	else
	{
		if (cursor)
			mongoc_cursor_destroy(cursor);

		return false;
	}

	if (cursor)
		mongoc_cursor_destroy(cursor);

	return true;
}

//-------------------------------------------------------------------------------------
bool Table_Onlines::eraseLog(const std::string& name)
{
	bson_t cond;
	bson_init(&cond);
	bson_append_utf8(&cond, "name", (int)strlen("name"), name.c_str(), name.size());

	bool result = deleteOnce(&cond);

	bson_destroy(&cond);

	return result;
}

//-------------------------------------------------------------------------------------
}
