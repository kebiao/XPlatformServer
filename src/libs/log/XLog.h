#ifndef X_LOG_H
#define X_LOG_H

#include "common/common.h"
#include "common/singleton.h"

namespace XServer {

class XLog : public Singleton<XLog>
{
public:
	XLog();
	~XLog();

	virtual bool initialize(const std::string& log4cxxConfig);
	virtual void finalise();

	void print_msg(const std::string& s);
	void debug_msg(const std::string& s);
	void error_msg(const std::string& s);
	void info_msg(const std::string& s);
	void warning_msg(const std::string& s);

	void closeLogger();

protected:

};


#define PRINT_MSG(m)				XServer::XLog::getSingleton().print_msg((m))								// 输出任何信息
#define ERROR_MSG(m)				XServer::XLog::getSingleton().error_msg((m))								// 输出一个错误
#define DEBUG_MSG(m)				XServer::XLog::getSingleton().debug_msg((m))								// 输出一个debug信息
#define INFO_MSG(m)					XServer::XLog::getSingleton().info_msg((m))								// 输出一个info信息
#define WARNING_MSG(m)				XServer::XLog::getSingleton().warning_msg((m))							// 输出一个警告信息
	
}

#endif // X_LOG_H
