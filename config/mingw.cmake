# check if we are under MinGW

message(STATUS "Checking for MinGW")
if(MINGW)
   message(STATUS "Checking for MinGW -- yes")
   set(HAVE_MINGW 1)
   set(__USE_MINGW_ANSI_STDIO 1)
else()
   message(STATUS "Checking for MinGW -- no")
   set(HAVE_MINGW 0)
   set(__USE_MINGW_ANSI_STDIO 0)
endif()
if(CMAKE_EXECUTABLE_SUFFIX)
   set(HAVE_EXECUTABLE_SUFFIX 1)
endif()
