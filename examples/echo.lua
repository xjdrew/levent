local levent = require "levent.levent"
local socket = require "levent.socket"

function handle_client(csock)
    local host, port = csock:getpeername()
    print("New connection from:", host, port)
    local msg, err
    while true do
        msg, err = csock:recv(1024)
        if not msg or #msg == 0 then
            if msg then
                print("recv failed1:", #msg, err)
            else
                print("recv failed2:", nil, err)
            end
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

