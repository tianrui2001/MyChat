# cmake 最低版本
cmake_minimum_required(VERSION 3.0)
project(main)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置头文件的搜索路径
# include_directories()

# 配置库文件搜索路径
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LIST *.cpp)

# 把.下面的所有源文件放入到SRC_LIST变量中
# aux_source_directory(. SRC_LIST)

# 可执行文件由SRC_LIST编译而来
add_executable(server ${SRC_LIST})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 链接muduo库和pthread库
target_link_libraries(server muduo_net muduo_base pthread)