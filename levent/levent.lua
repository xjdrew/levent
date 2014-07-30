local hub = require "levent.hub"

local levent = {}
local coroutine_pool = {}
local function co_create(f)
    local co = table.remove(coroutine_pool)
    if co == nil then
        co = coroutine.create(function(...)
            f(...)
            while true do
                f = nil
                coroutine_pool[#coroutine_pool + 1] = co
                f = coroutine.yield()
                f(coroutine.yield())
            end
        end)
    else
        coroutine.resume(co, f)
    end
    return co
end

function levent.sleep(sec)
    if sec <= 0 then
        local waiter = hub:waiter()
        hub.loop:run_callback(waiter.switch)
        waiter.get()
    else
        hub:wait(hub.loop:timer(sec))
    end
end

local function assert_resume(co, ...)
    local ok, msg = coroutine.resume(co, ...)
    assert(ok, msg)
end

function levent.spawn(f, ...)
    local co = co_create(f)
    hub.loop:run_callback(assert_resume, co, ...)
end

function levent.wait()
    hub:run()
end

function levent.start(f, ...)
    levent.spawn(f, ...)
    levent.wait()
end

function levent.waiter()
    return hub:waiter()
end

function levent.get_hub()
    return hub
end

return levent

