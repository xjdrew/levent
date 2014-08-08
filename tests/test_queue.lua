local levent = require "levent.levent"
local queue  = require "levent.queue"

local q = queue.queue(1)

local function consumer(count)
    for i=1, count do
        print("get:", q:get())
    end
end

local function producer(count)
    for i=1, count do
        q:put(i)
        print("put:", i)
    end
end

local function run()
    levent.spawn(consumer, 10)
    levent.spawn(producer, 10)
    levent.sleep(1)
end
levent.start(run)

