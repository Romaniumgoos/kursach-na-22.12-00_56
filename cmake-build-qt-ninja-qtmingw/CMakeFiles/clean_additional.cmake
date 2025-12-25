# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\app_console_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\app_console_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\app_gui_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\app_gui_autogen.dir\\ParseCache.txt"
  "app_console_autogen"
  "app_gui_autogen"
  )
endif()
