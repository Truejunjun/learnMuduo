# 定义了一个SRC_LIST变量，包含了该目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./model MODEL_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${MODEL_LIST})
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread)  # 三个库文件的顺序不能调转