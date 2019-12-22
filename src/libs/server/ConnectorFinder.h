#ifndef X_CONNECTOR_FINDER_H
#define X_CONNECTOR_FINDER_H

#include "common/common.h"
#include "protos/Commands.pb.h"

namespace XServer {

class XServerBase;

struct ServerInfo;

class ConnectorFinder
{
public:
	ConnectorFinder(XServerBase* pServer);
	virtual ~ConnectorFinder();

	void onFindingTick(void* userargs);
	void onTestingTick(void* userargs);
	void onCheckingTick(void* userargs);

	ServerInfo* calcBestServer();

	ServerInfo* found() {
		return found_;
	}

protected:
	XServerBase* pXServer_;

	struct event * timerEvent_;

	std::vector<ServerInfo*> connectorInfos_;

	ServerInfo* found_;

	int attempt_;
};

}

#endif // X_CONNECTOR_FINDER_H
