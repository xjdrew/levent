local levent = require "levent.levent"

function child(id)
    print("in child:", id)
end

function main()
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
