local levent     = require "levent.levent"
local lock       = require "levent.lock"
local exceptions = require "levent.exceptions"

local e = lock.event()
local as1 = lock.async_result()
local as2 = lock.async_result()
local as3 = lock.async_result()

function dump_stats()
    local t = levent.stats("all")
    for k,v in pairs(t) do
        print(k, v)
    end
end

function t1()
    print("t1 in")
    dump_stats()

    e:wait()
    print("t1 out ++++++++++++")
end

function t2()
    print("t2 in")
    dump_stats()

    e:wait()
    print("t2 do")
    e:clear()
    print("e clear")
    print(e:wait(5))
    print("t2 out +++++++++++")
end

local exception = exceptions.BaseException.new()
function t3()
    print("t3 in")
    assert(as1:get() == "as1")
    local ok, err = pcall(as2.get, as2)
    assert(not ok)
    assert(err == exception)
    as3:get(3)
    print("t3 out ++++++++++")
end

function start()
    levent.check_coroutine(10)

    print(e)
    levent.spawn(t1)
    levent.spawn(t2)
    print("-----------------:", 1)
    levent.sleep(0)
    print("-----------------:", 2)
    e:set()

    levent.spawn(t3)
    levent.sleep(0)
    as1:set("as1")
    as2:set_exception(exception)
    dump_stats()
end

levent.start(start)

