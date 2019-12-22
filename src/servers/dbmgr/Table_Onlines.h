#ifndef X_DATABASE_TABLE_ONLINES_H
#define X_DATABASE_TABLE_ONLINES_H

#include "Table.h"

namespace XServer {

class Table_Onlines : public Table
{
public:
	Table_Onlines(Database* pDatabase);
	virtual ~Table_Onlines();

	virtual bool initialize() override;
	virtual void finalise() override;
	
	virtual bool syncTableIndexs() override;
	virtual bool syncTable() override;
	
	bool writeLog(const std::string& name, uint64 objectID, ServerAppID appID);
	bool queryLog(const std::string& name, uint64& objectID, ServerAppID& appID);
	bool eraseLog(const std::string& name);

protected:
};

}

#endif // X_DATABASE_TABLE_ONLINES_H
