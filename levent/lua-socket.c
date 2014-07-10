/* lua-socket.c
 * author: xjdrew
 * date: 2014-07-10
 */

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 address families are supported
- Only SOCK_STREAM, SOCK_DGRAM socket type are supported

Module interface:
- socket.socket(family, type[, proto]) --> new socket object
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
*/

#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include "lua.h"
#include "lauxlib.h"

#define SOCKET_METATABLE "socket_metatable"

typedef struct _socket_t {
    int fd;
} socket_t;

inline static
socket_t* _getsock(lua_State *L, int index) {
	socket_t* sock = (socket_t*)luaL_checkudata(L, index, SOCKET_METATABLE);
	return sock;
}

/*
 * args: int domain, int type, int protocal
 */
static int
_socket(lua_State *L) {
    int domain = luaL_checkint(L, 1);
    int type   = luaL_checkint(L, 2);
    int proto  = 0;
		if(lua_gettop(L) > 2) {
			proto = luaL_checkint(L, 3);
		}

    int fd = socket(domain, type, proto);
    if(fd < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    socket_t *sock = (socket_t*) lua_newuserdata(L, sizeof(socket_t));
		luaL_getmetatable(L, SOCKET_METATABLE);
		lua_setmetatable(L, -2);
    sock->fd = fd;
    return 1;
}

/* socket object methods */
static int
_socket_gc(lua_State *L) {
	return 0;
}

static int
_socket_tostring(lua_State *L) {
	lua_pushstring(L, "socket");
	return 1;
}

static int
_socket_connect(lua_State *L) {
	socket_t *sock = _getsock(L, 1);
	return 0;
}

static int
_socket_send(lua_State *L) {
	return 0;
}

/* end */
static void
_add_int_constant(lua_State *L, const char* name, int value) {
	lua_pushinteger(L, value);
	lua_setfield(L, -2, name);
}

int
luaopen_socket_c(lua_State *L) {
    luaL_checkversion(L);

		// +construct socket metatable
		luaL_Reg socket_mt[] = {
				{"__gc", _socket_gc},
				{"__tostring",  _socket_tostring},
        {NULL, NULL}
		};

		luaL_Reg socket_methods[] = {
				{"connect", _socket_connect},
				{"send", _socket_send},
        {NULL, NULL}
		};

		luaL_newmetatable(L, SOCKET_METATABLE);
		luaL_setfuncs(L, socket_mt, 0);

		luaL_newlib(L, socket_methods);
		lua_setfield(L, -2, "__index");
		lua_pop(L, 1);
		// +end

    luaL_Reg l[] = {
        {"socket", _socket},
        {NULL, NULL}
    };

    luaL_newlib(L, l);
		// address family
		_add_int_constant(L, "AF_INET", AF_INET);
		_add_int_constant(L, "AF_INET6", AF_INET6);

		// socket type
		_add_int_constant(L, "SOCK_STREAM", SOCK_STREAM);
		_add_int_constant(L, "SOCK_DGRAM", SOCK_DGRAM);

    return 1;
}

