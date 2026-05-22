add_executable(matching_engine_benchmark
  src/matching_engine_benchmark.cpp
)

target_link_libraries(matching_engine_benchmark
  PRIVATE
    benchmark::benchmark_main
    matching_engine::matching_engine
)
