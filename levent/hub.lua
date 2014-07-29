local c         = require "levent.c"
local class     = require "levent.class"
local exception = require "levent.exception"
local loop      = require "levent.loop"

local BaseException = exception.BaseException
local unique = c.unique

local Waiter = class()
local Hub = class("Hub")

function Hub:_init()
    local co, main = coroutine.running()
    assert(main, "must in main coroutine")
    self.co = co
    self.loop = loop.new()
end

function Hub:waiter()
    return Waiter:new(self)
end

function Hub:wait(watcher)
    local waiter = self:waiter()
    local uuid = unique()
    watcher:start(waiter.switch, waiter, uuid)
    local ok, val = pcall(waiter.get, waiter)
    watcher:stop()
    if ok then
        assert(uuid == val)
    else
        error(val)
    end
end

function Hub:handle_error(co, msg)
    print("error:", co, msg)
end

function Hub:run()
    self.loop:run()
end

-- Waiter metamethods
function Waiter:_init(hub)
    self.hub = hub
    self.co = nil
    self.value = nil
    self.exception = false
end

function Waiter:_switch(value, exception)
    assert(self.exception == false)
    self.value = value
    self.exception = exception

    if self.co ~= nil then
        assert(coroutine.running() == self.hub.co, "must be in hub.co")
        local ok, msg = coroutine.resume(self.co)
        if not ok then
            self.hub:handle_error(self.co, msg)
        end
    end
end

function Waiter:switch(value)
    self:_switch(value, nil)
end

function Waiter:throw(exception)
    assert(class.isinstance(exception, BaseException))
    self:_switch(nil, exception)
end

function Waiter:get()
    if self.exception == false then
        self.co = coroutine.running()
        coroutine.yield()
    end

    if self.exception == nil then
        return self.value
    else
        error(self.exception)
    end
end

local hub = Hub:new()
return hub

