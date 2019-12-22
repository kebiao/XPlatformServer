PROJECT(XPlatformServer)

file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

aux_source_directory(. DIR_LIB_SRCS)

add_executable(connector ${DIR_LIB_SRCS} ${CURRENT_HEADERS})

target_link_libraries(connector common)
target_link_libraries(connector log)
target_link_libraries(connector server)
target_link_libraries(connector protos)
target_link_libraries(connector resmgr)
target_link_libraries(connector eventsystem)

IF(UNIX)
target_link_libraries(connector expat)
target_link_libraries(connector event event_core event_extra)
target_link_libraries(connector protobuf protoc)

target_link_libraries(connector gflags_nothreads)
target_link_libraries(connector log4cxx)

target_link_libraries(connector pthread)

ELSE(UNIX)
target_link_libraries(connector xml)

target_link_libraries(connector libevent libevent_core libevent_extras)
target_link_libraries(connector debug libprotobufd debug libprotocd)
target_link_libraries(connector optimized libprotobuf optimized libprotoc)

target_link_libraries(connector debug gflags_nothreads_static_d shlwapi)
target_link_libraries(connector optimized gflags_nothreads_static shlwapi)

target_link_libraries(connector debug log4cxx_d)
target_link_libraries(connector optimized log4cxx)

ENDIF(UNIX)

target_link_libraries(connector apr-1)
target_link_libraries(connector aprutil-1)

target_link_libraries(connector iniparser)



