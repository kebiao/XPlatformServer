#include "NetworkInterface.h"
#include "EventDispatcher.h"
#include "Session.h"
#include "TcpSocket.h"
#include "log/XLog.h"
#include "resmgr/ResMgr.h"

namespace XServer {

//-------------------------------------------------------------------------------------
NetworkInterface::NetworkInterface(EventDispatcher* pEventDispatcher, bool isInternalNetwork):
pEventDispatcher_(pEventDispatcher),
pEventListener_(NULL),
sessions_(),
isInternalNetwork_(isInternalNetwork)
{
#ifdef _WIN32
	static bool installed = false;
	if (!installed)
	{
		WSADATA wsa_data;
		WSAStartup(0x0201, &wsa_data);
	}
#endif
}

//-------------------------------------------------------------------------------------
NetworkInterface::~NetworkInterface()
{
	finalise();
}

//-------------------------------------------------------------------------------------		
bool NetworkInterface::initialize(const std::string& addr, uint16 port)
{
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	evutil_inet_pton(AF_INET, addr.c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);

	pEventListener_ = evconnlistener_new_bind(pEventDispatcher_->base(),
		listenEventCallback, this,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE, -1,
		(struct sockaddr *)&serverAddr, sizeof(serverAddr));

	if (!pEventListener_)
	{
		ERROR_MSG(fmt::format("NetworkInterface::initialize(): Could not create a listener! addr={}:{}\n", addr, port));
		return false;
	}

	evutil_make_socket_nonblocking(evconnlistener_get_fd(pEventListener_));
	evconnlistener_set_error_cb(pEventListener_, listenErrorCallback);

	std::string info = fmt::format("NetworkInterface::initialize(): Server socket({}) created on IP: {}:{}!\n", 
		(isInternalNetwork_ ? "Internal" : "External"), getListenerIP(), getListenerPort());

	INFO_MSG(info);

#if X_PLATFORM == PLATFORM_WIN32
	printf(info.c_str());
#endif

	return true;
}

//-------------------------------------------------------------------------------------
uint16 NetworkInterface::getListenerPort()
{
	if (!pEventListener_)
		return 0;

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));

	socklen_t len = sizeof(serverAddr);
	getsockname(evconnlistener_get_fd(pEventListener_),
		(struct sockaddr*)&serverAddr, &len);

	return ntohs(serverAddr.sin_port);
}

//-------------------------------------------------------------------------------------
std::string NetworkInterface::getListenerIP()
{
	if (!pEventListener_)
		return "";

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));

	socklen_t len = sizeof(serverAddr);
	getsockname(evconnlistener_get_fd(pEventListener_),
		(struct sockaddr*)&serverAddr, &len);

	std::string addr = inet_ntoa(serverAddr.sin_addr);
	if (addr == "0.0.0.0")
		addr = "localhost";

	return addr;
}

//-------------------------------------------------------------------------------------
void NetworkInterface::finalise(void)
{
	INFO_MSG(fmt::format("NetworkInterface::finalise()\n"));

	for (auto& item : sessions_)
		delete(item.second);

	sessions_.clear();

	if (pEventListener_)
	{
		evconnlistener_free(pEventListener_);
		pEventListener_ = NULL;
	}

	pEventDispatcher_ = NULL;
}

//-------------------------------------------------------------------------------------
Session* NetworkInterface::createSession(evutil_socket_t sock)
{
	return createSession_(uuid(), sock);
}

//-------------------------------------------------------------------------------------
Session* NetworkInterface::createSession_(SessionID sessionID, evutil_socket_t sock)
{
	return new Session(sessionID, sock, this, this->pEventDispatcher());
}

//-------------------------------------------------------------------------------------
void NetworkInterface::listenEventCallback(
	struct evconnlistener *listener,
	evutil_socket_t sock,
	struct sockaddr *addr,
	int len,
	void *ctx)
{
	// 检查是否是服务器ip
	std::string sockIP = TcpSocket::getSocketIP(sock);

	bool isServer = false;

	for (auto& item : ResMgr::getSingleton().serverConfig().server_addresses)
	{
		if (item[item.size() - 1] == '*')
		{
			size_t npos = sockIP.rfind('.', 10);

			if(item.size() >= npos)
			{
				for (int i = 0; i < npos; i++)
					if (item[i] != sockIP[i])
						continue;

				isServer = true;
				break;
			}
		}
		else
		{
			if (item == sockIP)
			{
				isServer = true;
				break;
			}
		}
	}

	NetworkInterface *pNetworkInterface = (NetworkInterface *)ctx;
	evutil_make_socket_nonblocking(sock);

	Session* pSession = pNetworkInterface->createSession(sock);
	pSession->isServer(pNetworkInterface->isInternalNetwork());
	pSession->connected(true);
	pNetworkInterface->addSession(pSession->id(), pSession);

	if (!pSession->initialize())
	{
		pNetworkInterface->removeSession(pSession->id());
		// delete pSession; in removeSession
		return;
	}

	if (pSession->isServer() && !isServer)
	{
		ERROR_MSG(fmt::format("NetworkInterface::listenEventCallback(): Illegal internal address: {}!\n", sockIP));
		pNetworkInterface->removeSession(pSession->id());
		// delete pSession; in removeSession
		return;
	}
}

//-------------------------------------------------------------------------------------
void NetworkInterface::listenErrorCallback(struct evconnlistener *listener, void *ctx)
{
	NetworkInterface *pNetworkInterface = (NetworkInterface *)ctx;

	if (pNetworkInterface)
		pNetworkInterface->onListenError();
}

//-------------------------------------------------------------------------------------
void NetworkInterface::onListenError()
{
	ERROR_MSG(fmt::format("NetworkInterface::onListenError(): listen error!\n"));
}

//-------------------------------------------------------------------------------------
bool NetworkInterface::addSession(SessionID id, Session* pSession)
{
	sessions_[id] = pSession;
	//DEBUG_MSG(fmt::format("NetworkInterface::addSession(): id={}, pSession={:p}!\n", id, (void*)pSession));
	return true;
}

//-------------------------------------------------------------------------------------
bool NetworkInterface::removeSession(SessionID id)
{
	SessionMap::iterator iter = sessions_.find(id);
	if (iter == sessions_.end())
	{
		ERROR_MSG(fmt::format("NetworkInterface::removeSession(): id({}) not found!\n", id));
		return false;
	}

	delete iter->second;
	sessions_.erase(iter);
	//DEBUG_MSG(fmt::format("NetworkInterface::removeSession(): id={}!\n", id));
	return true;
}

//-------------------------------------------------------------------------------------
void NetworkInterface::checkSessions()
{
	std::vector<SessionID> dels;

	for (auto& item : sessions_)
	{
		if (item.second->isTimeout())
		{
			dels.push_back(item.first);
		}
	}

	for (auto& item : dels)
	{
		Session* pSession = findSession(item);

		if (!pSession)
			continue;

		DEBUG_MSG(fmt::format("NetworkInterface::checkSessions(): session timeout! sessionID={}, addr={}!\n",
			pSession->id(), pSession->addr()));

		pSession->destroy();
	}
}

//-------------------------------------------------------------------------------------
}
