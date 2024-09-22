file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_route STATIC ${SOURCES})
add_executable(route_server main.cc)

target_link_libraries(route_server PRIVATE 
	lib_route
	protobuf::libprotobuf
	gRPC::grpc
	gRPC::grpc++
    gflags
)