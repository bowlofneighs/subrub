cmake_minimum_required(VERSION 3.15)
project(subrub VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find CURL
find_package(CURL REQUIRED)

# Find PocketSphinx (modern version 5.0+ has no SphinxBase dependency)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POCKETSPHINX REQUIRED pocketsphinx)

# Find cJSON
pkg_check_modules(CJSON REQUIRED libcjson)

# Find espeak-ng
pkg_check_modules(ESPEAK REQUIRED espeak-ng)

# Add executable
add_executable(subrub main.c)

# Include directories
target_include_directories(subrub PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${POCKETSPHINX_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
    ${ESPEAK_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(subrub PRIVATE
    CURL::libcurl
    ${POCKETSPHINX_LIBRARIES}
    ${CJSON_LIBRARIES}
    ${ESPEAK_LIBRARIES}
    m
)

# Add library directories
target_link_directories(subrub PRIVATE
    ${POCKETSPHINX_LIBRARY_DIRS}
    ${CJSON_LIBRARY_DIRS}
    ${ESPEAK_LIBRARY_DIRS}
)

# Compiler flags
target_compile_options(subrub PRIVATE
    -Wall
    -Wextra
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O3>
)

# Install target
install(TARGETS subrub DESTINATION bin)

# Display configuration info
message(STATUS "Building subrub")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  PocketSphinx found: ${POCKETSPHINX_FOUND}")
message(STATUS "  PocketSphinx version: ${POCKETSPHINX_VERSION}")
message(STATUS "  cJSON found: ${CJSON_FOUND}")
message(STATUS "  espeak-ng found: ${ESPEAK_FOUND}")