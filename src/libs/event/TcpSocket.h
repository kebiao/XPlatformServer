#ifndef X_TCP_SOCKET_H
#define X_TCP_SOCKET_H

#include "common/common.h"

namespace XServer {

class EventDispatcher;

class TcpSocket
{
public:
	TcpSocket(EventDispatcher* pEventDispatcher, socket_t sock);
	virtual ~TcpSocket();

	std::string addr();

	std::string getIP();
	uint16 getPort();

	static std::string getSocketIP(socket_t sock);
	static uint16 getSocketPort(socket_t sock);
	static int setnonblocking(socket_t sock, bool nonblocking);

	socket_t socket() const;
	
	void getAddr(struct sockaddr_in * dest, uint32 size) const;
	
	bool send(const uint8 *data, uint32 size);
	
	uint32 getRecvBufferLength() const;
	
	const uint8 * getRecvBuffer(uint32 size) const;
	
	bool recv(uint8 * dest, uint32_t size);
	
	void clearRecvBuffer();
	
	void close();
	
	bool isGood() const {
		return bufEvt_ != NULL;
	}

	struct bufferevent * getBufEvt() {
		return bufEvt_;
	}

	bool setcb(
		bufferevent_data_cb recvcb, bufferevent_data_cb sendcb,
		bufferevent_event_cb eventcb, void *cbarg);

	bool enable(short event);
	bool disable(short event);

protected:
	struct bufferevent* bufEvt_;
};

}

#endif // X_TCP_SOCKET_H
