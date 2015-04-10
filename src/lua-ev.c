/* lua-ev.c
 * author: xjdrew
 * date: 2014-07-13
 */
#include <assert.h>
#include <stdio.h>

#include "levent.h"
#include "ev.h"

#define LOOP_METATABLE "loop_metatable"
#define WATCHER_METATABLE(type) "watcher_" #type "_metatable"

#define LOG printf

#define METATABLE_BUILDER_NAME(type) create_metatable_##type
#define METATABLE_BUILDER(type, name) \
    static void METATABLE_BUILDER_NAME(type) (lua_State *L) { \
        if(luaL_newmetatable(L, name)) { \
            luaL_setfuncs(L, mt_##type, 0); \
            luaL_newlib(L, methods_##type); \
            lua_setfield(L, -2, "__index"); \
        }\
        lua_pop(L, 1); \
    }

#define CREATE_METATABLE(type, L) METATABLE_BUILDER_NAME(type)(L)

typedef struct loop_t {
    struct ev_loop *loop;
} loop_t;

/*
 *    internal function
 */
INLINE static loop_t* get_loop(lua_State *L, int index) {
    loop_t *lo = (loop_t*)luaL_checkudata(L, index, LOOP_METATABLE);
    return lo;
}

INLINE static void setloop(lua_State *L, struct ev_loop *loop) {
    loop_t *lo = (loop_t*)lua_newuserdata(L, sizeof(loop_t));
    luaL_getmetatable(L, LOOP_METATABLE);
    lua_setmetatable(L, -2);

    lo->loop = loop;
}

static int 
traceback (lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else {
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

// *L = ev_userdata(loop)
// L statck:
//  -1: trace
//  -2: ud
//  -3: lua function
static void watcher_cb(struct ev_loop *loop, void *w, int revents) {
    lua_State *L = ev_userdata(loop);
    int traceback = lua_gettop(L);
    int r;

    assert(L != NULL);
    assert(lua_isfunction(L, traceback));
    assert(lua_isfunction(L, traceback - 2));
    lua_pushvalue(L, traceback - 2); // function
    lua_pushvalue(L, traceback - 1); // ud
    lua_pushlightuserdata(L, (void*)w);
    lua_pushinteger(L, revents);

    r = lua_pcall(L, 3, 0, traceback);
    if(r == LUA_OK) {
        return;
    }
    LOG("watcher(%p) event:%x, callback failed, errcode:%d, msg: %s", w, revents, r, lua_tostring(L, -1));
    lua_pop(L, 1);
    return;
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
    unsigned int flags = (unsigned int)luaL_optinteger(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_default_loop(flags);
    setloop(L, loop);
    return 1;
}

static int new_loop(lua_State *L) {
    unsigned int flags = (unsigned int)luaL_optinteger(L, 1, EVFLAG_AUTO);
    struct ev_loop *loop = ev_loop_new(flags);
    setloop(L, loop);
    return 1;
}

static int loop_destroy(lua_State *L) {
    loop_t *lo = get_loop(L, 1);
    struct ev_loop* loop = lo->loop;
    if(loop) {
        lo->loop = 0;
        ev_loop_destroy(loop);
    }
    return 0;
}

static int loop_tostring(lua_State *L) {
    loop_t *lo = get_loop(L, 1);
    lua_pushfstring(L, "loop: %p", lo);
    return 1;
}

// void ev_*(loop)
#define LOOP_METHOD_VOID(name) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = get_loop(L, 1); \
        ev_##name(lo->loop); \
        return 0; \
    }

// int/unsigned/boolean/number ev_*(loop)
#define LOOP_METHOD_RET(name, type) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = get_loop(L, 1); \
        lua_push##type(L, ev_##name(lo->loop)); \
        return 1; \
    }

// void ev_*(loop, arg1)
#define LOOP_METHOD_VOID_ARG_ARG(name, carg) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = get_loop(L, 1); \
        carg a1 = luaL_checkinteger(L, 2); \
        ev_##name(lo->loop, a1); \
        return 0; \
    }

// int/unsigned/boolean/number ev_*(loop, arg1)
#define LOOP_METHOD_RET_ARG_ARG(name, type, carg) \
    static int loop_##name(lua_State *L) { \
        loop_t *lo = get_loop(L, 1); \
        carg a1 = luaL_checkinteger(L, 2); \
        lua_push##type(L, ev_##name(lo->loop, a1)); \
        return 1; \
    }

#define LOOP_METHOD_BOOL(name) LOOP_METHOD_RET(name, boolean)
#define LOOP_METHOD_INT(name) LOOP_METHOD_RET(name, integer)
#define LOOP_METHOD_UNSIGNED(name) LOOP_METHOD_RET(name, integer)
#define LOOP_METHOD_DOUBLE(name) LOOP_METHOD_RET(name, number)
#define LOOP_METHOD_VOID_INT(name) LOOP_METHOD_VOID_ARG_ARG(name, int)
#define LOOP_METHOD_BOOL_INT(name) LOOP_METHOD_RET_ARG_ARG(name, boolean, int)

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

// L statck:
//  4: ud
//  3: cb
//  2: flags
//  1: loop
static int loop_run(lua_State *L) {
    struct ev_loop *loop;
    loop_t *lo = get_loop(L, 1);
    int flags = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    luaL_checkany(L, 4);
    lua_pushcfunction(L, traceback);

    loop = lo->loop;
    ev_set_userdata(loop, L);
    lua_pushboolean(L, ev_run(loop, flags));
    ev_set_userdata(loop, NULL);
    return 1;
}

static const struct luaL_Reg mt_loop[] = {
    {"__gc", loop_destroy},
    {"__tostring", loop_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_loop[] = {
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
    {"_break", loop_break},
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
    INLINE static ev_##type* get_##type(lua_State *L, int index) { \
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

#define WATCHER_START(type) \
    static int type##_start(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        loop_t *lo = get_loop(L, 2); \
        ev_##type##_start(lo->loop, w); \
        return 1; \
    }

#define WATCHER_STOP(type) \
    static int type##_stop(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        loop_t *lo = get_loop(L, 2); \
        ev_##type##_stop(lo->loop, w); \
        return 1; \
    }

#define WATCHER_IS_ACTIVE(type) \
    static int type##_is_active(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        lua_pushboolean(L, ev_is_active(w)); \
        return 1; \
    }

#define WATCHER_IS_PENDING(type) \
    static int type##_is_pending(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        lua_pushboolean(L, ev_is_pending(w)); \
        return 1;\
    }

