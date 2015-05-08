local levent = require"levent.levent"
local mongo  = require"levent.mongo"

function prompt(msg)
    print("::", msg)
end

function main()
    local client = mongo.client({host = "127.0.0.1", port = 27017})
    local db = client:getDB("test")
    db.mydata:insert({name = "foo", id = 1})
    db.mydata:insert({name = "bar", id = 2})
    print("find:", db.mydata:findOne({id = 1}).name)
    levent.spawn(prompt, "hello world")
    print("find:", db.mydata:findOne({id = 2}).name)
end

levent.start(main)
