# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (XPlatformServer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)

#set(CMAKE_CXX_FLAGS_RELEASE "/MT")
#set(CMAKE_CXX_FLAGS_DEBUG "/MTd")

# include directories
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/libs
  ${PROJECT_SOURCE_DIR}/deps/libevent-2.1.8-stable/include
  ${PROJECT_SOURCE_DIR}/deps/apache-log4cxx-0.10.0/src/main/include
  ${PROJECT_SOURCE_DIR}/deps/apr/include
  ${PROJECT_SOURCE_DIR}/deps/apr-util/include
  ${PROJECT_SOURCE_DIR}/deps/iniparser/src
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/mongoc
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/libbson/src/bson
  ${PROJECT_SOURCE_DIR}/deps/openssl-1.1.0e/include
)

IF(UNIX)
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/deps/protobuf-3.2.0/src
  ${PROJECT_SOURCE_DIR}/deps/gflags-2.2.0/include
  ${PROJECT_SOURCE_DIR}/deps/redis/hiredis
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/libbson/src/bson
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/mongoc
)
ELSE(UNIX)
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/deps/libevent-2.1.8-stable/WIN32-Code/nmake
  ${PROJECT_SOURCE_DIR}/deps/protobuf-3.2.0/vsprojects/include
  ${PROJECT_SOURCE_DIR}/deps/gflags-2.2.0/vsprojects/include
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/libbson/build/src/bson
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/build/src/mongoc
  ${PROJECT_SOURCE_DIR}/deps/redis/hiredis/redis-win/deps/hiredis
)
ENDIF(UNIX)

# lib directories
LINK_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/libs
  ${PROJECT_SOURCE_DIR}/deps/libevent-2.1.8-stable
  ${PROJECT_SOURCE_DIR}/deps/libevent-2.1.8-stable/.libs
  ${PROJECT_SOURCE_DIR}/deps/apache-log4cxx-0.10.0/src/main/cpp/.libs
  ${PROJECT_SOURCE_DIR}/deps/apache-log4cxx-0.10.0/projects/libs
  ${PROJECT_SOURCE_DIR}/deps/apr/.libs
  ${PROJECT_SOURCE_DIR}/deps/apr/x64/LibR
  ${PROJECT_SOURCE_DIR}/deps/apr-util/.libs
  ${PROJECT_SOURCE_DIR}/deps/apr-util/x64/LibR
  ${PROJECT_SOURCE_DIR}/deps/apr-util/xml/expat/.libs
  ${PROJECT_SOURCE_DIR}/deps/apr-util/xml/expat/lib/x64/LibR
  ${PROJECT_SOURCE_DIR}/deps/protobuf-3.2.0/vsprojects/x64/Release
  ${PROJECT_SOURCE_DIR}/deps/protobuf-3.2.0/vsprojects/x64/Debug
  ${PROJECT_SOURCE_DIR}/deps/protobuf-3.2.0/src/.libs
  ${PROJECT_SOURCE_DIR}/deps/gflags-2.2.0/vsprojects/Release
  ${PROJECT_SOURCE_DIR}/deps/gflags-2.2.0/vsprojects/Debug
  ${PROJECT_SOURCE_DIR}/deps/gflags-2.2.0/lib
  ${PROJECT_SOURCE_DIR}/deps/iniparser
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/build/Debug
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/build/Release
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/.libs
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/libbson/build/Debug
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/libbson/build/Release
  ${PROJECT_SOURCE_DIR}/deps/mongo-c-driver-1.6.2/src/libbson/.libs
  ${PROJECT_SOURCE_DIR}/deps/openssl-1.1.0e/
  ${PROJECT_SOURCE_DIR}/deps/redis/hiredis/redis-win/msvs/hiredis/x64/Debug
  ${PROJECT_SOURCE_DIR}/deps/redis/hiredis/redis-win/msvs/hiredis/x64/Release
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# 预编译宏
ADD_DEFINITIONS(-DLOG4CXX_STATIC)
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

# 添加子目录
add_subdirectory(libs/common)
add_subdirectory(libs/log)
add_subdirectory(libs/event)
add_subdirectory(libs/server)
add_subdirectory(libs/protos)
add_subdirectory(libs/resmgr)
add_subdirectory(deps/iniparser/src)
add_subdirectory(servers/connector)
add_subdirectory(servers/halls)
add_subdirectory(servers/hallsmgr)
add_subdirectory(servers/dbmgr)
add_subdirectory(servers/login)
add_subdirectory(servers/roommgr)
add_subdirectory(servers/machine)
add_subdirectory(servers/directory)
add_subdirectory(servers/robot)

