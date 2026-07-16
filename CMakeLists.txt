# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

include_directories(main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(QuickReference)
add_compile_options(“-DLV_CONF_PATH=${CMAKE_SOURCE_DIR}/main/lv_conf.h”)