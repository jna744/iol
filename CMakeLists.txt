cmake_minimum_required(VERSION 3.13)

project(iol VERSION 0.1)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(IOL_PRIMARY_PROJECT true)
  set(CMAKE_EXPORT_COMPILE_COMMANDS true)
endif()

add_library(${PROJECT_NAME} "")

target_include_directories(
  ${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_subdirectory(src)

if(BUILD_TESTING AND IOL_PRIMARY_PROJECT)
  add_subdirectory(test)
endif()
