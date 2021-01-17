file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_library(Messaging ${SOURCES} ${HEADERS})
target_link_libraries(Messaging Hotline Richard)