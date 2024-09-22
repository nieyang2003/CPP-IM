find_package(GTest REQUIRED)
find_package(cpr REQUIRED)
include_directories(thirdparty)

add_executable(test test.cc thirdparty/easywsclient/easywsclient.cpp)
# 链接GTest库和其他依赖库
target_link_libraries(
    test PRIVATE 
    lib_proto
    GTest::GTest
    GTest::Main
    cpr::cpr
    gflags
)