# The CMake File specifically for the driver..
find_package(libASPL CONFIG REQUIRED)
find_library(LIB_CoreFoundation CoreFoundation REQUIRED)
find_library(LIB_CoreAudio CoreAudio REQUIRED)

# Configure the Library, and Link against libASPL
add_library(${DRIVER_NAME} MODULE 
    entry_point.cpp 
    driver.cpp
    device.cpp
)

target_link_libraries(${DRIVER_NAME} PRIVATE
    aspl::libASPL
    ${LIB_CoreFoundation}
    ${LIB_CoreAudio}
)

set_target_properties(${DRIVER_NAME} PROPERTIES
  OUTPUT_NAME "${DRIVER_NAME}"
  BUNDLE true
  BUNDLE_EXTENSION "driver"
  PREFIX ""
  SUFFIX ""
  MACOSX_BUNDLE ON
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
  MACOSX_BUNDLE_BUNDLE_NAME "${DRIVER_NAME}"
  MACOSX_BUNDLE_BUNDLE_VERSION "${DRIVER_VERSION}"
  MACOSX_BUNDLE_COPYRIGHT "${DRIVER_COPYRIGHT}"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${DRIVER_IDENTIFIER}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${DRIVER_VERSION}"
  )