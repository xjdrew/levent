local socket = require "levent.socket"
local levent = require "levent.levent"
local seri = require "levent.tpseri"
local _select = require "levent.select"

local mt = {}
mt.__index = mt

BUFLEN = 4096
TARGET_IP = "127.0.0.1"
TARGET_PORT = 80

function handle(client_sock)
    client_sock:setblocking(false)
    local target_sock, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    target_sock:setblocking(false)
    target_sock:connect(TARGET_IP, TARGET_PORT)
    while true do
        rlist, _ = _select.select_({target_sock, client_sock}) -- monitor read event only
        local out, data
        for _, sock in ipairs(rlist) do
            if sock == client_sock then
                out = target_sock
                data = client_sock:recv(BUFLEN)
            elseif sock == target_sock then
                out = client_sock
                data = client_sock:recv(BUFLEN)
            end
            if data and #data > 0 then
                out:sendall(data)
            else
                target_sock:close()
                client_sock:close()
                return
            end
        end
    end
end

local sock, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
assert(sock, err)
print("bind:", sock:bind("0.0.0.0", 8080))
print("listen:", sock:listen())
function start_server()
    while true do
        local csock, err = sock:accept()
        if not csock then
            print("accept failed:", err)
            break
        end
        print("new conn from:", csock:getpeername())
        levent.spawn(handle, csock)
    end
end

levent.start(start_server)
