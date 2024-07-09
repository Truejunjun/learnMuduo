cmake_minimum_required(VERSION 2.5)
project(ChatServer)


# 配置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g)

# 把生成的可执行文件放入bin中
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/include)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/thridparty)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/include/server)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/include/server/db)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/include/server/model)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/include/server/redis)
# 如果不行则尝试以下语句
# include_directories(${PROJECT_SOURCE_DIR}/bin)

# 加载源文件子目录
add_subdirectory(src)