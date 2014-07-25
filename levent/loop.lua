local class = require "levent.class"
local ev = require "event.c"

local Watcher = class("Watcher")
function Watcher:_init(loop, ...)
    self._loop = loop
    local w = self._new_watcher()
    w:init(...)
    self.cobj = w

    self._cb = nil
end

function Watcher:id()
    return self.cobj:id()
end

function Watcher:start(callback)
    assert(self._cb == nil, self.cobj)
    self._cb = callback
    self.cobj:start(self._loop.cobj)
end

function Watcher:stop()
    self._cb = nil
    self.cobj:stop(self._loop.cobj)
end

function Watcher:run_callback(revents)
    local ok, msg = xpcall(self._cb, debug.traceback, revents)
    if not ok then
        self._loop:handle_error(self, msg)
    end
end

function Watcher:is_active()
end

function Watcher:is_pending()
end

local Io = class("Io", Watcher)
Io._new_watcher = ev.new_io

local Timer = class("Timer", Watcher)
Timer._new_watcher = ev.new_timer

local Loop = class("Loop")
function Loop:_init()
    self.cobj = ev.default_loop()
    self.watchers = setmetatable({}, {__mode="v"})
end

function Loop:run(nowait, once)
    self.cobj:run(0, self.callback, self)
end

function Loop:verify()
end

function Loop:now()
    return self.cobj:now()
end

function Loop:_add_watchers(w)
    local mt = getmetatable(w)
    mt.__gc = function(self)
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

function Loop:handle_error(watcher, msg)
    print("error:", watcher, msg)
end

local loop = Loop:new()
for k,v in pairs(ev) do
    if type(v) ~= "function" then
        loop[k] = v
    end
end

return loop

