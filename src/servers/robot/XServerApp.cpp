#include "XServerApp.h"
#include "XRobot.h"
#include "XNetworkInterface.h"
#include "server/ServerMgr.h"
#include "resmgr/ResMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
XServerApp::XServerApp():
	botsNum_(100),
	perNum_(10),
	bots_()
{
}

//-------------------------------------------------------------------------------------
XServerApp::~XServerApp()
{
}

//-------------------------------------------------------------------------------------		
NetworkInterface* XServerApp::createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal)
{
	return new XNetworkInterface(pEventDispatcher, isInternal);
}

//-------------------------------------------------------------------------------------		
bool XServerApp::initialize(ServerAppID appID, ServerType appType, ServerGroupID appGID, const std::string& appName, int32 botsNum, int32 perNum)
{
	if (!XServerBase::initialize(appID, appType, appGID, appName))
		return false;

	botsNum_ = botsNum;
	perNum_ = perNum;
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::inInitialize()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool XServerApp::initializeEnd()
{
	return true;
}

//-------------------------------------------------------------------------------------
void XServerApp::finalise()
{
	XServerBase::finalise();

	for (auto& item : bots_)
	{
		delete item;
	}

	bots_.clear();
}

//-------------------------------------------------------------------------------------
void XServerApp::onTick(void* userargs)
{
	if (!pServerMgr_)
		return;

	int diffsize = botsNum_ - bots_.size();

	if (diffsize > 0)
	{
		diffsize = std::min<int>(diffsize, perNum_);
		for (int i = 0; i < diffsize; ++i)
		{
			XRobot* pXRobot = new XRobot();
			if (!pXRobot->initialize(this))
				return;

			bots_.push_back(pXRobot);
		}
	}
}

//-------------------------------------------------------------------------------------
}
