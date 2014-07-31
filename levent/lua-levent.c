/* lua-levent.c
 * author: xjdrew
 * date: 2014-07-24
 */
#include "levent.h"

static int unique(lua_State *L) {
    lua_newuserdata(L, 0);
    return 1;
}

static int topointer(lua_State *L) {
    luaL_checkany(L, 1);
    const void *p = lua_topointer(L, 1);
    lua_pushfstring(L, "%p", p);
    return 1;
}

int luaopen_levent_c(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        {"unique", unique},
        {"topointer", topointer},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

