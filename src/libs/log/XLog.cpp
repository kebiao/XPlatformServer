#include "XLog.h"
#include "resmgr/ResMgr.h"

#include "log4cxx/logger.h"
#include "log4cxx/logmanager.h"
#include "log4cxx/net/socketappender.h"
#include "log4cxx/fileappender.h"
#include "log4cxx/helpers/inetaddress.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/logstring.h"
#include "log4cxx/basicconfigurator.h"

#if X_PLATFORM == PLATFORM_WIN32
#pragma comment (lib, "Mswsock.lib")
#pragma comment( lib, "odbc32.lib" )
#endif

namespace XServer {

X_SINGLETON_INIT(XLog);

log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger(""));

XLog g_XLog;

//-------------------------------------------------------------------------------------
XLog::XLog()
{
}

//-------------------------------------------------------------------------------------
XLog::~XLog()
{
}

//-------------------------------------------------------------------------------------		
bool XLog::initialize(const std::string& log4cxxConfig)
{
	std::string cfg = ResMgr::getSingleton().matchRes(log4cxxConfig);
	if (cfg == log4cxxConfig)
	{
		fprintf(stderr, "[ERROR:] XLog::initialize(): not found %s!\n", cfg.c_str());
		getchar();
		return false;
	}

	log4cxx::PropertyConfigurator::configure(cfg.c_str());
	g_logger = log4cxx::Logger::getRootLogger();

	return true;
}

//-------------------------------------------------------------------------------------
void XLog::finalise(void)
{
}

//-------------------------------------------------------------------------------------
void XLog::closeLogger()
{
	g_logger = (const int)NULL;
	log4cxx::LogManager::shutdown();
}

//-------------------------------------------------------------------------------------
void XLog::print_msg(const std::string& s)
{
	LOG4CXX_INFO(g_logger, s);
}

//-------------------------------------------------------------------------------------
void XLog::error_msg(const std::string& s)
{
	LOG4CXX_ERROR(g_logger, s);
}

//-------------------------------------------------------------------------------------
void XLog::info_msg(const std::string& s)
{
	LOG4CXX_INFO(g_logger, s);
}

//-------------------------------------------------------------------------------------
void XLog::debug_msg(const std::string& s)
{
	LOG4CXX_DEBUG(g_logger, s);
}

//-------------------------------------------------------------------------------------
void XLog::warning_msg(const std::string& s)
{
	LOG4CXX_WARN(g_logger, s);
}

//-------------------------------------------------------------------------------------
}
