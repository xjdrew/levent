local levent = require "levent.levent"
local socket = require "levent.socket"

function handle_client(csock)
    local host, port = csock:getpeername()
    print("New connection from:", host, port)
    local msg, err
    while true do
        msg, err = csock:recv(1024)
        if not msg or #msg == 0 then
            print("recv failed:", #msg, err)
            break
        end
        local _, err = csock:sendall(msg)
        if err then
            print("send failed3:", err)
            break
        end
    end
    csock:close()
    print("Connection close:", host, port)
end

function start_server(ip, port)
    local sock, err = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if not sock then
        print("create failed:", err)
        return
    end

    local ok, err = sock:bind(ip, port)
    if not ok then
        print("bind failed:", err)
        return
    end

    local ok, err = sock:listen()
    if not ok then
        print("accept failed:", err)
        return
    end

    print("listen:", ip, port)
    while true do
        local csock, err = sock:accept()
        if not csock then
            print("accept failed:", err)
            break
        end
        levent.spawn(handle_client, csock)
    end
end

-- pre-launch 10000 coroutines 
-- levent.check_coroutine(10000)
levent.start(start_server, "0.0.0.0", 8858)

