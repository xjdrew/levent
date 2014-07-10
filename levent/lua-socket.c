/* lua-socket.c
 * author: xjdrew
 * date: 2014-07-10
 */ 
#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include "lua.h"
#include "lauxlib.h"

typedef struct _socket_t {
    int fd;
} socket_t;

/*
 * args: int domain, int type, int protocal
 */
int
_socket(lua_State *L) {
    int domain   = luaL_checkint(L, 1);
    int type     = luaL_checkint(L, 2);
    int protocal = luaL_checkint(L, 3);

    int fd = socket(domain, type, protocal);
    if(fd < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    socket_t *sock = (socket_t*) lua_newuserdata(L, sizeof(socket_t));
    sock->fd = fd;
    return 1;
}

int
luaopen_socket_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_Reg l[] = {
        {"socket", _socket},
        {NULL, NULL}
    };

    luaL_newlib(L, l);
    return 1;
}

