cmake_minimum_required(VERSION 3.24)
project(TensorStorePrebuild)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use ccache if available to speed up repeated TensorStore compilation in CI
find_program(CCACHE_EXECUTABLE ccache)
if(CCACHE_EXECUTABLE)
    message(STATUS "ccache found: ${CCACHE_EXECUTABLE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}" CACHE STRING "" FORCE)
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_EXECUTABLE}" CACHE STRING "" FORCE)
endif()

include(FetchContent)

# Keep URL + SHA256 in sync with root CMakeLists.txt
# Cache key hashes both this file and root CMakeLists.txt
FetchContent_Declare(
  tensorstore
  URL "https://github.com/google/tensorstore/archive/refs/tags/v0.1.80.tar.gz"
  URL_HASH SHA256=94866de34b6139d77d30e828a50f9e8df98e7dd68e848393470879aeb50ea7bf
)

FetchContent_MakeAvailable(tensorstore)

# Executable target forces TensorStore and all its transitive deps to be compiled.
# No Python, no pybind11, no filepattern needed.
add_executable(ts_prebuild_marker ts_prebuild_marker.cc)
target_link_libraries(ts_prebuild_marker PRIVATE
  tensorstore::tensorstore
  tensorstore::all_drivers
)
