project(buteo-sync-plugins-google)
cmake_minimum_required(VERSION 2.8.9)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Standard install paths
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Contacts REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Versit REQUIRED)

pkg_check_modules(BUTEOSYNCFW REQUIRED buteosyncfw5)
pkg_check_modules(ACCOUNTS REQUIRED accounts-qt5>=1.10)
pkg_check_modules(LIBSIGNON REQUIRED libsignon-qt5)

set(BUTEOSYNCFW_PLUGIN_PATH "/usr/lib/buteo-plugins-qt5")

# config file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/config.h"
               IMMEDIATE @ONLY)


# Coverage tools
OPTION(ENABLE_COVERAGE "Build with coverage analysis support" OFF)
if(ENABLE_COVERAGE)
    message(STATUS "Using coverage flags")
    find_program(COVERAGE_COMMAND gcov)
    if(NOT COVERAGE_COMMAND)
        message(FATAL_ERROR "gcov command not found")
    endif()
    SET(CMAKE_C_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
    SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
    include(${CMAKE_SOURCE_DIR}/cmake/lcov.cmake)
endif()

enable_testing()

add_subdirectory(storage-change-notifier)
add_subdirectory(buteo-contact-client)
add_subdirectory(google)
add_subdirectory(accounts)
add_subdirectory(tests)

# uninstall target
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}"
                  -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
