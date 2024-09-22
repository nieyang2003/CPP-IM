file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_varify STATIC ${SOURCES})
add_executable(varify_server main.cc)

# Find hiredis
find_path(HIREDIS_HEADER hiredis)
find_library(HIREDIS_LIB hiredis)

# Find redis-c++
find_path(REDIS_PLUS_PLUS_HEADER sw)
find_library(REDIS_PLUS_PLUS_LIB redis++)

target_include_directories(varify_server PRIVATE
    ${HIREDIS_HEADER}
    ${REDIS_PLUS_PLUS_HEADER}
)

target_link_libraries(varify_server PRIVATE 
	lib_varify
	protobuf::libprotobuf
	gRPC::grpc
	gRPC::grpc++
    gflags
    Poco::Foundation
    Poco::Net
	${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
)