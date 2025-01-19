# Zip Utils - clean, elegant, simple
#
# See https://www.wischik.com/lu/programmer/zip_utils.html

cmake_minimum_required(VERSION 3.10)
project(zip-utils LANGUAGES CXX)

# BUILD_SHARED_LIBS is a standard CMake variable, but we declare it here to make
# it prominent in the GUI.
option(BUILD_SHARED_LIBS "Build shared libraries(DLLs)." OFF)

# Use static MSVC runtime.
option(ZU_ENABLE_STATIC_MSVC_RUNTIME "Build with static MSVC runtime." ON)

# Use Address Sanitizer.
option(ZU_ENABLE_ASAN "Build with Address Sanitizer." OFF)

# Use unit tests.
option(ZU_ENABLE_UNIT_TESTS "Build and run unit-tests." ON)

# Compiler id for Apple Clang is now AppleClang.
if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

# Setup root directory of the zip-utils project.
set(ZU_ROOT_DIR "${CMAKE_SOURCE_DIR}" CACHE STRING
  "Directory root of the zip-utils project.")

## Package information.
set(PACKAGE_NAME   "zip-utils")

# C++ standard can be overridden when this is used as a sub-project.
if(NOT CMAKE_CXX_STANDARD)
  # This project requires C++17.
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif(NOT CMAKE_CXX_STANDARD)

# Git is required to build software version via Git.
find_package(Git)

if (NOT DEFINED PACKAGE_VERSION)
  if (NOT Git_FOUND)
    message(FATAL "[common]: Unable to extract Git software version - no Git.")
  endif()

  message(STATUS "[common]: Extracting Git software version.")

  if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    execute_process(COMMAND cmd /c "git rev-list --branches HEAD | find /v " " /c" OUTPUT_VARIABLE ZU_GIT_VERSION WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  else()
    execute_process(COMMAND bash "-c" "git rev-list --branches HEAD | wc -l | tr -d ' ' | tr -d '\n'" OUTPUT_VARIABLE ZU_GIT_VERSION WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  set(PACKAGE_VER_PATCH 2)
  math(EXPR PACKAGE_VER_TWEAK ${ZU_GIT_VERSION})
  message(STATUS
    "[common]: Build version from Git (patch.tweak): ${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}.")

  set(PACKAGE_VER_MAJOR 1)
  set(PACKAGE_VER_MINOR 5)

  set(PACKAGE_VERSION
    "${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}.${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}")
endif()
set(PACKAGE_STRING     "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME    "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT  "https://github.com/Source-Authors/zip-utils/issues")

# Default build type is Release unless overridden.
# On Windows default is Debug, so need to unify.
# See https://discourse.cmake.org/t/how-to-deal-with-ninja-setting-cmake-build-type-to-debug/281/3
set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build.")
set(ZU_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
  "CMake build type used to build ${PACKAGE_NAME}.")

project(${PACKAGE_NAME}
  VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}.${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}
  LANGUAGES CXX)

message(STATUS "[common]: CMake version: ${CMAKE_VERSION}.")
message(STATUS "[common]: ${PACKAGE_NAME} version: ${PACKAGE_VERSION}.")

## Common variables.

set(ZU_BINARY_DIR "${CMAKE_BINARY_DIR}" CACHE STRING
  "Directory of the zip-utils binaries output.")
set(ZU_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}" CACHE STRING
  "Name of the zip-utils build system.")

set(ZU_ENABLE_STD_ONLY OFF)

if (${ZU_SYSTEM_NAME} STREQUAL "Darwin")
  set(ZU_OS_MACOS ON CACHE BOOL "MacOS is build system.")
  set(ZU_ENABLE_STD_ONLY ON)
else()
  set(ZU_OS_MACOS OFF CACHE BOOL "MacOS is NOT build system.")
endif()

if (${ZU_SYSTEM_NAME} STREQUAL "Linux")
  set(ZU_OS_LINUX ON CACHE BOOL "Linux distro is build system.")
  set(ZU_ENABLE_STD_ONLY ON)
else()
  set(ZU_OS_LINUX OFF CACHE BOOL "Linux distro is NOT build system.")
endif()

if (${ZU_SYSTEM_NAME} STREQUAL "Windows")
  set(ZU_OS_WIN ON CACHE BOOL "Windows is build system.")
else()
  set(ZU_OS_WIN OFF CACHE BOOL "Windows is NOT build system.")
endif()

