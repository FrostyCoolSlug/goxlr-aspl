# The CMake File specifically for the driver..
find_package(libASPL CONFIG REQUIRED)

# Configure the Library, and Link against libASPL
add_library(${DRIVER_NAME} MODULE 
    entry_point.cpp 
    driver.cpp
)

target_link_libraries(${DRIVER_NAME} PRIVATE
    aspl::libASPL
)

