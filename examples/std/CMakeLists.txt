﻿# Zip Utils - clean, elegant, simple
#
# See https://www.wischik.com/lu/programmer/zip_utils.html

cmake_minimum_required(VERSION 3.10)
project(zip-utils-std LANGUAGES CXX)

## Package information.
set(PACKAGE_NAME   "zip-utils-std")

project(${PACKAGE_NAME}
  VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}.${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}
  LANGUAGES CXX)

message(STATUS "[common]: ${PACKAGE_NAME} version: ${PACKAGE_VERSION}.")

add_executable(${PACKAGE_NAME}
  ${ZU_ROOT_DIR}/XZip.h
  ${ZU_ROOT_DIR}/XUnzip.h
  ${ZU_ROOT_DIR}/XZresult.h
  ${ZU_ROOT_DIR}/XZip.cpp
  ${ZU_ROOT_DIR}/XUnzip.cpp
  std.cpp
)

if (ZU_OS_WIN)
  target_sources(${PACKAGE_NAME}
    PRIVATE
      resource.h
      ${ZU_BINARY_DIR}/manifests/enable-visual-styles.manifest
      ${ZU_ROOT_DIR}/build/win/manifests/dpi-aware.manifest
      ${ZU_ROOT_DIR}/build/win/manifests/heap-type.manifest
      ${ZU_ROOT_DIR}/build/win/manifests/supported-os.manifest
      ${ZU_ROOT_DIR}/build/win/manifests/ultra-high-scroll-resolution.manifest
      ${ZU_ROOT_DIR}/build/win/manifests/utf-8-code-page.manifest
      ${ZU_ROOT_DIR}/build/win/resource_scripts/windows_app_base.rc
  )
endif (ZU_OS_WIN)

target_include_directories(${PACKAGE_NAME}
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${ZU_BINARY_DIR}/build
)

target_compile_definitions(${PACKAGE_NAME}
  PRIVATE
    ZIP_STD=1
    STRICT=1
    _CRT_SECURE_NO_WARNINGS=1
    SRC_PRODUCT_INTERNAL_NAME_STRING="${PACKAGE_NAME}"
    SRC_PRODUCT_ORIGINAL_NAME_STRING="${PACKAGE_NAME}.exe"
)

set_target_properties(${PACKAGE_NAME}
  PROPERTIES
    VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
    SOVERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
)