#define WATCHER_SET_PRIORITY(type) \
    static int type##_set_priority(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        int priority = luaL_checkinteger(L, 2); \
        ev_set_priority(w, priority); \
        return 0; \
    }

#define WATCHER_GET_PRIORITY(type) \
    static int type##_get_priority(lua_State *L) { \
        ev_##type *w = get_##type(L, 1); \
        lua_pushinteger(L, ev_priority(w)); \
        return 1; \
    }

#define WATCHER_COMMON_METHODS(type) \
    WATCHER_NEW(type) \
    WATCHER_GET(type) \
    WATCHER_TOSTRING(type) \
    WATCHER_ID(type) \
    WATCHER_START(type) \
    WATCHER_STOP(type) \
    WATCHER_IS_ACTIVE(type) \
    WATCHER_IS_PENDING(type) \
    WATCHER_GET_PRIORITY(type) \
    WATCHER_SET_PRIORITY(type) \

#define WATCHER_METAMETHOD_ITEM(type, name) {#name, type##_##name}
#define WATCHER_METAMETHOD_TABLE(type) \
    WATCHER_METAMETHOD_ITEM(type, id), \
    WATCHER_METAMETHOD_ITEM(type, start), \
    WATCHER_METAMETHOD_ITEM(type, stop), \
    WATCHER_METAMETHOD_ITEM(type, init), \
    WATCHER_METAMETHOD_ITEM(type, is_active), \
    WATCHER_METAMETHOD_ITEM(type, is_pending), \
    WATCHER_METAMETHOD_ITEM(type, get_priority), \
    WATCHER_METAMETHOD_ITEM(type, set_priority)

// ev_io
WATCHER_COMMON_METHODS(io)

static int io_init(lua_State *L) {
    ev_io *w = get_io(L, 1);
    int fd = luaL_checkinteger(L, 2);
    int revents = luaL_checkinteger(L, 3);
    ev_io_init(w, watcher_cb, fd, revents);
    return 0;
}

