/* lua-ev.c
 * author: xjdrew
 * date: 2014-07-13
 */
#include "lua.h"
#include "lauxlib.h"

#include "ev.h"

#define LOOP_METATABLE "loop_metatable"
#define WATCHER_METATABLE(type) "watcher_" #type "_metatable"

#define METATABLE_BUILDER_NAME(type) create_metatable_##type
#define METATABLE_BUILDER(type, name) \
    static void METATABLE_BUILDER_NAME(type) (lua_State *L) { \
        luaL_newmetatable(L, name); \
        luaL_setfuncs(L, mt_##type, 0); \
        luaL_newlib(L, methods_##type); \
        lua_setfield(L, -2, "__index"); \
        lua_pop(L, 1); \
    }

#define CREATE_METATABLE(type, L) METATABLE_BUILDER_NAME(type)(L)

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

/*
 * loop function
 */
static int
_default_loop(lua_State *L) {
    unsigned int flags = luaL_optunsigned(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_default_loop(flags);
    _setloop(L, loop);
    return 1;
}

static int
_new_loop(lua_State *L) {
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

static const luaL_Reg mt_loop[] = {
    {"__gc", _loop_destroy},
    {"__tostring", _loop_tostring},
    {NULL, NULL}
};

static const luaL_Reg methods_loop[] = {
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

/*
 *  end
 */

// watcher
#define WATCHER_NEW(type) \
    static int _new_##type(lua_State *L) { \
        ev_##type *w = (ev_##type*)lua_newuserdata(L, sizeof(*w)); \
        luaL_getmetatable(L, WATCHER_METATABLE(type)); \
        lua_setmetatable(L, -2); \
        return 1;\
    }

#define WATCHER_GET(type) \
    inline static ev_##type* _get##type(lua_State *L, int index) { \
        ev_##type *w = (ev_##type*)luaL_checkudata(L, index, WATCHER_METATABLE(type)); \
        return w; \
    }

#define WATCHER_TOSTRING(type) \
    static int _tostring_##type(lua_State *L) { \
        ev_##type *w = _get##type(L, 1); \
        lua_pushfstring(L, "%s: %p", #type, w); \
        return 1; \
    }

// io
WATCHER_NEW(io)
WATCHER_GET(io)
WATCHER_TOSTRING(io)

static int
_io_init(lua_State *L) {
    return 0;
}

static int
_io_start(lua_State *L) {
    return 0;
}
static int
_io_stop(lua_State *L) {
    return 0;
}

static const luaL_Reg mt_io[] = {
    {"__tostring", _tostring_io},
    {NULL, NULL}
};

static const luaL_Reg methods_io[] = {
    {"init", _io_init},
    {"start", _io_start},
    {"stop", _io_stop},
    {NULL, NULL}
};

// timer
WATCHER_NEW(timer)
WATCHER_GET(timer)
WATCHER_TOSTRING(timer)

static int
_timer_init(lua_State *L) {
    return 0;
}

static int
_timer_start(lua_State *L) {
    return 0;
}

static int
_timer_stop(lua_State *L) {
    return 0;
}
static int
_timer_again(lua_State *L) {
    return 0;
}

static const luaL_Reg mt_timer[] = {
    {"__tostring", _tostring_timer},
    {NULL, NULL}
};

static const luaL_Reg methods_timer[] = {
    {"init", _timer_init},
    {"start", _timer_start},
    {"stop", _timer_stop},
    {"again", _timer_again},
    {NULL, NULL}
};

// create_metatable_*
METATABLE_BUILDER(loop, LOOP_METATABLE)
METATABLE_BUILDER(io, WATCHER_METATABLE(io))
METATABLE_BUILDER(timer, WATCHER_METATABLE(timer))

int
luaopen_event_c(lua_State *L) {
    luaL_checkversion(L);

    // call create metatable
    CREATE_METATABLE(loop, L);
    CREATE_METATABLE(io, L);
    CREATE_METATABLE(timer, L);

    luaL_Reg l[] = {
        {"version", _ev_version},
        {"default_loop", _default_loop},
        {"new_loop", _new_loop},
        {"new_io", _new_io},
        {"new_timer", _new_timer},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

