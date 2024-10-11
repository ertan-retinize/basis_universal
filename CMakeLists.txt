# Important: The Basis Universal encoder and transcoder libraries must be compiled with -fno-strict-aliasing.
cmake_minimum_required(VERSION 3.5)

project(basisu)
option(STATIC "static linking" FALSE)
option(SAN "sanitize" FALSE)
option(ZSTD "ZSTD support for KTX2 transcoding/encoding" TRUE)
option(OPENCL "OpenCL support in encoder" FALSE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Configure for MSVC builds
if (MSVC)
    option(SSE "SSE 4.1 support" TRUE)
    if (CMAKE_GENERATOR_PLATFORM STREQUAL Win32)
        set(BUILD_X64 0)
    else()
        set(BUILD_X64 1)
    endif()
    add_compile_options(/W4)
else()
    option(SSE "SSE 4.1 support" FALSE)
    option(BUILD_X64 "build 64-bit" TRUE)
endif()

# Output the current configuration options for debugging purposes
message("Initial BUILD_X64=${BUILD_X64}")
message("Initial CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("Initial SSE=${SSE}")
message("Initial ZSTD=${ZSTD}")
message("Initial OPENCL=${OPENCL}")
message("Initial SAN=${SAN}")

# Configure ZSTD and OpenCL dependencies if enabled
if ((NOT MSVC) AND OPENCL)
    find_package(OpenCL)
    message(STATUS "OpenCL found: ${OPENCL_FOUND}")
    message(STATUS "OpenCL includes: ${OpenCL_INCLUDE_DIRS}")
    message(STATUS "OpenCL libraries: ${OpenCL_LIBRARIES}")
endif()

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set architecture flags
if (BUILD_X64)
    message("Building 64-bit")
else()
    message("Building 32-bit")
endif()

# Configure SSE and SIMD support
if (SSE)
    message("SSE enabled")
else()
    message("SSE disabled")
endif()

# Configure ZSTD support
if (ZSTD)
    message("Zstandard enabled")
else()
    message("Zstandard disabled")
endif()

# Compiler flags for non-MSVC builds
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

    if (SAN)
        message("Enabling SAN")
        set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize=alignment")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SANITIZE_FLAGS}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${SANITIZE_FLAGS}")
    endif()

    set(CMAKE_CXX_FLAGS -std=c++11)
    set(GCC_COMPILE_FLAGS "-fvisibility=hidden -fPIC -fno-strict-aliasing -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall -Wextra")
    set(GCC_CXX_COMPILE_FLAGS "-fvisibility=hidden -fPIC -fno-strict-aliasing -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall -Wextra")

    if (NOT BUILD_X64)
        set(GCC_COMPILE_FLAGS "${GCC_COMPILE_FLAGS} -m32")
        set(GCC_CXX_COMPILE_FLAGS "${GCC_CXX_COMPILE_FLAGS} -m32")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_COMPILE_FLAGS}")
else()
    # SSE support for MSVC builds
    if (SSE)
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -DBASISU_SUPPORT_SSE=1")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DBASISU_SUPPORT_SSE=1")
    else()
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -DBASISU_SUPPORT_SSE=0")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DBASISU_SUPPORT_SSE=0")
    endif()
endif()

# Define the source files for the static library
set(ENCODER_LIB_SRC_LIST
        encoder/basisu_backend.cpp
        encoder/basisu_basis_file.cpp
        encoder/basisu_comp.cpp
        encoder/basisu_enc.cpp
        encoder/basisu_etc.cpp
        encoder/basisu_frontend.cpp
        encoder/basisu_gpu_texture.cpp
        encoder/basisu_pvrtc1_4.cpp
        encoder/basisu_resampler.cpp
        encoder/basisu_resample_filters.cpp
        encoder/basisu_ssim.cpp
        encoder/basisu_uastc_enc.cpp
        encoder/basisu_bc7enc.cpp
        encoder/jpgd.cpp
        encoder/basisu_kernels_sse.cpp
        encoder/basisu_opencl.cpp
        encoder/pvpngreader.cpp
        encoder/basisu_astc_hdr_enc.cpp
        encoder/3rdparty/android_astc_decomp.cpp
        encoder/3rdparty/tinyexr.cpp
        transcoder/basisu_transcoder.cpp
)

# Add ZSTD support if enabled
if (ZSTD)
    set(ENCODER_LIB_SRC_LIST ${ENCODER_LIB_SRC_LIST} zstd/zstd.c transcoder_interface.cpp)
endif()

# Create the static library for the encoder
add_library(basisu_encoder STATIC ${ENCODER_LIB_SRC_LIST})

# Create the basisu shared library and link against the static library
add_library(basisu SHARED basisu_tool.cpp transcoder_interface.cpp)
target_link_libraries(basisu PRIVATE basisu_encoder)

# Create the example executable
add_executable(examples example/example.cpp)
target_link_libraries(examples PRIVATE basisu_encoder)

# Define ZSTD-related compile definitions
if (ZSTD)
    target_compile_definitions(basisu PRIVATE BASISD_SUPPORT_KTX2_ZSTD=1)
    target_compile_definitions(examples PRIVATE BASISD_SUPPORT_KTX2_ZSTD=1)
else()
    target_compile_definitions(basisu PRIVATE BASISD_SUPPORT_KTX2_ZSTD=0)
    target_compile_definitions(examples PRIVATE BASISD_SUPPORT_KTX2_ZSTD=0)
endif()

# Link against OpenCL if enabled
if (OPENCL AND OPENCL_FOUND)
    target_include_directories(basisu PRIVATE ${OpenCL_INCLUDE_DIRS})
    target_include_directories(examples PRIVATE ${OpenCL_INCLUDE_DIRS})
    target_include_directories(basisu_encoder PRIVATE ${OpenCL_INCLUDE_DIRS})
    target_link_libraries(basisu PRIVATE ${OpenCL_LIBRARIES})
    target_link_libraries(examples PRIVATE ${OpenCL_LIBRARIES})
else()
    message("OpenCL is disabled.")
endif()

# Add platform-specific configurations
if (MSVC)
    set_target_properties(basisu PROPERTIES
            RUNTIME_OUTPUT_NAME "basisu"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

    set_target_properties(examples PROPERTIES
            RUNTIME_OUTPUT_NAME "examples"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()
