local levent = require "levent.levent"
local dns    = require "levent.dns"
local seri   = require "levent.tpseri"

local N = 1000
local finished = 0
function job(hostname, timeout)
    local ips, err = dns.resolve(hostname, timeout)
    finished = finished + 1
    if ips then
        print(string.format("%s = %s", hostname, table.concat(ips, ",")))
    else
        print(string.format("%s failed with %s", hostname, err))
    end
end

function start()
    local now = levent.now()
    for i=10,N do
        levent.spawn(job, string.format("%s.com", i))
    end
    levent.sleep(2)
    levent.exit()
    print(string.format("finished within 2 seconds: %d/%d", finished, N))
end

levent.start(start)

