PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成目标
add_executable(roommgr ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(roommgr common)
target_link_libraries(roommgr log)
target_link_libraries(roommgr server)
target_link_libraries(roommgr protos)
target_link_libraries(roommgr resmgr)
target_link_libraries(roommgr eventsystem)

IF(UNIX)
target_link_libraries(roommgr expat)
target_link_libraries(roommgr event event_core event_extra)
target_link_libraries(roommgr protobuf protoc)

target_link_libraries(roommgr gflags_nothreads)
target_link_libraries(roommgr log4cxx)

target_link_libraries(roommgr pthread)

ELSE(UNIX)
target_link_libraries(roommgr xml)

target_link_libraries(roommgr libevent libevent_core libevent_extras)
target_link_libraries(roommgr debug libprotobufd debug libprotocd)
target_link_libraries(roommgr optimized libprotobuf optimized libprotoc)

target_link_libraries(roommgr debug gflags_nothreads_static_d shlwapi)
target_link_libraries(roommgr optimized gflags_nothreads_static shlwapi)

target_link_libraries(roommgr debug log4cxx_d)
target_link_libraries(roommgr optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(roommgr apr-1)
target_link_libraries(roommgr aprutil-1)

target_link_libraries(roommgr iniparser)
