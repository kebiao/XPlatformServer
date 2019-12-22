#ifndef X_EVENT_COMMON_H
#define X_EVENT_COMMON_H

#include "common/common.h"

namespace XServer {

#pragma pack(push, 1)
struct PacketHeader
{
	uint16 msglen;
	uint16 msgcmd;

	inline void encode() {
		msglen = htons(msglen);
		msgcmd = htons(msgcmd);
	}

	inline void decode() {
		msglen = ntohs(msglen);
		msgcmd = htons(msgcmd);
	}
};
#pragma pack(pop)

}

#endif // X_EVENT_COMMON_H