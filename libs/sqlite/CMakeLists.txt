add_library(sqlite.hdll SHARED
    sqlite.c
    ${INCLUDES_BASE_DIR}/sqlite/src/sqlite3.c
)

set_as_hdll(sqlite)

target_include_directories(sqlite.hdll
    PRIVATE
    ${INCLUDES_BASE_DIR}/sqlite/src
)

target_link_libraries(sqlite.hdll
    libhl
)

install(
    TARGETS
        sqlite.hdll
    DESTINATION ${HDLL_DESTINATION}
)
