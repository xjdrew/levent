local c         = require "levent.c"
local class     = require "levent.class"
local exception = require "levent.exception"
local loop      = require "levent.loop"

local BaseException = exception.BaseException
local unique = c.unique

local Waiter = class()
local Hub = class("Hub")

local getcurrent = coroutine.running
local switch = coroutine.resume

function Hub:_init()
    local co, main = getcurrent()
    assert(main, "must in main coroutine")
    self.co = co
    self.loop = loop.new()
end

function Hub:yield()
end

function Hub:wait(watcher)
    local waiter = Waiter:new()
    local uuid = unique()
    watcher.start(waiter.switch, uuid)
end

function Hub:handle_error(co, msg)
    print("error:", co, msg)
end

function Hub:run()
    self.loop:run()
end

local _hub = nil
local function get_hub()
    if not __hub then
        _hub = Hub:new()
    end
    return _hub
end

function Waiter:_init()
    self.co = nil
    self.value = nil
    self.exception = false
end

function Waiter:switch(value)
    if self.co == nil then
        self.value = value
        self.exception = nil
    else
        local hub = get_hub()
        assert(getcurrent() == hub.co, "must be in hub.co")
        local ok, msg = xpcall(switch, debug.traceback, self.co, value)
        if not ok then
            hub:handle_error(self.co, msg)
        end
    end
end

function Waiter:throw(exception)
    assert(class.isinstance(exception, BaseException))
    if self.co == nil then
        self.exception = exception
    else
        assert(getcurrent() == hub.co, "must be in hub.co")
        local ok, msg = xpcall(switch, debug.traceback, self.co, exception)
        if not ok then
            get_hub():handle_error(self.co, msg)
        end
    end
end

function Waiter:get()
    if self.exception ~= false then
        if self.exception == nil then
            return self.value
        else
            error(self.exception)
        end
    else
        assert(not self.co, self.co)
        return get_hub():switch()
    end
end

local M = {}
function M.new_waiter()
    return Waiter:new()
end
M.get_hub = get_hub
return M

