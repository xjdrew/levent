local hub        = require "levent.hub"
local class      = require "levent.class"
local exceptions = require "levent.exceptions"

local Timeout = class("TimeoutException", exceptions.BaseException)

function Timeout:_init(seconds)
    self.seconds = seconds
    if not self.seconds or self.seconds <= 0 then
        return
    end
    self.timer = hub.loop:timer(self.seconds)
end

function Timeout:start()
    if self.timer then
        local co = coroutine.running()
        self.timer:start(hub.throw, hub, co, self)
    end
end

function Timeout:cancel()
    if self.timer then
        self.timer:stop()
    end
end

function Timeout:__tostring()
    return string.format("Timeout: %s seconds", self.seconds)
end

local timeout = {}

timeout.timeout = Timeout.new
function timeout.start_new(seconds)
    local t = Timeout.new(seconds)
    t:start()
    return t
end

function timeout.run(seconds, func, ...)
    local t = Timeout.new(seconds)
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

