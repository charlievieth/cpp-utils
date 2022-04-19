
# message(STATUS "Looking for sqlite3 dependency")
# if (NOT DEFINED ENV{PKG_CONFIG_PATH})
#     get_filename_component(sqlite_pkgconfig_opt "/opt/homebrew/opt/sqlite/lib/pkgconfig" REALPATH)
#     get_filename_component(sqlite_pkgconfig_local "/usr/local/opt/sqlite/lib/pkgconfig" REALPATH)
#     if (EXISTS "${sqlite_pkgconfig_opt}")
#         message(STATUS "Found sqlite3 pkgconfig: opt")
#         set(ENV{PKG_CONFIG_PATH} "${sqlite_pkgconfig_opt}")
#     elseif (EXISTS "${sqlite_pkgconfig_local}")
#         message(STATUS "Found sqlite3: pkgconfig: local")
#         set(ENV{PKG_CONFIG_PATH} "${sqlite_pkgconfig_local}")
#     else ()
#         message(STATUS "Missing sqlite3 pkgconfig")
#     endif ()
#     find_package (SQLite3 REQUIRED)
# endif ()

# NOTE: we are using a fork of SqliteCpp:
# https://github.com/charlievieth/SQLiteCpp/releases/tag/v3.38.2
message(STATUS "Looking for SQLiteCpp dependency")
find_package(PkgConfig REQUIRED)
if (USE_SYSTEM_SQLITECPP)
    find_package(SQLiteCpp 3.38.2 REQUIRED)
else ()
    find_package(SQLiteCpp 3.38.2 QUIET CONFIG)
endif ()

if (SQLiteCpp_FOUND)
    message(STATUS "Looking for SQLiteCpp dependency -- found")
    add_custom_target(sqlitecpp_dependency)
else ()
    message(STATUS "Looking for SQLiteCpp dependency -- not found")

    include(ExternalProject)
    ExternalProject_Add(sqlitecpp_dependency
        URL
            https://github.com/charlievieth/SQLiteCpp/archive/refs/tags/v3.38.2.tar.gz
        URL_HASH
            SHA256=235ea9bcace287916d6f4b07f4912b843d17677d0565f4cbf82590482bf9e0ff
        DOWNLOAD_NO_PROGRESS
            1
        UPDATE_COMMAND
            ""
        LOG_CONFIGURE
            1
        LOG_BUILD
            1
        LOG_INSTALL
            1
        # CMAKE_ARGS
        #     # TODO: link to the installed version of sqlite
        #     -DSQLITECPP_INTERNAL_SQLITE:BOOL=OFF
        #     -DSQLITECPP_RUN_CPPCHECK:BOOL=OFF
        CMAKE_CACHE_ARGS
            # TODO: link to the installed version of sqlite
            # -DSQLITECPP_INTERNAL_SQLITE:BOOL=OFF
            # Enable assertion handler
            -DSQLITECPP_ENABLE_ASSERT_HANDLER:BOOL=ON
            -DSQLITECPP_RUN_CPPCHECK:BOOL=OFF
            -DSQLITECPP_RUN_CPPLINT:BOOL=OFF
            -DSQLITECPP_USE_ASAN:BOOL=OFF
            -DSQLITECPP_BUILD_EXAMPLES:BOOL=OFF
            -DSQLITECPP_BUILD_TESTS:BOOL=OFF
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
            -DCMAKE_INSTALL_PREFIX:PATH=${DEPENDENCIES_INSTALL_PREFIX}/sqlitecpp_dependency
    )
    include(GNUInstallDirs)
    list(APPEND PREFIX_PATH "${DEPENDENCIES_INSTALL_PREFIX}/sqlitecpp_dependency")
    set(CMAKE_PREFIX_PATH ${PREFIX_PATH};${CMAKE_PREFIX_PATH}
        CACHE PATH "append sqlitecpp library into the search path"
            FORCE)
    message(STATUS "Found SqliteCpp dependency")
endif ()
