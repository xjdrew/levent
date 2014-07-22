local socket = require "socket.c"
local sock, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
assert(sock, err)
print("connect:", sock:connect("127.0.0.1", 8858))
print("peer name:", sock:getpeername())
print("sock name:", sock:getsockname())

local data = "hello, world!\n"
print("send:", sock:send(data, 100))
print("recv:", sock:recv(100))

