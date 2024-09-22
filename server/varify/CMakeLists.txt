file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_varify STATIC ${SOURCES})
add_executable(varify_server main.cc)

target_link_libraries(varify_server PRIVATE 
	lib_varify
	lib_proto
    gflags
    Poco::Foundation
    Poco::Net
	${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
)