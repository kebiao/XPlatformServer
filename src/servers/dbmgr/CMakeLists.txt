PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)
IF(UNIX)
list(REMOVE_ITEM DIR_LIB_SRCS "./win32fixes.c")
ENDIF(UNIX)

# 指定生成目标
add_executable(dbmgr ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(dbmgr common)
target_link_libraries(dbmgr log)
target_link_libraries(dbmgr server)
target_link_libraries(dbmgr protos)
target_link_libraries(dbmgr resmgr)
target_link_libraries(dbmgr eventsystem)

IF(UNIX)
target_link_libraries(dbmgr expat)
target_link_libraries(dbmgr event event_core event_extra)
target_link_libraries(dbmgr protobuf protoc)

target_link_libraries(dbmgr gflags_nothreads)
target_link_libraries(dbmgr log4cxx)

target_link_libraries(dbmgr pthread)

target_link_libraries(dbmgr hiredis)

target_link_libraries(dbmgr mongoc-priv)
target_link_libraries(dbmgr bson)

target_link_libraries(dbmgr crypto)
target_link_libraries(dbmgr ssl)

target_link_libraries(dbmgr rt)

ELSE(UNIX)
target_link_libraries(dbmgr xml)

target_link_libraries(dbmgr libevent libevent_core libevent_extras)
target_link_libraries(dbmgr debug libprotobufd debug libprotocd)
target_link_libraries(dbmgr optimized libprotobuf optimized libprotoc)

target_link_libraries(dbmgr debug gflags_nothreads_static_d shlwapi)
target_link_libraries(dbmgr optimized gflags_nothreads_static shlwapi)

target_link_libraries(dbmgr debug log4cxx_d)
target_link_libraries(dbmgr optimized log4cxx)

target_link_libraries(dbmgr debug hiredis_d)
target_link_libraries(dbmgr optimized hiredis)

target_link_libraries(dbmgr debug mongoc-static-1.0_d)
target_link_libraries(dbmgr optimized mongoc-static-1.0)
target_link_libraries(dbmgr debug bson-static-1.0_d)
target_link_libraries(dbmgr optimized bson-static-1.0)

target_link_libraries(dbmgr debug libcrypto)
target_link_libraries(dbmgr optimized libcrypto)
target_link_libraries(dbmgr debug libssl)
target_link_libraries(dbmgr optimized libssl)

target_link_libraries(dbmgr crypt32)
target_link_libraries(dbmgr secur32)
ENDIF(UNIX)

target_link_libraries(dbmgr apr-1)
target_link_libraries(dbmgr aprutil-1)

target_link_libraries(dbmgr iniparser)


