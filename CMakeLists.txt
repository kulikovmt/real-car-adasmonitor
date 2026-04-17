cmake_minimum_required(VERSION 3.14)
project(RealCarADASMonitor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OpenCV_DIR "C:/opencv/build" CACHE PATH "")
set(OpenCV_RUNTIME "vc16" CACHE STRING "")
find_package(OpenCV REQUIRED)

set(ONNXRUNTIME_DIR "C:/onnxruntime" CACHE PATH "")
include_directories(${ONNXRUNTIME_DIR}/include)
link_directories(${ONNXRUNTIME_DIR}/lib)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_executable(RealCarMonitor ${SRC_FILES})

target_include_directories(RealCarMonitor PRIVATE src)
target_link_libraries(RealCarMonitor PRIVATE ${OpenCV_LIBS} onnxruntime nlohmann_json::nlohmann_json)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

enable_testing()

file(GLOB_RECURSE TEST_FILES "tests/*.cpp")

set(SRC_FILES_NO_MAIN ${SRC_FILES})
list(FILTER SRC_FILES_NO_MAIN EXCLUDE REGEX "src/main\\.cpp$")

add_executable(RunTests ${TEST_FILES} ${SRC_FILES_NO_MAIN})

target_include_directories(RunTests PRIVATE src)
target_link_libraries(RunTests PRIVATE 
    GTest::gtest_main 
    GTest::gtest 
    ${OpenCV_LIBS} 
    onnxruntime
    nlohmann_json::nlohmann_json
)

add_test(NAME RunTests COMMAND RunTests)
