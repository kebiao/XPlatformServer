FILE(GLOB PROTO_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

FOREACH(PROTO_FILE ${PROTO_INPUT})
  message(STATUS "compile: ${PROTO_FILE}")
   EXEC_PROGRAM("\"${PROJECT_SOURCE_DIR}/deps/protobuf-3.2.0/vsprojects/x64/Release/protoc\" --proto_path=\"${CMAKE_CURRENT_SOURCE_DIR}\" --cpp_out=\"${CMAKE_CURRENT_SOURCE_DIR}\" \"${PROTO_FILE}\"" OUTPUT_VARIABLE X_OUTPUT)
   MESSAGE(STATUS ${X_OUTPUT})
ENDFOREACH(PROTO_FILE ${PROTO_INPUT})


# 在windows下VC能显示头文件
file(GLOB_RECURSE CURRENT_HEADERS *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS}) 

# 在windows下VC能显示proto文件
file(GLOB_RECURSE CURRENT_PROTOS *.proto)
source_group("Protos" FILES ${CURRENT_PROTOS}) 

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_LIB_SRCS 变量
aux_source_directory(. DIR_LIB_SRCS)

# 生成链接库
add_library (protos ${DIR_LIB_SRCS} ${CURRENT_HEADERS} ${CURRENT_PROTOS})
