# Copyright 2016 MongoDB Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(BSON_MAPPER)

set(LIBBSONCXX_REQUIRED_VERSION 3.0.2)
#set(LIBBSONCXX_REQUIRED_ABI_VERSION no_abi)
find_package(libbsoncxx ${LIBBSONCXX_REQUIRED_VERSION} REQUIRED)

# Update these as needed.
# TODO: read from file
set(BSON_MAPPER_VERSION_MAJOR 0)
set(BSON_MAPPER_VERSION_MINOR 0)
set(BSON_MAPPER_VERSION_PATCH 1)
set(BSON_MAPPER_VERSION_EXTRA "-alpha")
set(BSON_MAPPER_ABI_VERSION _noabi)

set(BSON_MAPPER_VERSION ${BSON_MAPPER_VERSION_MAJOR}.${BSON_MAPPER_VERSION_MINOR}.${BSON_MAPPER_VERSION_PATCH}${BSON_MAPPER_VERSION_EXTRA})
set(BSON_MAPPER_INLINE_NAMESPACE "v${BSON_MAPPER_ABI_VERSION}")
set(BSON_MAPPER_HEADER_INSTALL_DIR "include/bson_mapper/${BSON_MAPPER_INLINE_NAMESPACE}" CACHE INTERNAL "")

add_subdirectory(config)

set(bson_mapper_sources
   "file.cpp"
)

include_directories(
    ${LIBBSONCXX_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

link_directories(
    ${LIBBSONCXX_LIBRARY_DIRS}
)

add_library(bson_mapper_static STATIC
    ${bson_mapper_sources}
)

target_compile_definitions(bson_mapper_static PUBLIC BSON_MAPPER_STATIC)

set_target_properties(bson_mapper_static PROPERTIES
    OUTPUT_NAME bson_mapper
)

# Follow the boost convention to disambiguate the dll and static
# library names
if (WIN32)
    set_target_properties(bson_mapper_static PROPERTIES
        PREFIX lib
    )
endif()

add_library(bson_mapper SHARED
    ${bson_mapper_sources}
)

set_target_properties (bson_mapper PROPERTIES
    OUTPUT_NAME bson_mapper
    VERSION ${BSON_MAPPER_VERSION}
    DEFINE_SYMBOL BSON_MAPPER_EXPORTS
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    SOVERSION ${BSON_MAPPER_ABI_VERSION}
)

generate_export_header (bson_mapper
    BASE_NAME BSON_MAPPER
    EXPORT_MACRO_NAME BSON_MAPPER_API
    NO_EXPORT_MACRO_NAME BSON_MAPPER_PRIVATE
    EXPORT_FILE_NAME config/export.hpp
    STATIC_DEFINE BSON_MAPPER_STATIC
)

set(bson_mapper_libs ${LIBBSONCXX_LIBRARIES})

target_link_libraries(bson_mapper_static ${bson_mapper_libs})
target_link_libraries(bson_mapper PRIVATE ${bson_mapper_libs})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${BSON_MAPPER_HEADER_INSTALL_DIR}
    COMPONENT dev
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        REGEX ".+/test" EXCLUDE
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/config/export.hpp
    DESTINATION ${BSON_MAPPER_HEADER_INSTALL_DIR}/bson_mapper/config
    COMPONENT dev
)

install(TARGETS
    bson_mapper
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT dev
)

install(TARGETS
    bson_mapper_static
    ARCHIVE DESTINATION lib COMPONENT dev
)

set(PACKAGE_INCLUDE_INSTALL_DIRS ${BSON_MAPPER_HEADER_INSTALL_DIR})
set(PACKAGE_LIBRARY_INSTALL_DIRS lib)
set(PACKAGE_LIBRARIES bson_mapper)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  cmake/libbson_mapper-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/libbson_mapper-config.cmake
  INSTALL_DESTINATION lib/cmake/libbson_mapper-${BSON_MAPPER_VERSION}
  PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/libbson_mapper-config-version.cmake
  VERSION ${BSON_MAPPER_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/libbson_mapper-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libbson_mapper-config-version.cmake
  DESTINATION lib/cmake/libbson_mapper-${BSON_MAPPER_VERSION}
)

add_subdirectory(test)