if (ZU_OS_MACOS OR ZU_OS_LINUX)
  set(ZU_OS_POSIX ON CACHE BOOL "POSIX compatible os is build system.")
else()
  set(ZU_OS_POSIX OFF CACHE BOOL "POSIX compatible os is NOT build system.")
endif()

set(ZU_PRODUCT_NAME "zip-utils" CACHE STRING
  "Name of the zip-utils product.")

set(ZU_CMAKE_GENERATOR "${CMAKE_GENERATOR}" CACHE STRING
  "CMake generator used to generate build tree." FORCE)
set(ZU_CXX_COMPILER "${CMAKE_CXX_COMPILER}" CACHE STRING
  "Compiler used to build ${PACKAGE_NAME}.")
set(ZU_CXX_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}" CACHE STRING
  "Compiler id used to build ${PACKAGE_NAME}.")
set(ZU_CXX_COMPILER_FRONTEND_VARIANT "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}"
  CACHE STRING "Compiler frontend used to build ${PACKAGE_NAME}.")
set(ZU_EXPORT_COMPILE_COMMANDS ${CMAKE_EXPORT_COMPILE_COMMANDS} CACHE BOOL
  "Enable/Disable output of compile commands during generation.")

if (ZU_CXX_COMPILER_ID MATCHES "Clang")
  set(ZU_COMPILER_CLANG ON CACHE BOOL "Compiler is Clang.")
else()
  set(ZU_COMPILER_CLANG OFF CACHE BOOL "Compiler is NOT Clang.")
endif()

if (ZU_CXX_COMPILER_ID STREQUAL "GNU")
  set(ZU_COMPILER_GCC ON CACHE BOOL "Compiler is GCC.")
else()
  set(ZU_COMPILER_GCC OFF CACHE BOOL "Compiler is NOT GCC.")
endif()

if (ZU_CXX_COMPILER_ID STREQUAL "MSVC")
  set(ZU_COMPILER_MSVC ON CACHE BOOL "Compiler is MSVC.")
else()
  set(ZU_COMPILER_MSVC OFF CACHE BOOL "Compiler is NOT MSVC.")
endif()

## Common dependencies.

if (Git_FOUND)
  message(STATUS
    "[common]: Git version ${GIT_VERSION_STRING} found at '${GIT_EXECUTABLE}'.")

  # Get the current working branch.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${ZU_ROOT_DIR}
    OUTPUT_VARIABLE ZU_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get the latest commit hash.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY ${ZU_ROOT_DIR}
    OUTPUT_VARIABLE ZU_GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get the subject of the commit.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" log -1 --format=%s
    WORKING_DIRECTORY "${ZU_ROOT_DIR}"
    OUTPUT_VARIABLE ZU_GIT_COMMIT_SUBJECT
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  message(STATUS
    "[common]: Git using branch '${ZU_GIT_BRANCH}', "
    "commit ${ZU_GIT_COMMIT_HASH} | ${ZU_GIT_COMMIT_SUBJECT}.")
endif()

message(STATUS
  "[common]: ${PACKAGE_NAME} using CMake generator: ${ZU_CMAKE_GENERATOR}.")

# Select correct threading library.
if (NOT ZU_OS_WIN)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()
find_package(Threads REQUIRED)
list(APPEND CMAKE_REQUIRED_LIBRARIES Threads::Threads)
if (NOT CMAKE_THREAD_LIBS_INIT)
  set(ZU_THREADS_LIBRARY "system default" CACHE STRING
    "Threads library used to build ${PACKAGE_NAME}.")
else()
  set(ZU_THREADS_LIBRARY "${CMAKE_THREAD_LIBS_INIT}" CACHE STRING
    "Threads library used to build ${PACKAGE_NAME}.")
endif()
message(STATUS
  "[common]: ${PACKAGE_NAME} using threading library: ${ZU_THREADS_LIBRARY}.")

if (XCODE_VERSION)
  message(STATUS
    "[common]: ${PACKAGE_NAME} using XCode version: ${XCODE_VERSION}.")
endif()

message(STATUS "[common]: ${PACKAGE_NAME} cxx compiler: ${ZU_CXX_COMPILER}.")
message(STATUS "[common]: ${PACKAGE_NAME} build type: ${ZU_BUILD_TYPE}.")

