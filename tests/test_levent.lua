local levent = require "levent.levent"

function sleep(sec)
    levent.sleep(sec)
    print("sleep:", sec)
end

function child(id)
    print("in child:", id)
end

function main()
    for i=5,1,-1 do
        levent.spawn(sleep, i)
    end

    print("in main", 1)
    levent.spawn(child, 1)
    print("in main", 2)
    levent.spawn(child, 2)
    print("in main", 3)
    levent.spawn(child, 3)
    print("in main", 4)
    levent.spawn(child, 4)
    print("in main", 5)
end

levent.start(main)

