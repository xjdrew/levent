--[[
-- author: xjdrew
-- date: 2014-08-01
-- coroutine pools
--]]

local coroutine_pool = {}
local total = 0

local function co_raw_create()
    local co
    co = coroutine.create(function(f)
        f(coroutine.yield()) -- if error in f, running would keeps
        while true do
            coroutine_pool[#coroutine_pool + 1] = co
            local f = coroutine.yield()
            f(coroutine.yield())
        end
    end)
    total = total + 1
    coroutine_pool[#coroutine_pool + 1] = co
end

local function co_create(f)
    if #coroutine_pool == 0 then
        co_raw_create()
    end
    local co = table.remove(coroutine_pool)
    coroutine.resume(co, f)
    return co
end

local coroutines = {}
function coroutines.create(f)
    return co_create(f)
end

function coroutines.check(count)
    local diff = count - #coroutine_pool
    if diff > 0 then
        for i=1, diff do
            co_raw_create()
        end
    end
end

function coroutines.total()
    return total
end

function coroutines.running()
    return total - #coroutine_pool
end

function coroutines.cached()
    return #coroutine_pool
end
return coroutines

