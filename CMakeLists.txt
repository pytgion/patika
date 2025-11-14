cmake_minimum_required(VERSION 3.16)
project(patika_c VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(patika_core_c SHARED
    src/patika_core.c
    src/patika_mpsc.c
    src/patika_spsc.c
    src/patika_pool.c
    src/patika_map.c
    src/patika_pathfinding.c
    src/patika_snapshot.c
    src/patika_rng.c
    src/patika_commands.c
    src/patika_log.c
)

target_include_directories(patika_core_c
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_compile_definitions(patika_core_c
    PRIVATE
        PATIKA_EXPORTS
        $<$<CONFIG:Debug>:PATIKA_DEBUG=1>
)

if(MSVC)
    target_compile_options(patika_core_c PRIVATE /W4)
else()
    target_compile_options(patika_core_c PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O3>
    )
endif()

# Thread library
find_package(Threads REQUIRED)
target_link_libraries(patika_core_c PRIVATE Threads::Threads)

# Math library (Unix)
if(UNIX AND NOT APPLE)
    target_link_libraries(patika_core_c PRIVATE m)
endif()

set_target_properties(patika_core_c PROPERTIES
    OUTPUT_NAME "patika_core"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
)

include(GNUInstallDirs)

install(TARGETS patika_core_c
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "")
message(STATUS "Patika Pathfinding Library v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "")
