local levent = require "levent.levent"
local http   = require "levent.http"

function hello(response, request)
    local name = request:get_args()["name"] or "world"
    response:set_data(string.format("hello, %s", name))
    response:set_header("header1", "value1")
end

function main()
    local s = http.server("0.0.0.0", 8080)
    s:register {
        ["/index"] = hello,
    }
    print(s:serve())
end

levent.start(main)
