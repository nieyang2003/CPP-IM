file(GLOB SOURCES "*.cc" "*.cpp")
add_library(lib_transfer STATIC ${SOURCES})

add_executable(transfer_mysql mysql/main.cc)
target_link_libraries(transfer_mysql PRIVATE 
    lib_transfer
	lib_proto
    gflags
    Poco::Data
    Poco::DataMySQL
    ${RDKAFKA_LIB}
    ${RDKAFKA_PLUSPLUS_LIB}
)

add_executable(transfer_cassandra cassandra/main.cc)
target_link_libraries(transfer_cassandra PRIVATE 
    lib_transfer
	lib_proto
    gflags
    Poco::Data
    Poco::DataMySQL
    ${HIREDIS_LIB}
	${REDIS_PLUS_PLUS_LIB}
    ${RDKAFKA_LIB}
    ${RDKAFKA_PLUSPLUS_LIB}
    ${CASSANDRA_LIB}
)