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



# Configure libASPL as an external dependency..
include(ExternalProject)
ExternalProject_Add(libASPL
  URL "https://github.com/gavv/libASPL/archive/refs/heads/main.zip"
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/libASPL/libASPL-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib/libASPL/libASPL-build
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib/libASPL/libASPL-prefix
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

# Configure the Library, and Link against libASPL
add_library(${DRIVER_NAME} MODULE src/driver.cpp)
target_include_directories(${DRIVER_NAME}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lib/libASPL/libASPL-prefix/include
)
target_link_libraries(${DRIVER_NAME}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lib/libASPL/libASPL-prefix/lib/libASPL.a
)

