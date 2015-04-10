/* lua-http.c
 * author: xjdrew
 * date: 2014-09-19
 */

#include "http_parser.h"

#include "levent.h"

#define HTTP_PARSER_METATABLE "http_parser_metatable"

#define FIELD_TYPE "is_request"
#define FIELD_METHOD "method"
#define FIELD_REQUEST_URL "request_url"
#define FIELD_STATUS "status"
#define FIELD_STATUS_CODE "status_code"
#define FIELD_HEADERS "headers"
#define FIELD_BODY "body"
#define FIELD_KEEPALIVE "keepalive"
#define FIELD_UPGRADE "upgrade"
#define FIELD_HTTP_MAJOR "http_major"
#define FIELD_HTTP_MINOR "http_minor"
#define FIELD_COMPLETE "complete"

// parser cb
static int
on_message_begin(http_parser *parser) {
    lua_State *L = (lua_State*)parser->data;
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, FIELD_COMPLETE);
    return 0;
}

static int
on_url(http_parser *parser, const char *at, size_t length) {
    lua_State *L = (lua_State*)parser->data;
    lua_getfield(L, -1, FIELD_REQUEST_URL);
    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushlstring(L, at, length);
    } else {
        lua_pushlstring(L, at, length);
        lua_concat(L, 2);
    }
    lua_setfield(L, -2, FIELD_REQUEST_URL);

    // only request callback on_url
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, FIELD_TYPE);
    return 0;
}

static int
on_status(http_parser *parser, const char *at, size_t length) {
    lua_State *L = (lua_State*)parser->data;
    lua_getfield(L, -1, FIELD_STATUS);
    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushlstring(L, at, length);
    } else {
        lua_pushlstring(L, at, length);
        lua_concat(L, 2);
    }
    lua_setfield(L, -2, FIELD_STATUS);
    return 0;
}

static int
on_header_field(http_parser *parser, const char *at, size_t length) {
    lua_State *L = (lua_State*)parser->data;
    int len = luaL_len(L, -1);
    if(len % 2 == 0) {
        lua_pushlstring(L, at, length);
        lua_rawseti(L, -2, len+1);
    } else {
        lua_rawgeti(L, -1, len);
        lua_pushlstring(L, at, length);
        lua_concat(L, 2);
        lua_rawseti(L, -2, len);
    }
    return 0;
}

static int
on_header_value(http_parser *parser, const char *at, size_t length) {
    lua_State *L = (lua_State*)parser->data;
    int len = luaL_len(L, -1);
    if(len % 2 != 0) {
        lua_pushlstring(L, at, length);
        lua_rawseti(L, -2, len+1);
    } else {
        lua_rawgeti(L, -1, len);
        lua_pushlstring(L, at, length);
        lua_concat(L, 2);
        lua_rawseti(L, -2, len);
    }
    return 0;
}

static int
on_headers_complete(http_parser *parser) {
    int is_request;
    lua_State *L = (lua_State*)parser->data;
    // http version
    lua_pushinteger(L, parser->http_major);
    lua_setfield(L, -2, FIELD_HTTP_MAJOR);

    lua_pushinteger(L, parser->http_minor);
    lua_setfield(L, -2, FIELD_HTTP_MINOR);

    // optional field
    lua_getfield(L, -1, FIELD_TYPE);
    is_request = lua_toboolean(L, -1);
    lua_pop(L, 1);
    if(is_request) {
        lua_pushstring(L, http_method_str(parser->method));
        lua_setfield(L, -2, FIELD_METHOD);
    } else {
        // response
        lua_pushinteger(L, parser->status_code);
        lua_setfield(L, -2, FIELD_STATUS_CODE);
    }
    return 0;
}

static int
on_body(http_parser *parser, const char *at, size_t length) {
    lua_State *L = (lua_State*)parser->data;
    lua_getfield(L, -1, FIELD_BODY);
    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushlstring(L, at, length);
    } else {
        lua_pushlstring(L, at, length);
        lua_concat(L, 2);
    }
    lua_setfield(L, -2, FIELD_BODY);
    return 0;
}

