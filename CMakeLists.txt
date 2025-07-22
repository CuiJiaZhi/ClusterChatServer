# 设置cmake最低版本
cmake_minimum_required(VERSION 3.10)

# 设置工程
project(CLUSTERCHATSERVER LANGUAGES CXX)

# # 配置编译器选项
# set(CMAKE_CXX_FLAGS ${CMAKE_FXX_FLAGS} -g)

# 设置可执行文件的生成路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 设置头文件搜索路径
# 配置头文件搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/client
    ${PROJECT_SOURCE_DIR}/include/server/db
    ${PROJECT_SOURCE_DIR}/include/server/model
    ${PROJECT_SOURCE_DIR}/include/server/redis
    ${PROJECT_SOURCE_DIR}/thridparty
)

# 搜索子目录
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
