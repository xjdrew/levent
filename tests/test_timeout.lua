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
    local ok, exception = timeout.run(1, t1, 10)
    assert(not ok)
    print(exception)
    assert(timeout.run(10, t1, 1) == true)
    local ok, exception = pcall(t2)
    assert(not ok)
    print(exception)
end

levent.start(test)