static const struct luaL_Reg mt_io[] = {
    {"__tostring", io_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_io[] = {
    WATCHER_METAMETHOD_TABLE(io),
    {NULL, NULL}
};

// ev_timer
WATCHER_COMMON_METHODS(timer)

static int timer_init(lua_State *L) {
    ev_timer *w = get_timer(L, 1);
    double after = luaL_checknumber(L, 2);
    double repeat = luaL_optnumber(L, 3, 0);
    ev_timer_init(w, watcher_cb, after, repeat);
    return 0;
}

static int timer_again(lua_State *L) {
    ev_timer *w = get_timer(L, 1);
    loop_t *lo = get_loop(L, 2);
    ev_timer_again(lo->loop, w);
    return 0;
}

static const struct luaL_Reg mt_timer[] = {
    {"__tostring", timer_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_timer[] = {
    WATCHER_METAMETHOD_TABLE(timer),
    {"again", timer_again},
    {NULL, NULL}
};

// ev_signal
WATCHER_COMMON_METHODS(signal)

static int signal_init(lua_State *L) {
    ev_signal *w = get_signal(L, 1);
    int signum = luaL_checkinteger(L, 2);
    ev_signal_init(w, watcher_cb, signum);
    return 0;
}

static const struct luaL_Reg mt_signal[] = {
    {"__tostring", signal_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_signal[] = {
    WATCHER_METAMETHOD_TABLE(signal),
    {NULL, NULL}
};

// ev_prepare
WATCHER_COMMON_METHODS(prepare)

static int prepare_init(lua_State *L) {
    ev_prepare *w = get_prepare(L, 1);
    ev_prepare_init(w, watcher_cb);
    return 0;
}

static const struct luaL_Reg mt_prepare[] = {
    {"__tostring", prepare_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_prepare[] = {
    WATCHER_METAMETHOD_TABLE(prepare),
    {NULL, NULL}
};

// ev_check
WATCHER_COMMON_METHODS(check)

static int check_init(lua_State *L) {
    ev_check *w = get_check(L, 1);
    ev_check_init(w, watcher_cb);
    return 0;
}

static const struct luaL_Reg mt_check[] = {
    {"__tostring", check_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_check[] = {
    WATCHER_METAMETHOD_TABLE(check),
    {NULL, NULL}
};

// ev_idle
WATCHER_COMMON_METHODS(idle)

static int idle_init(lua_State *L) {
    ev_idle *w = get_idle(L, 1);
    ev_idle_init(w, watcher_cb);
    return 0;
}

static const struct luaL_Reg mt_idle[] = {
    {"__tostring", idle_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg methods_idle[] = {
    WATCHER_METAMETHOD_TABLE(idle),
    {NULL, NULL}
};

// create_metatable_*
METATABLE_BUILDER(loop, LOOP_METATABLE)
METATABLE_BUILDER(io, WATCHER_METATABLE(io))
METATABLE_BUILDER(timer, WATCHER_METATABLE(timer))
METATABLE_BUILDER(signal, WATCHER_METATABLE(signal))
METATABLE_BUILDER(prepare, WATCHER_METATABLE(prepare))
METATABLE_BUILDER(check, WATCHER_METATABLE(check))
METATABLE_BUILDER(idle, WATCHER_METATABLE(idle))

struct luaL_Reg ev_module_methods[] = {
    {"version", ev_version},
    {"default_loop", default_loop},
    {"new_loop", new_loop},
    {"new_io", new_io},
    {"new_timer", new_timer},
    {"new_signal", new_signal},
    {"new_prepare", new_prepare},
    {"new_check", new_check},
    {"new_idle", new_idle},
    {NULL, NULL}
};

LUALIB_API int luaopen_levent_ev_c(lua_State *L) {
    luaL_checkversion(L);

    // call create metatable
    CREATE_METATABLE(loop, L);
    CREATE_METATABLE(io, L);
    CREATE_METATABLE(timer, L);
    CREATE_METATABLE(signal, L);
    CREATE_METATABLE(prepare, L);
    CREATE_METATABLE(check, L);
    CREATE_METATABLE(idle, L);

    luaL_newlib(L, ev_module_methods);

    // add constant
    ADD_CONSTANT(L, EV_READ)
    ADD_CONSTANT(L, EV_WRITE);
    ADD_CONSTANT(L, EV_TIMER);
    ADD_CONSTANT(L, EV_PERIODIC);
    ADD_CONSTANT(L, EV_SIGNAL);
    ADD_CONSTANT(L, EV_CHILD);
    ADD_CONSTANT(L, EV_STAT);
    ADD_CONSTANT(L, EV_IDLE);
    ADD_CONSTANT(L, EV_PREPARE);
    ADD_CONSTANT(L, EV_CHECK);
    ADD_CONSTANT(L, EV_EMBED);
    ADD_CONSTANT(L, EV_FORK);
    ADD_CONSTANT(L, EV_CLEANUP);
    ADD_CONSTANT(L, EV_ASYNC);
    ADD_CONSTANT(L, EV_CUSTOM);
    ADD_CONSTANT(L, EV_ERROR);

    // break
    ADD_CONSTANT(L, EVBREAK_ONE);
    ADD_CONSTANT(L, EVBREAK_ALL);

    // priority
    ADD_CONSTANT(L, EV_MINPRI);
    ADD_CONSTANT(L, EV_MAXPRI);
    return 1;
}

