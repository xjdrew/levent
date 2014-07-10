local levent = require "levent.levent"

local function _worker(i)
	print("worker:", i, 1)
	levent.yield()
	print("worker:", i, 2)
end

levent.spawn(_worker, 1)
levent.spawn(_worker, 2)
levent.yield()
print("in main coroutine")
levent.spawn(_worker, 3)
levent.wait()

