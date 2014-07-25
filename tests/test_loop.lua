local levent = require "levent.levent"

local loop = levent.loop

local io = loop:io(0, loop.EV_READ)
io:start(function(revents)
    io:stop()
    print("in io:", revents)
end)

local timer = loop:timer(1,loop.EV_TIMER)
timer:start(function(revents)
    timer:stop()
    print("in timer:", revents)
end)

loop:run()

