cmake_minimum_required (VERSION 2.8)
project(Levent)
include(cmake/dist.cmake) 


#find lua
find_package(Lua)
if(NOT LUA_FOUND)
    message(FATAL_ERROR "Find lua failed")
endif()

include_directories(${LUA_INCLUDE_DIR})
add_library(socket SHARED levent/lua-socket.c)
set_target_properties(socket PROPERTIES PREFIX "")

