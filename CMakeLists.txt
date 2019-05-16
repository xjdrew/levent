cmake_minimum_required (VERSION 2.8)
project(Levent)

set(levent_cmake "config.cmake")
if (EXISTS $ENV{LEVENT_CMAKE})
    set(levent_cmake $ENV{LEVENT_CMAKE})
endif()
include(${levent_cmake})

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps")

#message("name:${CMAKE_SYSTEM_NAME}")
#message("linker flags:${CMAKE_SHARED_LINKER_FLAGS}")
#message("compiler: ${CMAKE_C_COMPILER}")
include_directories(${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/deps/http-parser)
include_directories(${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/deps/libev)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/libev/.libs/)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
endif()

add_definitions(-DHTTP_PARSER_STRICT=0)
if(WIN32)
	set(suffix ".dll")
    add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB -DHTTP_PARSER_STRICT=0)
else()
	set(suffix ".so")
endif()

macro (add_lua_library lname)
    add_library(${lname} SHARED ${ARGN})
    if(WIN32)
        target_link_libraries(${lname} ${LUA_LIBRARIES})
    endif()
    set_target_properties(${lname} PROPERTIES PREFIX "")
    set_target_properties(${lname} PROPERTIES SUFFIX ${suffix})
endmacro()

# levent.module
set(CSOURCE src/lua-socket.c src/lua-errno.c src/lua-levent.c src/lua-ev.c src/lua-http-parser.c deps/http-parser/http_parser.c)
set(CMONGO cext/lua-mongo.c)
set(CBSON  cext/lua-bson.c)

IF(NOT WIN32)
    add_lua_library(levent ${CSOURCE})
    # ignore compile warning: incompatible-pointer-types
    target_compile_options(levent PRIVATE -Wno-incompatible-pointer-types)

    target_link_libraries(levent ev)
    add_custom_command(TARGET levent
        PRE_BUILD
        COMMAND CFLAGS=-fPIC ./configure --enable-shared=no
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/libev)
    add_lua_library(mongo ${CMONGO})
    add_lua_library(bson  ${CBSON})
ELSE()
    set(CSOURCE ${CSOURCE} src/evwrap.c)
    add_lua_library(levent ${CSOURCE})
    add_lua_library(mongo ${CMONGO})
    add_lua_library(bson  ${CBSON})
ENDIF()
