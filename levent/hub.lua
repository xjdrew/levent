local class     = require "levent.class"
local exception = require "levent.exception"
local c         = require "levent.c"

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

local hub = Hub:new()

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
            hub:handle_error(self.co, msg)
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
        return hub:switch()
    end
end

local M = {}
M.hub = hub
M.waiter = function()
    return Waiter:new
end
return M

