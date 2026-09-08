add_library(ui.hdll ui_stub.c)

if(WIN32)
    set_target_properties(ui.hdll PROPERTIES SOURCES "ui_win.c;utils.cpp")
	target_link_libraries(ui.hdll Shell32 Ole32)
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
