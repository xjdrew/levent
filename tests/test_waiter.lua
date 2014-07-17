local levent = require "levent.levent"

local waiter = levent.new_waiter()
local timer  = levent.new_timer(0.1)
local value = "hello from waiter"
timer.start(waiter.switch, value)
assert(waiter.get() == value)


local waiter = levent.new_waiter()
local timer  = levent.new_timer(0.1)
local value = "hello from waiter"
timer.start(waiter.switch, value)
levent.sleep(0.2)
assert(waiter.get() == value)

