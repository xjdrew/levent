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
        _, err = csock:send_all(msg)
        if not err then
            break
        end
    end
    print("Connection close:", csock:getpeername())
end

function start_server()
    local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if not sock then
        print("create socket failed:", errcode)
        return
    end

    local ok, errcode = sock:bind("0.0.0.0", 8858)
    if not ok then
        print("bind socket address failed:", errcode)
        return
    end

    sock:listen()
    while true do
        local csock, err = sock:accept()
        if not csock then
            print("accept failed:", err)
            break
        end
        levent.spawn(handle_client, csock)
    end
end

levent.start(start_server)

