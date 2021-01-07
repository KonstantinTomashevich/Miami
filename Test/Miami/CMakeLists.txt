include_directories(${CMAKE_SOURCE_DIR}/Framework)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

macro(add_test_subdirectory DIRECTORY_NAME)
    add_subdirectory(${DIRECTORY_NAME})

    get_property(${DIRECTORY_NAME}_TEST_TARGETS
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY_NAME}
            PROPERTY BUILDSYSTEM_TARGETS)

    set_target_properties(${${DIRECTORY_NAME}_TEST_TARGETS}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Test)
endmacro()

add_test_subdirectory(Disco)
add_test_subdirectory(Hotline)
add_test_subdirectory(Janitor)
add_test_subdirectory(Richard)