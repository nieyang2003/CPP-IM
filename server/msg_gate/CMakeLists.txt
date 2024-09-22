# g++ -std=c++20 -pthread -o server main.cc -lprotobuf -lz -lssl -lcrypto -luv -luSockets

file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_msg STATIC ${SOURCES})
add_executable(msg_server main.cc)

find_library(LIB_USOCKETS uSockets REQUIRED)
find_library(LIB_UV uSockets REQUIRED)

target_link_libraries(msg_server PRIVATE
	lib_msg
	protobuf::libprotobuf
	gRPC::grpc
	gRPC::grpc++
    gflags
    Poco::Foundation
    Poco::Net
	OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
	${LIB_USOCKETS}
	${LIB_UV}
)