local levent = require "levent.levent"
local socket = require "levent.socket"
local timeout = require "levent.timeout"

local test_data = "hello"
function client()
    local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    assert(sock, errcode)
    print("begin connect:", sock:fileno())
    assert(sock:connect("127.0.0.1", 8859))
    print("connect succeed")
    sock:sendall(test_data)
    sock:close()
end

function start()
    local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if not sock then
        print("create socket failed:", errcode)
        return
    end

    local ok, errcode = sock:bind("0.0.0.0", 8859)
    if not ok then
        print("bind socket address failed:", errcode)
        return
    end

    sock:listen()
    sock:set_timeout(10)
    levent.spawn(client)
    print("begin accept:", sock:fileno())
    local csock, err = sock:accept()
    assert(csock, err)
    local data = csock:recv(1024)
    print("recv:", data)
    assert(data == test_data, data)
    csock:close()
end

levent.start(start)

