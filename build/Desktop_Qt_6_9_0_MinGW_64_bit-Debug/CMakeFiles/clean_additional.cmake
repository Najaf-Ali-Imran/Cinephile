# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Cinephile_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Cinephile_autogen.dir\\ParseCache.txt"
  "Cinephile_autogen"
  )
endif()
