file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

add_executable(Client ${SOURCES} ${HEADERS})
target_link_libraries(Client Messaging Evan Hotline Disco)
set_target_properties(Client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin"
        # Workaround for Visual Studio generator, that remove unnecessary Debug/Release directories.
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin)