# LANRADIO v0.1 requires ESP-IDF 5.x and Espressif Audio Development Framework
# (ADF).  Export ADF_PATH before configuring the project.
cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{ADF_PATH})
    message(FATAL_ERROR "ADF_PATH is not set. Install ESP-ADF and export ADF_PATH before building LANRADIO.")
endif()

include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lanradio_esp32s3)
