local levent     = require "levent.levent"
local lock       = require "levent.lock"

local function start()
    local s = lock.semaphore(0)
    assert(not pcall(s.acquire, s, 0.01))
    s:release()
    assert(pcall(s.acquire, s, 0.01))
end

levent.start(start)

