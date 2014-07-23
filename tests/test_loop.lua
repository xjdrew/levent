local levent = require "levent.levent"

local io = levent.loop:io(0, 1)
io:start(function(revents)
    io:stop()
    print("in io:", revents)
end)

local timer = levent.loop:timer(1,1)
timer:start(function(v)
    timer:stop()
    print("in timer:", v)
end, 1329)

loop:run()

