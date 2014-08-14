local levent = require "levent.levent"
local signal = require "levent.signal"

local function run()
    local s
    s = signal.signal(signal.SIGINT, function()
        print("in SIGINT")
        s:cancel()
    end)
end

levent.start(run)

