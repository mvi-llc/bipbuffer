file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(unit_tests_shm ${TEST_SOURCES})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(unit_tests_shm PRIVATE ${CLANG_WARNING_FLAGS})
endif()
target_include_directories(unit_tests_shm PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(unit_tests_shm Catch2::Catch2 SharedMemory)
add_test(NAME unit_tests_shm COMMAND unit_tests_shm --colour-mode ansi)
