# g++ -std=c++20 -pthread -o server main.cc -lprotobuf -lz -lssl -lcrypto -luv -luSockets

file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_msg_gate STATIC ${SOURCES})
add_executable(msg_gate_server main.cc)

find_library(LIB_USOCKETS uSockets REQUIRED)
find_library(LIB_UV uSockets REQUIRED)

target_link_libraries(msg_gate_server PRIVATE
    lib_msg_gate
	lib_proto
    gflags
    Poco::Foundation
    Poco::Net
    Poco::Data
    Poco::DataMySQL
	OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
	${LIB_USOCKETS}
	${LIB_UV}
    ${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
)