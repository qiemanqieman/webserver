cmake_minimum_required(VERSION 3.0.0)
project(webserver VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

include(CTest)
enable_testing()
add_subdirectory(src)
add_subdirectory(test)

# tmp.cpp 用来进行临时的调试代码bug
add_executable(tmp tmp.cpp)
target_link_libraries(tmp ${PROJECT_NAME}_lib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
