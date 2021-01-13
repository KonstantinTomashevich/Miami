find_package(Boost COMPONENTS headers system REQUIRED)
file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_library(Hotline ${SOURCES} ${HEADERS})
target_link_libraries(Hotline Evan Disco Boost::headers Boost::system)