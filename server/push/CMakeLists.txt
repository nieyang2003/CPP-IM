file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_push STATIC ${SOURCES})
add_executable(push_server main.cc)

target_link_libraries(push_server PRIVATE
    lib_push
	lib_proto
    gflags
    Poco::Foundation
    Poco::Net
    Poco::Data
    Poco::DataMySQL
    ${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
)