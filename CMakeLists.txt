cmake_minimum_required(VERSION 3.19)
project(Osspan)

set(APP_NAME Osspan)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
set(BUILD_SAMPLE OFF CACHE BOOL "")
add_subdirectory(osssdk)
add_subdirectory(yaml)

set(CPACK_GENERATOR "OSSPAN")
include(CPack)
