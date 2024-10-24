cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(SharedMemory LANGUAGES CXX)

option(BUILD_TESTS "Build tests" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(CppWarnings)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)

set(SHARED_MEMORY_SOURCES
  src/SharedMemory.cpp
)

set(BIP_BUFFER_SOURCES
  src/BipBufferHeader.cpp
  src/BipBufferReader.cpp
  src/BipBufferWriter.cpp
  src/BipBufferWriterReservation.cpp
)

add_library(SharedMemory SHARED ${SHARED_MEMORY_SOURCES})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(SharedMemory PRIVATE ${CLANG_WARNING_FLAGS})
endif()
target_include_directories(SharedMemory PUBLIC include)

add_library(SharedMemoryStatic STATIC ${SHARED_MEMORY_SOURCES})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(SharedMemoryStatic PRIVATE ${CLANG_WARNING_FLAGS})
endif()
target_include_directories(SharedMemoryStatic PUBLIC include)

add_library(BipBuffer SHARED ${BIP_BUFFER_SOURCES})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(BipBuffer PRIVATE ${CLANG_WARNING_FLAGS})
endif()
target_include_directories(BipBuffer PUBLIC include)

add_library(BipBufferStatic STATIC ${BIP_BUFFER_SOURCES})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(BipBufferStatic PRIVATE ${CLANG_WARNING_FLAGS})
endif()
target_include_directories(BipBufferStatic PUBLIC include)

if(BUILD_TESTS)
  include(CPM)
  cpmaddpackage("gh:catchorg/Catch2@3.5.4")
  include(CTest)

  # Work around dependencies including with `-I` instead of `-isystem`
  get_target_property(CATCH2_INCLUDE_DIRS Catch2::Catch2 INTERFACE_INCLUDE_DIRECTORIES)

  enable_testing()
  add_subdirectory(benchmarks)
  add_subdirectory(tests)
endif()
