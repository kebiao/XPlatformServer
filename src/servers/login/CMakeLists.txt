PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成目标
add_executable(login ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(login common)
target_link_libraries(login log)
target_link_libraries(login server)
target_link_libraries(login protos)
target_link_libraries(login resmgr)
target_link_libraries(login eventsystem)

IF(UNIX)
target_link_libraries(login expat)
target_link_libraries(login event event_core event_extra)
target_link_libraries(login protobuf protoc)

target_link_libraries(login gflags_nothreads)
target_link_libraries(login log4cxx)

target_link_libraries(login pthread)

ELSE(UNIX)
target_link_libraries(login xml)

target_link_libraries(login libevent libevent_core libevent_extras)
target_link_libraries(login debug libprotobufd debug libprotocd)
target_link_libraries(login optimized libprotobuf optimized libprotoc)

target_link_libraries(login debug gflags_nothreads_static_d shlwapi)
target_link_libraries(login optimized gflags_nothreads_static shlwapi)

target_link_libraries(login debug log4cxx_d)
target_link_libraries(login optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(login apr-1)
target_link_libraries(login aprutil-1)

target_link_libraries(login iniparser)
