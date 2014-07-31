--[[
-- author: xjdrew
-- date: 2014-07-31
--]]
local class   = require "levent.class"
local hub     = require "levent.hub"
local timeout = require "levent.timeout"

--[[
-- A synchronization primitive that allows one coroutine to wake up one or more others.
-- see tests/test_event.lua for example
--]]
local Event = class("Event")

function Event:_init()
    self.links = {}
    self.todo = {}
    self.flag = false
    self.notifier = false
end

function Event:set()
    self.flag = true
    for k,v in pairs(self.links) do
        self.todo[k] = v
    end

    if not self.notifier then
        self.notifier = true
        hub.loop:run_callback(self._notify, self)
    end
end

function Event:clear()
    self.flag = false
end

function Event:wait(sec)
    if self.flag then
        return self.flag
    end

    local t = timeout.start_new(sec)
    local waiter = hub:waiter()
    self.links[waiter] = true
    local ok, val = pcall(waiter.get, waiter)
    self.links[waiter] = nil
    t:cancel()
    if not ok then
        error(val)
    end
    return ok
end

function Event:_notify()
    while true do
        local waiter = next(self.todo)
        if not waiter then
            break
        end
        self.todo[waiter] = nil
        waiter:switch(true)
    end
    self.notifier = false
end

--[[
-- A one-time event that stores a value or an exception.
--]]
local AsyncResult = class("AsyncResult")

local event = {}
event.event = Event.new
event.async_result = AsyncResult.new
return event