if (ZU_OS_WIN)
  set(SRC_PRODUCT_NAME "Zip Utils")

  # Generate common product version info.
  configure_file(
    ${ZU_ROOT_DIR}/build/product_version_config.h.cmake.in
    ${ZU_BINARY_DIR}/build/product_version_config.h
  )

  # Generate app version info.
  configure_file(
    ${ZU_ROOT_DIR}/build/app_version_config.h.cmake.in
    ${ZU_BINARY_DIR}/build/app_version_config.h
  )

  # Generate visual styles manifest info.
  configure_file(
    ${ZU_ROOT_DIR}/build/win/manifests/enable-visual-styles.manifest.cmake.in
    ${ZU_BINARY_DIR}/manifests/enable-visual-styles.manifest
  )
endif()

if (ZU_ENABLE_STD_ONLY)
  # Use only stdlib things.
  add_compile_definitions(ZIP_STD=1)
endif (ZU_ENABLE_STD_ONLY)

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use the highest warning level for Visual Studio.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

  # Disable C++ exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs- /EHc-")
  add_definitions(-D_HAS_EXCEPTIONS=0)

  # Disable RTTI.
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")

  if (ZU_ENABLE_ASAN)
    message(STATUS "[options]: ASAN enabled.")

    # Use AddressSanitizer.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
  endif (ZU_ENABLE_ASAN)

  if (ZU_ENABLE_STATIC_MSVC_RUNTIME)
    # Use static runtime.
    foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if (${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif()
    endforeach()
  endif (ZU_ENABLE_STATIC_MSVC_RUNTIME)
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use -Wall for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

  # Use -Wextra for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

  # Use -Werror for clang only.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
  endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

  # Disable C++ exceptions.
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  # Disable RTTI.
  string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

  if (ZU_ENABLE_ASAN)
    message(STATUS "[options]: ASAN enabled.")

    # Use AddressSanitizer.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  endif (ZU_ENABLE_ASAN)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

add_library(${PACKAGE_NAME} "")

target_sources(${PACKAGE_NAME}
  PRIVATE
    XZip.cpp
    XUnzip.cpp

  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/XZip.h>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/XUnzip.h>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/XZresult.h>
)

target_include_directories(${PACKAGE_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)

if (ZU_OS_WIN)
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      STRICT=1
      _CRT_SECURE_NO_WARNINGS=1
  )
endif (ZU_OS_WIN)

set_target_properties(${PACKAGE_NAME}
  PROPERTIES
    VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
    SOVERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
)

if (BUILD_SHARED_LIBS)
  if (ZU_OS_WIN)
    target_sources(${PACKAGE_NAME}
      PRIVATE
        resource.h
        ${ZU_ROOT_DIR}/build/win/resource_scripts/windows_dll_base.rc
    )
  endif (ZU_OS_WIN)

  target_include_directories(${PACKAGE_NAME}
    PRIVATE
      ${PROJECT_SOURCE_DIR}
      ${ZU_BINARY_DIR}/build
  )

  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      ZU_ZIP_BUILD_ITSELF=1
      ZU_ZIP_BUILD_SHARED=1
      ZU_UNZIP_BUILD_ITSELF=1
      ZU_UNZIP_BUILD_SHARED=1

    INTERFACE
      ZU_ZIP_BUILD_SHARED=1
      ZU_UNZIP_BUILD_SHARED=1
  )
  
  if (ZU_OS_WIN)
    target_compile_definitions(${PACKAGE_NAME}
      PRIVATE
        SRC_PRODUCT_INTERNAL_NAME_STRING="${PACKAGE_NAME}"
        SRC_PRODUCT_ORIGINAL_NAME_STRING="${PACKAGE_NAME}.dll"
    )
  endif (ZU_OS_WIN)
endif(BUILD_SHARED_LIBS)

if (ZU_ENABLE_UNIT_TESTS)
  enable_testing()

  add_subdirectory(examples/modify)
  add_test(NAME zip-utils-modify COMMAND zip-utils-modify)

  if (ZU_OS_WIN)
    add_subdirectory(examples/progress)
    add_test(NAME zip-utils-progress COMMAND zip-utils-progress)

    add_subdirectory(examples/simple)
    add_test(NAME zip-utils-simple COMMAND zip-utils-simple)

    add_subdirectory(examples/test)
    add_test(NAME zip-utils-test COMMAND zip-utils-test)
  endif (ZU_OS_WIN)

  add_subdirectory(examples/std)
  add_test(NAME zip-utils-std COMMAND zip-utils-std)
endif (ZU_ENABLE_UNIT_TESTS)

