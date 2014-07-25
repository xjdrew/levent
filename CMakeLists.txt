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

include_directories(${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libev)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libev/.libs/)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
endif()

macro (add_lua_library lname)
    add_library(${lname} SHARED ${ARGN})
    set_target_properties(${lname} PROPERTIES PREFIX "")
    set_target_properties(${lname} PROPERTIES SUFFIX ".so")
endmacro()

# socket
add_lua_library(socket levent/lua-socket.c)
add_lua_library(errno levent/lua-errno.c)
add_lua_library(levent levent/lua-levent.c)

# libev 
#set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/libev/Makefile PROPERTIES GENERATED true)
#add_custom_command (OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/libev/Makefile
#    COMMAND ./configure --enable-shared=no
#    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libev)
#add_custom_target(ev_makefile SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libev/Makefile)

add_lua_library(event levent/lua-event.c)
#add_dependencies(event ev_makefile)
#set_target_properties(event PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libev/.libs/libev.a)
target_link_libraries(event ev)
add_custom_command(TARGET event
    PRE_BUILD
    COMMAND CFLAGS=-fPIC ./configure --enable-shared=no
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libev)

