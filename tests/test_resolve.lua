local seri   = require "levent.tpseri"
local socket = require "levent.socket.c"

local function resolve(host)
    print("resolve:", host)
    local addrs, err = socket.resolve(host)
    if addrs then
        print(seri(addrs))
    else
        print(err)
    end
end

resolve("ipv6.google.com")
resolve("www.google.com")

