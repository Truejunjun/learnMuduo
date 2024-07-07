cmake_minimum_required(VERSION 2.5)
project(ChatServer)

# 把生成的可执行文件放入bin中
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息，以及启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")   # 设置编译选项

# 定义参与编译的源代码文件，两种方式功能一致
# aux_source_directory(. SRC_LIST)
add_executable(server ${SRC_LIST})