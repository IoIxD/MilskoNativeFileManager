cmake_minimum_required(VERSION 3.30)
project(MNFMExample)

set(MNFM_BUILD_EXAMPLES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} example.c)

find_package(PkgConfig)
if(${PkgConfig_FOUND})
  pkg_check_modules(MILSKO QUIET milsko)
  if(${MILSKO_FOUND})
    target_include_directories(${PROJECT_NAME} PRIVATE ${MILSKO_INCLUDE_DIRS})
    target_link_options(${PROJECT_NAME} PRIVATE ${MILSKO_LDFLAGS})
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ../include)
target_link_libraries(${PROJECT_NAME} PRIVATE Mw MNFM)
