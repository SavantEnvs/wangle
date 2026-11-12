cmake_minimum_required(VERSION 3.14)
project(mayhem_harnesses LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Wangle is built as a subproject of this one, with the flags build.sh passes in.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../wangle wangle)

set(MAYHEM_OUT "/mayhem" CACHE PATH "where the harness binaries land")
set(STANDALONE_FUZZ_MAIN "" CACHE FILEPATH "LLVM run-once driver (C file)")
set(FUZZ_ENGINE "-fsanitize=fuzzer" CACHE STRING "libFuzzer link flag")

# The run-once driver is C: compile it as C so its LLVMFuzzerTestOneInput reference is not mangled.
add_library(standalone_main OBJECT ${STANDALONE_FUZZ_MAIN})
set_source_files_properties(${STANDALONE_FUZZ_MAIN} PROPERTIES LANGUAGE C)

foreach(h IN ITEMS
    fuzz_line_decoder
    fuzz_length_field_decoder
    fuzz_ssl_session_cache_data
    fuzz_tls_ticket_seeds
    fuzz_persistent_cache_load)
  add_executable(${h} ${CMAKE_CURRENT_SOURCE_DIR}/${h}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/lsan_off.cc)
  target_link_libraries(${h} PRIVATE wangle ${FUZZ_ENGINE})
  set_target_properties(${h} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MAYHEM_OUT})

  add_executable(${h}-standalone ${CMAKE_CURRENT_SOURCE_DIR}/${h}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/lsan_off.cc
    $<TARGET_OBJECTS:standalone_main>)
  target_link_libraries(${h}-standalone PRIVATE wangle)
  set_target_properties(${h}-standalone PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${MAYHEM_OUT})
endforeach()
