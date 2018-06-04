add_library(ui.hdll SHARED ui_stub.c)

if(WIN32)
    set_target_properties(ui.hdll PROPERTIES SOURCES ui_win.c)
endif()

set_as_hdll(ui)

target_link_libraries(ui.hdll
    libhl
)

install(
    TARGETS
        ui.hdll
    DESTINATION ${HDLL_DESTINATION}
)
