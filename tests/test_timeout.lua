local levent  = require "levent.levent"
local timeout = require "levent.timeout"

function t1(sec)
    print("sleep:", sec)
    levent.sleep(sec)
    print("sleep:", sec, "succeed")
end

function t2()
    timeout.start_new(1)
    levent.sleep(2)
end

function test()
    print("----------:1")
    local ok, exception = timeout.run(1, t1, 15)
    print(ok, exception)
    assert(not ok)
    print(exception)

    print("----------:2")
    assert(timeout.run(10, t1, 1) == true)

    print("----------:3")
    local ok, exception = pcall(t2)
    assert(not ok)
    print(exception)

    print("----------:4")
    print(timeout.run(5, function()
        return timeout.run(2, t1, 5)
    end))

    print("----------:5")
    print(timeout.run(2, function()
        return timeout.run(5, t1, 10)
    end))

    levent.sleep(5)
end

levent.start(test)

