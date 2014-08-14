local socket = require "levent.socket"
local levent = require "levent.levent"
local seri = require "levent.tpseri"
local _select = require "levent.select"

BUFLEN = 4096
TARGET_IP = "127.0.0.1"
TARGET_PORT = 80

flush_socket = function(client, target)
    while true do
        local req, err = client:recv(BUFLEN)
        if req and #req > 0 then
            target:sendall(req)
            --print("send to server")
        else
            client:close()
            target:close()
            --print("break keep request")
            break
        end
    end
end

function handle(client_sock)
    local target_sock, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    local ok, err = target_sock:connect(TARGET_IP, TARGET_PORT)
    if not ok then
        print("connect err:", err)
        client_sock:close()
        return
    end
    levent.spawn(flush_socket, client_sock, target_sock)
    flush_socket(target_sock, client_sock)
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
        --print("new conn from:", csock:getpeername())
        levent.spawn(handle, csock)
    end
end

levent.start(start_server)
