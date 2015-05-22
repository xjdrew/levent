/* lua-errno.c
 * author: xjdrew
 * date: 2014-07-24
 */
#include <string.h>
#include <errno.h>

#include "levent.h"

#ifdef _WIN32
#include <windows.h>

#undef EINVAL
#define EINVAL WSAEINVAL

#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK

#undef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS

#undef EALREADY
#define EALREADY WSAEALREADY

#undef EISCONN
#define EISCONN WSAEISCONN

#undef EBADF
#define EBADF WSAEBADF
#endif

static int lstrerror(lua_State *L) {
    int errnum = (int)luaL_checkinteger(L, 1);
    lua_pushstring(L, strerror(errnum));
    return 1;
}

static const struct luaL_Reg errno_module_methods[] = {
    {"strerror", lstrerror},
    {NULL, NULL}
};

LUALIB_API int luaopen_levent_errno_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_newlib(L, errno_module_methods);
    ADD_CONSTANT(L, EINVAL);
    ADD_CONSTANT(L, EWOULDBLOCK);
    ADD_CONSTANT(L, EINPROGRESS);
    ADD_CONSTANT(L, EALREADY);
    ADD_CONSTANT(L, EAGAIN);
    ADD_CONSTANT(L, EISCONN);
    ADD_CONSTANT(L, EBADF);

    return 1;
}

