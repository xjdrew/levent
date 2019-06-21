local levent = require "levent.levent"
local dns    = require "levent.dns"
local seri   = require "levent.tpseri"

local function resolve(host, qtype)
    print("resolve:", qtype, host)
    local answers, err = dns.resolve(host, qtype, 1)
    if answers then
        print(seri(answers))
    else
        print(err)
    end
end

local function run()
    resolve("www.google.com", dns.QTYPE.AAAA)
    resolve("www.google.com")
    resolve("114.114.114.114")
    resolve("0:0:0:0:0:FFFF:204.152.189.116", dns.QTYPE.AAAA)
    resolve("a.114.114.114")
end

levent.start(run)
