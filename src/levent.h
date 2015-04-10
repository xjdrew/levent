#ifndef LEVENT_H
#define LEVENT_H

#include "lua.h"
#include "lauxlib.h"

/* for compatible with lua5.1 */
#ifndef LUA_OK
#define LUA_OK 0
#endif

#ifdef _WIN32
#define INLINE __inline
#else
#define INLINE inline
#endif

INLINE static void
_add_unsigned_constant(lua_State *L, const char* name, unsigned int value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, name);
}

#define ADD_CONSTANT(L, name) _add_unsigned_constant(L, #name, name);

#endif //LEVENT_H

