#ifndef X_COMMON_H
#define X_COMMON_H

#define XPLATFORMSERVER_VERSION 100

// common include	
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <math.h>
#include <assert.h> 
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>  
#include <cstring>  
#include <vector>
#include <map>
#include <list>
#include <set>
#include <deque>
#include <limits>
#include <algorithm>
#include <utility>
#include <functional>
#include <cctype>
#include <iterator>
#include <random>
#include <chrono>
#include <condition_variable>
#include <future>

#include "common/format.h"

#ifndef _WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#pragma warning(disable:4996)
#pragma warning(disable:4819)
#pragma warning(disable:4049)
#pragma warning(disable:4217)
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Rpcrt4.lib")
#include <io.h>
#include <time.h> 
//#define FD_SETSIZE 1024
#ifndef WIN32_LEAN_AND_MEAN 
#include <winsock2.h>		// 必须在windows.h之前包含， 否则网络模块编译会出错
#include <mswsock.h> 
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> 
#include <unordered_map>
#include <functional>
#include <memory>
#define _SCL_SECURE_NO_WARNINGS
#else
// linux include
#include <dirent.h>
#include <errno.h>
#include <float.h>
#include <pthread.h>	
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <iconv.h>
#include <langinfo.h>   /* CODESET */
#include <stdint.h>
#include <signal.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <tr1/unordered_map>
#include <tr1/functional>
#include <tr1/memory>
#include <linux/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/resource.h> 
#include <linux/errqueue.h>
#endif

#include <signal.h>

#if !defined( _WIN32 )
# include <pwd.h>
#else
#define SIGHUP	1
#define SIGINT	2
#define SIGQUIT 3
#define SIGUSR1 10
#define SIGPIPE 13
#define SIGCHLD 17
#define SIGSYS	32
#endif

#define PLATFORM_WIN32			0
#define PLATFORM_UNIX			1
#define PLATFORM_APPLE			2

#define UNIX_FLAVOUR_LINUX		1
#define UNIX_FLAVOUR_BSD		2
#define UNIX_FLAVOUR_OTHER		3
#define UNIX_FLAVOUR_OSX		4

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#  define X_PLATFORM PLATFORM_WIN32
#elif defined( __INTEL_COMPILER )
#  define X_PLATFORM PLATFORM_INTEL
#elif defined( __APPLE_CC__ )
#  define X_PLATFORM PLATFORM_APPLE
#else
#  define X_PLATFORM PLATFORM_UNIX
#endif

#define COMPILER_MICROSOFT 0
#define COMPILER_GNU	   1
#define COMPILER_BORLAND   2
#define COMPILER_INTEL     3
#define COMPILER_CLANG     4

#ifdef _MSC_VER
#  define X_COMPILER COMPILER_MICROSOFT
#elif defined( __INTEL_COMPILER )
#  define X_COMPILER COMPILER_INTEL
#elif defined( __BORLANDC__ )
#  define X_COMPILER COMPILER_BORLAND
#elif defined( __GNUC__ )
#  define X_COMPILER COMPILER_GNU
#elif defined( __clang__ )
#  define X_COMPILER COMPILER_CLANG
	
#else
#  pragma error "FATAL ERROR: Unknown compiler."
#endif