static int
on_message_complete(http_parser *parser) {
    lua_State *L;
    int i, tidx, len;

    L = (lua_State*)parser->data;
    tidx = lua_absindex(L, -1);

    // compose headers
    len = luaL_len(L, tidx);
    if(len % 2 != 0) {
        return 0;
    }

    L = (lua_State*)parser->data;
    lua_getfield(L, tidx, FIELD_HEADERS);
    if(!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, tidx, FIELD_HEADERS);
    }

    for(i=1; i<len; i=i+2) {
        lua_rawgeti(L, tidx, i); // header field
        lua_rawgeti(L, tidx, i+1); // header value
        lua_settable(L, -3);
        lua_pushnil(L);
        lua_rawseti(L, tidx, i);
        lua_pushnil(L);
        lua_rawseti(L, tidx, i + 1);
    }
    lua_pop(L, 1); // pop headers table

    // upgrade
    if(parser->upgrade) {
        lua_pushboolean(L, parser->upgrade);
        lua_setfield(L, tidx, FIELD_UPGRADE);
    }

    // keep alive
    lua_pushboolean(L, http_should_keep_alive(parser));
    lua_setfield(L, tidx, FIELD_KEEPALIVE);

    lua_pushboolean(L, 1);
    lua_setfield(L, tidx, FIELD_COMPLETE);

    /* parse one message once, so pause here */
    http_parser_pause(parser, 1);
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
    int is_connect, port;
    struct http_parser_url u;

    buf = luaL_checklstring(L, 1, &len);
    if (lua_gettop(L) > 1) {
        is_connect = lua_toboolean(L, 2);
    } else {
        is_connect = 0;
    }

    if(http_parser_parse_url(buf, len, is_connect, &u) != 0) {
        return 0;
    }
    lua_createtable(L, 0, UF_MAX);
    set_url_field(L, "schema", UF_SCHEMA, &u, buf, -2);
    set_url_field(L, "host", UF_HOST, &u, buf, -2);
    set_url_field(L, "request_path", UF_PATH, &u, buf, -2);
    set_url_field(L, "query_string", UF_QUERY, &u, buf, -2);
    set_url_field(L, "fragment", UF_FRAGMENT, &u, buf, -2);
    set_url_field(L, "userinfo", UF_USERINFO, &u, buf, -2);

    if(u.port != 0) {
        lua_pushinteger(L, u.port);
        lua_setfield(L, -2, "port");
    }
    return 1;
}

static int
lhttp_errno_name(lua_State *L) {
    int err = luaL_checkinteger(L, 1);
    lua_pushstring(L, http_errno_name(err));
    return 1;
}

static int
lhttp_errno_description(lua_State *L) {
    int err = luaL_checkinteger(L, 1);
    lua_pushstring(L, http_errno_description(err));
    return 1;
}

INLINE static http_parser*
get_http_parser(lua_State *L, int index) {
    http_parser *parser = (http_parser*) luaL_checkudata(L, index, HTTP_PARSER_METATABLE);
    return parser;
}

static int lnew(lua_State *L) {
    enum http_parser_type type = luaL_optinteger(L, 1, HTTP_BOTH);
    http_parser *parser = (http_parser*) lua_newuserdata(L, sizeof(http_parser));
    http_parser_init(parser, type);
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
    size_t len;
    http_parser *parser = get_http_parser(L, 1);
    const char *data = luaL_checklstring(L, 2, &len);
    size_t from = (size_t)luaL_optinteger(L, 3, 0);
    luaL_checktype(L, 4, LUA_TTABLE);

    if(len < from) {
        return luaL_argerror(L, 3, "should be less than length of argument #2");
    }

    parser->data = (void*)L;
    lua_pushinteger(L, http_parser_execute(parser, &settings, data + from, len - from));
    if(HPE_PAUSED == parser->http_errno) {
        http_parser_pause(parser, 0);
    }
    if(HPE_OK == parser->http_errno) {
        return 1;
    }
    lua_pushinteger(L, parser->http_errno);
    return 2;
}

static const struct luaL_Reg http_parser_metamethods[] = {
    {"execute", lparser_execute},
    {NULL, NULL}
};

LUALIB_API int luaopen_levent_http_c(lua_State *L) {
    luaL_checkversion(L);

    if(luaL_newmetatable(L, HTTP_PARSER_METATABLE)) {
        luaL_newlib(L, http_parser_metamethods);
        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1);

    luaL_newlib(L, http_module_methods);
    ADD_CONSTANT(L, HTTP_REQUEST);
    ADD_CONSTANT(L, HTTP_RESPONSE);
    ADD_CONSTANT(L, HTTP_BOTH);
    return 1;
}

