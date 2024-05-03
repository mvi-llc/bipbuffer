file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(benchmark_shm ${TEST_SOURCES})
target_compile_options(benchmark_shm PRIVATE ${PROJECT_WARNING_FLAGS})
target_include_directories(benchmark_shm PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(benchmark_shm Catch2::Catch2 SharedMemory)
add_test(NAME benchmark_shm COMMAND benchmark_shm --colour-mode ansi)
