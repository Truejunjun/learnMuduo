cmake_minimum_required(VERSION 2.5)
project(mymuduo)

# mymuduo库最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息，以及启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")   # 设置编译选项

# 定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
# 编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})



# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()
# 表示生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
# add_executable(server ${SRC_LIST})
# 表示server需要链接以下三个库
# target_link_libraries(server muduo_net muduo_base ...)