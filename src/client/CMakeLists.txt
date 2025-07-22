# 设置需要编译的源文件列表
aux_source_directory(. SRC_LISTS)

# 生成可执行文件
add_executable(client
    ${SRC_LISTS}
)

# 链接静态库文件
target_link_libraries(client
    pthread
)