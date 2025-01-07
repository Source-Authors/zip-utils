# Zip Utils - clean, elegant, simple
#
# See https://www.wischik.com/lu/programmer/zip_utils.html

cmake_minimum_required(VERSION 3.10)
project(zip-utils-test LANGUAGES CXX)

## Package information.
set(PACKAGE_NAME   "zip-utils-test")

project(${PACKAGE_NAME}
  VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}.${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}
  LANGUAGES CXX)

message(STATUS "[common]: ${PACKAGE_NAME} version: ${PACKAGE_VERSION}.")

add_executable(${PACKAGE_NAME}
  ${ZIP_UTILS_ROOT_DIR}/XZip.h
  ${ZIP_UTILS_ROOT_DIR}/XUnzip.h
  ${ZIP_UTILS_ROOT_DIR}/XZresult.h
  test.cpp
)

if (ZIP_UTILS_OS_WIN)
  target_sources(${PACKAGE_NAME}
    PRIVATE
      resource.h
      test.rc
      ${ZIP_UTILS_BINARY_DIR}/manifests/enable-visual-styles.manifest
      ${ZIP_UTILS_ROOT_DIR}/build/win/manifests/dpi-aware.manifest
      ${ZIP_UTILS_ROOT_DIR}/build/win/manifests/heap-type.manifest
      ${ZIP_UTILS_ROOT_DIR}/build/win/manifests/supported-os.manifest
      ${ZIP_UTILS_ROOT_DIR}/build/win/manifests/ultra-high-scroll-resolution.manifest
      ${ZIP_UTILS_ROOT_DIR}/build/win/manifests/utf-8-code-page.manifest
      ${ZIP_UTILS_ROOT_DIR}/build/win/resource_scripts/windows_app_base.rc
  )
endif (ZIP_UTILS_OS_WIN)

target_link_libraries(${PACKAGE_NAME} zip-utils)

target_include_directories(${PACKAGE_NAME}
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${ZIP_UTILS_BINARY_DIR}/build
)

target_compile_definitions(${PACKAGE_NAME}
  PRIVATE
    STRICT=1
    SRC_PRODUCT_INTERNAL_NAME_STRING="${PACKAGE_NAME}"
    SRC_PRODUCT_ORIGINAL_NAME_STRING="${PACKAGE_NAME}.exe"
)

target_compile_definitions(${PACKAGE_NAME}
  PRIVATE
    STRICT=1
)

set_target_properties(${PACKAGE_NAME}
  PROPERTIES
    VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
    SOVERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
)
