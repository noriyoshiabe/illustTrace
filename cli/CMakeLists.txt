cmake_minimum_required(VERSION 3.5)

project(illustrace-cli)
set(TARGET_NAME illustrace)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
find_package(OpenCV)
find_package(Cario)
find_package(LibXml2)

add_definitions(-Wall)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEBUG)
endif ()

add_subdirectory (./nalib ${CMAKE_CURRENT_BINARY_DIR}/nalib)

add_definitions(-std=c++11)
add_subdirectory (../core ${CMAKE_CURRENT_BINARY_DIR}/core)

set(SOURCES
  main.cpp
  CLI.cpp
  View.cpp
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ../core
)

add_executable(${TARGET_NAME} ${SOURCES})
target_link_libraries(${TARGET_NAME} illustrace-core)
target_link_libraries(${TARGET_NAME} nalib)
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${CAIRO_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${LIBXML2_LIBRARIES})
