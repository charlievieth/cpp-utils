# Create an executable from the sub projects.
add_executable(histdb main.cc)

# message(STATUS "CLI11_INCLUDE_DIR: ${CLI11_INCLUDE_DIR}")
# include_directories(${CLI11_INCLUDE_DIR})
# target_include_directories(histdb ${CLI11_INCLUDE_DIR})

target_link_libraries(histdb PRIVATE
	absl::base
	absl::strings
	SQLiteCpp
	sqlite3)
# target_link_libraries(histdb INTERFACE CLI11::CLI11)
target_link_libraries(histdb INTERFACE CLI::CLI)

include(GNUInstallDirs)
install(TARGETS histdb
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
