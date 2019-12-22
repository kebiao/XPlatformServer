PROJECT(XPlatformServer)

file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

aux_source_directory(. DIR_LIB_SRCS)

add_executable(directory ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

target_link_libraries(directory common)
target_link_libraries(directory log)
target_link_libraries(directory server)
target_link_libraries(directory protos)
target_link_libraries(directory resmgr)
target_link_libraries(directory eventsystem)

IF(UNIX)
target_link_libraries(directory expat)
target_link_libraries(directory event event_core event_extra)
target_link_libraries(directory protobuf protoc)

target_link_libraries(directory gflags_nothreads)
target_link_libraries(directory log4cxx)

target_link_libraries(directory pthread)

ELSE(UNIX)
target_link_libraries(directory xml)

target_link_libraries(directory libevent libevent_core libevent_extras)
target_link_libraries(directory debug libprotobufd debug libprotocd)
target_link_libraries(directory optimized libprotobuf optimized libprotoc)

target_link_libraries(directory debug gflags_nothreads_static_d shlwapi)
target_link_libraries(directory optimized gflags_nothreads_static shlwapi)

target_link_libraries(directory debug log4cxx_d)
target_link_libraries(directory optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(directory apr-1)
target_link_libraries(directory aprutil-1)

target_link_libraries(directory iniparser)

