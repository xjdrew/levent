local Class = require "levent.class"
local ev = require "event.c"

local Watcher = Class("Watcher")
function Watcher:_init(loop)
    self.loop = loop
end

function Watcher:id()
end

function Watcher:start(callback, ...)
end

function Watcher:stop()
end

function Watcher:run_callback(revents)
end

function Watcher:is_active()
end

function Watcher:is_pending()
end

function Watcher:__gc()
    self:stop()
end

local Io = Class("Io", Watcher)
local Timer = class("Timer", Timer)

local Loop = Class("Loop")
function Loop:_init()
    self.impl = ev.default_loop()
    self.watchers = setmetatable({}, {__mode="v"})
end

function Loop:run(nowait, once)
    self.impl:run(0, self.callback, self)
end

function Loop:verify()
end

function Loop:now()
    return self.impl:now()
end

function Loop:io(fd, events)
    local o = Io:new(self, fd, events)
    self.watchers[o:id()] = o
    return o
end

function Loop:timer(after, rep)
    local o = Timer:new(self, after, rep)
    self.watchers[o:id()] = o
    return o
end

function Loop:callback(id, revents)
    local w = assert(self.watchers[id], id)
end

local loop = Loop:new()
return loop

