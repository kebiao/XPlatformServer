#ifndef X_SERVER_APP_H
#define X_SERVER_APP_H

#include "common/common.h"
#include "common/singleton.h"
#include "server/XServerBase.h"

namespace XServer {

class XRobot;

class XServerApp : public XServerBase
{
public:
	XServerApp();
	~XServerApp();
	
	virtual NetworkInterface* createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal) override;

	bool initialize(ServerAppID appID, ServerType appType, ServerGroupID appGID, const std::string& appName, int32 botsNum, int32 perNum);
	virtual void finalise() override;

	virtual bool initializeBegin() override;
	virtual bool inInitialize() override;
	virtual bool initializeEnd() override;

	virtual void onTick(void* userargs) override;

protected:
	int32 botsNum_;
	int32 perNum_;

	std::vector<XRobot*> bots_;
};

}

#endif // X_SERVER_APP_H
