local levent = require "levent.levent"
local socket = require "levent.socket"

local function test()
    local sock, err = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 17)
    assert(sock, err)
    print("connect:", sock:connect("127.0.0.1", 8859))
    print("peer name:", sock:getpeername())
    print("sock name:", sock:getsockname())
    print("send:", sock:send("hello, world!\n"))
    print("recv1:", #sock:recv(2000))
    print("recv2:", #sock:recv(100))
    print("----")
end
levent.start(test)
