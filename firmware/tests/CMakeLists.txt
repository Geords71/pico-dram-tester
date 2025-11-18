cmake_minimum_required(VERSION 3.11)

set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/..)

project(test_pmemtest LANGUAGES C)

find_package(PicoTest)

add_executable(test_pmemtest)
target_sources(test_pmemtest
    PRIVATE
        main.c
        test_something.c
)
target_link_libraries(test_pmemtest
    PRIVATE
        PicoTest::PicoTest
)
enable_testing()
picotest_discover_tests(test_pmemtest
    TEST_LIST_OPTION "-l"
)