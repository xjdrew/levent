local socket = require "socket.c"

local host = "www.example.com"
local port = 80

local addrs, err = socket.resolve(host)
if not addrs or #addrs == 0 then
    print("resolve host falied:", host, err)
else
    local addr = addrs[1]
    local sock = socket.socket(addr.family, socket.SOCK_STREAM)
    print("connect:", sock:connect(addr.addr, port))
    sock:send(string.format("GET / HTTP/1.1\r\nHOST:%s\r\n\r\n", host))
    print("recv:", sock:recv(1024))
end

