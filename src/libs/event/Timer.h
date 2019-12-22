#ifndef X_TIMER_H
#define X_TIMER_H

#include "common/common.h"

namespace XServer {

class EventDispatcher;

class Timer
{
public:
	static const int Seconds;

	typedef std::function<void(void*)> TimerHandler;

	struct cb_arg
	{
		struct event *ev;
		struct timeval tv;
		void* userargs;
		Timer* pTimer;
		TimerHandler pHandler;
		int round, roundMax;
	};

public:
	Timer(EventDispatcher* pEventDispatcher);
	virtual ~Timer();

	struct event* addTimer(uint32_t interval, int round, TimerHandler timeout_cb, void* userargs);
	bool delTimer(struct event * ev);

private:
	static void eventHandler(evutil_socket_t fd, short events, void *ctx);

protected:
	EventDispatcher* pEventDispatcher_;
};

}

#endif // X_TIMER_H
