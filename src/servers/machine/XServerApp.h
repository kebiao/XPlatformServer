#ifndef X_SERVER_APP_H
#define X_SERVER_APP_H

#include "common/common.h"
#include "common/singleton.h"
#include "server/XServerBase.h"


namespace XServer {

class XServerApp : public XServerBase
{
	struct ThreadCreateRoomResult
	{
		ThreadCreateRoomResult()
		{
			errcode = ServerError::OK;
			sys_error = 0;

			ip = "";
			port = 0;

			pid = 0;
			roomID = 0;
		}

		ThreadCreateRoomResult(const ThreadCreateRoomResult& result)
		{
			errcode = result.errcode;
			sys_error = result.sys_error;

			ip = result.ip;
			port = result.port;

			roomID = result.roomID;
			pid = result.pid;
		}

		ServerError errcode;
		int sys_error;
		std::string ip;
		int port;
		unsigned long pid;
		ObjectID roomID;
	};

	struct CreateRoomContext
	{
		enum State
		{
			WaitCreate = 0,
			Creating = 1,
			Finished = 2,
			CreateTimeout = 3,
			GameTimeout = 4,
			WaitReport = 5,
			WaitGameOver = 6,
			WaitKillGameServer = 7,
		};

		CreateRoomContext()
		{
			state = WaitCreate;
			lastTime = getTimeStamp();
		}

		bool isCreateTimeout()
		{
			time_t diff = getTimeStamp() - lastTime;
			return requestInfos.gametime() * TIME_SECONDS < diff;
		}

		bool isGameTimeout()
		{
			time_t diff = getTimeStamp() - lastTime;
			return (requestInfos.gametime() * TIME_SECONDS + (60 * TIME_SECONDS)) < diff;
		}

		bool isKillTimeout()
		{
			return getTimeStamp() >= lastTime;
		}

		void setState(State s) {
			state = s;
		}

		void setKillTime(time_t secs)
		{
			state = WaitKillGameServer;
			lastTime = getTimeStamp() + (secs * TIME_SECONDS);
		}

		CMD_Machine_RequestCreateRoom requestInfos;
		State state;
		time_t lastTime;
		
		std::future<ThreadCreateRoomResult> createResultFuture;
		ThreadCreateRoomResult createResult;
	};

public:
	XServerApp();
	~XServerApp();
	
	virtual NetworkInterface* createNetworkInterface(EventDispatcher* pEventDispatcher, bool isInternal) override;

	virtual bool initializeBegin() override;
	virtual bool inInitialize() override;
	virtual bool initializeEnd() override;
	virtual void finalise() override;

	virtual void onUpdateServerInfoToSession(CMD_UpdateServerInfos& infos) override;

	virtual void onTick(void* userargs) override;

	bool addPendingCreateRoom(const CMD_Machine_RequestCreateRoom& packet);
	bool removePendingCreateRoom(ObjectID roomID);
	CreateRoomContext* findPendingCreateRoom(ObjectID roomID);

	void onSessionRequestCreateRoom(Session* pSession, const CMD_Machine_RequestCreateRoom& packet);
	void onSessionRoomSrvReportAddr(Session* pSession, const CMD_Machine_RoomSrvReportAddr& packet);
	void onSessionRoomSrvGameOverReport(Session* pSession, const CMD_Machine_OnRoomSrvGameOverReport& packet);

	bool startRoomServerProcess(CreateRoomContext* context);

#if X_PLATFORM != PLATFORM_WIN32
	static ThreadCreateRoomResult startLinuxProcess(CMD_Machine_RequestCreateRoom context);
#else
	static ThreadCreateRoomResult startWindowsProcess(CMD_Machine_RequestCreateRoom context);
#endif

	bool killProcess(unsigned long pid);

	void onCreateRoomServerProcessFinished(CreateRoomContext& pcr);

protected:
	std::map<ObjectID, CreateRoomContext> createRoomContexts_;
};

}

#endif // X_SERVER_APP_H
