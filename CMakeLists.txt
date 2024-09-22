cmake_minimum_required(VERSION 3.20)
project(ychat C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
include_directories(server)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

find_package(Poco REQUIRED COMPONENTS Foundation Net Util Data DataMySQL)
find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# 找到 hiredis
find_path(HIREDIS_HEADER hiredis)
find_library(HIREDIS_LIB hiredis)

# 找到 redis-c++
find_path(REDIS_PLUS_PLUS_HEADER sw)
find_library(REDIS_PLUS_PLUS_LIB redis++)

# 找到librdkafka库和rdkafka++
find_library(RDKAFKA_LIB rdkafka REQUIRED)
find_library(RDKAFKA_PLUSPLUS_LIB rdkafka++ REQUIRED)

# 找到libcassandra库
find_library(CASSANDRA_LIB cassandra REQUIRED)

# 添加头文件目录和库
include_directories(
    ${HIREDIS_HEADER}
    ${REDIS_PLUS_PLUS_HEADER}
)

add_subdirectory(server)
add_subdirectory(tests)