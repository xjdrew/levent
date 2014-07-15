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

typedef struct loop_t {
    struct ev_loop *loop;
} loop_t;

/*
 *    internal function
 */
inline static loop_t* getloop(lua_State *L, int index) {
    loop_t *lo = (loop_t*)luaL_checkudata(L, index, LOOP_METATABLE);
    return lo;
}

inline static void setloop(lua_State *L, struct ev_loop *loop) {
    loop_t *lo = (loop_t*)lua_newuserdata(L, sizeof(loop_t));
    luaL_getmetatable(L, LOOP_METATABLE);
    lua_setmetatable(L, -2);

    lo->loop = loop;
}

/*
 *    end
 */

static int ev_version(lua_State *L) {
    lua_pushinteger(L, ev_version_major());
    lua_pushinteger(L, ev_version_minor());
    return 2;
}

/*
 * loop function
 */
static int default_loop(lua_State *L) {
    unsigned int flags = luaL_optunsigned(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_default_loop(flags);
    setloop(L, loop);
    return 1;
}

static int new_loop(lua_State *L) {
    unsigned int flags = luaL_optunsigned(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_loop_new(flags);
    setloop(L, loop);
    return 1;
}

static int loop_destroy(lua_State *L) {
    loop_t *lo = getloop(L, 1);
    struct ev_loop* loop = lo->loop;
    if(loop) {
        lo->loop = 0;
        ev_loop_destroy(loop);
    }
    return 0;
}

static int loop_tostring(lua_State *L) {
    loop_t *lo = getloop(L, 1);
    lua_pushfstring(L, "loop: %p", lo);
    return 1;
}

// void ev_*(loop)
#define LOOP_METHOD_VOID(name) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = getloop(L, 1); \
        ev_##name(lo->loop); \
        return 0; \
    }

// int/unsigned/boolean/number ev_*(loop)
#define LOOP_METHOD_RET(name, type) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = getloop(L, 1); \
        lua_push##type(L, ev_##name(lo->loop)); \
        return 1; \
    }

// void ev_*(loop, arg1)
#define LOOP_METHOD_VOID_ARG_ARG(name, carg, larg) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = getloop(L, 1); \
        carg a1 = luaL_check##larg(L, 2); \
        ev_##name(lo->loop, a1); \
        return 0; \
    }

// int/unsigned/boolean/number ev_*(loop, arg1)
#define LOOP_METHOD_RET_ARG_ARG(name, type, carg, larg) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = getloop(L, 1); \
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
LOOP_METHOD_VOID(verify)

LOOP_METHOD_DOUBLE(now)
LOOP_METHOD_VOID(now_update)
LOOP_METHOD_VOID_INT(break)
LOOP_METHOD_VOID(ref)
LOOP_METHOD_VOID(unref)
LOOP_METHOD_UNSIGNED(pending_count)

static int loop_run(lua_State *L) {
    loop_t *lo = getloop(L, 1);
    int flags = luaL_checkint(L, 2);

    struct ev_loop *loop = lo->loop;
    ev_set_userdata(loop, L);
    lua_pushboolean(L, ev_run(loop, flags));
    ev_set_userdata(loop, NULL);
    return 1;
}

static const luaL_Reg mt_loop[] = {
    {"__gc", loop_destroy},
    {"__tostring", loop_tostring},
    {NULL, NULL}
};

static const luaL_Reg methods_loop[] = {
    {"loop_fork", loop_loop_fork},
    {"is_default_loop", loop_is_default_loop},
    {"iteration", loop_iteration},
    {"depth", loop_depth},
    {"backend", loop_backend},
    {"suspend", loop_suspend},
    {"resume", loop_resume},
    {"run", loop_run},
    {"verify", loop_verify},

    {"now", loop_now},
    {"now_update", loop_now_update},
    {"break", loop_break},
    {"ref", loop_ref},
    {"unref", loop_unref},
    {"pending_count", loop_pending_count},

    {NULL, NULL}
};

/*
 *  end
 */

// watcher
#define WATCHER_NEW(type) \
    static int new_##type(lua_State *L) { \
        ev_##type *w = (ev_##type*)lua_newuserdata(L, sizeof(*w)); \
        luaL_getmetatable(L, WATCHER_METATABLE(type)); \
        lua_setmetatable(L, -2); \
        return 1;\
    }

#define WATCHER_GET(type) \
    inline static ev_##type* get_##type(lua_State *L, int index) { \
        ev_##type *w = (ev_##type*)luaL_checkudata(L, index, WATCHER_METATABLE(type)); \
        return w; \
    }

#define WATCHER_TOSTRING(type) \
    static int type##_tostring(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        lua_pushfstring(L, "%s: %p", #type, w); \
        return 1; \
    }

#define WATCHER_ID(type) \
    static int type##_id(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        lua_pushlightuserdata(L, (void*)w); \
        return 1; \
    }

#define WATCHER_IS_ACTIVE(type) 
#define WATCHER_IS_PENDING(type) 
#define WATCHER_SET_PRIORITY(type) 
#define WATCHER_GET_PRIORITY(type) 

// io
WATCHER_NEW(io)
WATCHER_GET(io)
WATCHER_ID(io)
WATCHER_TOSTRING(io)

static void io_cb(struct ev_loop *loop, ev_io *w, int revents) {
    // lua_State *L = ev_userdata(loop);
}

static int io_init(lua_State *L) {
    ev_io *w = get_io(L, 1);

    return 0;
}

static int io_start(lua_State *L) {
    return 0;
}

static int io_stop(lua_State *L) {
    return 0;
}

static const luaL_Reg mt_io[] = {
    {"__tostring", io_tostring},
    {NULL, NULL}
};

static const luaL_Reg methods_io[] = {
    {"init", io_init},
    {"id", io_id},
    {"start", io_start},
    {"stop", io_stop},
    {NULL, NULL}
};

// timer
WATCHER_NEW(timer)
WATCHER_GET(timer)
WATCHER_ID(timer)
WATCHER_TOSTRING(timer)

static int timer_init(lua_State *L) {
    return 0;
}

static int timer_start(lua_State *L) {
    return 0;
}

static int timer_stop(lua_State *L) {
    return 0;
}
static int timer_again(lua_State *L) {
    return 0;
}

static const luaL_Reg mt_timer[] = {
    {"__tostring", timer_tostring},
    {NULL, NULL}
};

static const luaL_Reg methods_timer[] = {
    {"init", timer_init},
    {"id", timer_id},
    {"start", timer_start},
    {"stop", timer_stop},
    {"again", timer_again},
    {NULL, NULL}
};

// create_metatable_*
METATABLE_BUILDER(loop, LOOP_METATABLE)
METATABLE_BUILDER(io, WATCHER_METATABLE(io))
METATABLE_BUILDER(timer, WATCHER_METATABLE(timer))

int luaopen_event_c(lua_State *L) {
    luaL_checkversion(L);

    // call create metatable
    CREATE_METATABLE(loop, L);
    CREATE_METATABLE(io, L);
    CREATE_METATABLE(timer, L);

    luaL_Reg l[] = {
        {"version", ev_version},
        {"default_loop", default_loop},
        {"new_loop", new_loop},
        {"new_io", new_io},
        {"new_timer", new_timer},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

