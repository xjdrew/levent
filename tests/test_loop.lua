local loop = require "levent.loop"
local loop = loop.new()

local _cb
function _cb(name, ...)
    print("in cb:", name, ...)
    if name == "timer" then
        loop:run_callback(_cb, "timer's child")
    end
    if name == "main" then
        loop:run_callback(_cb, "main's child")
    end
end

loop:run_callback(_cb, "main")

local io = loop:io(0, loop.EV_READ)
print(io)
print("is_active:", io:is_active())
print("is_pending:", io:is_pending())

io:start(function(revents)
    io:stop()
    print("is_active:", io:is_active())
    print("is_pending:", io:is_pending())
    print("in io:", revents)
    loop:run_callback(_cb, "io")
end)

print("is_active:", io:is_active())
print("is_pending:", io:is_pending())

local timer = loop:timer(1)
timer:start(function(revents)
    timer:stop()
    print("in timer:", revents)
    loop:run_callback(_cb, "timer")
end)

local signal = loop:signal(3)
signal:start(function(revents)
    signal:stop()
    print("in signal:", revents)
end)

loop:run()

