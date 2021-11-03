--[[
-- author: xjdrew
-- date: 2014-07-17
--]]
local assert     = assert

local hub_class  = require "levent.hub"
local coroutines = require "levent.coroutines"
local exceptions = require "levent.exceptions"

local kill_error = exceptions.KillError.new()

local levent = {}

levent.kill_error = kill_error

local hub = hub_class.new()

function levent.now()
    return hub.loop:now()
end

function levent.sleep(sec)
    if sec < 0 then
        sec = 0
    end

    hub:wait(hub.loop:timer(sec))
end

local stats = {
    running = coroutines.running,
    cached = coroutines.cached,
    total = coroutines.total
}

function levent.stats(item)
    if item == "all" then
        local t = {}
        for k,f in pairs(stats) do
            t[k] = f()
        end
        return t
    end

    local f = stats[item]
    if f then
        return f()
    end
    return nil
end

levent.check_coroutine = coroutines.check

function levent.spawn(f, ...)
    local co = coroutines.create(f)
    hub.loop:run_callback(coroutines.resume, co, ...)
    return co
end

function levent.kill(co)
    hub.loop:run_callback(hub.throw, hub, co, kill_error)
end

function levent.start(f, ...)
    levent.spawn(f, ...)
    hub:run()
end

function levent.exit()
    hub:exit()
end

function levent.waiter()
    return hub:waiter()
end

function levent.get_hub()
    return hub
end

return levent

