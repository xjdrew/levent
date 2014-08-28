local socket = require "levent.socket.c"
for k,v in pairs(socket) do
	print(k,v)
end

local sock, errmsg = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print(sock, errmsg)
print("connect:", sock:connect("127.0.0.1", 8858))
print("send:", sock:send("hello, world!\n"))
print("recv:", sock:recv(100))

