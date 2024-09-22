file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_route STATIC ${SOURCES})
add_executable(route_server main.cc)

target_link_libraries(route_server PRIVATE 
	lib_route
	lib_proto
    gflags
    ${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
)