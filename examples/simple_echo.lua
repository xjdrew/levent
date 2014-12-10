local levent = require "levent.levent"
local socket = require "levent.socket"

function handle_client(csock)
    local host, port = csock:getpeername()
    print("New connection from:", host, port)
    local msg, err
    while true do
        msg, err = csock:recv(1024)
        if not msg or #msg == 0 then
            break
        end
        local _, err = csock:sendall(msg)
        if err then
            break
        end
    end
    csock:close()
end

function start_server()
    local sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    local ok, errcode = sock:bind("0.0.0.0", 8858)
    assert(ok, errcode)
    sock:listen()
    while true do
        local csock, err = sock:accept()
        if not csock then
            break
        end
        levent.spawn(handle_client, csock)
    end
end

levent.start(start_server)

