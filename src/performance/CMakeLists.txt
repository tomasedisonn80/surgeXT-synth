# Offline measurement only: none of this code is linked into the plugin.
add_executable(surge-cpu-bench CpuBenchmark.cpp)
target_link_libraries(surge-cpu-bench PRIVATE surge::surge-common)
target_compile_definitions(surge-cpu-bench PRIVATE
  SURGE_BENCH_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
  SURGE_BENCH_CONFIGURATION="$<CONFIG>"
  SURGE_BENCH_LUA_IMPL="${SURGE_LUA_IMPL}"
  SURGE_BENCH_LTO_REQUESTED=$<BOOL:${ENABLE_LTO}>
)

# Standalone correctness gate for the optimized per-block modulation mask.
add_executable(surge-modulation-mask-test ModulationMaskTest.cpp)
target_link_libraries(surge-modulation-mask-test PRIVATE surge::surge-common)

# Deterministic state/lifecycle fixtures for baseline-versus-candidate PCM comparison.
add_executable(surge-dsp-regression DspRegression.cpp)
target_link_libraries(surge-dsp-regression PRIVATE surge::surge-common)

# Correctness gates only; shared CI machines must not impose a timing threshold.
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(NAME cpu-modulation-mask COMMAND surge-modulation-mask-test)
set_tests_properties(cpu-modulation-mask PROPERTIES
  WORKING_DIRECTORY "${SURGE_SOURCE_DIR}"
  TIMEOUT 60
  LABELS "cpu;correctness")
add_test(NAME cpu-smoke
  COMMAND "${Python3_EXECUTABLE}" "${SURGE_SOURCE_DIR}/scripts/performance/run_smoke.py"
    --benchmark "$<TARGET_FILE:surge-cpu-bench>"
    --regression "$<TARGET_FILE:surge-dsp-regression>"
    --evidence-dir "${CMAKE_CURRENT_BINARY_DIR}/cpu-smoke-evidence")
set_tests_properties(cpu-smoke PROPERTIES
  WORKING_DIRECTORY "${SURGE_SOURCE_DIR}"
  TIMEOUT 300
  RUN_SERIAL TRUE
  LABELS "cpu;correctness")
