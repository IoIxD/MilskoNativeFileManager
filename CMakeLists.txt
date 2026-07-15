cmake_minimum_required(VERSION 4.0)
project(MNFM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIX)
  SET(platform_sources src/linux.c)
endif()

add_library(${PROJECT_NAME} src/widget.c ${platform_sources})

target_include_directories(${PROJECT_NAME} PUBLIC ./include)

if(UNIX)
  find_package(DBus1 REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE ${DBus1_INCLUDE_DIRS})
endif()

add_subdirectory(example)
