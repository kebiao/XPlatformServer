#ifndef X_NETWORKINTERFACE_H
#define X_NETWORKINTERFACE_H

#include "common/common.h"

namespace XServer {

class EventDispatcher;
class TcpSocket;
class Session;

class NetworkInterface 
{
public:
	NetworkInterface(EventDispatcher* pEventDispatcher, bool isInternalNetwork);
	virtual ~NetworkInterface();

	bool initialize(const std::string& addr, uint16 port);
	void finalise();

	bool isInternalNetwork() const {
		return isInternalNetwork_;
	}

	EventDispatcher* pEventDispatcher() {
		return pEventDispatcher_;
	}

	virtual void onListenError();

	typedef std::map<SessionID, Session*> SessionMap;
	SessionMap& sessions() {
		return sessions_;
	}

	size_t sessionNum() const {
		return sessions_.size();
	}

	Session* findSession(SessionID id)
	{
		SessionMap::iterator iter = sessions_.find(id);
		if (iter != sessions_.end())
			return iter->second;

		return NULL;
	}

	bool hasSession(SessionID id)
	{
		return findSession(id) != NULL;
	}

	Session* createSession(evutil_socket_t sock);
	virtual Session* createSession_(SessionID sessionID, evutil_socket_t sock);
	bool addSession(SessionID id, Session* pSession);
	bool removeSession(SessionID id);

	void checkSessions();

	uint16 getListenerPort();
	std::string getListenerIP();

private:
	static void listenEventCallback(
	struct evconnlistener *listener,
		evutil_socket_t sock,
	struct sockaddr *addr,
		int len,
		void *ctx);

	static void listenErrorCallback(struct evconnlistener *listener, void *ctx);

protected:
	EventDispatcher* pEventDispatcher_;
	struct evconnlistener *pEventListener_;

	SessionMap sessions_;
	bool isInternalNetwork_;
};

}

#endif // X_NETWORKINTERFACE_H
