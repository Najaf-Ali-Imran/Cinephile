# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\appCinephile_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\appCinephile_autogen.dir\\ParseCache.txt"
  "appCinephile_autogen"
  )
endif()
