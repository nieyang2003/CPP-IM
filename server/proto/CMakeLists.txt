file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_proto ${SOURCES})
target_link_libraries(lib_proto PUBLIC protobuf::libprotobuf gRPC::grpc gRPC::grpc++)
target_include_directories(lib_proto PUBLIC ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})