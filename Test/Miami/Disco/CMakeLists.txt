file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_executable(TestDisco ${SOURCES} ${HEADERS})
target_link_libraries(TestDisco Boost::unit_test_framework Disco)
