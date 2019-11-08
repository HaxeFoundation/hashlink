add_library(mysql.hdll SHARED
    socket.c
    sha1.c
    my_proto.c
    my_api.c
    mysql.c
)

set_as_hdll(mysql)

target_include_directories(mysql.hdll
    PRIVATE
)

target_link_libraries(mysql.hdll
    libhl
)

install(
    TARGETS
        mysql.hdll
    DESTINATION ${HDLL_DESTINATION}
)
