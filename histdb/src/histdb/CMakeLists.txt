# Create an executable from the sub projects.
add_executable(histdb main.cc)

target_link_libraries(histdb
	absl::base
	absl::strings
	SQLiteCpp
	sqlite3)

include(GNUInstallDirs)
install(TARGETS histdb
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