namespace XServer {

#ifndef TCHAR
#ifdef _UNICODE
	typedef wchar_t												TCHAR;
#else
	typedef char												TCHAR;
#endif
#endif

typedef unsigned char											uchar;
typedef unsigned short											ushort;
typedef unsigned int											uint;
typedef unsigned long											ulong;

#define charptr													char*
#define const_charptr											const char*

#if X_COMPILER != COMPILER_GNU
typedef signed __int64											int64;
typedef signed __int32											int32;
typedef signed __int16											int16;
typedef signed __int8											int8;
typedef unsigned __int64										uint64;
typedef unsigned __int32										uint32;
typedef unsigned __int16										uint16;
typedef unsigned __int8											uint8;
typedef INT_PTR													intptr;
typedef UINT_PTR        										uintptr;
#else
#define MAX_PATH												255
typedef int64_t													int64;
typedef int32_t													int32;
typedef int16_t													int16;
typedef int8_t													int8;
typedef uint64_t												uint64;
typedef uint32_t												uint32;
typedef uint16_t												uint16;
typedef uint8_t													uint8;
typedef uint16_t												WORD;
typedef uint32_t												DWORD;
#ifdef _LP64
typedef int64													intptr;
typedef uint64													uintptr;
#else
typedef int32													intptr;
typedef uint32													uintptr;
#endif
#endif

typedef evutil_socket_t											socket_t;
#define SOCKET_T_INVALID										-1

typedef uint64													SessionID;
#define SESSION_ID_INVALID										0

typedef uint64													ServerAppID;
#define SERVER_APP_ID_INVALID									0

typedef uint64													ServerGroupID;
#define SERVER_GROUP_ID_INVALID									0

typedef uint64													ObjectID;
#define OBJECT_ID_INVALID										0

typedef uint8													GameMode;
#define GAME_MODE_INVALID										0

typedef uint64													GameID;
#define GAME_ID_INVALID											0

// 包最大长度
#define PACKET_LENGTH_MAX										65535

// 服务器类型
enum class ServerType
{
	SERVER_TYPE_UNKNOWN =										0,
	SERVER_TYPE_CONNECTOR =										1,
	SERVER_TYPE_HALLS =											2,
	SERVER_TYPE_HALLSMGR =										3,
	SERVER_TYPE_LOGIN =											4,
	SERVER_TYPE_ROOM =											5,
	SERVER_TYPE_ROOMMGR =										6,
	SERVER_TYPE_DBMGR =											7,
	SERVER_TYPE_MACHINE =										8,
	SERVER_TYPE_DIRECTORY =										9,
	SERVER_TYPE_ROBOT =											10,
	SERVER_TYPE_CLIENT =										11,
	SERVER_TYPE_MAX =											12
};

static char ServerType2Str[][256] = {
	"SERVER_TYPE_UNKNOWN",
	"SERVER_TYPE_CONNECTOR",
	"SERVER_TYPE_HALLS",
	"SERVER_TYPE_HALLSMGR",
	"SERVER_TYPE_LOGIN",
	"SERVER_TYPE_ROOM",
	"SERVER_TYPE_ROOMMGR",
	"SERVER_TYPE_DBMGR",
	"SERVER_TYPE_MACHINE",
	"SERVER_TYPE_DIRECTORY",
	"SERVER_TYPE_ROBOT"，
	"SERVER_TYPE_CLIENT"
};

static char ServerType2Name[][256] = {
	"unknown",
	"connector",
	"halls",
	"hallsmgr",
	"login",
	"room",
	"roommgr",
	"dbmgr",
	"machine",
	"directory",
	"robot",
	"client"
};

/** 安全的释放一个指针内存 */
#define SAFE_RELEASE(i)	\
	if (i) \
		{ \
			delete i; \
			i = NULL; \
		}

/** 安全的释放一个指针数组内存 */
#define SAFE_RELEASE_ARRAY(i) \
	if (i) \
		{ \
			delete[] i; \
			i = NULL; \
		}

inline int x_replace(std::string& str, const std::string& pattern, const std::string& newpat)
{
	int count = 0;
	const size_t nsize = newpat.size();
	const size_t psize = pattern.size();

	for (size_t pos = str.find(pattern, 0);
	pos != std::string::npos;
		pos = str.find(pattern, pos + nsize))
	{
		str.replace(pos, psize, newpat);
		count++;
	}

	return count;
}

template<typename T>
inline void x_split(const std::basic_string<T>& s, T c, std::vector< std::basic_string<T> > &v)
{
	if (s.size() == 0)
		return;

	typename std::basic_string< T >::size_type i = 0;
	typename std::basic_string< T >::size_type j = s.find(c);

	while (j != std::basic_string<T>::npos)
	{
		std::basic_string<T> buf = s.substr(i, j - i);

		if (buf.size() > 0)
			v.push_back(buf);

		i = ++j; j = s.find(c, j);
	}

	if (j == std::basic_string<T>::npos)
	{
		std::basic_string<T> buf = s.substr(i, s.length() - i);
		if (buf.size() > 0)
			v.push_back(buf);
	}
}

inline void x_split(const std::string& s, const std::string& c, std::vector< std::string > &v)
{
	if (s.size() == 0)
		return;

	typename std::string::size_type i = 0;
	typename std::string::size_type j = s.find(c);

	while (j != std::string::npos)
	{
		std::string buf = s.substr(i, j - i);

		if (buf.size() > 0)
			v.push_back(buf);

		i = ++j; j = s.find(c, j);
	}

	if (j == std::string::npos)
	{
		std::string buf = s.substr(i, s.length() - i);
		if (buf.size() > 0)
			v.push_back(buf);
	}
}

inline time_t getTimeStamp()
{
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = 
		std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
	time_t timestamp = tmp.count();
	//std::time_t timestamp = std::chrono::system_clock::to_time_t(tp);  
	return timestamp;
}

#define TIME_SECONDS 1000

/** sleep 跨平台 */
#if X_PLATFORM == PLATFORM_WIN32
inline void sleep(uint32 ms)
{
	::Sleep(ms);
}
#else
inline void sleep(uint32 ms)
{
	struct timeval tval;
	tval.tv_sec = ms / 1000;
	tval.tv_usec = (ms * 1000) % 1000000;
	select(0, NULL, NULL, NULL, &tval);
}
#endif

/** 判断平台是否为小端字节序 */
inline bool isPlatformLittleEndian()
{
	int n = 1;
	return *((char*)&n) ? true : false;
}

/** 设置环境变量 */
#if X_PLATFORM == PLATFORM_WIN32
inline void setenv(const std::string& name, const std::string& value, int overwrite)
{
	_putenv_s(name.c_str(), value.c_str());
}
#else
// Linux下面直接使用setenv
#endif

inline char* wchar2char(const wchar_t* ts, size_t* outlen = NULL)
{
	int len = (int)((wcslen(ts) + 1) * sizeof(wchar_t));
	char* ccattr = (char *)malloc(len);
	memset(ccattr, 0, len);

	size_t slen = wcstombs(ccattr, ts, len);

	if (outlen)
	{
		if ((size_t)-1 != slen)
			*outlen = slen;
		else
			*outlen = 0;
	}

	return ccattr;
};

inline wchar_t* char2wchar(const char* cs, size_t* outlen = NULL)
{
	int len = (int)((strlen(cs) + 1) * sizeof(wchar_t));
	wchar_t* ccattr = (wchar_t *)malloc(len);
	memset(ccattr, 0, len);

	size_t slen = mbstowcs(ccattr, cs, len);

	if (outlen)
	{
		if ((size_t)-1 != slen)
			*outlen = slen;
		else
			*outlen = 0;
	}

	return ccattr;
};

inline uint64 uuid()
{
	static int workid = 1;
	static int seqid = 0;
	static uint64_t last_stamp = getTimeStamp();

	uint64_t uniqueId = 0;
	uint64_t nowtime = getTimeStamp();
	uniqueId = nowtime << 22;
	uniqueId |= (workid & 0x3ff) << 12;

	if (nowtime < last_stamp) 
	{
		printf("uuid(): error!\n");
		assert(false); 
	}
	if (nowtime == last_stamp) 
	{
		if (seqid++ == 0x1000)
		{
			do {
				nowtime = getTimeStamp();
			} while (nowtime <= last_stamp);

			seqid = 0;
		}
	}
	else 
	{
		seqid = 0;
	}

	last_stamp = nowtime;

	uniqueId |= seqid & 0xFFF;

	return uniqueId;
}

}

#endif // X_COMMON_H