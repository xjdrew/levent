local levent = require "levent.levent"

local io = levent.loop:io(0, 1)
io:start(function(v)
    io:stop()
    print("in io:", v)
end, 1328)

local timer = levent.loop:timer(1,1)
timer:start(function(v)
    timer:stop()
    print("in timer:", v)
end, 1329)

loop:run()

