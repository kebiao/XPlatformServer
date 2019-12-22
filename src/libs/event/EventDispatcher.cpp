#include "EventDispatcher.h"
#include "log/XLog.h"

namespace XServer {

//-------------------------------------------------------------------------------------
EventDispatcher::EventDispatcher():
	base_(NULL)
{
}

//-------------------------------------------------------------------------------------
EventDispatcher::~EventDispatcher()
{
	finalise();

	if (NULL != base_)
	{
		event_base_free(base_);
		base_ = NULL;
	}
}

//-------------------------------------------------------------------------------------		
bool EventDispatcher::initialize()
{
	base_ = event_base_new();
	if (!base_) {
		ERROR_MSG(fmt::format("EventDispatcher::initialize(): Could not initialize libevent!\n"));
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void EventDispatcher::finalise(void)
{
}

//-------------------------------------------------------------------------------------
bool EventDispatcher::dispatch()
{
	return event_base_dispatch(base_) == 0;
}

//-------------------------------------------------------------------------------------
bool EventDispatcher::breakDispatch()
{
	return event_base_loopbreak(base_) == 0;
}

//-------------------------------------------------------------------------------------
bool EventDispatcher::exitDispatch(float secs)
{
	struct timeval ten_sec;

	ten_sec.tv_sec = secs;
	ten_sec.tv_usec = 0;

	return event_base_loopexit(base_, &ten_sec) == 0;
}

//-------------------------------------------------------------------------------------
struct event * EventDispatcher::add_watch_signal(int signal, event_callback_fn signal_cb, void* cbargs)
{
	struct event *signal_event = evsignal_new(base_, signal, signal_cb, (void *)cbargs);

	if (!signal_event || event_add(signal_event, NULL) < 0) {
		ERROR_MSG(fmt::format("EventDispatcher::add_watch_signal(): Could not create/add a signal({}) event!\n", signal));
		return NULL;
	}

	DEBUG_MSG(fmt::format("EventDispatcher::add_watch_signal(): signal({})\n", signal));
	return signal_event;
}

//-------------------------------------------------------------------------------------
bool EventDispatcher::del_watch_signal(struct event * ev)
{
	bool ret = evsignal_del(ev) == 0;
	event_free(ev);
	return ret;
}

//-------------------------------------------------------------------------------------
}
