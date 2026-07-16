cmake_minimum_required(VERSION 3.30)
project(MNFM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MNFM_BUILD_EXAMPLES OFF)

if(UNIX AND NOT MSYS)
  SET(platform_sources src/linux.c)
endif()
if(WIN32)
  SET(platform_sources src/win32.c)
endif()

add_library(${PROJECT_NAME} STATIC src/widget.c ${platform_sources})

target_include_directories(${PROJECT_NAME} PUBLIC ./include)
target_link_libraries(${PROJECT_NAME} PUBLIC Mw)

find_package(PkgConfig)
if(${PkgConfig_FOUND})
  pkg_check_modules(MILSKO QUIET milsko)
  if(${MILSKO_FOUND})
    target_include_directories(${PROJECT_NAME} PRIVATE ${MILSKO_INCLUDE_DIRS})
  endif()
endif()

if(UNIX AND NOT MSYS)
  find_package(DBus1 REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE ${DBus1_INCLUDE_DIRS})
endif()

if(MNFM_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()
