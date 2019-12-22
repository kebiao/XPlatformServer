#ifndef X_DATABASE_TABLE_ACCOUNT_H
#define X_DATABASE_TABLE_ACCOUNT_H

#include "Table.h"

namespace XServer {

class Table_Accounts : public Table
{
public:
	Table_Accounts(Database* pDatabase);
	virtual ~Table_Accounts();

	virtual bool initialize() override;
	virtual void finalise() override;
	
	virtual bool syncTableIndexs() override;

	void writeAccount(Session* pSession, const std::string& commitAccountName, const std::string& password, const std::string& datas, const std::string& playerName, int modelID, int exp, int gold);
	void queryAccount(Session* pSession, const std::string& commitAccountName, const std::string& password, ServerAppID queryAppID, int queryType, ObjectID objectID);
	void updateAccountData(Session* pSession, const std::string& commitAccountName, const std::string& playerName, int modelID, int exp, int gold);

protected:
};

}

#endif // X_DATABASE_TABLE_ACCOUNT_H
