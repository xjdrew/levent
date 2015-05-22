/* lua-socket.c
 * author: xjdrew
 * date: 2014-07-10
 */

/*
This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 address families are supported
- Only SOCK_STREAM, SOCK_DGRAM socket type are supported
- Only IPPROTO_TCP, IPPROTO_UDP protocal type are supported
- Don't support dns lookup, must be numerical network address

Module interface:
- socket.socket(family, type[, proto]) --> new socket object
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.resolve(hostname), hostname can be anything recognized by getaddrinfo
*/
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define close closesocket
#define errno WSAGetLastError()
#else
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "levent.h"

#define SOCKET_METATABLE "socket_metatable"
/*
#if !defined(NI_MAXHOST)
#define NI_MAXHOST 1025
#endif

#if !defined(NI_MAXSERV)
#define NI_MAXSERV 32
#endif
*/

typedef struct _sock_t {
    int fd;
    int family;
    int type;
    int protocol;
#ifdef _WIN32
    // default: libev suppose you input operating-system file handle on windows
    int handle;
#endif
} socket_t;

/* 
 *   internal function 
 */
INLINE static socket_t* 
_getsock(lua_State *L, int index) {
    socket_t* sock = (socket_t*)luaL_checkudata(L, index, SOCKET_METATABLE);
    return sock;
}

INLINE static void
_setsock(lua_State *L, int fd, int family, int type, int protocol) {
#ifdef SO_NOSIGPIPE
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&on, sizeof(on));
#endif

    socket_t *nsock = (socket_t*) lua_newuserdata(L, sizeof(socket_t));
    luaL_getmetatable(L, SOCKET_METATABLE);
    lua_setmetatable(L, -2);

    nsock->fd = fd;
    nsock->family = family;
    nsock->type = type;
    nsock->protocol = protocol;
#ifdef _WIN32
    nsock->handle = _open_osfhandle(fd, 0);
#endif
}

static const char*
_addr2string(struct sockaddr *sa, char *buf, int buflen)
{
    const char *s;
    if (sa->sa_family == AF_INET)
        s = inet_ntop(sa->sa_family, (const void*) &((struct sockaddr_in*)sa)->sin_addr, buf, buflen);
    else
        s = inet_ntop(sa->sa_family, (const void*) &((struct sockaddr_in6*)sa)->sin6_addr, buf, buflen);
    return s;
}

static int
_getsockaddrarg(socket_t *sock, const char *host, const char *port, struct addrinfo **res) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = sock->family;
    hints.ai_socktype = sock->type;
    hints.ai_protocol = sock->protocol;
    hints.ai_flags = AI_NUMERICHOST;

    return getaddrinfo(host, port, &hints, res);
}

static int
_getsockaddrlen(socket_t *sock, socklen_t *len) {
    switch(sock->family) {
        case AF_INET:
            *len = sizeof(struct sockaddr_in);
            return 1;
        case AF_INET6:
            *len = sizeof(struct sockaddr_in6);
            return 1;
    }
    return 0;
}

static int
_makeaddr(lua_State *L, struct sockaddr *addr, int addrlen) {
    char ip[NI_MAXHOST];
    char port[NI_MAXSERV];
    int err = getnameinfo(addr, addrlen, ip, sizeof(ip), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
    if(err != 0) {
        lua_pushnil(L);
        lua_pushinteger(L, err);
        return 2;
    }
    lua_pushstring(L, ip);
    lua_pushstring(L, port);
    lua_tonumber(L, -1);
    return 2;
}

INLINE static int
_push_result(lua_State *L, int err) {
    if (err == 0) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, err);
        return 2;
    }
}

/*
 *    end
 */

/*
 *   args: int domain, int type, int protocal
 */
