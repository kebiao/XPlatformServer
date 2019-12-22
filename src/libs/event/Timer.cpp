#include "Timer.h"
#include "EventDispatcher.h"
#include "log/XLog.h"

namespace XServer {

const int Timer::Seconds = 1000;

//-------------------------------------------------------------------------------------
Timer::Timer(EventDispatcher* pEventDispatcher):
pEventDispatcher_(pEventDispatcher)
{
}

//-------------------------------------------------------------------------------------
Timer::~Timer()
{
}

//-------------------------------------------------------------------------------------
struct event * Timer::addTimer(uint32_t interval, int round, TimerHandler timeout_cb, void* userargs)
{
	struct timeval tv;
	evutil_timerclear(&tv);

	cb_arg* arg = new cb_arg();
	arg->ev = NULL;
	arg->pTimer = this;
	arg->userargs = userargs;
	arg->pHandler = timeout_cb;
	arg->round = 0;
	arg->roundMax = round;

	struct event *timeout_ev = event_new(pEventDispatcher_->base(), -1, (round > 1 || round == -1) ? EV_PERSIST : 0, eventHandler, (void*)arg);
	if (!timeout_ev)
	{
		ERROR_MSG(fmt::format("Timer::addTimer(): event_new error!\n"));
		return NULL;
	}

	arg->ev = timeout_ev;

	tv.tv_sec = interval / 1000;
	tv.tv_usec = interval % 1000 * 1000;

	if (0 != evtimer_add(timeout_ev, &tv))
	{
		event_free(timeout_ev);
		ERROR_MSG(fmt::format("Timer::addTimer(): evtimer_add error!\n"));
		return NULL;
	}

	return timeout_ev;
}

//-------------------------------------------------------------------------------------
bool Timer::delTimer(struct event * ev)
{
	return evtimer_del(ev) == 0;
}

//-------------------------------------------------------------------------------------
void Timer::eventHandler(evutil_socket_t fd, short events, void *ctx)
{
	cb_arg *arg = (cb_arg *)ctx;
	Timer* pTimer = arg->pTimer;

	++arg->round;

	if (arg->roundMax >= 0 && arg->round >= arg->roundMax)
	{
		event_del(arg->ev);
		arg->pHandler(arg->userargs);
		delete arg;
	}
	else
	{
		arg->pHandler(arg->userargs);
	}
}

//-------------------------------------------------------------------------------------
}
