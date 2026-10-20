cmake_minimum_required(VERSION 3.16)
project(OpenVRArduinoHMD LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
   NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR
   NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  message(FATAL_ERROR "This build supports Linux x86-64. Use the existing Visual Studio project on Windows.")
endif()

add_subdirectory(OpenVR/samples/driver_arduinohmd)
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
