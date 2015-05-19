local class = require "levent.class"
local ev = require "levent.ev.c"

local function callback(func, ...)
    if select("#", ...) > 0 then
        local args = {...}
        return function()
            func(table.unpack(args))
        end
    end
    return func
end

local Watcher = class("Watcher")
function Watcher:_init(name, loop, ...)
    self.loop = loop
    local w = ev["new_" .. name]()
    w:init(...)
    self.cobj = w

    self._cb = nil
    self._args = nil
end

function Watcher:id()
    return self.cobj:id()
end

function Watcher:start(func, ...)
    assert(self._cb == nil, self.cobj)

    self._cb = func
    if select("#", ...) > 0 then
        self._args = {...}
    end
    self.cobj:start(self.loop.cobj)
end

function Watcher:stop()
    self._cb = nil
    self._args = nil

    self.cobj:stop(self.loop.cobj)
end

function Watcher:run_callback(revents)
    -- unused revents
    local ok, msg
    if self._args then
        ok, msg = xpcall(self._cb, debug.traceback, table.unpack(self._args))
    else
        ok, msg = xpcall(self._cb, debug.traceback)
    end
    if not ok then
        self.loop:handle_error(self, msg)
    end
end

function Watcher:is_active()
    return self.cobj:is_active()
end

function Watcher:is_pending()
    return self.cobj:is_pending()
end

function Watcher:get_priority()
    return self.cobj:get_priority()
end

function Watcher:set_priority(priority)
    return self.cobj:set_priority(priority)
end

-- register special watcher cls
local watcher_cls = {
}

local Loop = class("Loop")
function Loop:_init()
    self.cobj = ev.default_loop()
    self.watchers = setmetatable({}, {__mode="v"})

    -- register prepare
    self._callbacks = {}
    self._prepare = self:_create_watcher("prepare")
    self._prepare:start(function(revents)
        self:_run_callback(revents)
    end)
    self.cobj:unref()
end

function Loop:run(--[[nowait, once]])
    local flags = 0
    self.cobj:run(flags, self.callback, self)
end

function Loop:_break(how)
    if not how then
        how = ev.EVBREAK_ALL
    end
    self.cobj:_break(how)
end

function Loop:verify()
    return self.cobj:verify()
end

function Loop:now()
    return self.cobj:now()
end

function Loop:_add_watchers(w)
    local mt = getmetatable(w)
    mt.__tostring = function(self)
        return tostring(self.cobj)
    end

    mt.__gc = function(self)
        self:stop()
    end
    self.watchers[w:id()] = w
end

function Loop:_create_watcher(name, ...)
    local cls = watcher_cls[name] or Watcher
    local o = cls.new(name, self, ...)
    self:_add_watchers(o)
    return o
end

function Loop:io(fd, events)
    return self:_create_watcher("io", fd, events)
end

function Loop:timer(after, rep)
    return self:_create_watcher("timer", after, rep)
end

function Loop:signal(signum)
    return self:_create_watcher("signal", signum)
end

function Loop:callback(id, revents)
    local w = assert(self.watchers[id], id)
    w:run_callback(revents)
end

function Loop:_run_callback(revents)
    while #self._callbacks > 0 do
        local callbacks = self._callbacks
        self._callbacks = {}
        for _, cb in ipairs(callbacks) do
            self.cobj:unref()

            local ok, msg = xpcall(cb, debug.traceback)
            if not ok then
                self:handle_error(self._prepare, msg)
            end
        end
    end
end

function Loop:run_callback(func, ...)
    self._callbacks[#self._callbacks + 1] = callback(func, ...)
    self.cobj:ref()
end

function Loop:handle_error(watcher, msg)
    print("error:", watcher, msg)
end

local loop = {}
function loop.new(...)
    local obj = Loop.new()
    for k,v in pairs(ev) do
        if type(v) ~= "function" then
            obj[k] = v
        end
    end
    return obj
end

return loop

