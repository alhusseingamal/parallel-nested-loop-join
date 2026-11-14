cmake_minimum_required(VERSION 3.18)
project(parallel_db LANGUAGES CXX CUDA)

# Set C++ and CUDA standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Specify target GPU architectures
set(CMAKE_CUDA_ARCHITECTURES 61 75 86 89)

set(SOURCE_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Source files
set(SOURCES
    ${SOURCE_FILE_DIR}/main.cpp
    ${SOURCE_FILE_DIR}/config_loader.cpp
    ${SOURCE_FILE_DIR}/db_utils.cpp
    ${SOURCE_FILE_DIR}/joins.cu
)

# Create executable
add_executable(program ${SOURCES})

# Ensure joins.cu is treated as CUDA
set_source_files_properties(joins.cu PROPERTIES LANGUAGE CUDA)

# Force nvcc to be used for linking and disable separable compilation
set_target_properties(program PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    CUDA_SEPARABLE_COMPILATION OFF
    LINKER_LANGUAGE CUDA
    POSITION_INDEPENDENT_CODE ON
)

# Optional: Pass flags to nvcc to match manual build behavior
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -fPIC")


# Optional: Uncomment and adjust if you reintroduce include directories
set(UTILS_DIR "src/utils")
set(UTILS_DIR "src/headers")
message(STATUS "${HEADERS_DIR}")
message(STATUS "${UTILS_DIR}")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(program
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/headers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)