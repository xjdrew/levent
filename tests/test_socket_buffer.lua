local levent = require "levent.levent"
local socket = require "levent.socket"
local timeout = require "levent.timeout"

local test_data = "hello"
function client(host)
    local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    assert(sock, errcode)
    print("begin connect:", sock:fileno())
    local ok, exception = sock:connect(host, 8859)
    if not ok then
        error("connect failed:" .. exception)
    end
    print("connect succeed")
    sock:sendall(test_data)
    levent.sleep(1)
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
    levent.spawn(client, "127.0.0.1")
    print("begin accept:", sock:fileno())
    local csock, err = sock:accept()
    assert(csock, err)

    local data = csock:recv(#test_data * 2)
    print("recv:", data)
    assert(data == test_data .. test_data, data)
    csock:close()
end

levent.start(start)

