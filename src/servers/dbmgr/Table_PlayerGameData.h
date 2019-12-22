#ifndef X_DATABASE_TABLE_PLAYERGAMEDATA_H
#define X_DATABASE_TABLE_PLAYERGAMEDATA_H

#include "Table.h"

namespace XServer {

class Table_PlayerGameData : public Table
{
public:
	Table_PlayerGameData(Database* pDatabase);
	virtual ~Table_PlayerGameData();

	virtual bool initialize() override;
	virtual void finalise() override;
	
	virtual bool syncTableIndexs() override;
	virtual bool syncTable() override;
	
	void writePlayerGameData(Session* pSession, uint64 gameID, uint32 gameMode, uint64 playerID, int32 score, int32 topscore, int32 victory, int32 defeat);
	void queryPlayerGameData(Session* pSession, uint64 gameID, uint32 gameMode, uint64 playerID);

protected:
};

}

#endif // X_DATABASE_TABLE_PLAYERGAMEDATA_H
