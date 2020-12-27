cmake_minimum_required(VERSION 3.12.4)
project(Miami)

if (MIAMI_ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif ()

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(Framework)

if (MIAMI_APPS)
    add_subdirectory(App)
endif ()

if (MIAMI_UNIT_TESTS)
    add_subdirectory(Test)
endif ()
