#ifndef X_XOBJECT_H
#define X_XOBJECT_H

#include "common/common.h"
#include "protos/ServerCommon.pb.h"
#include "protos/Commands.pb.h"

namespace XServer {

class Session;

class XObject
{
public:
	XObject(ObjectID oid, Session* pSession);
	virtual ~XObject();

	ObjectID id() const {
		return id_;
	}

	void onClientConnected(Session* pSession);
	void onClientDisconnected(Session* pSession);

	bool isDestroyed() const {
		return isDestroyed_;
	}

	virtual void destroy();
	virtual void onDestroy();

	Session* pSession() {
		return pSession_;
	}

protected:
	ObjectID id_;
	Session* pSession_;
	bool isDestroyed_;
};

typedef std::shared_ptr<XObject> XObjectPtr;

}

#endif // X_XOBJECT_H
