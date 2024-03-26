# 用来临时调试代码bug

set(BINARY tmp)
file(GLOB_RECURSE TMP_SOURCES LIST_DIRECTORIES true *.h *.cpp)
set(SOURCES ${TMP_SOURCES})
add_executable(${BINARY} ${SOURCES})
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib pthread)

# add_executable(reverse ReverseList.cpp)