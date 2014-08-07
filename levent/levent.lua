--[[
-- author: xjdrew
-- date: 2014-07-17
--]]
local hub        = require "levent.hub"
local coroutines = require "levent.coroutines"

local levent = {}

function levent.now()
    return hub.loop:now()
end

function levent.sleep(sec)
    if sec <= 0 then
        local waiter = hub:waiter()
        hub.loop:run_callback(waiter.switch, waiter)
        waiter:get()
    else
        hub:wait(hub.loop:timer(sec))
    end
end

local function assert_resume(co, ...)
    local ok, msg = coroutine.resume(co, ...)
    assert(ok, msg)
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
    hub.loop:run_callback(assert_resume, co, ...)
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

