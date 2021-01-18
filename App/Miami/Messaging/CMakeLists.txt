file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_library(Messaging ${SOURCES} ${HEADERS})
# TODO: Separate Richard data container from main framework, so messaging
#       would depend only on data container instead of whole framework.
target_link_libraries(Messaging Hotline Richard)