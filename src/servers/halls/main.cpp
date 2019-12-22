#include "common/common.h"
#include "XServerApp.h"
#include "log/XLog.h"
#include <gflags/gflags.h>

DEFINE_uint64(id, 0, "the server id");
DEFINE_uint64(groupid, 0, "the server groupID");
DEFINE_string(name, "unknown", "the server name");

int main(int argc, char *argv[])
{
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	XServer::XServerApp app;

	if (app.initialize(FLAGS_id, XServer::ServerType::SERVER_TYPE_HALLS, FLAGS_groupid, FLAGS_name))
		app.run();

	app.finalise();

	return 0;
}
