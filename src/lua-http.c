/* lua-http.c
 * author: xjdrew
 * date: 2014-09-19
 */

#include "http_parser.h"

#include "levent.h"

INLINE static void
set_url_field(lua_State *L, const char* field, enum http_parser_url_fields uf, 
        struct http_parser_url *u, const char* buf, int index) {
    if(u->field_set & (1 << uf)) {
        lua_pushlstring(L, buf + u->field_data[uf].off, u->field_data[uf].len);
        lua_setfield(L, index, field);
    }
}

static int
lparse_url(lua_State *L) {
    size_t len;
    const char *buf;
    int port;

    buf = luaL_checklstring(L, 1, &len);
    struct http_parser_url u;
    if(http_parser_parse_url(buf, len, 0, &u) != 0) {
        return 0;
    }
    lua_createtable(L, 0, UF_MAX);
    set_url_field(L, "schema", UF_SCHEMA, &u, buf, -2);
    set_url_field(L, "host", UF_HOST, &u, buf, -2);
    set_url_field(L, "path", UF_PATH, &u, buf, -2);
    set_url_field(L, "query", UF_QUERY, &u, buf, -2);
    set_url_field(L, "fragment", UF_FRAGMENT, &u, buf, -2);
    set_url_field(L, "userinfo", UF_USERINFO, &u, buf, -2);

    if(u.port != 0) {
        lua_pushunsigned(L, u.port);
        lua_setfield(L, -2, "port");
    }
    return 1;
}

static const struct luaL_Reg http_module_methods[] = {
    {"parse_url", lparse_url},
    {NULL, NULL}
};

LUALIB_API int luaopen_levent_http_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_newlib(L, http_module_methods);
    return 1;
}

