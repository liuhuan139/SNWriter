cmake_minimum_required(VERSION 3.16)
project(SNWriter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

add_executable(SNWriter
    src/main.cpp
    src/MainWindow.cpp
    src/BinaryPatcher.cpp
)

target_include_directories(SNWriter PRIVATE
    ${GTKMM_INCLUDE_DIRS}
    src
)

target_compile_options(SNWriter PRIVATE ${GTKMM_CFLAGS_OTHER})
target_link_directories(SNWriter PRIVATE ${GTKMM_LIBRARY_DIRS})
target_link_libraries(SNWriter PRIVATE ${GTKMM_LIBRARIES})
