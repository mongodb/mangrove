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

project(MONGO_ODM)

set(LIBBSONCXX_REQUIRED_VERSION 3.0.1)
set(LIBBSONCXX_REQUIRED_ABI_VERSION v_noabi)
find_package(libbsoncxx ${LIBBSONCXX_REQUIRED_VERSION} REQUIRED)

set(LIBMONGOCXX_REQUIRED_VERSION 3.0.1)
set(LIBMONGOCXX_REQUIRED_ABI_VERSION v_noabi)
find_package(libmongocxx ${LIBMONGOCXX_REQUIRED_VERSION} REQUIRED)

set(LIBBSON_REQUIRED_VERSION 1.3.4)
set(LIBBSON_REQUIRED_ABI_VERSION 1.0)
find_package(LibBSON ${LIBBSON_REQUIRED_VERSION} REQUIRED)

set(LIBMONGOC_REQUIRED_VERSION 1.3.4)
set(LIBMONGOC_REQUIRED_ABI_VERSION 1.0)
find_package(LibMongoC ${LIBMONGOC_REQUIRED_VERSION} REQUIRED)

# Update these as needed.
# TODO: read from file
set(MONGO_ODM_VERSION_MAJOR 0)
set(MONGO_ODM_VERSION_MINOR 0)
set(MONGO_ODM_VERSION_PATCH 1)
set(MONGO_ODM_VERSION_EXTRA "-alpha")
set(MONGO_ODM_ABI_VERSION _noabi)

set(MONGO_ODM_VERSION ${MONGO_ODM_VERSION_MAJOR}.${MONGO_ODM_VERSION_MINOR}.${MONGO_ODM_VERSION_PATCH}${MONGO_ODM_VERSION_EXTRA})
set(MONGO_ODM_INLINE_NAMESPACE "v${MONGO_ODM_ABI_VERSION}")
set(MONGO_ODM_HEADER_INSTALL_DIR "include/mongo_odm/${MONGO_ODM_INLINE_NAMESPACE}" CACHE INTERNAL "")

add_subdirectory(config)

set(mongo_odm_sources
   "odm_collection.cpp"
)

include_directories(
    ${LIBBSONCXX_INCLUDE_DIRS}
    ${LIBMONGOCXX_INCLUDE_DIRS}
    ${LIBBSON_INCLUDE_DIRS}
    ${LIBMONGOC_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

link_directories(
    ${LIBBSONCXX_LIBRARY_DIRS}
    ${LIBMONGOCXX_LIBRARY_DIRS}
    ${LIBBSON_LIBRARY_DIRS}
    ${LIBMONGOC_LIBRARY_DIRS}
)

add_library(mongo_odm_static STATIC
   ${mongo_odm_sources}
)

target_compile_definitions(mongo_odm_static PUBLIC MONGO_ODM_STATIC)

set_target_properties(mongo_odm_static PROPERTIES
    OUTPUT_NAME mongo_odm
)

# Follow the boost convention to disambiguate the dll and static
# library names
if (WIN32)
    set_target_properties(mongo_odm_static PROPERTIES
        PREFIX lib
    )
endif()

add_library(mongo_odm SHARED
    ${mongo_odm_sources}
)

set_target_properties (mongo_odm PROPERTIES
    OUTPUT_NAME mongo_odm
    VERSION ${MONGO_ODM_VERSION}
    DEFINE_SYMBOL MONGO_ODM_EXPORTS
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    SOVERSION ${MONGO_ODM_ABI_VERSION}
)

generate_export_header (mongo_odm
    BASE_NAME MONGO_ODM
    EXPORT_MACRO_NAME MONGO_ODM_API
    NO_EXPORT_MACRO_NAME MONGO_ODM_PRIVATE
    EXPORT_FILE_NAME config/export.hpp
    STATIC_DEFINE MONGO_ODM_STATIC
)

set(mongo_odm_libs ${LIBBSONCXX_LIBRARIES} ${LIBMONGOCXX_LIBRARIES} ${LIBMONGOC_LIBRARIES} ${LIBBSON_LIBRARIES})

target_link_libraries(mongo_odm_static bson_mapper_static ${mongo_odm_libs})
target_link_libraries(mongo_odm PUBLIC bson_mapper PRIVATE ${mongo_odm_libs})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${MONGO_ODM_HEADER_INSTALL_DIR}
    COMPONENT dev
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        REGEX ".+/test" EXCLUDE
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/config/export.hpp
    DESTINATION ${MONGO_ODM_HEADER_INSTALL_DIR}/mongo_odm/config
    COMPONENT dev
)

install(TARGETS
    mongo_odm
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT dev
)

install(TARGETS
    mongo_odm_static
    ARCHIVE DESTINATION lib COMPONENT dev
)

set(PACKAGE_INCLUDE_INSTALL_DIRS ${MONGO_ODM_HEADER_INSTALL_DIR})
set(PACKAGE_LIBRARY_INSTALL_DIRS lib)
set(PACKAGE_LIBRARIES mongo_odm)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  cmake/libmongo_odm-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/libmongo_odm-config.cmake
  INSTALL_DESTINATION lib/cmake/libmongo_odm-${MONGO_ODM_VERSION}
  PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/libmongo_odm-config-version.cmake
  VERSION ${MONGO_ODM_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/libmongo_odm-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libmongo_odm-config-version.cmake
  DESTINATION lib/cmake/libmongo_odm-${MONGO_ODM_VERSION}
)

add_subdirectory(test)
