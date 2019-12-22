#ifndef X_SESSION_H
#define X_SESSION_H

#include "event/common.h"
#include "protos/Commands.pb.h"

namespace XServer {

class NetworkInterface;
class EventDispatcher;
class TcpSocket;

#define PARSE_PACKET() \
	if (!packet.ParseFromArray(data, header_.msglen))\
	{ \
		ERROR_MSG(fmt::format("Session::onProcessPacket_(): packet parsing error! {}, size={}, sessionID={}, {}\n", \
			CMD_Name((CMD)header_.msgcmd), header_.msglen, id(), pTcpSocket()->addr())); \
	\
		return false; \
	} \


class Session
{
public:
	Session(SessionID sessionID, socket_t sock, NetworkInterface* pNetworkInterface, EventDispatcher* pEventDispatcher);
	virtual ~Session();

	NetworkInterface* pNetworkInterface() {
		return pNetworkInterface_;
	}
	
	EventDispatcher* pEventDispatcher();

	SessionID id() const {
		return id_;
	}

	void appID(ServerAppID appid) {
		appID_ = appid;
	}

	ServerAppID appID() const {
		return appID_;
	}

	void appType(ServerType apptype) {
		appType_ = apptype;
	}

	ServerType appType() const {
		return appType_;
	}

	const char* typeName() const {
		return ServerType2Name[(int)appType_];
	}

	std::string addr();

	bool initialize();
	void finalise();

	void isServer(bool v) {
		isServer_ = v;
	}

	bool isServer() const {
		return isServer_;
	}

	TcpSocket* pTcpSocket() {
		return pTcpSocket_;
	}

	PacketHeader& packetHeader() {
		return header_;
	}

	bool send(const uint8 *data, uint32 size);
	bool encryptSend(int32 cmd, const uint8 *data, uint32 size);
	bool sendPacket(int32 cmd, const uint8 *data, uint32 size);
	bool sendPacket(int32 cmd, const ::google::protobuf::Message& packet);
	bool forwardPacket(SessionID requestorSessionID, int32 cmd, const ::google::protobuf::Message& packet);

	bool connected() const {
		return connected_;
	}

	void connected(bool v)
	{
		connected_ = v;
	}

	bool isTimeout();

	std::string getIP();
	uint16 getPort();

	int64 getRoundTripTime() const {
		return rtt_;
	}

	time_t ping();

	bool onProcessPacket_(SessionID requestorSessionID, uint8 * data, uint32_t size);

	virtual bool onProcessPacket(SessionID requestorSessionID, const uint8* data, int32 size) {
		return true;
	}

	bool isDestroyed() const {
		return destroyTimerEvent_ != NULL;
	}

	void destroy();

protected:
	void close();

	virtual void onRecv();
	virtual void onSent();

	virtual void onHello(const CMD_Hello& packet);
	virtual void onHelloCB(const CMD_HelloCB& packet);

	virtual void onHeartbeat(const CMD_Heartbeat& packet);
	virtual void onHeartbeatCB(const CMD_HeartbeatCB& packet);

	virtual void onVersionNotMatch(const CMD_Version_Not_Match& packet);

	virtual void onUpdateServerInfos(const CMD_UpdateServerInfos& packet);

	virtual void onQueryServerInfos(const CMD_QueryServerInfos& packet);
	virtual void onQueryServerInfosCB(const CMD_QueryServerInfosCB& packet);

	virtual void onPing(const CMD_Ping& packet);
	virtual void onPong(const CMD_Pong& packet);

	virtual bool onForwardPacket(const CMD_ForwardPacket& packet);

	virtual void onConnected();
	virtual void onDisconnected();

	void onDestroyTimer(void* userargs);
	
private:
	bool decryptSend(const uint8 *data, uint32 size);
	void handleEvent(short events);

	static void recvCallback(struct bufferevent *bev, void *data);
	static void sendCallback(struct bufferevent *bev, void *data);
	static void eventCallback(struct bufferevent *bev, short events, void *data);

protected:
	SessionID id_;

	ServerAppID appID_;
	ServerType appType_;

	NetworkInterface* pNetworkInterface_;
	EventDispatcher* pEventDispatcher_;

	TcpSocket* pTcpSocket_;

	bool headerRcved_;
	PacketHeader header_;

	bool isServer_;
	bool connected_;

	time_t lastReceivedTime_;

	int64 rtt_;

	struct event * destroyTimerEvent_;
};

}

#endif // X_SESSION_H
