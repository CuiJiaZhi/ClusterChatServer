# 设置需要编译的源文件列表
aux_source_directory(. SRC_LISTS)
aux_source_directory(./db MYSQL_LISTS)
aux_source_directory(./model MODEL_LISTS)
aux_source_directory(./redis REDIS_LISTS)

# 生成可执行文件
add_executable(server
    ${SRC_LISTS}
    ${MYSQL_LISTS}
    ${MODEL_LISTS}
    ${REDIS_LISTS}
)

# 链接静态库文件
target_link_libraries(server
    muduo_net
    muduo_base
    mysqlclient
    hiredis
    pthread
)