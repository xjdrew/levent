local socket = require "levent.socket.c"
local sock, err = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
assert(sock, err)
print("connect:", sock:connect("::1", 8858))
print("peer name:", sock:getpeername())
print("sock name:", sock:getsockname())
print("send:", sock:send("hello, world!\n"))
print("recv:", sock:recv(100))