static int
_socket(lua_State *L) {
    int family    = luaL_checkinteger(L, 1);
    int type      = luaL_checkinteger(L, 2);
    int protocol  = luaL_optinteger(L,3,0);

    int fd = socket(family, type, protocol);
    if(fd < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    _setsock(L, fd, family, type, protocol);
    return 1;
}

// deprecated: may hang up, use dns.resolve for cooperative dns query
static int
_resolve(lua_State *L) {
    const char* host = luaL_checkstring(L, 1);
    struct addrinfo *res = 0;
	int err, i;
	char buf[sizeof(struct in6_addr)];

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    err = getaddrinfo(host, NULL, &hints, &res);
    if(err != 0) {
        lua_pushnil(L);
        lua_pushinteger(L, err);
        return 2;
    }

    i = 1;
    lua_newtable(L);
    while(res) {
        // ignore all unsupported address
        if((res->ai_family == AF_INET || res->ai_family == AF_INET6) && res->ai_socktype == SOCK_STREAM) {
            lua_createtable(L, 0, 2);
            lua_pushinteger(L, res->ai_family);
            lua_setfield(L, -2, "family");
            lua_pushstring(L, _addr2string(res->ai_addr, buf, sizeof(buf)));
            lua_setfield(L, -2, "addr");
            lua_rawseti(L, -2, i++);
        }
        res = res->ai_next;
    }
    return 1;
}

static int
_normalize_ip(lua_State *L) {
    char buf[sizeof(struct in6_addr)];
    char str[INET6_ADDRSTRLEN];
    const char* host = luaL_checkstring(L, 1);
    int ipv6 = lua_toboolean(L, 2);
    int domain = ipv6?AF_INET6:AF_INET;

    if (inet_pton(domain, host, buf) <= 0) {
        lua_pushnil(L);
        return 1;
    }

    if(inet_ntop(domain, buf, str, INET6_ADDRSTRLEN) == NULL) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, str);
    return 1;
}

/* socket object methods */
static int
_sock_setblocking(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int block = lua_toboolean(L, 2);
#ifdef _WIN32
	u_long argp = block?0:1;
	ioctlsocket(sock->fd, FIONBIO, &argp);
#else
	int flag = fcntl(sock->fd, F_GETFL, 0);
	if (flag == -1) {
		flag = 0;
	}
    if (block) {
        flag &= (~O_NONBLOCK);
    } else {
        flag |= O_NONBLOCK;
    }
    fcntl(sock->fd, F_SETFL, flag);
#endif

    return 0;
}

