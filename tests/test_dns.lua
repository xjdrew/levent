local levent = require "levent.levent"
local dns    = require "levent.dns"
local seri   = require "levent.tpseri"

local function resolve(host, ipv6)
    print("resolve:", ipv6 and "ipv6" or "ipv4", host)
    local answers, err = dns.resolve(host, ipv6, 1)
    if answers then
        print(seri(answers))
    else
        print(err)
    end
end

local function run()
    resolve("www.google.com", true)
    resolve("www.google.com")
    resolve("114.114.114.114")
    resolve("0:0:0:0:0:FFFF:204.152.189.116", true)
    resolve("a.114.114.114")
end

levent.start(run)
