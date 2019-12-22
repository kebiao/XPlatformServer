PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成目标
add_executable(hallsmgr ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(hallsmgr common)
target_link_libraries(hallsmgr log)
target_link_libraries(hallsmgr server)
target_link_libraries(hallsmgr protos)
target_link_libraries(hallsmgr resmgr)
target_link_libraries(hallsmgr eventsystem)

IF(UNIX)
target_link_libraries(hallsmgr expat)
target_link_libraries(hallsmgr event event_core event_extra)
target_link_libraries(hallsmgr protobuf protoc)

target_link_libraries(hallsmgr gflags_nothreads)
target_link_libraries(hallsmgr log4cxx)

target_link_libraries(hallsmgr pthread)

ELSE(UNIX)
target_link_libraries(hallsmgr xml)

target_link_libraries(hallsmgr libevent libevent_core libevent_extras)
target_link_libraries(hallsmgr debug libprotobufd debug libprotocd)
target_link_libraries(hallsmgr optimized libprotobuf optimized libprotoc)

target_link_libraries(hallsmgr debug gflags_nothreads_static_d shlwapi)
target_link_libraries(hallsmgr optimized gflags_nothreads_static shlwapi)

target_link_libraries(hallsmgr debug log4cxx_d)
target_link_libraries(hallsmgr optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(hallsmgr apr-1)
target_link_libraries(hallsmgr aprutil-1)

target_link_libraries(hallsmgr iniparser)