static int
_sock_connect(lua_State *L) {
	int err;
	const char *host, *port;
	struct addrinfo *res = 0;
    socket_t *sock = _getsock(L, 1);
    host = luaL_checkstring(L, 2);
    luaL_checkinteger(L, 3);
    port = lua_tostring(L, 3);

    err = _getsockaddrarg(sock, host, port, &res);
    if(err != 0) {
        lua_pushinteger(L, err);
        return 1;
    }

    err = connect(sock->fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if(err != 0) {
        return _push_result(L, errno);
    } else {
        return _push_result(L, err);
    }
}

static int
_sock_recv(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    size_t len = (lua_Unsigned)luaL_checkinteger(L, 2);
	int nread;

#ifdef _WIN32
	char *buf = (char*)lua_newuserdata(L, len);
#else
	char buf[len];
#endif

    nread = recv(sock->fd, buf, len, 0);
    if(nread < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushlstring(L, buf, nread);
    return 1;
}

static int
_sock_send(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    size_t len;
    const char* buf = luaL_checklstring(L, 2, &len);
    size_t from = luaL_optinteger(L, 3, 0);
    int flags = 0;
	int nwrite;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif

    if (len <= from) {
        return luaL_argerror(L, 3, "should be less than length of argument #2");
    }

    nwrite = send(sock->fd, buf+from, len - from, flags);
    if(nwrite < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushinteger(L, nwrite);
    return 1;
}

static int
_sock_recvfrom(lua_State *L) {
    socklen_t addr_len;
	struct sockaddr *addr;
	int nread;

	socket_t *sock = _getsock(L, 1);
    size_t len = (lua_Unsigned)luaL_checkinteger(L, 2);
#ifdef _WIN32
	char *buf = (char*)lua_newuserdata(L, len);
#else
	char buf[len];
#endif

    if(!_getsockaddrlen(sock, &addr_len)) {
        return luaL_argerror(L, 1, "bad family");
    }

    addr = (struct sockaddr*)lua_newuserdata(L, addr_len);
    nread = recvfrom(sock->fd, buf, len, 0, addr, &addr_len);
    if(nread < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushlstring(L, buf, nread);
    return _makeaddr(L, addr, addr_len) + 1;
}

static int
_sock_sendto(lua_State *L) {
    const char *host, *port, *buf;
    size_t from, len;
    int flags = 0;
    int err, nwrite;
    struct addrinfo *res = 0;

    socket_t *sock = _getsock(L, 1);
    host = luaL_checkstring(L, 2);
    luaL_checkinteger(L, 3);
    port = lua_tostring(L, 3);
    
    buf = luaL_checklstring(L, 4, &len);
    from = luaL_optinteger(L, 5, 0);
    
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif

    if (len <= from) {
        return luaL_argerror(L, 5, "should be less than length of argument #4");
    }

   
    err = _getsockaddrarg(sock, host, port, &res);
    if(err != 0) {
        lua_pushnil(L);
        lua_pushinteger(L, err);
        return 1;
    }

    nwrite = sendto(sock->fd, buf + from, len - from, flags, res->ai_addr, res->ai_addrlen);
    if(nwrite < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushinteger(L, nwrite);
    return 1;
}

static int
_sock_bind(lua_State *L) {
    const char* host, *port;
    int err;
    struct addrinfo *res = 0;

    socket_t *sock = _getsock(L, 1);
    host = luaL_checkstring(L, 2);
    luaL_checkinteger(L, 3);
    port = lua_tostring(L, 3);

    err = _getsockaddrarg(sock, host, port, &res);
    if(err != 0) {
        lua_pushinteger(L, err);
        return 1;
    }

    err = bind(sock->fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (err != 0) {
        return _push_result(L, errno);
    } else {
        return _push_result(L, err);
    }
}

static int
_sock_listen(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int backlog = luaL_optinteger(L, 2, 256);
    int err = listen(sock->fd, backlog);
    if(err != 0) {
        return _push_result(L, errno);
    } else {
        return _push_result(L, err);
    }
}

static int
_sock_accept(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int fd = accept(sock->fd, NULL, NULL);
    if(fd < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    _setsock(L, fd, sock->family, sock->type, sock->protocol);
    return 1;
}

static int
_sock_fileno(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
#ifdef _WIN32
    lua_pushinteger(L, sock->handle);
#else
    lua_pushinteger(L, sock->fd);
#endif
    return 1;
}

static int
_sock_getpeername(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    socklen_t len;
    struct sockaddr *addr;
    int err;

    if(!_getsockaddrlen(sock, &len)) {
        return luaL_argerror(L, 1, "bad family");
    }

    addr = (struct sockaddr*)lua_newuserdata(L, len);
    err = getpeername(sock->fd, addr, &len);
    if(err < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    return _makeaddr(L, addr, len);
}

static int
_sock_getsockname(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    socklen_t len;
    struct sockaddr *addr;
    int err;

    if(!_getsockaddrlen(sock, &len)) {
        return luaL_argerror(L, 1, "bad family(%d)");
    }

    addr = (struct sockaddr*)lua_newuserdata(L, len);
    err = getsockname(sock->fd, addr, &len);
    if(err < 0) {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    return _makeaddr(L, addr, len);
}

static int
_sock_getsockopt(lua_State *L) {
    int err;

    socket_t *sock = _getsock(L, 1);
    int level = luaL_checkinteger(L, 2);
    int optname = luaL_checkinteger(L, 3);
    socklen_t buflen = luaL_optinteger(L, 4, 0);

    if(buflen > 1024) {
        return luaL_argerror(L, 4, "should less than 1024");
    }

    if(buflen == 0) {
        int flag = 0;
        socklen_t flagsize = sizeof(flag);
        err = getsockopt(sock->fd, level, optname, (void*)&flag, &flagsize);
        if(err < 0) {
            goto failed;
        }
        lua_pushinteger(L, flag);
    } else {
        void *optval = lua_newuserdata(L, buflen);
        err = getsockopt(sock->fd, level, optname, optval, &buflen);
        if(err < 0) {
            goto failed;
        }
        lua_pushlstring(L, optval, buflen);
    }
    return 1;
failed:
    lua_pushnil(L);
    lua_pushinteger(L, errno);
    return 2;
}

static int
_sock_setsockopt(lua_State *L) {
    const char* buf;
    size_t buflen;
    int type, err;

    socket_t *sock = _getsock(L, 1);
    int level = luaL_checkinteger(L, 2);
    int optname = luaL_checkinteger(L, 3);
    luaL_checkany(L, 4);

    type = lua_type(L, 4);
    if(type == LUA_TSTRING) {
        buf = luaL_checklstring(L, 4, &buflen);
    } else if(type == LUA_TNUMBER) {
        int flag = luaL_checkinteger(L, 4);
        buf = (const char*)&flag;
        buflen = sizeof(flag);
    } else {
        return luaL_argerror(L, 4, "unsupported type");
    }

    err = setsockopt(sock->fd, level, optname, buf, buflen);
    if(err < 0) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int
_sock_close(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    int fd = sock->fd;
    if(fd != -1) {
        sock->fd = -1;
        close(fd);
    }
    return 0;
}

static int
_sock_tostring(lua_State *L) {
    socket_t *sock = _getsock(L, 1);
    lua_pushfstring(L, "socket: %p", sock);
    return 1;
}

/* end */

// +construct socket metatable
static const struct luaL_Reg socket_mt[] = {
    {"__gc", _sock_close},
    {"__tostring",  _sock_tostring},
    {NULL, NULL}
};

static const struct luaL_Reg socket_methods[] = {
    {"setblocking", _sock_setblocking},
    {"connect", _sock_connect},

    {"recv", _sock_recv},
    {"send", _sock_send},

    {"recvfrom", _sock_recvfrom},
    {"sendto", _sock_sendto},

    {"bind", _sock_bind},
    {"listen", _sock_listen},
    {"accept", _sock_accept},

    {"fileno", _sock_fileno},
    {"getpeername", _sock_getpeername},
    {"getsockname", _sock_getsockname},

    {"getsockopt", _sock_getsockopt},
    {"setsockopt", _sock_setsockopt},

    {"close", _sock_close},
    {NULL, NULL}
};

static const struct luaL_Reg socket_module_methods[] = {
    {"socket", _socket},
    {"resolve", _resolve},
    {"normalize_ip", _normalize_ip},
    {NULL, NULL}
};

#ifdef _WIN32
void os_fini(void) {
    WSACleanup();
}

void os_init() {
    WSADATA WSAData;
    int ret = WSAStartup(0x0101, &WSAData);
    if(ret == 0) {
        atexit(os_fini);
    } else {
        printf("init socket failed:%d\n", ret);
        exit(1);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if(fdwReason == DLL_PROCESS_ATTACH) {
        os_init();
    }
    return TRUE;
}
#endif


LUALIB_API int luaopen_levent_socket_c(lua_State *L) {
    luaL_checkversion(L);

    if(luaL_newmetatable(L, SOCKET_METATABLE)) {
        luaL_setfuncs(L, socket_mt, 0);

        luaL_newlib(L, socket_methods);
        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1);
    // +end

    luaL_newlib(L, socket_module_methods);
    // address family
    ADD_CONSTANT(L, AF_INET);
    ADD_CONSTANT(L, AF_INET6);

    // socket type
    ADD_CONSTANT(L, SOCK_STREAM);
    ADD_CONSTANT(L, SOCK_DGRAM);

    // protocal type
    ADD_CONSTANT(L, IPPROTO_TCP);
    ADD_CONSTANT(L, IPPROTO_UDP);

    // sock opt
    ADD_CONSTANT(L, SOL_SOCKET);

    ADD_CONSTANT(L, SO_REUSEADDR);
    ADD_CONSTANT(L, SO_LINGER);
    ADD_CONSTANT(L, SO_KEEPALIVE);
    ADD_CONSTANT(L, SO_SNDBUF);
    ADD_CONSTANT(L, SO_RCVBUF);
#ifdef SO_REUSEPORT
    ADD_CONSTANT(L, SO_REUSEPORT);
#endif
#ifdef SO_NOSIGPIPE
    ADD_CONSTANT(L, SO_NOSIGPIPE);
#endif 
#ifdef SO_NREAD
    ADD_CONSTANT(L, SO_NREAD);
#endif
#ifdef SO_NWRITE
    ADD_CONSTANT(L, SO_NWRITE);
#endif
#ifdef SO_LINGER_SEC
    ADD_CONSTANT(L, SO_LINGER_SEC);
#endif
    return 1;
}

