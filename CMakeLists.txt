cmake_minimum_required(VERSION 3.16)
project(patika_c VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(PATIKA_BUILD_TESTS "Build tests" ON)

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

install(FILES include/patika.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/patika/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/patika
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# TESTS
# ============================================================================
if(PATIKA_BUILD_TESTS)
    enable_testing()
    
    # Download Unity if not present
    include(FetchContent)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.5.2
    )
    FetchContent_MakeAvailable(unity)
    
    # Create a static library for testing (includes internal symbols)
    add_library(patika_core_test STATIC
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
    
    target_include_directories(patika_core_test
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_compile_definitions(patika_core_test
        PRIVATE
            PATIKA_EXPORTS
            $<$<CONFIG:Debug>:PATIKA_DEBUG=1>
    )
    
    target_link_libraries(patika_core_test PRIVATE Threads::Threads)
    
    if(UNIX AND NOT APPLE)
        target_link_libraries(patika_core_test PRIVATE m)
    endif()
    
    # Helper function to create tests
    function(add_patika_test test_name)
        add_executable(${test_name} src/tests/${test_name}.c)
        target_link_libraries(${test_name} 
            PRIVATE 
            patika_core_test
            unity
            Threads::Threads
        )
        target_include_directories(${test_name} 
            PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()
    
    # Unit Tests
    add_patika_test(test_agent_pool)
    add_patika_test(test_barrack_pool)
    add_patika_test(test_map)
    add_patika_test(test_mpsc_queue)
    add_patika_test(test_spsc_queue)
    add_patika_test(test_rng)
    add_patika_test(test_commands)
    add_patika_test(test_pathfinding)
    
    # Integration Tests
    add_patika_test(test_integration_basic)
    add_patika_test(test_integration_multi_agent)
    
    # Stress Tests
    add_patika_test(test_stress_queues)
    add_patika_test(test_stress_capacity)
    
    message(STATUS "Tests enabled - run with: ctest --output-on-failure")
endif()

# Copy test runner scripts to build directory
if(PATIKA_BUILD_TESTS AND UNIX)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
        ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
        COPYONLY
    )
    file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )
endif()

message(STATUS "")
message(STATUS "Patika Pathfinding Library v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Tests: ${PATIKA_BUILD_TESTS}")
message(STATUS "")
