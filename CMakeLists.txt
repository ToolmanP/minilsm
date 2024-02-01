cmake_minimum_required(VERSION 3.20)
project(minilsm)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_compile_options(-Wall -Wextra -O0 -ggdb3)
add_executable(lsm_smoke1 test/lsm_smoke1.cc)
add_executable(lsm_smoke2 test/lsm_smoke2.cc)
add_executable(lsm_correctness test/lsm_correctness.cc)
add_executable(lsm_persistence test/lsm_persistence.cc)

set(CMAKE_SOURCE_DIR src)

add_library(minilsm STATIC 
src/kvstore.cc
src/sstable/ssblock.cc
src/sstable/sslevel.cc
src/sstable/sstable.cc)
target_include_directories(minilsm PUBLIC include)

target_link_libraries(lsm_smoke1 minilsm)
target_link_libraries(lsm_smoke2 minilsm)
target_link_libraries(lsm_correctness minilsm)
target_link_libraries(lsm_persistence minilsm)


enable_testing()
add_test(NAME smoke1 COMMAND lsm_smoke1)
add_test(NAME smoke2 COMMAND lsm_smoke2)
add_test(NAME correctness COMMAND lsm_correctness)
add_test(NAME persistence COMMAND lsm_persistence -t)


