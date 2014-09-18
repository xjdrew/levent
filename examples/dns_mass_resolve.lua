local levent = require "levent.levent"
local dns    = require "levent.dns"
local seri   = require "levent.tpseri"
local queue  = require "levent.queue"

local N = 1000
local finished = 0
local workers = 100

dns.server = "8.8.8.8"

function job(q)
    while true do
        local hostname = q:get()
        local ips, err = dns.resolve(hostname, false)
        finished = finished + 1
        if ips then
            print(string.format("%s = %s", hostname, table.concat(ips, ",")))
        else
            print(string.format("%s failed with %s", hostname, err))
        end
    end
end

function start()
    local q = queue.queue()
    for i=1, workers do
        levent.spawn(job, q)
    end

    for i=10,N+10 do
        q:put(string.format("%s.com", i))
    end

    q:join(2)
    levent.exit()
    print(string.format("finished within 2 seconds: %d/%d", finished, N))
end

levent.start(start)

