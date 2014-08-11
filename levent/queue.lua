--[[
-- author: xjdrew
-- date: 2014-08-08
--]]

local class   = require "levent.class"
local hub     = require "levent.hub"
local timeout = require "levent.timeout"
local lock    = require "levent.lock"

local Queue = class("Queue")

-- if maxsize is nil or less than 0, the queue size is infinite
function Queue:_init(maxsize)
    if not maxsize or maxsize <= 0 then
        self.maxsize = nil
    else
        self.maxsize = maxsize
    end
    self.items = {}
    self.length = 0

    self.getters = {}
    self.putters = {}

    self.cond = lock.event()
    self.cond:set()

    self.is_in_put_unlock = false
    self.is_in_get_unlock = false
end

function Queue:_put_unlock()
    while true do
        if self:is_full() then
            break
        end

        local waiter = next(self.putters)
        if not waiter then
            break
        end
        waiter:switch(true)
    end
    self.is_in_put_unlock = false
end

function Queue:_get_unlock()
    while true do
        if self.length == 0 then
            self.cond:set()
            break
        end

        local waiter = next(self.getters)
        if not waiter then
            break
        end
        waiter:switch(true)
    end
    self.is_in_get_unlock = false
end

function Queue:is_full()
    if self.maxsize and self.maxsize == #self.items then
        return true
    end
    return false
end

function Queue:__len()
    return self.length
end

function Queue:_get()
    assert(self.length > 0)
    local item = table.remove(self.items, 1)
    self.length = self.length - 1
    if not self.is_in_put_unlock and next(self.putters) then
        self.is_in_put_unlock = true
        hub.loop:run_callback(self._put_unlock, self)
    end
    return item
end

function Queue:_put(item)
    self.length = self.length + 1
    self.items[self.length] = item
    self.cond:clear()
    if not self.is_in_get_unlock and next(self.getters) then
        self.is_in_get_unlock = true
        hub.loop:run_callback(self._get_unlock, self)
    end
end

function Queue:put(item, sec)
    if not self.maxsize or self.length < self.maxsize then
        self:_put(item)
        return
    end
    local waiter = hub:waiter()
    self.putters[waiter] = true

    local t = timeout.start_new(sec)
    local ok, val = pcall(waiter.get, waiter)
    self.putters[waiter] = nil
    t:cancel()
    if not ok then
        error(val)
    end
    self:_put(item)
end

function Queue:get(sec)
    if self.length > 0 then
        return self:_get()
    end
    local waiter = hub:waiter()
    self.getters[waiter] = true

    local t = timeout.start_new(sec)
    local ok, val = pcall(waiter.get, waiter)
    self.getters[waiter] = nil
    t:cancel()
    if not ok then
        error(val)
    end
    return self:_get()
end

function Queue:join(sec)
    self.cond:wait(sec)
end

local M = {}
M.queue = Queue.new
return M

