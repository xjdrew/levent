local Class = require "levent.class"
local ev = require "event.c"

local Watcher = Class("Watcher")
function Watcher:_init(loop, ...)
    self._loop = loop
    local w = self._new_watcher()
    w:init(...)
    self._watcher = w
end

function Watcher:id()
end

function Watcher:start(callback)
end

function Watcher:stop()
end

function Watcher:run_callback(revents)
end

function Watcher:is_active()
end

function Watcher:is_pending()
end

local Io = Class("Io", Watcher)
Io._new_watcher = ev.new_io

local Timer = class("Timer", Timer)
Io._new_watcher = ev.new_timer

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

function Loop:_add_watchers(w)
    local mt = getmetatable(w)
    mt.__gc == function(self)
        self:stop()
    end
    self.watchers[w:id()] = w
end

function Loop:io(fd, events)
    local o = Io:new(self, fd, events)
    self:_add_watchers(o)
    return o
end

function Loop:timer(after, rep)
    local o = Timer:new(self, after, rep)
    self:_add_watchers(o)
    return o
end

function Loop:callback(id, revents)
    local w = assert(self.watchers[id], id)
    w:run_callback(revents)
end

local loop = Loop:new()
return loop

