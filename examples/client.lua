local levent = require "levent.levent"
local socket = require "levent.socket"

local function test()
    local sock, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    assert(sock, err)
    print("connect:", sock:connect("127.0.0.1", 8858))
    print("peer name:", sock:getpeername())
    print("sock name:", sock:getsockname())

    local data = "hello, world!\n"
    print("send:", sock:send(data))
    print("recv1:", #sock:recv(5000))
    print("recv2:", #sock:recv(100))
end
levent.start(test)

