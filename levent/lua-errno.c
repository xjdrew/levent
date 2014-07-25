/* lua-ev.c
 * author: xjdrew
 * date: 2014-07-24
 */
#include <errno.h>

#include "levent.h"

int luaopen_errno_c(lua_State *L) {
    luaL_checkversion(L);
    lua_newtable(L);

    ADD_CONSTANT(L, EINVAL);
    ADD_CONSTANT(L, EWOULDBLOCK);
    ADD_CONSTANT(L, EINPROGRESS);
    ADD_CONSTANT(L, EALREADY);
    ADD_CONSTANT(L, EAGAIN);
    ADD_CONSTANT(L, EISCONN);
    ADD_CONSTANT(L, EBADF);
    return 1;
}

