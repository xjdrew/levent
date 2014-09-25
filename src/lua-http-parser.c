/* lua-http.c
 * author: xjdrew
 * date: 2014-09-19
 */

#include "http_parser.h"

#include "levent.h"

#define HTTP_PARSER_METATABLE "http_parser_metatable"

// parser cb
static int
on_message_begin(http_parser *parser) {
    return 0;
}

static int
on_url(http_parser *parser, const char *at, size_t length) {
    return 0;
}

static int
on_status(http_parser *parser, const char *at, size_t length) {
    return 0;
}

static int
on_header_field(http_parser *parser, const char *at, size_t length) {
    return 0;
}

static int
on_header_value(http_parser *parser, const char *at, size_t length) {
    return 0;
}

static int
on_headers_complete(http_parser *parser) {
    return 0;
}

static int
on_body(http_parser *parser, const char *at, size_t length) {
    return 0;
}

static int
on_message_complete(http_parser *parser, const char *at, size_t length) {
    return 0;
}

static const struct http_parser_settings settings = {
    on_message_begin,
    on_url,
    on_status,
    on_header_field,
    on_header_value,
    on_headers_complete,
    on_body,
    on_message_complete
};

static int
lversion(lua_State *L) {
    unsigned long version = http_parser_version();
    unsigned major = (version >> 16) & 255;
    unsigned minor = (version >> 8) & 255;
    unsigned patch = version & 255;
    lua_pushfstring(L, "http_parser v%d.%d.%d", major, minor, patch);
    return 1;
}

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

static int
lhttp_errno_name(lua_State *L) {
    int err = luaL_checkint(L, 1);
    lua_pushstring(L, http_errno_name(err));
    return 1;
}

static int
lhttp_errno_description(lua_State *L) {
    int err = luaL_checkint(L, 1);
    lua_pushstring(L, http_errno_description(err));
    return 1;
}

static int lnew(lua_State *L) {
    enum http_parser_type type = luaL_optint(L, 1, HTTP_BOTH);
    http_parser *parser = (http_parser*) lua_newuserdata(L, sizeof(http_parser));
    luaL_getmetatable(L, HTTP_PARSER_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static const struct luaL_Reg http_module_methods[] = {
    {"version", lversion},
    {"parse_url", lparse_url},
    {"http_errno_name", lhttp_errno_name},
    {"http_errno_description", lhttp_errno_description},
    {"new", lnew},
    {NULL, NULL}
};

static int lparser_execute(lua_State *L) {
    return 0;
}

static int lparser_should_keep_alive(lua_State *L) {
    return 0;
}

static int lparser_pause(lua_State *L) {
    return 0;
}

static int lparser_body_is_final(lua_State *L) {
    return 0;
}

static const struct luaL_Reg http_parser_metamethods[] = {
    {"execute", lparser_execute},
    {"should_keep_alive", lparser_should_keep_alive},
    {"pause", lparser_pause},
    {"body_is_final", lparser_body_is_final},
    {NULL, NULL}
};

LUALIB_API int luaopen_levent_http_c(lua_State *L) {
    luaL_checkversion(L);

    if(luaL_newmetatable(L, HTTP_PARSER_METATABLE)) {
        luaL_newlib(L, http_module_methods);
        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1);

    luaL_newlib(L, http_module_methods);
    ADD_CONSTANT(L, HTTP_REQUEST);
    ADD_CONSTANT(L, HTTP_RESPONSE);
    ADD_CONSTANT(L, HTTP_BOTH);
    return 1;
}

