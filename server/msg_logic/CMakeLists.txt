file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_msg_logic STATIC ${SOURCES})
add_executable(msg_logic_server main.cc)

target_link_libraries(msg_logic_server PRIVATE
    lib_msg_logic
	lib_proto
    gflags
    Poco::Foundation
    Poco::Net
    Poco::Data
    Poco::DataMySQL
    ${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
    ${RDKAFKA_LIB}
    ${RDKAFKA_PLUSPLUS_LIB}
)