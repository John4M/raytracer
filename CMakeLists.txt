cmake_minimum_required(VERSION 3.10)
project(RayTracing CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(inOneWeekend src/main.cc)

include(CTest)
if(BUILD_TESTING)
    add_executable(sampler_test tests/sampler_test.cc)
    target_include_directories(sampler_test PRIVATE src)
    add_test(NAME stratified_path_sampler COMMAND sampler_test)
endif()