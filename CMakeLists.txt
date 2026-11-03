cmake_minimum_required(VERSION 3.14)
project(MultibandStudio VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/iPlug2" CACHE PATH "Path to iPlug2")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MultibandStudio.cpp
    MultibandStudio.h
    resources/resource.h
  FORMATS AU
)
