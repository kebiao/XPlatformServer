#include "TcpSocket.h"
#include "EventDispatcher.h"
#include "log/XLog.h"

namespace XServer {

#define RECV_BUFFER bufferevent_get_input(bufEvt_)
#define SEND_BUFFER bufferevent_get_output(bufEvt_)

//-------------------------------------------------------------------------------------
TcpSocket::TcpSocket(EventDispatcher* pEventDispatcher, socket_t sock):
bufEvt_(NULL)
{
	bufEvt_ = bufferevent_socket_new(pEventDispatcher->base(),
		sock, BEV_OPT_CLOSE_ON_FREE);
}

//-------------------------------------------------------------------------------------
TcpSocket::~TcpSocket()
{
	if(bufEvt_)
	{
		bufferevent_free(bufEvt_);
		bufEvt_ = NULL;
	}
}

//-------------------------------------------------------------------------------------
socket_t TcpSocket::socket() const
{
	return (!bufEvt_) ? SOCKET_T_INVALID : bufferevent_getfd(bufEvt_);
}

//-------------------------------------------------------------------------------------
bool TcpSocket::send(const uint8 *data, uint32 size)
{
	if (!bufEvt_)
	{
		return false;
	}

	return (-1 != bufferevent_write(bufEvt_, data, size));
}

//-------------------------------------------------------------------------------------
bool TcpSocket::recv(uint8 * dest, uint32 size)
{
	return (bufEvt_) ? (-1 != evbuffer_remove(RECV_BUFFER, (void *)dest, size)) : false;
}

//-------------------------------------------------------------------------------------
uint32 TcpSocket::getRecvBufferLength() const
{
	return (bufEvt_) ? (uint32)evbuffer_get_length(RECV_BUFFER) : 0;
}

//-------------------------------------------------------------------------------------
const uint8* TcpSocket::getRecvBuffer(uint32_t size) const
{
	return (bufEvt_) ? evbuffer_pullup(RECV_BUFFER, size) : NULL;
}

//-------------------------------------------------------------------------------------
void TcpSocket::clearRecvBuffer()
{
	if(bufEvt_)
		evbuffer_drain(RECV_BUFFER, UINT32_MAX);
}

//-------------------------------------------------------------------------------------
void TcpSocket::close()
{
	//disable(EV_READ | EV_WRITE);
	//clearRecvBuffer();
	//evutil_closesocket(socket());

	if(bufEvt_)
		bufferevent_free(bufEvt_);
		
	bufEvt_ = NULL;
}

//-------------------------------------------------------------------------------------
std::string TcpSocket::addr()
{
	struct sockaddr_in peerAddr;
	getAddr(&peerAddr, sizeof(peerAddr));
	return fmt::format("{}:{}", inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
}

//-------------------------------------------------------------------------------------
std::string TcpSocket::getSocketIP(socket_t sock)
{
	struct sockaddr_in peerAddr;
	socklen_t addrSize = sizeof(struct sockaddr_in);
	getpeername(sock, (struct sockaddr *)&peerAddr, &addrSize);
	return inet_ntoa(peerAddr.sin_addr);
}

//-------------------------------------------------------------------------------------
uint16 TcpSocket::getSocketPort(socket_t sock)
{
	struct sockaddr_in peerAddr;
	socklen_t addrSize = sizeof(struct sockaddr_in);
	getpeername(sock, (struct sockaddr *)&peerAddr, &addrSize);
	return ntohs(peerAddr.sin_port);
}

int TcpSocket::setnonblocking(socket_t sock, bool nonblocking)
{
#ifdef unix
	int val = nonblocking ? O_NONBLOCK : 0;
	return ::fcntl(sock, F_SETFL, val);
#else
	u_long val = nonblocking ? 1 : 0;
	return ::ioctlsocket(sock, FIONBIO, &val);
#endif
}

//-------------------------------------------------------------------------------------
void TcpSocket::getAddr(struct sockaddr_in * dest, uint32 size) const
{
	socklen_t addrSize = sizeof(struct sockaddr_in);
	if ((NULL == bufEvt_) || ((socklen_t)size < addrSize))
	{
		return;
	}

	getpeername(socket(), (struct sockaddr *)dest, (socklen_t*)&addrSize);
}

//-------------------------------------------------------------------------------------
std::string TcpSocket::getIP()
{
	return TcpSocket::getSocketIP(socket());
}

//-------------------------------------------------------------------------------------
uint16 TcpSocket::getPort()
{
	return TcpSocket::getSocketPort(socket());
}

//-------------------------------------------------------------------------------------
bool TcpSocket::setcb(
	bufferevent_data_cb recvcb, bufferevent_data_cb sendcb,
	bufferevent_event_cb eventcb, void *cbarg)
{
	bufferevent_setcb(bufEvt_, recvcb, sendcb, eventcb, cbarg);
	if (-1 == bufferevent_enable(bufEvt_, EV_READ | EV_WRITE))
	{
		bufferevent_free(bufEvt_);
		bufEvt_ = NULL;
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool TcpSocket::enable(short event)
{
	if (-1 == bufferevent_enable(bufEvt_, event))
	{
		bufferevent_free(bufEvt_);
		bufEvt_ = NULL;
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool TcpSocket::disable(short event)
{
	if (-1 == bufferevent_disable(bufEvt_, event))
	{
		bufferevent_free(bufEvt_);
		bufEvt_ = NULL;
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
}
