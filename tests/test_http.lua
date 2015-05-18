local levent = require "levent.levent"
local lock   = require "levent.lock"
local http   = require "levent.http"

local urls = {
    "http://www.google.com",
    "http://yahoo.com",
}

local port = 8080
local httpd

function start_server(e)
    httpd = http.server("0.0.0.0", port)
    httpd:register {
        ["/hello"] = function(rsp, req)
            local name = req:get_args()["name"] or "world"
            rsp:set_data(name)
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
        assert(rsp:get_data() == data)
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

