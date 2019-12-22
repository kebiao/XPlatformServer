PROJECT(XPlatformServer)

# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成目标
add_executable(robot ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

# 添加链接库
target_link_libraries(robot common)
target_link_libraries(robot log)
target_link_libraries(robot server)
target_link_libraries(robot protos)
target_link_libraries(robot resmgr)
target_link_libraries(robot eventsystem)

IF(UNIX)
target_link_libraries(robot expat)
target_link_libraries(robot event event_core event_extra)
target_link_libraries(robot protobuf protoc)

target_link_libraries(robot gflags_nothreads)
target_link_libraries(robot log4cxx)

target_link_libraries(robot pthread)

ELSE(UNIX)
target_link_libraries(robot xml)

target_link_libraries(robot libevent libevent_core libevent_extras)
target_link_libraries(robot debug libprotobufd debug libprotocd)
target_link_libraries(robot optimized libprotobuf optimized libprotoc)

target_link_libraries(robot debug gflags_nothreads_static_d shlwapi)
target_link_libraries(robot optimized gflags_nothreads_static shlwapi)

target_link_libraries(robot debug log4cxx_d)
target_link_libraries(robot optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(robot apr-1)
target_link_libraries(robot aprutil-1)

target_link_libraries(robot iniparser)
