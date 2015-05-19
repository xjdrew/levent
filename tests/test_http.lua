local levent = require "levent.levent"
local lock   = require "levent.lock"
local http   = require "levent.http"

local port = 8080
local httpd

function start_server(e)
    httpd = http.server("0.0.0.0", port)
    httpd:register {
        ["/hello"] = function(rsp, req)
            local name = req:get_args()["name"] or "world"
            rsp:set_data(name)
        end,

        ["/add"] = function(rsp, req)
            local args = req:get_args()
            local a = args["a"] and tonumber(args["a"]) or 0
            local b = args["b"] and tonumber(args["b"]) or 0
            rsp:set_data({
                c = a + b
            })
        end,
    }
    e:set()
    print(httpd:serve())
end

function start_client(n)
    cli = http.client("127.0.0.1", port)
    for i=1,n do
        local data = "中国" .. i
        local req = http.request("POST", "http://127.0.0.1/hello", {name=data})
        local ok, err = cli:send(req)
        assert(ok, err)
        local rsp = cli:get_response()
        print(rsp:get_data())
        assert(rsp:get_data() == data)
    end

    for i=1, n do
        local a = i
        local b = i+1
        local req = http.request("POST", "http://127.0.0.1/add", {a=a, b=b})
        local ok, err = cli:send(req)
        assert(ok, err)
        local rsp = cli:get_response()
        local c = tonumber(rsp:get_args()["c"])
        assert(c == a + b)
        print(string.format("%d + %d = %d", a, b, c))
    end
    cli:close()
end

function main()
    local e = lock.event()
    levent.spawn(start_server, e)
    e:wait()
    start_client(5)
    httpd:close()
end

levent.start(main)

