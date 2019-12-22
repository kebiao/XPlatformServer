#include "common/common.h"
#include "XServerApp.h"
#include "log/XLog.h"
#include <gflags/gflags.h>

DEFINE_uint64(id, 0, "the server id");
DEFINE_uint64(groupid, 0, "the server groupID");
DEFINE_string(name, "unknown", "the server name");
DEFINE_int32(botsNum, 100, "maximum number of robots");
DEFINE_int32(perNum, 1, "create number per time");

int main(int argc, char *argv[])
{
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	XServer::int32 botsNum = FLAGS_botsNum;
	XServer::int32 perNum = FLAGS_perNum;

	XServer::XServerApp app;

	if (app.initialize(FLAGS_id, XServer::ServerType::SERVER_TYPE_ROBOT, FLAGS_groupid, FLAGS_name, botsNum, perNum))
		app.run();

	app.finalise();

	return 0;
}
