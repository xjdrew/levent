cmake_minimum_required (VERSION 2.8)
project(Levent)
include(cmake/dist.cmake) 

#message("name:${CMAKE_SYSTEM_NAME}")
#message("linker flags:${CMAKE_SHARED_LINKER_FLAGS}")
#message("compiler: ${CMAKE_C_COMPILER}")

#find lua
find_package(Lua)
if(NOT LUA_FOUND)
    message(FATAL_ERROR "Find lua failed")
endif()

include_directories(${LUA_INCLUDE_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
endif()

add_library(socket SHARED levent/lua-socket.c)
set_target_properties(socket PROPERTIES PREFIX "")
set_target_properties(socket PROPERTIES SUFFIX ".so")
#target_link_libraries(socket ${LUA_LIBRARIES})

