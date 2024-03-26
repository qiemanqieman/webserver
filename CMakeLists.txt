cmake_minimum_required(VERSION 3.0.0)
project(webserver VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

include(CTest)
enable_testing()
add_subdirectory(src) # 项目代码
add_subdirectory(test) # 测试代码
add_subdirectory(tmp) # 用来临时调试代码bug


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
