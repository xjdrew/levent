local ev = require "levent.ev.c"
print(ev.version())
local loop = ev.default_loop()
print(loop)
print(loop:now())

local watchers = {}
local function _cb(loop, id, revents)
    print(loop, id, revents)
    local w = assert(watchers[id], id)
    w:stop(loop)
    loop:_break(2)
end

local io = ev.new_io()
print(io, io:id())
watchers[io:id()] = io

io:init(0, 1)
io:start(loop)

local timer = ev.new_timer()
print(timer, timer:id())
watchers[timer:id()] = timer

timer:init(1, 1)
timer:start(loop)
loop:run(0, _cb, loop)
