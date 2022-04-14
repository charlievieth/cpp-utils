# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${histdb_SOURCE_DIR}/include/histdb/*.h")

# set(HEADER_LIST "${histdb_SOURCE_DIR}/include/histdb/exception.h")
# set(HEADER_LIST "${histdb_SOURCE_DIR}/include/histdb/command.h")
# set(HEADER_LIST "${histdb_SOURCE_DIR}/include/histdb/session_id.h")

# Make an automatic library - will be static or dynamic based on user setting
add_library(histdb_library exception.cc database.cc command.cc session_id.cc ${HEADER_LIST})
# add_library(histdb_library STATIC command.cc ${HEADER_LIST})

# WARN
target_link_libraries(histdb_library PRIVATE SQLiteCpp absl::strings)

# We need this directory, and users of our library will need it too
target_include_directories(histdb_library PUBLIC ../include)

# TODO: do we need both libraries?
# target_link_libraries(main SQLiteCpp)
# target_link_libraries(main absl::strings)

# All users of this library will need at least C++17
target_compile_features(histdb_library PUBLIC cxx_std_17)

# main
add_executable(main main.cc)
target_link_libraries(main PRIVATE histdb_library SQLiteCpp absl::strings)
# target_link_libraries(main absl::strings)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST})
