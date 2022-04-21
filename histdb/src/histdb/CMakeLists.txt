# Create an executable from the sub projects.
add_executable(histdb main.cc)

target_link_libraries(histdb PRIVATE
	absl::base
	absl::strings
	SQLiteCpp
	sqlite3)
target_link_libraries(histdb INTERFACE CLI11::CLI11)

include(GNUInstallDirs)
install(TARGETS histdb
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
