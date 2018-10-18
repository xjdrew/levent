local levent = require "levent.levent"

function sleep(sec)
    levent.sleep(sec)
    print("[sleep]", sec)
end

function child(id)
    print("[child]", id)
end

function main()
    local count = 5
    for i=count,1,-1 do
        levent.spawn(sleep, i)
    end

    for i=1,count do
        print("[main] create child", i)
        levent.spawn(child, i)
    end
end

levent.start(main)

