local socket = require "socket.c"
for k,v in pairs(socket) do
	print(k,v)
end

local sock, errmsg = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print(sock, errmsg)

