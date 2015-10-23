local levent = require"levent.levent"
local mongo  = require"ext.mongo"

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

    db.mydata:batch_insert({
        {name = "lihua", id = 9},
        {name = "hanmeimei", id = 10},
    })

    assert(db.mydata:findOne({id=9}).name == "lihua")
    assert(db.mydata:findOne({id=10}).name == "hanmeimei")
end

levent.start(main)
