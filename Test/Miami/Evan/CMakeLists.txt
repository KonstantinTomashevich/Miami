file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_executable(TestEvan ${SOURCES} ${HEADERS})
target_link_libraries(TestEvan Boost::unit_test_framework Evan)