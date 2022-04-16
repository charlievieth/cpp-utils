message(STATUS "Looking for SQLiteCpp dependency")

message(STATUS "Looking for SQLiteCpp dependency -- not found")

if (NOT DEFINED ENV{PKG_CONFIG_PATH})
    get_filename_component(sqlite_pkgconfig_opt "/opt/homebrew/opt/sqlite/lib/pkgconfig" REALPATH)
    get_filename_component(sqlite_pkgconfig_local "/usr/local/opt/sqlite/lib/pkgconfig" REALPATH)
    if (EXISTS "${sqlite_pkgconfig_opt}")
        message(STATUS "Found sqlite pkgconfig: opt")
        set(ENV{PKG_CONFIG_PATH} "${sqlite_pkgconfig_opt}")
    elseif (EXISTS "${sqlite_pkgconfig_local}")
        message(STATUS "Found sqlite pkgconfig: local")
        set(ENV{PKG_CONFIG_PATH} "${sqlite_pkgconfig_local}")
    else ()
        message(STATUS "Missing sqlite pkgconfig")
    endif ()
endif ()

find_package(PkgConfig REQUIRED)
if (USE_SYSTEM_SQLITECPP)
    find_package(SQLiteCpp 3.1.1 REQUIRED)
else ()
    find_package(SQLiteCpp 3.1.1 QUIET CONFIG)
endif ()
if (SQLiteCpp_FOUND)
    message(STATUS "Looking for SQLiteCpp dependency -- found")
    add_custom_target(sqlitecpp_dependency)
else ()
    message(STATUS "Looking for SQLiteCpp dependency -- not found")

    include(FetchContent)
    FetchContent_Declare(sqlitecpp_dependency
        URL
            https://github.com/SRombauts/SQLiteCpp/archive/refs/tags/3.1.1.tar.gz
        URL_HASH
            SHA256=b00b1efad985dd415c3b62b01252cae20d9f2c32dcac435ddd8a105e9d4ddcde
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
        CMAKE_ARGS
            # TODO: link to the installed version of sqlite
            -DSQLITECPP_INTERNAL_SQLITE:BOOL=OFF
        CMAKE_CACHE_ARGS
            # TODO: link to the installed version of sqlite
            -DSQLITECPP_INTERNAL_SQLITE:BOOL=OFF
            # Enable assertion handler
            -DSQLITECPP_ENABLE_ASSERT_HANDLER:BOOL=ON
            -DSQLITECPP_RUN_CPPCHECK:BOOL=OFF
            -DSQLITECPP_RUN_CPPLINT:BOOL=OFF
            -DSQLITECPP_USE_ASAN:BOOL=OFF
            -DSQLITECPP_BUILD_EXAMPLES:BOOL=OFF
            -DSQLITECPP_BUILD_TESTS:BOOL=OFF
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
            -DCMAKE_INSTALL_PREFIX:PATH=${DEPENDENCIES_INSTALL_PREFIX}/sqlitecpp_dependency
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp
    )

    FetchContent_MakeAvailable(sqlitecpp_dependency)

    # Verify or install dependencies
    include(GNUInstallDirs)
    list(APPEND PREFIX_PATH "${DEPENDENCIES_INSTALL_PREFIX}/sqlitecpp_dependency")
    set(CMAKE_PREFIX_PATH ${PREFIX_PATH};${CMAKE_PREFIX_PATH}
        CACHE PATH "append sqlitecpp library into the search path"
            FORCE)
endif ()
