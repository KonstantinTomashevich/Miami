include_directories(${CMAKE_SOURCE_DIR}/Framework)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_subdirectory(Disco)
add_subdirectory(Hotline)
add_subdirectory(Janitor)
add_subdirectory(Richard)