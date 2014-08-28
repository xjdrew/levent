cmake_minimum_required (VERSION 2.8)
project(Levent)
include(cmake/dist.cmake) 

#message("name:${CMAKE_SYSTEM_NAME}")
#message("linker flags:${CMAKE_SHARED_LINKER_FLAGS}")
#message("compiler: ${CMAKE_C_COMPILER}")

message(${LUA_LIBRARIES})

#find lua
find_package(Lua)
if(NOT LUA_FOUND)
    message(FATAL_ERROR "Find lua failed")
endif()

include_directories(${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/deps/libev)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/libev/.libs/)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
endif()

if(WIN32)
	set(suffix ".dll")
	add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB)
else()
	set(suffix ".so")
endif()

macro (add_lua_library lname)
    add_library(${lname} SHARED ${ARGN})
    target_link_libraries(${lname} ${LUA_LIBRARIES})
    set_target_properties(${lname} PROPERTIES PREFIX "")
    set_target_properties(${lname} PROPERTIES SUFFIX ${suffix})
endmacro()

# levent.module
IF(NOT WIN32)
    add_lua_library(levent levent/lua-socket.c levent/lua-errno.c levent/lua-levent.c levent/lua-ev.c)
    target_link_libraries(levent ev)
    add_custom_command(TARGET levent
        PRE_BUILD
        COMMAND CFLAGS=-fPIC ./configure --enable-shared=no
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/libev)
ELSE()
    add_lua_library(levent levent/lua-socket.c levent/lua-errno.c levent/lua-levent.c levent/lua-ev.c levent/evwrap.c)
ENDIF()

# levent deps module
add_lua_library(struct deps/struct/struct.c)

