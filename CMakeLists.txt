# Copyright (c) 2018-2022 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0057 NEW)
project(YOMM2 VERSION 1.3.0)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost dependency
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(find_or_download_package)
find_or_download_package(Boost INSTALL_WITH_YOMM)
message(STATUS "Using Boost libraries from ${Boost_INCLUDE_DIRS}")

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-save-temps)
  endif()
endif()

if(MSVC)
  add_compile_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/EHsc)
endif()

option(YOMM2_DEBUG_MACROS "Set to ON to debug macros" OFF)
if(${YOMM2_DEBUG_MACROS})
  message(STATUS "Macro debugging enabled")
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CMAKE_SOURCE_DIR}/dev/ppfc)
endif()

add_subdirectory(src)

option(YOMM2_ENABLE_TESTS "Set to ON to build tests" OFF)
include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(YOMM2_ENABLE_BENCHMARKS
  "Set to ON to build benchmarks" OFF
  "YOMM2_ENABLE_TESTS" OFF
)
if(${YOMM2_ENABLE_TESTS})
  message(STATUS "Tests enabled")
  if(${YOMM2_ENABLE_BENCHMARKS})
    message(STATUS "Benchmarks enabled")
    find_or_download_package(benchmark)
  endif()
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

option(YOMM2_ENABLE_EXAMPLES "Set to ON to build examples" ON)
if(${YOMM2_ENABLE_EXAMPLES})
  message(STATUS "Examples enabled")
  add_subdirectory(examples)
endif()

option(YOMM2_ENABLE_DOC "Set to ON to generate tutorials and reference" OFF)

if(${YOMM2_ENABLE_DOC})
  message(STATUS "Documentation generation enabled")
    set(readme_md "${CMAKE_SOURCE_DIR}/README.md")
    set(readme_cpp "${CMAKE_SOURCE_DIR}/tutorials/readme.cpp")
    add_custom_command(
        OUTPUT "${readme_md}"
        COMMAND ${CMAKE_SOURCE_DIR}/dev/code2md "${readme_cpp}" "${readme_md}"
        DEPENDS "${readme_cpp}")
    add_custom_target(README ALL DEPENDS "${readme_md}")
    add_subdirectory(reference.in)
  add_subdirectory(tutorials)
endif()

## Install instruction
# Create version file for cmake package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  YOMM2ConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY SameMajorVersion
)
# Create targets file of yomm2
install(EXPORT YOMM2Targets
  FILE YOMM2Targets.cmake
  NAMESPACE YOMM2::
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/YOMM2
)
# Configure package config (tells using code about dependencies)
configure_package_config_file(
  cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/YOMM2Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/YOMM2
)
# Copy config files to install directory
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/YOMM2Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/YOMM2ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/YOMM2
)

install(DIRECTORY include/yorel DESTINATION include)
