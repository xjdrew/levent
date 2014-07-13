/* lua-ev.c
 * author: xjdrew
 * date: 2014-07-13
 */
#include "lua.h"
#include "lauxlib.h"

#include "ev.h"
static int
_ev_version(lua_State *L) {
    lua_pushinteger(L, ev_version_major());
    lua_pushinteger(L, ev_version_minor());
    return 2;
}

int
luaopen_event_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_Reg l[] = {
        {"version", _ev_version},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

