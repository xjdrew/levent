local levent = require "levent.levent"
local dns    = require "levent.dns"
local seri   = require "levent.tpseri"

local function test()
    local answers, err = dns.resolve("www.google.com", 1)
    if answers then
        print(seri(answers))
    else
        print(err)
    end
end

local function run()
    test()
    test()
end

levent.start(run)
