PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成目标
add_executable(halls ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(halls common)
target_link_libraries(halls log)
target_link_libraries(halls server)
target_link_libraries(halls protos)
target_link_libraries(halls resmgr)
target_link_libraries(halls eventsystem)

IF(UNIX)
target_link_libraries(halls expat)
target_link_libraries(halls event event_core event_extra)
target_link_libraries(halls protobuf protoc)

target_link_libraries(halls gflags_nothreads)
target_link_libraries(halls log4cxx)

target_link_libraries(halls pthread)

ELSE(UNIX)
target_link_libraries(halls xml)

target_link_libraries(halls libevent libevent_core libevent_extras)
target_link_libraries(halls debug libprotobufd debug libprotocd)
target_link_libraries(halls optimized libprotobuf optimized libprotoc)

target_link_libraries(halls debug gflags_nothreads_static_d shlwapi)
target_link_libraries(halls optimized gflags_nothreads_static shlwapi)

target_link_libraries(halls debug log4cxx_d)
target_link_libraries(halls optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(halls apr-1)
target_link_libraries(halls aprutil-1)

target_link_libraries(halls iniparser)