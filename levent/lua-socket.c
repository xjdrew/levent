/* lua-socket.c
 * author: xjdrew
 * date: 2014-07-10
 */

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 address families are supported
- Only SOCK_STREAM, SOCK_DGRAM socket type are supported
- Only IPPROTO_TCP, IPPROTO_UDP protocal type are supported
- Don't support dns lookup, must be numerical network address

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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "lua.h"
#include "lauxlib.h"

#define SOCKET_METATABLE "socket_metatable"

typedef struct _socket_t {
    int fd;
    int family;
    int type;
    int protocol;
} socket_t;

/* 
 *   internal function 
 */
inline static
socket_t* _getsock(lua_State *L, int index) {
    socket_t* sock = (socket_t*)luaL_checkudata(L, index, SOCKET_METATABLE);
    return sock;
}

static int
_getsockaddrarg(socket_t *sock, const char *host, const char *port, struct addrinfo **res) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = sock->family;
    hints.ai_socktype = sock->type;
    hints.ai_protocol = sock->protocol;
    hints.ai_flags = AI_NUMERICHOST;

    return getaddrinfo(host, port, &hints, res);
}

/*
 *    end
 */

/*
 *   args: int domain, int type, int protocal
 */
static int
_socket(lua_State *L) {
    int family    = luaL_checkint(L, 1);
    int type      = luaL_checkint(L, 2);
    int protocol  = 0;
    if(lua_gettop(L) > 2) {
        protocol = luaL_checkint(L, 3);
    }

    int fd = socket(family, type, protocol);
    if(fd < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    socket_t *sock = (socket_t*) lua_newuserdata(L, sizeof(socket_t));
    luaL_getmetatable(L, SOCKET_METATABLE);
    lua_setmetatable(L, -2);

    sock->fd = fd;
    sock->family = family;
    sock->type = type;
    sock->protocol = protocol;
    return 1;
}

/* socket object methods */
static int
_socket_setblocking(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int block = lua_toboolean(L, 2);
    int flag = fcntl(sock->fd, F_GETFL, 0);
    if (block) {
        flag &= (~O_NONBLOCK);
    } else {
        flag |= O_NONBLOCK;
    }
    fcntl(sock->fd, F_SETFL, flag);
    return 0;
}

static int
_socket_connect(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    const char *host = luaL_checkstring(L, 2);
    luaL_checkint(L, 3);
    const char *port = lua_tostring(L, 3);

    struct addrinfo *res = 0;
    int err = _getsockaddrarg(sock, host, port, &res);
    if(err != 0) {
        lua_pushinteger(L, err);
        return 1;
    }

    err = connect(sock->fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if(err != 0) {
        lua_pushinteger(L, errno);
    } else {
        lua_pushinteger(L, err);
    }
    return 1;
}

static int
_socket_recv(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    size_t len = luaL_checkunsigned(L, 2);
    char* buf = lua_newuserdata(L, len);
    ssize_t nread = recv(sock->fd, buf, len, 0);
    if(nread < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushlstring(L, buf, nread);
    return 1;
}

static int
_socket_send(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    size_t len;
    const char* buf = luaL_checklstring(L, 2, &len);
    int flags = MSG_NOSIGNAL;

    int n = send(sock->fd, buf, len, flags);
    if(n < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushinteger(L, n);
    return 1;
}

static int
_socket_recvfrom(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    return 0;
}

static int
_socket_sendto(lua_State *L) {
    return 0;
}

static int
_socket_bind(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    const char *host = luaL_checkstring(L, 2);
    luaL_checkint(L, 3);
    const char *port = lua_tostring(L, 3);

    struct addrinfo *res = 0;
    int err = _getsockaddrarg(sock, host, port, &res);
    if(err != 0) {
        lua_pushinteger(L, err);
        return 1;
    }

    err = bind(sock->fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if(err != 0) {
        lua_pushinteger(L, errno);
    } else {
        lua_pushinteger(L, err);
    }
    return 1;
}

static int
_socket_listen(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int backlog = luaL_optnumber(L, 2, 5);
    int err = listen(sock->fd, backlog);
    if(err != 0) {
        lua_pushinteger(L, errno);
    } else {
        lua_pushinteger(L, err);
    }
    return 1;
}

static int
_socket_accept(lua_State *L) {
}

static int
_socket_fileno(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    lua_pushinteger(L, sock->fd);
    return 1;
}

static int
_socket_close(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int fd = sock->fd;
    if(fd != -1) {
        sock->fd = -1;
        close(fd);
    }
    return 0;
}

static int
_socket_tostring(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    lua_pushfstring(L, "socket: %p", sock);
    return 1;
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
        {"__gc", _socket_close},
        {"__tostring",  _socket_tostring},
        {NULL, NULL}
    };

    luaL_Reg socket_methods[] = {
        {"setblocking", _socket_setblocking},
        {"connect", _socket_connect},
        {"recv", _socket_recv},
        {"send", _socket_send},
        {"sendto", _socket_sendto},
        {"bind", _socket_bind},
        {"listen", _socket_listen},
        {"accept", _socket_accept},
        {"fileno", _socket_fileno},
        {"close", _socket_close},
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

    // protocal type
    _add_int_constant(L, "IPPROTO_TCP", IPPROTO_TCP);
    _add_int_constant(L, "IPPROTO_UDP", IPPROTO_UDP);

    return 1;
}

