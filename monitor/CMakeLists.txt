cmake_minimum_required(VERSION 3.16)

option(NO_ISA_EXTENSIONS "Disable ISA extensions (don't pass -march=native or -mcpu=native to the compiler)" OFF)
option(NO_STATISTICS "Disable calculation of statistics" ON)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/version.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project(
    monitor
    LANGUAGES C CXX
    VERSION ${TRACY_VERSION_STRING}
)

set(TRACY_MANUAL_LIFETIME ON)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/vendor.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/server.cmake)


set(PROGRAM_FILES
    src/monitor.cpp
    ../public/TracyClient.cpp
)

add_executable(${PROJECT_NAME} ${PROGRAM_FILES} ${COMMON_FILES})
target_link_libraries(${PROJECT_NAME} bfd)
target_compile_options(${PROJECT_NAME} PRIVATE -DTRACY_ENABLE=1 -DTRACY_DELAYED_INIT -DTRACY_MANUAL_LIFETIME -DTRACY_NO_FRAME_IMAGE)
set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
