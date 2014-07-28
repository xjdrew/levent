local levent = {}
local Hub  = require "levent.hub"

local hub = Hub.get_hub()

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

function levent.spawn(f, ...)
    local co = co_create(f)
    hub.loop:run_callback(coroutine.resume, co, ...)
end

function levent.start(f, ...)
    levent.spawn(f, ...)
    hub:run()
end

return levent

