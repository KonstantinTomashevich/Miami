file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_library(Richard ${SOURCES} ${HEADERS})
target_link_libraries(Richard Evan)