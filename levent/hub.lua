local c          = require "levent.c"
local class      = require "levent.class"
local exceptions = require "levent.exceptions"
local loop       = require "levent.loop"

local unique = c.unique

local Waiter = class("Waiter")
local Hub = class("Hub")

local cancel_wait_error = exceptions.CancelWaitError.new()

function Hub:_init()
    local co, main = coroutine.running()
    assert(main, "must in main coroutine")
    self.co = co
    self.loop = loop.new()
    self.waiters = setmetatable({}, {__mode="v"})
end

function Hub:waiter()
    return Waiter.new(self)
end

function Hub:wait(watcher)
    local waiter = self:waiter()
    local uuid = unique()
    watcher:start(waiter, uuid)
    local ok, val = pcall(waiter.get, waiter)
    watcher:stop()
    if ok then
        assert(uuid == val)
    else
        error(val)
    end
end

function Hub:_cancel_wait(watcher, err)
    if not watcher:is_active() then
        return
    end
    if not class.isinstance(watcher._cb, Waiter) then
        return
    end
    watcher._cb:throw(err or cancel_wait_error)
end

function Hub:cancel_wait(watcher, err)
    self.loop:run_callback(self._cancel_wait, self, watcher, err)
end

function Hub:switch(co, value)
    local waiter = assert(self.waiters[co], co)
    waiter:switch(value)
end

function Hub:throw(co, exception)
    local waiter = assert(self.waiters[co], co)
    waiter:throw(exception)
end

function Hub:handle_error(co, msg)
    print("error:", co, msg)
end

function Hub:_yield(waiter)
    local co = coroutine.running()
    assert(not self.waiters[co], co)
    self.waiters[co] = waiter
    coroutine.yield()
    self.waiters[co] = nil
end

function Hub:run()
    self.loop:run()
end

function Hub:exit()
    self.loop:_break()
end

-- Waiter metamethods
function Waiter:_init(hub)
    self.hub = hub
    self.co = nil
    self.value = nil
    self.exception = false
end

function Waiter:_switch(value, exception)
    assert(not exception or exceptions.is_exception(exception), exception)
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
    self:_switch(nil, exception)
end

-- for 
function Waiter:__call(value)
    self:_switch(value, nil)
end

function Waiter:get()
    if self.exception == false then
        self.co = coroutine.running()
        self.hub:_yield(self)
    end

    if self.exception == nil then
        return self.value
    else
        error(self.exception)
    end
end

local hub = Hub.new()
return hub

