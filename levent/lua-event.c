/* lua-ev.c
 * author: xjdrew
 * date: 2014-07-13
 */
#include "lua.h"
#include "lauxlib.h"

#include "ev.h"

#define LOOP_METATABLE "loop_metatable"

typedef struct _loop_t {
    struct ev_loop *loop;
} loop_t;

/*
 *    internal function
 */
inline static loop_t*
_getloop(lua_State *L, int index) {
    loop_t *lo = (loop_t*)luaL_checkudata(L, index, LOOP_METATABLE);
    return lo;
}

inline static void
_setloop(lua_State *L, struct ev_loop *loop) {
    loop_t *lo = (loop_t*)lua_newuserdata(L, sizeof(loop_t));
    luaL_getmetatable(L, LOOP_METATABLE);
    lua_setmetatable(L, -2);

    lo->loop = loop;
}

/*
 *    end
 */

static int
_ev_version(lua_State *L) {
    lua_pushinteger(L, ev_version_major());
    lua_pushinteger(L, ev_version_minor());
    return 2;
}

static int
_default_loop(lua_State *L) {
    unsigned int flags = luaL_optunsigned(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_default_loop(flags);
    _setloop(L, loop);
    return 1;
}

static int
_loop_new(lua_State *L) {
    unsigned int flags = luaL_optunsigned(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_loop_new(flags);
    _setloop(L, loop);
    return 1;
}

static int
_loop_destroy(lua_State *L) {
    loop_t *lo = _getloop(L, 1);
    struct ev_loop* loop = lo->loop;
    if(loop) {
        lo->loop = 0;
        ev_loop_destroy(loop);
    }
    return 0;
}

static int
_loop_tostring(lua_State *L) {
    loop_t *lo = _getloop(L, 1);
    lua_pushfstring(L, "loop: %p", lo);
    return 1;
}

// void ev_*(loop)
#define LOOP_METHOD_VOID(name) \
    static int _##name(lua_State *L) { \
        loop_t *lo = _getloop(L, 1); \
        ev_##name(lo->loop); \
        return 0; \
    }

// int/unsigned/boolean/number ev_*(loop)
#define LOOP_METHOD_RET(name, type) \
    static int _##name(lua_State *L) { \
        loop_t *lo = _getloop(L, 1); \
        lua_push##type(L, ev_##name(lo->loop)); \
        return 1; \
    }

// void ev_*(loop, arg1)
#define LOOP_METHOD_VOID_ARG_ARG(name, carg, larg) \
    static int _##name(lua_State *L) { \
        loop_t *lo = _getloop(L, 1); \
        carg a1 = luaL_check##larg(L, 2); \
        ev_##name(lo->loop, a1); \
        return 0; \
    }

// int/unsigned/boolean/number ev_*(loop, arg1)
#define LOOP_METHOD_RET_ARG_ARG(name, type, carg, larg) \
    static int _##name(lua_State *L) { \
        loop_t *lo = _getloop(L, 1); \
        carg a1 = luaL_check##larg(L, 2); \
        lua_push##type(L, ev_##name(lo->loop, a1)); \
        return 1; \
    }

#define LOOP_METHOD_BOOL(name) LOOP_METHOD_RET(name, boolean)
#define LOOP_METHOD_INT(name) LOOP_METHOD_RET(name, integer)
#define LOOP_METHOD_UNSIGNED(name) LOOP_METHOD_RET(name, unsigned)
#define LOOP_METHOD_DOUBLE(name) LOOP_METHOD_RET(name, number)
#define LOOP_METHOD_VOID_INT(name) LOOP_METHOD_VOID_ARG_ARG(name, int, int)
#define LOOP_METHOD_BOOL_INT(name) LOOP_METHOD_RET_ARG_ARG(name, boolean, int, int)

LOOP_METHOD_VOID(loop_fork)
LOOP_METHOD_BOOL(is_default_loop)
LOOP_METHOD_UNSIGNED(iteration)
LOOP_METHOD_UNSIGNED(depth)
LOOP_METHOD_UNSIGNED(backend)
LOOP_METHOD_VOID(suspend)
LOOP_METHOD_VOID(resume)
LOOP_METHOD_BOOL_INT(run)
LOOP_METHOD_VOID(verify)

LOOP_METHOD_DOUBLE(now)
LOOP_METHOD_VOID(now_update)
LOOP_METHOD_VOID_INT(break)
LOOP_METHOD_VOID(ref)
LOOP_METHOD_VOID(unref)
LOOP_METHOD_UNSIGNED(pending_count)

static void
create_loop_metatables(lua_State *L) {
    luaL_Reg loop_mt[] = {
        {"__gc", _loop_destroy},
        {"__tostring", _loop_tostring},
        {NULL, NULL}
    };

    luaL_Reg loop_methods[] = {
        {"loop_fork", _loop_fork},
        {"is_default_loop", _is_default_loop},
        {"iteration", _iteration},
        {"depth", _depth},
        {"backend", _backend},
        {"suspend", _suspend},
        {"resume", _resume},
        //{"run", _run},
        {"verify", _verify},

        {"now", _now},
        {"now_update", _now_update},
        //{"break", _break},
        {"ref", _ref},
        {"unref", _unref},
        {"pending_count", _pending_count},

        {NULL, NULL}
    };

    luaL_newmetatable(L, LOOP_METATABLE);
    luaL_setfuncs(L, loop_mt, 0);

    luaL_newlib(L, loop_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

int
luaopen_event_c(lua_State *L) {
    luaL_checkversion(L);

    // create metatable
    create_loop_metatables(L);

    luaL_Reg l[] = {
        {"version", _ev_version},
        {"default_loop", _default_loop},
        {"loop_new", _loop_new},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

