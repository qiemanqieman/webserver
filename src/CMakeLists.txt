set(BINARY ${CMAKE_PROJECT_NAME})

#1
# add_executable(webserver main.cpp)

#2 
file(GLOB_RECURSE EXE_SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES ${EXE_SOURCES})
add_executable(${BINARY} ${SOURCES})
target_link_libraries(${BINARY} pthread)

# 为了让单元测试的时候src下的代码能被作为静态链接库使用
add_library(${BINARY}_lib STATIC ${SOURCES})

# add_executable(reverse ReverseList.cpp)