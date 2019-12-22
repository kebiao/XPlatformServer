PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成目标
add_executable(machine ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(machine common)
target_link_libraries(machine log)
target_link_libraries(machine server)
target_link_libraries(machine protos)
target_link_libraries(machine resmgr)
target_link_libraries(machine eventsystem)

IF(UNIX)
target_link_libraries(machine expat)
target_link_libraries(machine event event_core event_extra)
target_link_libraries(machine protobuf protoc)

target_link_libraries(machine gflags_nothreads)
target_link_libraries(machine log4cxx)

target_link_libraries(machine pthread)

ELSE(UNIX)
target_link_libraries(machine xml)

target_link_libraries(machine libevent libevent_core libevent_extras)
target_link_libraries(machine debug libprotobufd debug libprotocd)
target_link_libraries(machine optimized libprotobuf optimized libprotoc)

target_link_libraries(machine debug gflags_nothreads_static_d shlwapi)
target_link_libraries(machine optimized gflags_nothreads_static shlwapi)

target_link_libraries(machine debug log4cxx_d)
target_link_libraries(machine optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(machine apr-1)
target_link_libraries(machine aprutil-1)

target_link_libraries(machine iniparser)
