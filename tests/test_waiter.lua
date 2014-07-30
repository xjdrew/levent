local levent = require "levent.levent"

function t1()
    local waiter = levent.waiter()
    local timer  = levent.get_hub().loop:timer(0.1)
    local value = "hello from waiter 1"
    timer:start(waiter.switch, waiter, value)
    levent.sleep(0.2)
    assert(waiter:get() == value)
    print(value)
end

function t2()
    local waiter = levent.waiter()
    local timer  = levent.get_hub().loop:timer(0.1)
    local value = "hello from waiter 2"
    timer:start(waiter.switch, waiter, value)
    assert(waiter:get() == value)
    print(value)
end

levent.spawn(t1)
levent.spawn(t2)
levent.wait()

