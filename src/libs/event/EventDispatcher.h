#ifndef X_EVENT_DISPATCHER_H
#define X_EVENT_DISPATCHER_H

#include "common/common.h"

namespace XServer {

class EventDispatcher
{
public:
	EventDispatcher();
	virtual ~EventDispatcher();

	bool initialize();
	void finalise();

	bool dispatch();
	bool breakDispatch();

	bool exitDispatch(float secs = 1.0f);

	event_base* base() {
		return base_;
	}

	struct event * add_watch_signal(int signal, event_callback_fn signal_cb, void* cbargs);
	bool del_watch_signal(struct event * ev);

protected:
	struct event_base *base_;
};

}

#endif // X_EVENT_DISPATCHER_H
