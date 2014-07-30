local hub       = require "levent.hub"
local class     = require "levent.class"
local exception = require "levent.exception"

local Timeout = class("TimeoutException", exception.BaseException)

function Timeout:_init(seconds)
    self.seconds = seconds
    self.timer = hub.loop:timer(self.seconds)
end

function Timeout:start()
    local co = coroutine.running()
    self.timer:start(hub.throw, hub, co, self)
end

function Timeout:cancel()
    self.timer:stop()
end

function Timeout:__tostring()
    return string.format("Timeout: %s seconds", self.seconds)
end

local timeout = {}

function timeout.timeout(seconds)
    return Timeout:new(seconds)
end

function timeout.start_new(seconds)
    local t = Timeout:new(seconds)
    t:start()
    return t
end

function timeout.run(seconds, func, ...)
    local t = Timeout:new(seconds)
    t:start()
    local args = {pcall(func, ...)}
    t:cancel()
    if args[1] then
        return table.unpack(args)
    else
        if args[2] == t then
            return false, t
        else
            error(args[2])
        end
    end
end

function timeout.is_timeout(obj)
    return class.isinstance(obj, Timeout)
end
return timeout

