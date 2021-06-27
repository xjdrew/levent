
local function client_func(host)
    print("levent client_func is in a sub thread")

    local levent = require "levent.levent"
    local socket = require "levent.socket"

    levent.start_in_thread(function()
        levent.sleep(1)

        local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        assert(sock, errcode)
        print("begin connect:", sock:fileno())
        local ok, exception = sock:connect(host, 8859)
        if not ok then
            error("connect failed:" .. exception)
        end

        levent.sleep(2)

        print("connect succeed")

        local count = 1024 * 10
        local max_size = 64 * 1024 - 1
        for _ = 1, count do
            local size = math.random(10, max_size)
            local data = string.rep('a', size)

            local s = string.pack(">s2", data)
            sock:sendall(s)

            local s1 = sock:recv(2)
            local r_size = string.unpack(">I2", s1)
            assert(r_size == size)
            local r_data = sock:recv(r_size)
            assert(r_data == data)
        end
        sock:close()
    end)
end

local function server_func()
    print("levent server_func is in a sub thread")

    local levent = require "levent.levent"
    local socket = require "levent.socket"

    levent.start_in_thread(function()
        local sock, errcode = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        assert(sock, errcode)

        assert(sock:bind("0.0.0.0", 8859))

        sock:listen()
        sock:set_timeout(10)
        print("begin accept:", sock:fileno())
        local csock, err = sock:accept()
        assert(csock, err)

        while true do
            local s1 = csock:recv(2)
            if s1 == "" then
                print("client disconnected")
                break
            end

            local size = string.unpack(">I2", s1)
            local data = csock:recv(size)
            assert(data and #data > 0)

            local s = string.pack(">s2", data)
            csock:sendall(s)
        end

        csock:close()
        sock:close()
    end)
end

local function start()
    local levent = require "levent.levent"
    local effil = require "effil"

    print("levent start is in the main thread, start in 1 second")
    levent.sleep(1)

    local list = {}

    list[#list + 1] = effil.thread(server_func)()

    local host = "127.0.0.1"
    list[#list + 1] = effil.thread(client_func)(host)

    for i = 1, #list do
        list[i]:wait()
    end
end

local levent = require "levent.levent"
levent.start(start)

