cmake_minimum_required(VERSION 3.15)
project(StardustParticleSystem C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/material_types.c
    src/particle_pool.c
    src/spatial_chunks.c
    src/particle_system.c
    src/black_hole.c
    src/gdextension_interface.c
)

# Header files
set(HEADERS
    include/material_types.h
    include/particle_pool.h
    include/spatial_chunks.h
    include/particle_system.h
    include/black_hole.h
    include/gdextension_interface.h
)

# Create static library
add_library(stardust_particles STATIC ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(stardust_particles PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stardust_particles PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Build options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_SHARED_LIBS "Build as shared library" OFF)

if(BUILD_SHARED_LIBS)
    set_target_properties(stardust_particles PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME "stardust_particles"
    )
else()
    set_target_properties(stardust_particles PROPERTIES
        OUTPUT_NAME "stardust_particles"
    )
endif()

# Testing
if(BUILD_TESTS)
    add_executable(test_particle_system
        tests/test_particle_system.c
        ${SOURCES} ${HEADERS}
    )
    target_include_directories(test_particle_system PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(test_particle_system m)
endif()