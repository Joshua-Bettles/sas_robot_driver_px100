#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "sas_robot_driver_px100::sas_robot_driver_px100" for configuration ""
set_property(TARGET sas_robot_driver_px100::sas_robot_driver_px100 APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(sas_robot_driver_px100::sas_robot_driver_px100 PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libsas_robot_driver_px100.so"
  IMPORTED_SONAME_NOCONFIG "libsas_robot_driver_px100.so"
  )

list(APPEND _cmake_import_check_targets sas_robot_driver_px100::sas_robot_driver_px100 )
list(APPEND _cmake_import_check_files_for_sas_robot_driver_px100::sas_robot_driver_px100 "${_IMPORT_PREFIX}/lib/libsas_robot_driver_px100.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
