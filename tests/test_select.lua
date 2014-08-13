local levent  = require "levent.levent"
local socket  = require "levent.socket"
local sel     = require "levent.select"

local test_word = "hello world"

local function client()
    local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    assert(sock, errcode)
    sock:setblocking(false)
    print("begin connect")
    sock:connect("127.0.0.1", 8859)
    sel.select_(nil, {sock})
    sock:sendall(test_word)
    sock:close()
end

local function start()
    local sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock:setblocking(false)
    if not sock:bind("0.0.0.0", 8859) then
        print("bind failed")
        return
    end

    sock:listen()
    levent.spawn(client)
    sel.select_({sock})
    local csock = sock:accept()
    print("accept:", csock)
    sel.select_({csock})
    local data = csock:recv(1024)
    assert(#data >= 0)
    print("recv:", data)
    if #data > 0 then
        assert(data == test_word, data)
    end
end

levent.start(start)

