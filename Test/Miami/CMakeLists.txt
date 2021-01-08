include_directories(${CMAKE_SOURCE_DIR}/Framework)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

macro(add_test_subdirectory DIRECTORY_NAME)
    add_subdirectory(${DIRECTORY_NAME})

    get_property(${DIRECTORY_NAME}_TEST_TARGETS
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY_NAME}
            PROPERTY BUILDSYSTEM_TARGETS)

    set_target_properties(${${DIRECTORY_NAME}_TEST_TARGETS}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Test
            # Workaround for Visual Studio generator, that remove unnecessary Debug/Release directories.
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Test
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Test)
endmacro()

set(TEST_TARGETS)
add_test_subdirectory(Disco)
add_test_subdirectory(Evan)
add_test_subdirectory(Hotline)
add_test_subdirectory(Janitor)
add_test_subdirectory(Richard)

set(TEST_TARGETS_EXECUTABLES ${TEST_TARGETS})

if (WIN32)
    list(TRANSFORM TEST_TARGETS_EXECUTABLES APPEND ".exe")
    set(SCRIPT_NAME "TestMiami.bat")
elseif (UNIX)
    list(TRANSFORM TEST_TARGETS_EXECUTABLES APPEND "./")
    set(SCRIPT_NAME "TestMiami.sh")
endif ()

list(JOIN TEST_TARGETS_EXECUTABLES " && " TEST_TARGETS_COMMAND)
file(WRITE "${CMAKE_BINARY_DIR}/Bin/Test/${SCRIPT_NAME}" "${TEST_TARGETS_COMMAND}")

add_custom_target(TestMiami
        COMMAND ${SCRIPT_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Test)

foreach (DEPENDENCY IN LISTS TEST_TARGETS)
    add_dependencies(TestMiami ${DEPENDENCY})
endforeach ()