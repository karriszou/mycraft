cmake_minimum_required(VERSION 3.16)

set(NAME "mycraft")

project(${NAME})

set(CMAKE_C_FLAGS "-std=c99 -Wall -Wl,-O2 ${CMAKE_C_FLAGS}")

file(GLOB_RECURSE SOURCES ./src/*.c)

add_executable(${NAME}
  ${SOURCES}
)

set(DEPS_DIR "${PROJECT_SOURCE_DIR}/lib")


#target_include_directories(${NAME} PUBLIC
#  ${DEPS_DIR}
#)

target_link_directories(${NAME} PUBLIC
  ${DEPS_DIR}
)

target_link_libraries(${NAME}
  curl
  glfw3
  GL
  dl
  pthread
  m
)
