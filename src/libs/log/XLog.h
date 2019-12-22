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


#define PRINT_MSG(m)				XServer::XLog::getSingleton().print_msg((m))								// ����κ���Ϣ
#define ERROR_MSG(m)				XServer::XLog::getSingleton().error_msg((m))								// ���һ������
#define DEBUG_MSG(m)				XServer::XLog::getSingleton().debug_msg((m))								// ���һ��debug��Ϣ
#define INFO_MSG(m)					XServer::XLog::getSingleton().info_msg((m))								// ���һ��info��Ϣ
#define WARNING_MSG(m)				XServer::XLog::getSingleton().warning_msg((m))							// ���һ��������Ϣ
	
}

#endif // X_LOG_H
