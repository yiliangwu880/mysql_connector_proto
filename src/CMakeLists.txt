#project name
CMAKE_MINIMUM_REQUIRED(VERSION 2.8) #cmake最低版本需求，不加入此行会受到警告信息
PROJECT(HELLO) #项目名称　　

include_directories(system ../include/)
include_directories(system ./)
include_directories(system ./utility/)
include_directories(system ../include/mysql_connector/include)
#include_directories(after  ${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE SRCS  ${PROJECT_SOURCE_DIR} "*.cc")
file(GLOB_RECURSE SRCS2  ${PROJECT_SOURCE_DIR} "*.cpp")
file(GLOB_RECURSE PROTO_SRC  ${PROJECT_SOURCE_DIR}/proto "*.h") #proto导出c++文件
file(GLOB_RECURSE PROTO_INPUT  ${PROJECT_SOURCE_DIR}/proto "*.proto")

LINK_DIRECTORIES(
  #静态链接库
  ${PROJECT_SOURCE_DIR}/../lib

  #系统目录
  /usr/local/lib
  /use/lib
)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -pthread  -DDEBUG -Werror -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -pthread  -Werror -DDEBUG -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -fexceptions -fnon-call-exceptions")
	
ADD_EXECUTABLE(hello ${SRCS} ${SRCS2}) 

#增加系统依赖库
ADD_library(pthread SHARED IMPORTED)
# 设置链接库
target_link_libraries(hello libprotobuf.a libmysqlcppconn-static.a dl)

