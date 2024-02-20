cmake_minimum_required(VERSION 3.13)

# Define the Project Name
project(GoXLR_ASPL CXX)

# Set Compiler Options..
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set some Variables used inside the Driver (and for Generating the plist)..
set(DRIVER_NAME "GoXLRDevice")
set(DRIVER_VERSION "0.0.1")
set(DRIVER_COPYRIGHT "Copyright (c) GoXLR on Linux Team")
set(DRIVER_IDENTIFIER "com.github.goxlr-on-linux.goxlr-utility")
set(DRIVER_UID "7E14FAEA-C962-469B-8FE9-76C0EEB1A796")
set(DRIVER_ENTRYPOINT "GoXLREntryPoint")



# Configure and install external libraries..
include(lib/CMakeLists.txt)

add_subdirectory(src)