local levent = require "levent.levent"
local socket = require "levent.socket"

function _send_all(csock, data)
    local c = 0
    while c < #data do
        local nsend, err = csock:send(data, c)
        if not nsend then
            c = c + nsend
        else
            return false, err
        end
    end
    return true
end

function handle_client(csock)
    print("New connection from:", csock:getpeername())
    local msg, err
    while true do
        msg, err = csock:recv(1024)
        if not msg or #msg == 0 then
            break
        end
        _, err = _send_all(csock, msg)
        if not err then
            break
        end
    end
    print("Connection close:", csock:getpeername())
end

local sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
assert(sock:bind("0.0.0.0", 8858) == 0, "bind socket address failed")
sock:listen()

while true do
    local csock, err = sock:accept()
    if not csock then
        print("accept failed:", err)
        break
    end
    levent.spawn(handle_client, csock)
end

