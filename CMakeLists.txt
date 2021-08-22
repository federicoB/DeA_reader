cmake_minimum_required(VERSION 3.10)

project(dea_reader)

set(CMAKE_C_STANDARD 99)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_library(LIBUSB_LIBRARY
        NAMES usb
        PATH_SUFFIXES "lib" "lib32" "lib64")

add_executable(dea_reader deaprotocol.c wvutils.c main.c)
target_link_libraries(dea_reader ${LIBUSB_LIBRARY})
target_link_libraries(dea_reader m)
